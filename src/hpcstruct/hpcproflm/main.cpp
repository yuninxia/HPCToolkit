// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

//****************************************************************************
// system include files
//****************************************************************************

#include <algorithm>
#include <exception>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>


//****************************************************************************
// local include files
//****************************************************************************

#include "Args.hpp"

#include "../../hpcprof/stdshim/filesystem.hpp"
#include "../../common/lean/hpcrun-fmt.h"
#include "../../common/diagnostics.h"
#include "../../common/StrUtil.hpp"


namespace fs = hpctoolkit::stdshim::filesystem;


//****************************************************************************
// macros
//****************************************************************************

#define GPUBIN_SUFFIX_LEN 6 // strlen("gpubin")


//****************************************************************************
// local data
//****************************************************************************

enum {
  LM_KEEP = 0,
  LM_SKIP_NO_ANALYZE,
  LM_SKIP_NULL,
  LM_SKIP_FILTER,
};

static const char *filter_strings[] = {
  "logical/",
  "kernel_symbols/",
  "/opt/aurora/.*/libmpi.so",
  0
};

static std::vector <std::string> includeVec;
static std::vector <std::string> excludeVec;


//****************************************************************************
// internal operations
//****************************************************************************

//
// Append builtin filter_strings with command-line --exclude options.
//
static void
makeFilterVec(Args & args)
{
  includeVec.clear();
  excludeVec.clear();

  StrUtil::tokenize_str(args.include_string, CLP_SEPARATOR, includeVec);
  StrUtil::tokenize_str(args.exclude_string, CLP_SEPARATOR, excludeVec);

  for (int i = 0; filter_strings[i]; i++) {
    excludeVec.push_back(std::string(filter_strings[i]));
  }
}


// Return reason for skip so we can inform the user.
static int
skipLoadMapEntry(loadmap_entry_t* x)
{
  // if this load map entry doesn't need to be analyzed
  if (!(x->flags & LOADMAP_ENTRY_ANALYZE)) return LM_SKIP_NO_ANALYZE;

  // ignore empty names
  if (x->name == NULL) return LM_SKIP_NULL;

  // check include list first, this wins over exclude
  for (unsigned int i = 0; i < includeVec.size(); i++) {
    auto patn = std::regex{".*" + includeVec[i] + ".*"};
    if (std::regex_match(x->name, patn)) {
      return LM_KEEP;
    }
  }

  // finally, exclude list
  for (unsigned int i = 0; i < excludeVec.size(); i++) {
    auto patn = std::regex{".*" + excludeVec[i] + ".*"};
    if (std::regex_match(x->name, patn)) {
      return LM_SKIP_FILTER;
    }
  }

  return LM_KEEP;
}


// for any gpubin, erase any kernel name hash following "gpubin" the name
static std::string
getLoadModuleName(loadmap_entry_t* x)
{
  std::string name(x->name);
  size_t pos = name.find("gpubin.");
  if (pos != std::string::npos) {
    // erase kernel hash suffix from the gpubin name
    name.erase(pos+GPUBIN_SUFFIX_LEN);
  }

  return name;
}


static bool
readFooter(FILE* fs, hpcrun_fmt_footer_t &footer)
{
  fseek(fs, 0, SEEK_END);
  size_t footer_position = ftell(fs) - SF_footer_SIZE;
  fseek(fs, footer_position, SEEK_SET);

  return hpcrun_fmt_footer_fread(&footer, fs) == HPCFMT_OK;
}


