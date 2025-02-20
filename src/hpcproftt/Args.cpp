// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//***************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   [The purpose of this file]
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

//************************* System Include Files ****************************

#include <iostream>
using std::cerr;
using std::endl;

#include <string>
using std::string;

//*************************** User Include Files ****************************

#include "../common/lean/gcc-attr.h"
#include "../common/hpctoolkit-version.h"

#include "Args.hpp"

#include "../common/Util.hpp"

#include "../common/diagnostics.h"
#include "../common/Trace.hpp"
#include "../common/StrUtil.hpp"

//*************************** Forward Declarations **************************

#define ARG_Throw(streamArgs) DIAG_ThrowX(Args::Exception, streamArgs)

// Cf. DIAG_Die.
#define ARG_ERROR(streamArgs)                                        \
  { std::ostringstream WeIrDnAmE;                                    \
    WeIrDnAmE << streamArgs /*<< std::ends*/;                        \
    printError(std::cerr, WeIrDnAmE.str());                          \
    exit(1); }

//***************************************************************************

static const char* usage_summary =
"profile-file [profile-file]*\n";

static const char* usage_details =
                 "hpcproftt generates textual dumps of call path profiles\n"
                 "recorded by hpcrun.  The profile list may contain one or\n"
                 "more call path profiles.\n"
                 "\n"
                 "Options:\n"
                 "  -V, --version        Print version information.\n"
                 "  -h, --help           Print this help.\n"
                 "  -l, --lm             Print the load modules only.\n"
     "  -g, --grep           Show the sparse metrics in a format that is easy to grep.\n";

#define CLP CmdLineParser
#define CLP_SEPARATOR "!!!"

// Note: Changing the option name requires changing the name in Parse()
CmdLineParser::OptArgDesc Args::optArgs[] = {
  // General
  { 'V', "version",         CLP::ARG_NONE, CLP::DUPOPT_CLOB, NULL,
     NULL },
  { 'h', "help",            CLP::ARG_NONE, CLP::DUPOPT_CLOB, NULL,
     NULL },
  { 'g', "grep",            CLP::ARG_NONE, CLP::DUPOPT_CLOB, NULL,  //YUMENG
     NULL },
  { 'l', "lm",              CLP::ARG_NONE, CLP::DUPOPT_CLOB, NULL,
     NULL },
  CmdLineParser_OptArgDesc_NULL_MACRO // SGI's compiler requires this version
};

#undef CLP

//***************************************************************************
// Args
//***************************************************************************

Args::Args()
{
  Ctor();
}


Args::Args(int argc, const char* const argv[])
{
  Ctor();
  parse(argc, argv);
}


void
Args::Ctor()
{
  obj_metricsAsPercents = true;
  obj_showSourceCode = false;
  obj_procThreshold = 1;

  Diagnostics_SetDiagnosticFilterLevel(1);


  // Analysis::Args
  prof_metrics = Analysis::Args::MetricFlg_Thread;
  profflat_computeFinalMetricValues = true;

  out_db_experiment = "";
  db_dir            = "";
  db_copySrcFiles   = false;

  out_txt           = "-";
  txt_summary       = TxtSum_fPgm | TxtSum_fLM;
  txt_srcAnnotation = false;
}


Args::~Args()
{
}


void
Args::printUsage(std::ostream& os) const
{
  os << "Usage: \n"
     << "  " << getCmd() << " " << usage_summary << endl
     << usage_details << endl;
}


void
Args::printError(std::ostream& os, const char* msg) /*const*/
{
  os << getCmd() << ": " << msg << endl
     << "Try '" << getCmd() << " --help' for more information." << endl;
}

void
Args::printError(std::ostream& os, const std::string& msg) /*const*/
{
  printError(os, msg.c_str());
}


const std::string&
Args::getCmd() /*const*/
{
  // avoid error messages with: .../bin/hpcproftt-bin
  static string cmd = "hpcproftt";
  return cmd; // parser.getCmd();
}


void
Args::parse(int argc, const char* const argv[])
{
  try {
    // -------------------------------------------------------
    // Parse the command line
    // -------------------------------------------------------
    parser.parse(optArgs, argc, argv);

    // -------------------------------------------------------
    // Sift through results, checking for semantic errors
    // -------------------------------------------------------

    // Special options that should be checked first
    if (parser.isOpt("help")) {
      printUsage(std::cerr);
      exit(1);
    }
    if (parser.isOpt("version")) {
      hpctoolkit_print_version(getCmd().c_str());
      exit(1);
    }
    if (parser.isOpt("grep")) { //YUMENG
      sm_easyToGrep = true;
    }
    if (parser.isOpt("lm")) {
      // print the load modules only
      Analysis::Util::option = Analysis::Util::OutputOption_t::Print_LoadModule_Only;
    }

    // FIXME: sanity check that options correspond to mode

    // Check for required arguments
    unsigned int numArgs = parser.getNumArgs();
    if ( !(numArgs >= 1) ) {
      ARG_ERROR("Incorrect number of arguments!");
    }

    profileFiles.resize(numArgs);
    for (unsigned int i = 0; i < numArgs; ++i) {
      profileFiles[i] = parser.getArg(i);
    }
  }
  catch (const CmdLineParser::ParseError& x) {
    ARG_ERROR(x.what());
  }
  catch (const CmdLineParser::Exception& x) {
    DIAG_EMsg(x.message());
    exit(1);
  }
  catch (const Args::Exception& x) {
    ARG_ERROR(x.what());
  }
}


// Cf. lib/analysis/ArgsHPCProf::parseArg_metric()
void
Args::parseArg_metric(Args* args, const string& value, const char* errTag)
{
  if (value == "thread") {
    if (args) {
      Analysis::Args::MetricFlg_set(args->prof_metrics,
                                    Analysis::Args::MetricFlg_Thread);
    }
  }
  else if (value == "sum") {
    if (args) {
      Analysis::Args::MetricFlg_clear(args->prof_metrics,
                                      Analysis::Args::MetricFlg_StatsAll);
      Analysis::Args::MetricFlg_set(args->prof_metrics,
                                    Analysis::Args::MetricFlg_StatsSum);
    }
  }
  else if (value == "stats") {
    if (args) {
      Analysis::Args::MetricFlg_clear(args->prof_metrics,
                                      Analysis::Args::MetricFlg_StatsSum);
      Analysis::Args::MetricFlg_set(args->prof_metrics,
                                    Analysis::Args::MetricFlg_StatsAll);
    }
  }
  else {
    ARG_Throw(errTag << ": Unexpected value received: '" << value << "'");
  }
}


void
Args::dump(std::ostream& os) const
{
  os << "Args.cmd= " << getCmd() << endl;
  Analysis::Args::dump(os);
}
