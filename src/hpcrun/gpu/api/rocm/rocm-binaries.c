
// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// system includes
//******************************************************************************

#include <fcntl.h>
#include <gelf.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************


#include "../../../../common/lean/spinlock.h"
#include "../../../../common/lean/crypto-hash.h"
#include "../../../../common/lean/elf-helper.h"
#include "../../../files.h"
#include "../../../libmonitor/monitor.h" // enable and disable threads
#include "../../../memory/hpcrun-malloc.h"
#include "../../../messages/messages.h"
#include "../common/gpu-binary.h"

#include "rocm-binaries.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// macros
//******************************************************************************

#define FILE_URI_PREFIX      "file://"
#define MEMORY_URI_PREFIX    "memory://"

#define CONST_STRLEN(c)  (sizeof(c) - 1)

#define EQ_CONST_STRING(s, const) (strncmp(s, const, CONST_STRLEN(const)) == 0)



//******************************************************************************
// type declarations
//******************************************************************************

typedef struct amd_function_table {
  size_t size;
  char** names;
  uint64_t* addrs;
} amd_function_table_t;


typedef struct amd_gpu_binary {
  long unsigned size;
  amd_function_table_t function_table;
  uint32_t amd_gpu_module_id;
  bool load_module_unused;
  char* buf;
  char* uri;
  struct amd_gpu_binary* next;
} amd_gpu_binary_t;



//******************************************************************************
// private variables
//******************************************************************************

static amd_gpu_binary_t* binary_list = NULL;

// A spin lock to serialize two AMD GPU binary operations:
// 1. parse and add a code object to the binary list
// 2. look up a function name from the the binary list
static spinlock_t rocm_binary_list_lock;



//******************************************************************************
// private operations
//******************************************************************************

// TODO:
// construct_amd_gpu_symbols parses the ELF symbol and extract
// the string names of functions.
//
// We have similar code in different places that iterate over
// the ELF symbol table, and doing slightly different things.
// At least, NVIDIA support code iterates over symbol table
// to relocate functions, and hpcfnbounds code iterates over
// symbol table to find function starts.
// It would be good to refactor these ELF operations into common
// code.

static bool
is_degenerate_amdgpu_binary
(
  char* buf,
  long unsigned size
)
{
  Elf *elf = elf_memory(buf, size);

  if (elf) {
    GElf_Ehdr ehdr;
    GElf_Ehdr *e = gelf_getehdr(elf, &ehdr);
    elf_end(elf);
    if (e && e->e_machine == EM_AMDGPU) return false;
  }

  return true;
}


static void
construct_amd_gpu_symbols
(
  Elf *elf,
  amd_function_table_t * ft
)
{
  // Initialize elf_help_t to handle extended numbering
  elf_helper_t eh;
  elf_helper_initialize(elf, &eh);

  // Get section name section index to find ".strtab"
  size_t shstrndx;
  if (elf_getshdrstrndx(elf, &shstrndx) != 0) return;

  // Find .symtab and .strtab sections
  Elf_Scn *scn = NULL;
  Elf_Scn *symtab_scn = NULL;
  Elf_Scn *strtab_scn = NULL;
  while ((scn = elf_nextscn(elf, scn)) != NULL) {
    GElf_Shdr shdr;
    if (!gelf_getshdr(scn, &shdr)) continue;
    if (shdr.sh_type == SHT_SYMTAB) {
      symtab_scn = scn;
      continue;
    }
    char *name = elf_strptr(elf, shstrndx , shdr.sh_name);
    if (name == NULL) continue;
    if (strcmp(name, ".strtab") == 0) {
      strtab_scn = scn;
    }
  }

  // Get total number of symbols in .symtab
  GElf_Shdr symtab;
  gelf_getshdr(symtab_scn, &symtab);

  int nsymbols = 0;
  if (symtab.sh_entsize > 0) { // avoid divide by 0
    nsymbols = symtab.sh_size / symtab.sh_entsize;
    if (nsymbols <= 0) return;
  } else {
    return;
  }

  Elf_Data *symtab_data = elf_getdata(symtab_scn, NULL);
  if (symtab_data == NULL) return;

  // Get total number of function symbols in .symtab
  size_t nfuncs = 0;
  for (int i = 0; i < nsymbols; i++) {
    GElf_Sym sym;
    GElf_Sym *symp = NULL;
    int section_index;
    symp = elf_helper_get_symbol(&eh, i, &sym, &section_index);
    if (symp) { // symbol properly read
      int symtype = GELF_ST_TYPE(sym.st_info);
      if (sym.st_shndx == SHN_UNDEF) continue;
      if (symtype != STT_FUNC) continue;
      nfuncs++;
    }
  }

  // Get symbol name string table
  Elf_Data *strtab_data = elf_getdata(strtab_scn, NULL);
  if (strtab_data == NULL) return;
  char* symbol_name_buf = (char*)(strtab_data->d_buf);

  // Initialize our function table
  ft->size = nfuncs;
  ft->names = (char**)malloc(sizeof(char*) * ft->size);
  ft->addrs = (uint64_t*)malloc(sizeof(uint64_t) * ft->size);
  int index = 0;

  // Put each function symbol into our function table
  for (int i = 0; i < nsymbols; i++) {
    GElf_Sym sym;
    GElf_Sym *symp = NULL;
    int section_index;
    symp = elf_helper_get_symbol(&eh, i, &sym, &section_index);
    if (symp) { // symbol properly read
      int symtype = GELF_ST_TYPE(sym.st_info);
      if (sym.st_shndx == SHN_UNDEF) continue;
      if (symtype != STT_FUNC) continue;
      ft->names[index] = symbol_name_buf + sym.st_name;
      ft->addrs[index] = sym.st_value;
      ++index;
    }
  }
#if DEBUG != 0
  fprintf(stderr, "Dump AMD GPU functions\n");
  for (size_t i = 0; i < ft->size; ++i) {
    fprintf(stderr, "Function %s, at address %lx\n", ft->names[i], ft->addrs[i]);
  }
#endif
}