static bool
readLoadmap(FILE *infs, hpcrun_fmt_footer_t &footer,
            std::unordered_set<std::string> &loadModules,
            std::unordered_set<std::string> &filterModules)
{
  fseek(infs, footer.loadmap_start, SEEK_SET);

  loadmap_t loadmap_tbl;
  int ret = hpcrun_fmt_loadmap_fread(&loadmap_tbl, infs, malloc);
  if (ret != HPCFMT_OK || (uint64_t)ftell(infs) != footer.loadmap_end) {
    return false;
  }

  for (uint32_t i = 0; i < loadmap_tbl.len; i++) {
    loadmap_entry_t* x = &loadmap_tbl.lst[i];
    std::string name = getLoadModuleName(x);
    int skip = skipLoadMapEntry(x);

    if (skip == LM_KEEP) {
      loadModules.insert(name);
    }
    else if (skip == LM_SKIP_FILTER) {
      filterModules.insert(name);
    }
  }

  return true;
}


static void
processProfile(const fs::path &path, std::unordered_set<std::string> &loadModules,
               std::unordered_set<std::string> &filterModules)
{
   std::string filename = path;
   const char *fnm = filename.c_str();

   FILE* fs = hpcio_fopen_r(fnm);
   if (fs) {
    hpcrun_fmt_footer_t footer;
    bool status = readFooter(fs, footer);
    if (status) {
      status = readLoadmap(fs, footer, loadModules, filterModules);
    }
    DIAG_WMsgIf(status == false, "unable to extract loadmap from profile " << filename);
    fclose(fs);
   }
}


static int
processMeasurementsDirectory(Args &args)
{
  int status = 0;
  const fs::path path(args.measurements_directory);
  std::string pathname = path;

  if (!fs::is_directory(path)) {
    DIAG_EMsg(pathname << " is not a directory");
    status = 1;
  } else {
    std::vector<fs::path> hpcrunFiles;
    for (auto const& dir_entry : fs::directory_iterator(path)) {
      if (dir_entry.path().extension() == ".hpcrun") {
        hpcrunFiles.push_back(dir_entry.path());
      }
    }
    if (hpcrunFiles.begin() == hpcrunFiles.end()) {
      DIAG_EMsg("directory " << pathname <<  " does not contain any HPCToolkit profiles");
      status = 1;
    } else {
      std::unordered_set<std::string> loadModules;
      std::unordered_set<std::string> filterModules;

      #pragma omp parallel shared(loadModules, filterModules)
      {
        std::unordered_set<std::string> privateLoadModules;
        std::unordered_set<std::string> privateFilterModules;

        #pragma omp for
        for (size_t i = 0; i < hpcrunFiles.size(); i++) {
          processProfile(hpcrunFiles[i], privateLoadModules, privateFilterModules);
        }
        #pragma omp critical
        {
          loadModules.merge(std::move(privateLoadModules));
          filterModules.merge(std::move(privateFilterModules));
        }
      }

      // the analyzed load modules go to stdout and into all.lm
      std::vector <std::string> strVec;
      for (const auto& lm: loadModules) {
        strVec.push_back(lm);
      }
      std::sort(strVec.begin(), strVec.end(), std::less<std::string>());

      for (const auto& lm: strVec) {
        std::cout << lm << "\n";
      }

      // display the filtered modules on stderr, if any
      if (! filterModules.empty()) {
        strVec.clear();
        for (const auto& lm: filterModules) {
          strVec.push_back(lm);
        }
        std::sort(strVec.begin(), strVec.end(), std::less<std::string>());

        std::cerr << "INFO: modules not analyzed:" << std::endl;
        for (const auto& lm: strVec) {
          std::cerr << lm << "\n";
        }
        std::cerr << std::endl;
      }
    }
  }

  return status;
}



//****************************************************************************
// interface operations
//****************************************************************************

int
main(int argc, char* const* argv)
{
  int ret;

  try {
    Args args(argc, argv);  // exits if error on command line
    makeFilterVec(args);
    ret = processMeasurementsDirectory(args);
  }
  catch (const Diagnostics::Exception& x) {
    DIAG_EMsg(x.message());
    ret = 1;
  }
  catch (...) {
    DIAG_EMsg("unknown exception encountered");
    ret = 1;
  }

  return ret;
}
