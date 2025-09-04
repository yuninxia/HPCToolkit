# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

fs = import('fs')

_nogen_headers = files(
  'xed-agen.h',
  'xed-attributes.h',
  'xed-chip-features.h',
  'xed-common-defs.h',
  'xed-common-hdrs.h',
  'xed-cpuid-rec.h',
  'xed-decode.h',
  'xed-decoded-inst-api.h',
  'xed-decoded-inst.h',
  'xed-disas.h',
  'xed-encode-check.h',
  'xed-encode-direct.h',
  'xed-encode.h',
  'xed-encoder-hl.h',
  'xed-flags.h',
  'xed-format-options.h',
  'xed-get-time.h',
  'xed-iform-map.h',
  'xed-ild.h',
  'xed-immdis.h',
  'xed-immed.h',
  'xed-init.h',
  'xed-inst.h',
  'xed-interface.h',
  'xed-isa-set.h',
  'xed-operand-action.h',
  'xed-operand-values-interface.h',
  'xed-patch.h',
  'xed-portability.h',
  'xed-print-info.h',
  'xed-reg-class.h',
  'xed-rep-prefix.h',
  'xed-state.h',
  'xed-types.h',
  'xed-util.h',
  'xed-version.h',
)
install_headers(_nogen_headers, install_dir: _public_incdir)

_cp = find_program('copy-wrapper.py')

_gen_headers = [fs.copyfile(_build_defs_h, fs.name(_build_defs_h))]
foreach header : _nogen_headers
  _gen_headers += fs.copyfile(header, fs.name(header))
endforeach
foreach header : _dec_headers + _enc_headers
  _gen_headers += custom_target(
    output: fs.name(header.full_path()),
    input: header,
    command: [_cp, '@INPUT@', '@OUTPUT@'],
  )
endforeach
_incdirs = include_directories('..')