// TODO:
// Eventually, we want to write the URI into our load map rather than copying the binary into a file.
// To handle this long-term goal, the URI parsing would have to be code shared by hpcrun, hpcprof,
// and hpcstruct. We can move function parse_amd_gpu_binary_uri to prof-lean directory and refactor
// the function to return something that can help identifies the GPU binary specified by the URI.

static void
parse_uri
(
  const char *uri,
  char **loc,
  unsigned long long *offset,
  unsigned long *size
)
{
  *loc = strdup(uri);
  char *separator = *loc;

  // path is separated by either # or ?
  while (*separator != '#' && *separator != '?')
    ++separator;
  *separator = 0;

  // Get the offset field in the uri
  char* uri_suffix = separator + 1;
  char* index = strstr(uri_suffix, "offset=") + strlen("offset=");
  char* endptr;
  *offset = strtoull(index, &endptr, 0);

  // Get the size field in the uri
  index = strstr(uri_suffix, "size=") + strlen("size=");
  *size = strtoul(index, &endptr, 0);

  PRINT("Parsing URI: (spec=%s, offset=0x%llx, size=0x%lx)\n",
        *loc, *offset, *size);
}


static char *
uri_to_data
(
  const char *uri,
  unsigned long long offset,
  unsigned long size
)
{
  char *buf = 0;
  if (EQ_CONST_STRING(uri, FILE_URI_PREFIX)) {
    // read the AMD GPU binary from disk
    const char *filepath = uri + CONST_STRLEN(FILE_URI_PREFIX);

    int rfd = open(filepath, O_RDONLY);
    if (rfd < 0) {
      PRINT("Cannot open file URI '%s'\n", filepath);
      return 0;
    }

    if (lseek(rfd, offset, SEEK_SET) < 0) {
      PRINT("Cannot seek file URI '%s' to offset=0x%llx\n",
            filepath, offset);
      return 0;
    }

    buf = (char *) malloc(size);
    ssize_t read_bytes = read(rfd, buf, size);
    if (read_bytes != size) {
      PRINT("\tfail to read file, read %ld\n", read_bytes);
      perror(NULL);
      free(buf);
      buf = 0;
    }
  } else if (EQ_CONST_STRING(uri, MEMORY_URI_PREFIX)) {
    // copy the AMD GPU binary from memory
    buf = (char *) malloc(size);
    memcpy(buf, (char *) offset, size);
  }

  return buf;
}


static uint32_t
parse_amd_gpu_binary_uri
(
  const char *uri,
  amd_gpu_binary_t *bin
)
{
  char *prefix;
  unsigned long long offset;
  parse_uri(uri, &prefix, &offset, &bin->size);

  // copy URI data
  bin->buf = uri_to_data(prefix, offset, bin->size);

  if (bin->buf == 0) goto finish;

  // compute hash string for the binary
  char binary_hash[CRYPTO_HASH_STRING_LENGTH];
  crypto_compute_hash_string(bin->buf, bin->size, binary_hash,
                             CRYPTO_HASH_STRING_LENGTH);

  // create file name
  char gpu_file_path[PATH_MAX];
  char load_module_name_fullpath[PATH_MAX] = {'\0'};
  gpu_binary_path_generate(binary_hash, gpu_file_path,
                           load_module_name_fullpath);

  // don't write degenerate binaries
  if (is_degenerate_amdgpu_binary(bin->buf, bin->size)) {
   fprintf(stderr,
        "WARNING: hpcrun: rocprofiler codeobject callback provided a degenerate AMD GPU binary: %s\n"
        "         This may leave you unable to attribute some or all GPU activity to source code.\n", gpu_file_path);
  } else {
    // write this GPU binary if it hasn't already been written by another process
    gpu_binary_store(load_module_name_fullpath, (const void*)(bin->buf),
                     bin->size);
  }

  bin->amd_gpu_module_id = hpcrun_loadModule_add(gpu_file_path);

  // FIXME: this indicates that all AMD load modules are used without
  // seeing any calls to kernels within. this ensures that binary analysis of
  // AMD GPU binaries will be performed.
  hpcrun_loadmap_lock();
  load_module_t *lm = hpcrun_loadmap_findById(bin->amd_gpu_module_id);
  if (lm) {
    hpcrun_loadModule_flags_set(lm, LOADMAP_ENTRY_ANALYZE);
    bin->load_module_unused = false;
  }
  hpcrun_loadmap_unlock();

  bin->load_module_unused = false;

finish:
  free(prefix);

  return bin->amd_gpu_module_id;
}


