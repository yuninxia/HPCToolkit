// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//***************************************************************************
//
// File: RelocateCubin.cpp
//
// Purpose:
//   Implementation of in-memory cubin relocation.
//
// Description:
//   The associated implementation performs in-memory relocation of a cubin so
//   that all text segments and functions are non-overlapping. Following
//   relocation
//     - each text segment begins at its offset in the segment
//     - each function, which is in a unique text segment, has its symbol
//       adjusted to point to the new position of the function in its relocated
//       text segment
//     - addresses in line map entries are adjusted to account for the new
//       offsets of the instructions to which they refer
//
//***************************************************************************

//******************************************************************************
// system includes
//******************************************************************************

#include <assert.h>
#include <gelf.h>
#include <string.h>
#include <algorithm>
#include <iostream>


//******************************************************************************
// local includes
//******************************************************************************

#include "ElfHelper.hpp"
#include "RelocateCubin.hpp"

#include "../common/lean/elf-helper.h"



//******************************************************************************
// macros
//******************************************************************************

#define DEBUG_LINE_SECTION_NAME ".debug_line"

#define DEBUG_INFO_SECTION_NAME ".debug_info"

#define CASESTR(n) case n: return #n
#define section_index(n) (n-1)

#define DEBUG_CUBIN_RELOCATION 0


//---------------------------------------------------------
// NVIDIA CUDA line map relocation types
//   type name gleaned using cuobjdump
//   value gleaned by examining binaries
//---------------------------------------------------------
#define R_NV_32                 0x01
#define R_NV_64                 0x02
#define R_NV_G32                0x03
#define R_NV_G64                0x04

#define RELOC_32(x) (x == R_NV_32 || x == R_NV_G32)
#define RELOC_64(x) (x == R_NV_64 || x == R_NV_G64)



//******************************************************************************
// type definitions
//******************************************************************************

typedef std::vector<Elf_Scn *> Elf_SectionVector;
typedef std::vector<Elf64_Addr> Elf_SymbolVector;
typedef std::vector<int64_t> Elf_SymbolSectionIndexVector;



//******************************************************************************
// private functions
//******************************************************************************

static size_t
sectionOffset
(
 Elf_SectionVector *sections,
 unsigned sindex
)
{
  // if section index is out of range, return 0
  if (sections->size() < sindex) return 0;

  GElf_Shdr shdr;
  if (!gelf_getshdr((*sections)[sindex], &shdr)) return 0;

  return shdr.sh_offset;
}


#if DEBUG_CUBIN_RELOCATION
static const char *
binding_name
(
 GElf_Sym *s
)
{
  switch(GELF_ST_BIND(s->st_info)){
    CASESTR(STB_LOCAL);
    CASESTR(STB_GLOBAL);
    CASESTR(STB_WEAK);
  default: return "UNKNOWN";
  }
}
#endif


// properly size the relocation update to an address based on the
// relocation type
static void
applyRelocation(void *addr, unsigned rel_type, uint64_t rel_value)
{
  if (RELOC_64(rel_type)) {
    uint64_t *addr64 = (uint64_t *) addr;
    *addr64 = rel_value;
  } else if (RELOC_32(rel_type)) {
    uint32_t *addr32 = (uint32_t *) addr;
    *addr32 = rel_value;
  } else {
    assert(false && "Invalid relocation type");
    std::abort();
  }
}


static void
applyRELrelocation
(
 char *line_map,
 Elf_SymbolVector *symbol_values,
 GElf_Rel *rel
)
{
  // get the symbol that is the basis for the relocation
  unsigned sym_index = GELF_R_SYM(rel->r_info);

  // determine the type of relocation
  unsigned rel_type = GELF_R_TYPE(rel->r_info);

  // get the new offset of the aforementioned symbol
  unsigned sym_value = (*symbol_values)[sym_index];

  // compute address where a relocation needs to be applied
  void *addr = (void *)(line_map + rel->r_offset);

  // update the address based on the symbol value associated with the
  // relocation entry.
  applyRelocation(addr, rel_type, sym_value);
}


static void
applyRELArelocation
(
 char *debug_info,
 Elf_SymbolVector *symbol_values,
 GElf_Rela *rela
)
{
  // get the symbol that is the basis for the relocation
  unsigned sym_index = GELF_R_SYM(rela->r_info);

  // determine the type of relocation
  unsigned rel_type = GELF_R_TYPE(rela->r_info);

  // get the new offset of the aforementioned symbol
  unsigned sym_value = (*symbol_values)[sym_index];

  // compute the address where a relocation needs to be applied
  void *addr = (unsigned long *) (debug_info + rela->r_offset);

  // update the address in based on the symbol value and addend in the
  // relocation entry
  applyRelocation(addr, rel_type, sym_value + rela->r_addend);
}


