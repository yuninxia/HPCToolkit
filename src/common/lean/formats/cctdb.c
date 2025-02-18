// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
//
// Purpose:
//   Low-level types and functions for reading/writing cct.db
//
//   See doc/FORMATS.md.
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

#include "cctdb.h"

#include "primitive.h"

#include <string.h>

static_assert('a' == 0x61, "Byte encoding isn't ASCII?");
static const char fmt_cctdb_magic[14] = "HPCTOOLKITctxt";
const char fmt_cctdb_footer[8] = "__ctx.db";

enum fmt_version_t fmt_cctdb_check(const char hdr[16], uint8_t* minorVer) {
  if(memcmp(hdr, fmt_cctdb_magic, sizeof fmt_cctdb_magic) != 0)
    return fmt_version_invalid;
  if(hdr[0xe] != FMT_DB_MajorVersion)
    return fmt_version_major;
  if(minorVer != NULL) *minorVer = hdr[0xf];
  if(hdr[0xf] < FMT_CCTDB_MinorVersion)
    return fmt_version_backward;
  return hdr[0xf] > FMT_CCTDB_MinorVersion
         ? fmt_version_forward : fmt_version_exact;
}

void fmt_cctdb_fHdr_read(fmt_cctdb_fHdr_t* hdr, const char d[FMT_CCTDB_SZ_FHdr]) {
  hdr->szCtxInfo = fmt_u64_read(d+0x10);
  hdr->pCtxInfo = fmt_u64_read(d+0x18);
}
void fmt_cctdb_fHdr_write(char d[FMT_CCTDB_SZ_FHdr], const fmt_cctdb_fHdr_t* hdr) {
  memcpy(d, fmt_cctdb_magic, sizeof fmt_cctdb_magic);
  d[0x0e] = FMT_DB_MajorVersion;
  d[0x0f] = FMT_CCTDB_MinorVersion;
  fmt_u64_write(d+0x10, hdr->szCtxInfo);
  fmt_u64_write(d+0x18, hdr->pCtxInfo);
}

void fmt_cctdb_ctxInfoSHdr_read(fmt_cctdb_ctxInfoSHdr_t* hdr, const char d[FMT_CCTDB_SZ_CtxInfoSHdr]) {
  hdr->pCtxs = fmt_u64_read(d+0x00);
  hdr->nCtxs = fmt_u32_read(d+0x08);
  hdr->szCtx = d[0x0c];
}
void fmt_cctdb_ctxInfoSHdr_write(char d[FMT_CCTDB_SZ_CtxInfoSHdr], const fmt_cctdb_ctxInfoSHdr_t* hdr) {
  fmt_u64_write(d+0x00, hdr->pCtxs);
  fmt_u32_write(d+0x08, hdr->nCtxs);
  d[0x0c] = FMT_CCTDB_SZ_CtxInfo;
}

void fmt_cctdb_ctxInfo_read(fmt_cctdb_ctxInfo_t* ci, const char d[FMT_CCTDB_SZ_CtxInfo]) {
  ci->valueBlock.nValues = fmt_u64_read(d+0x00);
  ci->valueBlock.pValues = fmt_u64_read(d+0x08);
  ci->valueBlock.nMetrics = fmt_u16_read(d+0x10);
  ci->valueBlock.pMetricIndices = fmt_u64_read(d+0x18);
}
void fmt_cctdb_ctxInfo_write(char d[FMT_CCTDB_SZ_CtxInfo], const fmt_cctdb_ctxInfo_t* ci) {
  fmt_u64_write(d+0x00, ci->valueBlock.nValues);
  fmt_u64_write(d+0x08, ci->valueBlock.pValues);
  fmt_u16_write(d+0x10, ci->valueBlock.nMetrics);
  memset(d+0x12, 0, 6);
  fmt_u64_write(d+0x18, ci->valueBlock.pMetricIndices);
  memset(d+0x20, 0, FMT_CCTDB_SZ_CtxInfo - 0x20);
}

void fmt_cctdb_pVal_read(fmt_cctdb_pVal_t* val, const char d[FMT_CCTDB_SZ_PVal]) {
  val->profIndex = fmt_u32_read(d+0x00);
  val->value = fmt_f64_read(d+0x04);
}
void fmt_cctdb_pVal_write(char d[FMT_CCTDB_SZ_PVal], const fmt_cctdb_pVal_t* val) {
  fmt_u32_write(d+0x00, val->profIndex);
  fmt_f64_write(d+0x04, val->value);
}

void fmt_cctdb_mIdx_read(fmt_cctdb_mIdx_t* idx, const char d[FMT_CCTDB_SZ_MIdx]) {
  idx->metricId = fmt_u16_read(d+0x00);
  idx->startIndex = fmt_u64_read(d+0x02);
}
void fmt_cctdb_mIdx_write(char d[FMT_CCTDB_SZ_MIdx], const fmt_cctdb_mIdx_t* idx) {
  fmt_u16_write(d+0x00, idx->metricId);
  fmt_u64_write(d+0x02, idx->startIndex);
}