static int
known_uri
(
  const char* uri
)
{
  for (amd_gpu_binary_t * bin = binary_list; bin != NULL; bin = bin->next) {
    if (strcmp(uri, bin->uri) == 0) {
      return bin->amd_gpu_module_id;
    }
  }
  return 0;
}


static uint32_t
parse_amd_gpu_binary
(
  const char* uri
)
{
  uint32_t load_module_id = -1;

  // handle only file and memory URIs
  if (!EQ_CONST_STRING(uri, FILE_URI_PREFIX) &&
      !EQ_CONST_STRING(uri, MEMORY_URI_PREFIX)) {
    return load_module_id;
  }

  load_module_id = known_uri(uri);
  if (load_module_id) return load_module_id;

  // Handle a new AMD GPU binary
  amd_gpu_binary_t* bin = (amd_gpu_binary_t*) malloc(sizeof(amd_gpu_binary_t));
  bin->uri = strdup(uri);
  bin->next = binary_list;
  binary_list = bin;
  // Parse URI to extract the binary
  load_module_id = parse_amd_gpu_binary_uri(uri, bin);

  // Parse the ELF symbol table
  elf_version(EV_CURRENT);
  Elf *elf = elf_memory(bin->buf, bin->size);
  if (elf != 0) {
    construct_amd_gpu_symbols(elf, &(bin->function_table));
    elf_end(elf);
  }

  return load_module_id;
}

// TODO:
// lookup_amd_function is currently implemented as a linear search.
// We would hope to get help from AMD that roctracer will directly
// tell us the offset of a launched kernel, so that we do not have
// to do name matching.
//
// If we got no help, then we would need to refactor the code to
// to handle large GPU binaries. We will need to use more efficient
// lookup data structure, a splay tree or a trie.

//---------------------------------------------------------------------
//   device_id is necessary to ensure that a kernel maps back to a
//   binary suitable for the specified device. however, device_id is a
//   small integer device index and AMD hasn't told us how we can use
//   this to map back to a device kind so we can limit this search to
//   only binaries suitable for the device kind.
//---------------------------------------------------------------------
static ip_normalized_t
lookup_amd_function
(
 int device_id,
 const char *kernel_name
)
{
  ip_normalized_t nip;
  nip.lm_id = 0;
  nip.lm_ip = 0;

  for (amd_gpu_binary_t *bin = binary_list; bin != NULL; bin = bin->next) {
    amd_function_table_t *ft = &(bin->function_table);
    for (size_t i = 0; i < ft->size; ++i) {
      if (strcmp(kernel_name, ft->names[i]) == 0) {
        nip.lm_id = bin->amd_gpu_module_id;
        nip.lm_ip = (uintptr_t)(ft->addrs[i]);
        if (bin->load_module_unused) {
          hpcrun_loadmap_lock();
          load_module_t *lm = hpcrun_loadmap_findById(nip.lm_id);
          if (lm) {
            hpcrun_loadModule_flags_set(lm, LOADMAP_ENTRY_ANALYZE);
            bin->load_module_unused = false;
          }
          hpcrun_loadmap_unlock();
        }
        return nip;
      }
    }
  }

  return nip;
}



//******************************************************************************
// interface operations
//******************************************************************************

ip_normalized_t
rocm_binary_function_lookup
(
 int device_id,
 const char* kernel_name
)
{
  // TODO:
  // 1. Currently we support multiple GPU binaries, but assume that kernel is unique
  //    across GPU binaries.
  spinlock_lock(&rocm_binary_list_lock);
  ip_normalized_t nip = lookup_amd_function(device_id, kernel_name);
  PRINT("HIP launch kernel %s, lm_ip %lx\n", kernel_name, nip.lm_ip);
  spinlock_unlock(&rocm_binary_list_lock);
  return nip;
}


uint32_t
rocm_binary_uri_add
(
  const char *uri
)
{
  spinlock_lock(&rocm_binary_list_lock);
  uint32_t load_module_id = parse_amd_gpu_binary(uri);
  spinlock_unlock(&rocm_binary_list_lock);

  return load_module_id;
}


void
rocm_binary_uri_list_init
(
  void
)
{
  spinlock_init(&rocm_binary_list_lock);
}