static void
applyRELrelocations
(
 Elf_SymbolVector *symbol_values,
 char *line_map,
 int n_relocations,
 Elf_Data *relocations_data
)
{
  //----------------------------------------------------------------
  // apply each line map relocation
  //----------------------------------------------------------------
  for (int i = 0; i < n_relocations; i++) {
    GElf_Rel rel_v;
    GElf_Rel *rel = gelf_getrel(relocations_data, i, &rel_v);
    applyRELrelocation(line_map, symbol_values, rel);
  }
}


static void
applyRELArelocations
(
 Elf_SymbolVector *symbol_values,
 char *debug_info,
 int n_relocations,
 Elf_Data *relocations_data
)
{
  //----------------------------------------------------------------
  // apply each debug info relocation
  //----------------------------------------------------------------
  for (int i = 0; i < n_relocations; i++) {
    GElf_Rela rela_v;
    GElf_Rela *rela = gelf_getrela(relocations_data, i, &rela_v);
    applyRELArelocation(debug_info, symbol_values, rela);
  }
}


// if the cubin contains a line map section and a matching line map relocations
// section, apply the relocations to the line map
static void
relocateLineMap
(
 char *cubin_ptr,
 Elf *elf,
 Elf_SectionVector *sections,
 Elf_SymbolVector *symbol_values,
 elf_helper_t* eh
)
{
  GElf_Ehdr ehdr_v;
  GElf_Ehdr *ehdr = gelf_getehdr(elf, &ehdr_v);
  if (ehdr) {
    unsigned line_map_scn_index;
    char *line_map = NULL;

    //-------------------------------------------------------------
    // scan through the sections to locate a line map, if any
    //-------------------------------------------------------------
    int index = 0;
    for (auto si = sections->begin(); si != sections->end(); si++, index++) {
      Elf_Scn *scn = *si;
      GElf_Shdr shdr;
      if (!gelf_getshdr(scn, &shdr)) continue;
      if (shdr.sh_type == SHT_PROGBITS) {
        const char *section_name = elf_strptr(elf, eh->section_string_index, shdr.sh_name);
        if (strcmp(section_name, DEBUG_LINE_SECTION_NAME) == 0) {
          // remember the index of line map section. we need this index to find
          // the corresponding relocation section.
          line_map_scn_index = index;

          // compute line map position from start of cubin and the offset
          // of the line map section in the cubin
          line_map = cubin_ptr + shdr.sh_offset;

          // found the line map, so we are done with the linear scan of sections
          break;
        }
      }
    }

    //-------------------------------------------------------------
    // if a line map was found, look for its relocations section
    //-------------------------------------------------------------
    if (line_map) {
      int index = 0;
      for (auto si = sections->begin(); si != sections->end(); si++, index++) {
        Elf_Scn *scn = *si;
        GElf_Shdr shdr;
        if (!gelf_getshdr(scn, &shdr)) continue;
        if (shdr.sh_type == SHT_REL || shdr.sh_type == SHT_RELA)  {
          if (section_index(shdr.sh_info) == line_map_scn_index) {
            // We may find either REL relocation or RELA relocation
            // the relocation section refers to the line map section
#if DEBUG_CUBIN_RELOCATION
            std::cout << "line map relocations section name: "
              << elf_strptr(elf, eh->section_string_index, shdr.sh_name)
              << std::endl;
#endif
            //-----------------------------------------------------
            // invariant: have a line map and its relocations.
            // use new symbol values to relocate the line map entries.
            //-----------------------------------------------------

            // if the relocation entry size is not positive, give up
            if (shdr.sh_entsize > 0) {
              int n_relocations = shdr.sh_size / shdr.sh_entsize;
#if DEBUG_CUBIN_RELOCATION
              std::cout << "n_relocations: " << n_relocations << std::endl;
#endif
              if (n_relocations > 0) {
                Elf_Data *relocations_data = elf_getdata(scn, NULL);
                if (shdr.sh_type == SHT_RELA) {
                  applyRELArelocations(symbol_values, line_map,
                    n_relocations, relocations_data);
                } else {
                  applyRELrelocations(symbol_values, line_map,
                    n_relocations, relocations_data);
                }
              }
            }
            return;
          }
        }
      }
    }
  }
}


