// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
//
// File:
//   module-ignore-map.c
//
// Purpose:
//   implementation of a map of load modules that should be omitted
//   from call paths for synchronous samples
//
//
//***************************************************************************

//***************************************************************************
// system includes
//***************************************************************************

#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>   // open
#include <dlfcn.h>  // dlopen
#include <limits.h>  // PATH_MAX
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>



//***************************************************************************
// elfutils includes
//***************************************************************************

#include <elf.h>
#include <libelf.h>
#include <gelf.h>



//***************************************************************************
// local includes
//***************************************************************************

#include "../common/lean/pfq-rwlock.h"
#include "libmonitor/monitor.h"
#include "loadmap.h"
#include "module-ignore-map.h"



//***************************************************************************
// macros
//***************************************************************************

#define MODULE_IGNORE_DEBUG 0

#if MODULE_IGNORE_DEBUG
#define PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define PRINT(...)
#endif

#define MODULES_MAX 1024



//***************************************************************************
// static data
//***************************************************************************

static const char *IGNORE_FNS[] = {
  "cuLaunchKernel",
  "cudaLaunchKernel",
  "cuptiActivityEnable",
  "rocprofiler_iterate_info",
  "roctracer_set_properties",  // amd roctracer library
  "amd_dbgapi_initialize",     // amd debug library
  "hipKernelNameRefByPtr",     // amd hip runtime
  "hsa_init",                  // amd hsa runtime
  "hpcrun_malloc",             // hpcrun library
  "clIcdGetPlatformIDsKHR",    // libigdrcl.so(intel opencl)
  "clGetPlatformInfo",         // OpenCL
  "zeKernelCreate"             // libze_intel_gpu.so (intel L0) ISSUE: not getting ignored
};

#define NUM_FNS (sizeof IGNORE_FNS / sizeof IGNORE_FNS[0])

static load_module_t *modules[MODULES_MAX];

static unsigned int modules_cnt = 0;

static pfq_rwlock_t modules_lock;



//***************************************************************************
// private operations
//***************************************************************************

static int
pseudo_module_p
(
  char *name
)
{
    // last character in the name
    char lastchar = 0;  // get the empty string case right

    while (*name) {
      lastchar = *name;
      name++;
    }

    // pseudo modules have [name] in /proc/self/maps
    // because we store [vdso] in hpctooolkit's measurement directory,
    // it actually has the name /path/to/measurement/directory/[vdso].
    // checking the last character tells us it is a virtual shared library.
    return lastchar == ']';
}


static int
search_functions_in_module(Elf *e, GElf_Shdr* secHead, Elf_Scn *section)
{
  Elf_Data *data;
  char *symName;
  uint64_t count;
  GElf_Sym curSym;
  uint64_t i, ii,symType, symBind;

  data = elf_getdata(section, NULL);           // use it to get the data
  if (data == NULL || secHead->sh_entsize == 0) return -1;
  count = (secHead->sh_size)/(secHead->sh_entsize);
  for (ii=0; ii<count; ii++) {
    gelf_getsym(data, ii, &curSym);
    symName = elf_strptr(e, secHead->sh_link, curSym.st_name);
    symType = GELF_ST_TYPE(curSym.st_info);
    symBind = GELF_ST_BIND(curSym.st_info);

    // the .dynsym section can contain undefined symbols that represent imported symbols.
    // We need to find functions defined in the module.
    if ( (symType == STT_FUNC) && (symBind == STB_GLOBAL) && (curSym.st_value != 0)) {
      for (i = 0; i < NUM_FNS; ++i) {
        if (strcmp(symName, IGNORE_FNS[i]) == 0) {
          return i;
        }
      }
    }
        }
  return -1;
}



//***************************************************************************
// interface operations
//***************************************************************************

void
module_ignore_map_init
(
 void
)
{
  modules_cnt = 0;
  pfq_rwlock_init(&modules_lock);
}