// if the cubin contains a line map section and a matching line map relocations
// section, apply the relocations to the line map
static void
relocateDebugInfo
(
 char *cubin_ptr,
 Elf *elf,
 Elf_SectionVector *sections,
 Elf_SymbolVector *symbol_values,
 elf_helper_t* eh
)
{
  GElf_Ehdr ehdr_v;
  GElf_Ehdr *ehdr = gelf_getehdr(elf, &ehdr_v);
  if (ehdr) {
    unsigned debug_info_scn_index;
    char *debug_info = NULL;

    //-------------------------------------------------------------
    // scan through the sections to locate debug info, if any
    //-------------------------------------------------------------
    int index = 0;
    for (auto si = sections->begin(); si != sections->end(); si++, index++) {
      Elf_Scn *scn = *si;
      GElf_Shdr shdr;
      if (!gelf_getshdr(scn, &shdr)) continue;
      if (shdr.sh_type == SHT_PROGBITS) {
        const char *section_name = elf_strptr(elf, eh->section_string_index, shdr.sh_name);
        if (strcmp(section_name, DEBUG_INFO_SECTION_NAME) == 0) {
          // remember the index of line map section. we need this index to find
          // the corresponding relocation section.
          debug_info_scn_index = index;

          // compute debug info position from start of cubin and the offset
          // of the debug info section in the cubin
          debug_info = cubin_ptr + shdr.sh_offset;

          // found the debug info, so we are done with the linear scan of sections
          break;
        }
      }
    }

    //-------------------------------------------------------------
    // if debug info was found, look for its relocations section
    //-------------------------------------------------------------
    if (debug_info) {
      int index = 0;
      for (auto si = sections->begin(); si != sections->end(); si++, index++) {
        Elf_Scn *scn = *si;
        GElf_Shdr shdr;
        if (!gelf_getshdr(scn, &shdr)) continue;
        if (shdr.sh_type == SHT_RELA)  {
          if (section_index(shdr.sh_info) == debug_info_scn_index) {
            // the relocation section refers to the line map section
#if DEBUG_CUBIN_RELOCATION
            std::cout << "debug info relocations section name: "
              << elf_strptr(elf, eh->section_string_index, shdr.sh_name)
              << std::endl;
#endif
            //-----------------------------------------------------
            // invariant: have debug info and its relocations.
            // use new symbol values to relocate debug info entries.
            //-----------------------------------------------------

            // if the relocation entry size is not positive, give up
            if (shdr.sh_entsize > 0) {
              int n_relocations = shdr.sh_size / shdr.sh_entsize;
              if (n_relocations > 0) {
                Elf_Data *relocations_data = elf_getdata(scn, NULL);
                applyRELArelocations(symbol_values, debug_info,
                  n_relocations, relocations_data);
              }
            }
            return;
          }
        }
      }
    }
  }
}


static Elf_SymbolVector *
relocateSymbolsHelper
(
 Elf *elf,
 GElf_Ehdr *ehdr,
 GElf_Shdr *shdr,
 Elf_SectionVector *sections,
 Elf_Scn *scn,
 elf_helper_t *eh
)
{
  Elf_SymbolVector *symbol_values = NULL;
  std::size_t nsymbols = 0;
  if (shdr->sh_type != SHT_SYMTAB)
    std::abort();
  if (shdr->sh_entsize > 0) { // avoid divide by 0
    nsymbols = shdr->sh_size / shdr->sh_entsize;
  }
  if (nsymbols <= 0) return NULL;

  Elf_Data *datap = elf_getdata(scn, NULL);

  if (datap) {
    symbol_values = new Elf_SymbolVector(nsymbols);
    std::vector<std::pair<std::size_t, GElf_Sym>> symbols;

    // Read in all the function symbols, and store in the symbols vector.
    // Also adjust the symbol values to be absolute instead of relative to section start.
    symbols.reserve(nsymbols);
    for (std::size_t idx = 0; idx < nsymbols; ++idx) {
      GElf_Sym sym_buf;
      int section_index;
      GElf_Sym* sym = elf_helper_get_symbol(eh, idx, &sym_buf, &section_index);
      if (sym != nullptr && sym->st_shndx != SHN_UNDEF && GELF_ST_TYPE(sym->st_info) == STT_FUNC) {
        symbols.push_back({idx, *sym});
        auto& my_sym = symbols.back().second;

        // Adjust the symbol's value to be absolute file offset instead of relative to the section.
        my_sym.st_value += sectionOffset(sections, section_index(section_index));
        (*symbol_values)[idx] = my_sym.st_value;
      }
    }

    // Sort the symbols by their values.
    std::sort(symbols.begin(), symbols.end(),
        [](const auto& a, const auto& b) {
          return a.second.st_value < b.second.st_value;
        });

    // Trim any functions that overlap with later functions.
    std::size_t last_idx;
    GElf_Sym* last_sym = nullptr;
    for (auto& [idx, next_sym]: symbols) {
      if (last_sym != nullptr) {
        last_sym->st_size = std::min(last_sym->st_size, next_sym.st_value - last_sym->st_value);

        // At this point we're done editing last_sym, so update the ELF.
        gelf_update_sym(datap, last_idx, last_sym);
      }
      last_idx = idx;
      last_sym = &next_sym;
    }

    // The last symbol in the table, if present, has not yet been updated in the ELF. Do so now.
    if (!symbols.empty()) {
      gelf_update_sym(datap, symbols.back().first, &symbols.back().second);
    }
  }
  return symbol_values;
}