bool
module_ignore_map_module_id_lookup
(
 uint16_t module_id
)
{
  // Read path
  size_t i;
  bool result = false;
  pfq_rwlock_read_lock(&modules_lock);
  for (i = 0; i < modules_cnt; ++i) {
    if (modules[i]->id == module_id) {
      /* current module should be ignored */
      result = true;
      break;
    }
  }
  pfq_rwlock_read_unlock(&modules_lock);
  return result;
}


bool
module_ignore_map_module_lookup
(
 load_module_t *module
)
{
  return module_ignore_map_lookup(module->dso_info->start_addr,
                                  module->dso_info->end_addr);
}


bool
module_ignore_map_inrange_lookup
(
 void *addr
)
{
  return module_ignore_map_lookup(addr, addr);
}


bool
module_ignore_map_lookup
(
 void *start,
 void *end
)
{
  // Read path
  size_t i;
  bool result = false;
  pfq_rwlock_read_lock(&modules_lock);
  for (i = 0; i < modules_cnt; ++i) {
    if (modules[i]->dso_info->start_addr <= start &&
        modules[i]->dso_info->end_addr >= end) {
      /* current module should be ignored */
      result = true;
      break;
    }
  }
  pfq_rwlock_read_unlock(&modules_lock);
  return result;
}


bool
module_ignore_map_ignore
(
  load_module_t* lm
)
{
  if (lm == NULL) return false;

  // Update path
  // Only one thread could update the flag,
  // Guarantee dlopen modules before notification are updated.
  bool result = false;
  pfq_rwlock_node_t me;
  pfq_rwlock_write_lock(&modules_lock, &me);


  load_module_t *module = lm;

  if (!pseudo_module_p(module->name)) {
    // Ignore the module if cannot resolve the path
    char resolved_path[PATH_MAX];
    if (realpath(module->name, resolved_path) == NULL) {
      pfq_rwlock_write_unlock(&modules_lock, &me);
      return result;
    }

    int fd = open (resolved_path, O_RDONLY);
    if (fd < 0) {
      pfq_rwlock_write_unlock(&modules_lock, &me);
      return false;
    }
    struct stat stat;
    if (fstat (fd, &stat) < 0) {
      close(fd);
      pfq_rwlock_write_unlock(&modules_lock, &me);
      return false;
    }

    char* buffer = (char*) mmap (NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (buffer == NULL) {
      close(fd);
      pfq_rwlock_write_unlock(&modules_lock, &me);
      return false;
    }
    elf_version(EV_CURRENT);
    Elf *elf = elf_memory(buffer, stat.st_size);
    Elf_Scn *scn = NULL;
    GElf_Shdr secHead;

    while ((scn = elf_nextscn(elf, scn)) != NULL) {
      gelf_getshdr(scn, &secHead);
      // Only search .dynsym section
      if (secHead.sh_type != SHT_DYNSYM) continue;
      int module_ignore_index = search_functions_in_module(elf, &secHead, scn);
      if (module_ignore_index != -1) {
        assert(modules_cnt < MODULES_MAX);
        modules[modules_cnt++] = module; // append module at end of table
        result = true;
        break;
      }
    }
    munmap(buffer, stat.st_size);
    close(fd);
  }
  pfq_rwlock_write_unlock(&modules_lock, &me);
  return result;
}


bool
module_ignore_map_delete
(
 load_module_t* lm
)
{
  size_t i;
  bool result = false;
  pfq_rwlock_node_t me;
  pfq_rwlock_write_lock(&modules_lock, &me);
  for (i = 0; i < modules_cnt; ++i) {
    if (modules[i] == lm) {
      modules[i] = modules[modules_cnt - 1]; // swap last into current slot
      modules_cnt--; // decrement count
      break;
    }
  }
  pfq_rwlock_write_unlock(&modules_lock, &me);
  return result;
}