static Elf_SymbolVector *
relocateSymbols
(
  Elf *elf,
  Elf_SectionVector *sections,
  elf_helper_t* eh
)
{
  Elf_SymbolVector *symbol_values = NULL;
  GElf_Ehdr ehdr_v;
  GElf_Ehdr *ehdr = gelf_getehdr(elf, &ehdr_v);
  if (ehdr) {
    for (auto si = sections->begin(); si != sections->end(); si++) {
      Elf_Scn *scn = *si;
      GElf_Shdr shdr;
      if (!gelf_getshdr(scn, &shdr)) continue;
      if (shdr.sh_type == SHT_SYMTAB) {
#if DEBUG_CUBIN_RELOCATION
        std::cout << "relocating symbols in section " << elf_strptr(elf, eh->section_string_index, shdr.sh_name)
          << std::endl;
#endif
        symbol_values = relocateSymbolsHelper(elf, ehdr, &shdr, sections, scn, eh);
        break; // AFAIK, there can only be one symbol table
      }
    }
  }
  return symbol_values;
}


// cubin text segments all start at offset 0 and are thus overlapping.
// relocate each segment of type SHT_PROGBITS (a program text
// or data segment) so that it begins at its offset in the
// cubin. when this function returns, text segments no longer overlap.
static void
relocateProgramDataSegments
(
  Elf *elf,
  Elf_SectionVector *sections,
  elf_helper_t* eh
)
{
  GElf_Ehdr ehdr_v;
  GElf_Ehdr *ehdr = gelf_getehdr(elf, &ehdr_v);
  if (ehdr) {
    for (auto si = sections->begin(); si != sections->end(); si++) {
      Elf_Scn *scn = *si;
      GElf_Shdr shdr;
      if (!gelf_getshdr(scn, &shdr)) continue;
      if (shdr.sh_type == SHT_PROGBITS) {
#if DEBUG_CUBIN_RELOCATION
        std::cout << "relocating section " << elf_strptr(elf, eh->section_string_index, shdr.sh_name)
        << " to " << std::hex << shdr.sh_offset << std::dec
          << std::endl;
#endif
        // update a segment so it's starting address matches its offset in the
        // cubin.
        shdr.sh_addr = shdr.sh_offset;
        gelf_update_shdr(scn, &shdr);
      }
    }
  }
}



//******************************************************************************
// interface functions
//******************************************************************************

bool
relocateCubin
(
 char *cubin_ptr,
 size_t cubin_size,
 Elf *cubin_elf
)
{
  bool success = false;

  elf_helper_t eh;
  elf_helper_initialize(cubin_elf, &eh);

  Elf_SectionVector *sections = elfGetSectionVector(cubin_elf);
  if (sections) {
    relocateProgramDataSegments(cubin_elf, sections, &eh);
    Elf_SymbolVector *symbol_values = relocateSymbols(cubin_elf, sections, &eh);
    if (symbol_values) {
      relocateLineMap(cubin_ptr, cubin_elf, sections, symbol_values, &eh);
      relocateDebugInfo(cubin_ptr, cubin_elf, sections, symbol_values, &eh);
      delete symbol_values;
      success = true;
    }
    delete sections;
  }

  return success;
}
