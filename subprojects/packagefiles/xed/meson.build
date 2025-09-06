# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

project(
  'xed',
  'c',
  version: '2023.10.11',
  default_options: ['default_library=static', 'c_std=c99'],
  meson_version: '>=1.1.0',
)

fs = import('fs')

_pyver = ['>=3.8', '<4']
python = find_program('python3', 'python', version: _pyver, required: false)
if not python.found()
  _pynames = []
  foreach ver : range(8, 15)
    _pynames += [f'python3.@ver@']
  endforeach
  python = find_program(_pynames, version: _pyver)
endif

_public_incdir = get_option('includedir') / 'xed'


# There are several dependencies between the options. Validate that they are upheld.
if not get_option('avx')
  assert(not get_option('ivb'), 'Requires AVX')
  assert(not get_option('hsw'), 'Requires AVX')
  assert(not get_option('bdw'), 'Requires AVX')
  assert(not get_option('skl'), 'Requires AVX')
  assert(not get_option('skx'), 'Requires AVX')
  assert(not get_option('clx'), 'Requires AVX')
  assert(not get_option('cpx'), 'Requires AVX')
  assert(not get_option('tgl'), 'Requires AVX')
  assert(not get_option('adl'), 'Requires AVX')
  assert(not get_option('spr'), 'Requires AVX')
  assert(not get_option('cnl'), 'Requires AVX')
  assert(not get_option('icl'), 'Requires AVX')
  assert(not get_option('knl'), 'Requires AVX')
  assert(not get_option('knm'), 'Requires AVX')
  assert(not get_option('gnr'), 'Requires AVX')
  assert(not get_option('srf'), 'Requires AVX')
  assert(not get_option('arl'), 'Requires AVX')
  assert(not get_option('lnl'), 'Requires AVX')
  assert(not get_option('cwf'), 'Requires AVX')
  assert(not get_option('ptl'), 'Requires AVX')
  assert(not get_option('emr'), 'Requires AVX')
  assert(not get_option('future'), 'Requires AVX')
  assert(not get_option('avx512'), 'Requires AVX')
endif
if not get_option('knl')
  assert(not get_option('knm'), 'Requires KNL')
endif
if not get_option('avx512')
  assert(not get_option('skx'), 'Requires AVX512')
  assert(not get_option('clx'), 'Requires AVX512')
  assert(not get_option('cpx'), 'Requires AVX512')
  assert(not get_option('cnl'), 'Requires AVX512')
  assert(not get_option('icl'), 'Requires AVX512')
  assert(not get_option('tgl'), 'Requires AVX512')
  assert(not get_option('spr'), 'Requires AVX512')
  assert(not get_option('knl'), 'Requires AVX512')
  assert(not get_option('knm'), 'Requires AVX512')
  assert(not get_option('gnr'), 'Requires AVX512')
  assert(not get_option('future'), 'Requires AVX512')
endif
if not get_option('ivb')
  assert(not get_option('hsw'), 'Requires IVB')
endif
if not get_option('hsw')
  assert(not get_option('bdw'), 'Requires HSW')
endif
if not get_option('bdw')
  assert(not get_option('skl'), 'Requires BDW')
endif
if not get_option('skl')
  assert(not get_option('skx'), 'Requires SKL')
endif
if not get_option('skx')
  assert(not get_option('cnl'), 'Requires SKX')
  assert(not get_option('clx'), 'Requires SKX')
  assert(not get_option('cpx'), 'Requires SKX')
endif
if not get_option('cnl')
  assert(not get_option('icl'), 'Requires CNL')
endif
if not get_option('icl')
  assert(not get_option('tgl'), 'Requires ICL')
  assert(not get_option('lakefield'), 'Requires ICL')
endif
if not get_option('tgl')
  assert(not get_option('cet'), 'Requires TGL')
  assert(not get_option('spr'), 'Requires TGL')
endif
if not get_option('adl')
  assert(not get_option('arl'), 'Requires ADL')
endif
if not get_option('spr')
  assert(not get_option('emr'), 'Requires SPR')
endif
if not get_option('emr')
  assert(not get_option('gnr'), 'Requires EMR')
endif
if not get_option('arl')
  assert(not get_option('lnl'), 'Requires ARL')
endif
if not get_option('srf')
  assert(not get_option('cwf'), 'Requires CWF')
endif
if not get_option('lnl')
  assert(not get_option('ptl'), 'Requires LNL')
endif
if not get_option('gnr') or not get_option('cwf') or not get_option('ptl')
  assert(not get_option('future'), 'Requires GNR and CWF and PTL')
endif


# Set a whole bunch of #defines in the project for our use
_cdata = configuration_data()

_cdata.set_quoted('XED_VERSION', meson.project_version())
add_project_arguments('-DXED_VERSION=' + _cdata.get('XED_VERSION'), language: 'c')

if get_option('default_library') in ['shared', 'both']
  _cdata.set('XED_DLL', true)
  add_project_arguments('-DXED_DLL', language: 'c')
endif

if get_option('messages')
  _cdata.set('XED_MESSAGES', true)
  add_project_arguments('-DXED_MESSAGES', language: 'c')
endif
if get_option('buildtype').startswith('debug')
  _cdata.set('XED_ASSERTS', true)
  add_project_arguments('-DXED_ASSERTS', language: 'c')
endif

if get_option('amd_enabled')
  _cdata.set('XED_AMD_ENABLED', true)
  add_project_arguments('-DXED_AMD_ENABLED', language: 'c')
endif
if get_option('via_enabled')
  _cdata.set('XED_VIA_ENABLED', true)
  add_project_arguments('-DXED_VIA_ENABLED', language: 'c')
endif
if get_option('avx')
  _cdata.set('XED_AVX', true)
  add_project_arguments('-DXED_AVX', language: 'c')
endif
if get_option('knl') or get_option('knm') or get_option('skx') or get_option('clx') \
  or get_option('cpx') or get_option('cnl') or get_option('icl') or get_option('tgl') \
  or get_option('spr') or get_option('gnr')
  _cdata.set('XED_SUPPORTS_AVX512', true)
  add_project_arguments('-DXED_SUPPORTS_AVX512', language: 'c')
endif
if get_option('mpx')
  _cdata.set('XED_MPX', true)
  add_project_arguments('-DXED_MPX', language: 'c')
endif
if get_option('cet')
  _cdata.set('XED_CET', true)
  add_project_arguments('-DXED_CET', language: 'c')
endif
if get_option('future')
  _cdata.set('XED_APX', true)
  add_project_arguments('-DXED_APX', language: 'c')
endif
_cdata.set('XED_SUPPORTS_SHA', true)
if get_option('icl')
  _cdata.set('XED_SUPPORTS_WBNOINVD', true)
  add_project_arguments('-DXED_SUPPORTS_WBNOINVD', language: 'c')
endif

if get_option('decoder')
  _cdata.set('XED_DECODER', true)
  add_project_arguments('-DXED_DECODER', language: 'c')
endif
if get_option('encoder')
  _cdata.set('XED_ENCODER', true)
  add_project_arguments('-DXED_ENCODER', language: 'c')
endif
# if get_option('enc2')
#   _cdata.set('XED_ENC2_ENCODER', true)
#   add_project_arguments('-DXED_ENC2_ENCODER', language: 'c')
# endif

# Collect the set of "configuration" files that specify which files to build
# NB: These need to be strings so we can extract the correct parent path
_cfgs = ['datafiles/files.cfg', 'datafiles/files-xregs.cfg']
if not get_option('avx')
  _cfgs += ['datafiles/files-xmm.cfg']
endif
if get_option('via_enabled')
  _cfgs += ['datafiles/via/files-via-padlock.cfg']
endif
if get_option('amd_enabled')
  _cfgs += ['datafiles/amd/files-amd.cfg']
  if get_option('avx')
    _cfgs += ['datafiles/amd/amdxop/files.cfg']
  endif
endif

if get_option('mpx')
  _cfgs += ['datafiles/mpx/files.cfg']
endif
if get_option('cet')
  _cfgs += ['datafiles/cet/files.cfg']
endif

_cfgs += [
  # Silvermont & Ivybridge
  'datafiles/rdrand/files.cfg',
  # Goldmont
  'datafiles/glm/files.cfg',
  'datafiles/sha/files.cfg',
  'datafiles/xsaveopt/files.cfg',
  'datafiles/xsaves/files.cfg',
  'datafiles/xsavec/files.cfg',
  'datafiles/clflushopt/files.cfg',
  'datafiles/rdrand/files.cfg',
  'datafiles/rdseed/files.cfg',
  'datafiles/fsgsbase/files.cfg',
  'datafiles/smap/files.cfg',
  'datafiles/xsaveopt/files.cfg',
  # Goldmont plus
  'datafiles/sgx/files.cfg',
  'datafiles/rdpid/files.cfg',
  'datafiles/pt/files.cfg',
  # Tremont
  'datafiles/tremont/files.cfg',
  'datafiles/movdir/files.cfg',
  'datafiles/waitpkg/files.cfg',
  'datafiles/cldemote/files.cfg',
]

if get_option('lakefield')
  _cfgs += ['datafiles/lakefield/files.cfg']
endif

if get_option('avx')
  _cfgs += ['datafiles/avx/files.cfg', 'datafiles/xsaveopt/files.cfg']
  if get_option('ivb')
    _cfgs += [
      'datafiles/fsgsbase/files.cfg',
      'datafiles/rdrand/files.cfg',
      'datafiles/ivbavx/files.cfg',
    ]
  endif
  if get_option('hsw')
    _cdata.set('XED_SUPPORTS_LZCNT_TZCNT', true)
    add_project_arguments('-DXED_SUPPORTS_LZCNT_TZCNT', language: 'c')
    _cfgs += [
      'datafiles/hswavx/files.cfg',
      'datafiles/hswbmi/files.cfg',
      'datafiles/hsw/files.cfg',
    ]
  endif
  if get_option('bdw')
    _cfgs += [
      'datafiles/bdw/files.cfg',
      'datafiles/smap/files.cfg',
      'datafiles/rdseed/files.cfg',
    ]
  endif
  if get_option('skl') or get_option('skx')
    _cfgs += [
      'datafiles/skl/files.cfg',
      'datafiles/sgx/files.cfg',
      'datafiles/xsaves/files.cfg',
      'datafiles/xsavec/files.cfg',
      'datafiles/clflushopt/files.cfg',
    ]
  endif
  if get_option('skx')
    _cfgs += [
      'datafiles/skx/files.cfg',
      'datafiles/pku/files.cfg',
      'datafiles/clwb/files.cfg',
    ]
  endif
  if get_option('clx')
    _cfgs += ['datafiles/clx/files.cfg', 'datafiles/vnni/files.cfg']
  endif
  if get_option('cpx')
    _cfgs += ['datafiles/cpx/files.cfg', 'datafiles/avx512-bf16/files.cfg']
  endif
  if get_option('knl')
    _cfgs += ['datafiles/knl/files.cfg']
  endif
  if get_option('knm')
    _cfgs += [
      'datafiles/knm/files.cfg',
      'datafiles/4fmaps-512/files.cfg',
      'datafiles/4vnniw-512/files.cfg',
      'datafiles/vpopcntdq-512/files.cfg',
    ]
  endif

  if get_option('skx') or get_option('knl') or get_option('knm')
    _cfgs += ['datafiles/avx512f/files.cfg', 'datafiles/avx512cd/files.cfg']
  endif
  if get_option('skx')
    _cfgs += ['datafiles/avx512-skx/files.cfg']
  endif
  if get_option('cnl')
    _cfgs += [
      'datafiles/cnl/files.cfg',
      'datafiles/sha/files.cfg',
      'datafiles/avx512ifma/files.cfg',
      'datafiles/avx512vbmi/files.cfg',
    ]
  endif
  if get_option('icl')
    _cfgs += [
      'datafiles/icl/files.cfg',
      'datafiles/wbnoinvd/files.cfg',
      'datafiles/sgx-enclv/files.cfg',
      'datafiles/pconfig/files.cfg',
      'datafiles/rdpid/files.cfg',
      'datafiles/bitalg/files.cfg',
      'datafiles/vbmi2/files.cfg',
      'datafiles/vnni/files.cfg',
      'datafiles/gfni-vaes-vpcl/files-sse.cfg',
      'datafiles/gfni-vaes-vpcl/files-avx-avx512.cfg',
      'datafiles/vpopcntdq-512/files.cfg',
      'datafiles/vpopcntdq-vl/files.cfg',
    ]
  endif
  if get_option('tgl')
    _cfgs += [
      'datafiles/tgl/files.cfg',
      'datafiles/cet/files.cfg',
      'datafiles/movdir/files.cfg',
      'datafiles/vp2intersect/files.cfg',
      'datafiles/keylocker/files.cfg',
    ]
  endif
  if get_option('adl')
    _cfgs += [
      'datafiles/adl/files.cfg',
      'datafiles/hreset/files.cfg',
      'datafiles/avx-vnni/files.cfg',
      'datafiles/keylocker/files.cfg',
      'datafiles/cldemote/files.cfg',
    ]
  endif
  if get_option('spr')
    _cfgs += [
      'datafiles/spr/files.cfg',
      'datafiles/hreset/files.cfg',
      'datafiles/uintr/files.cfg',
      'datafiles/cldemote/files.cfg',
      'datafiles/avx-vnni/files.cfg',
      'datafiles/amx-spr/files.cfg',
      'datafiles/waitpkg/files.cfg',
      'datafiles/avx512-bf16/files.cfg',
      'datafiles/enqcmd/files.cfg',
      'datafiles/tsx-ldtrk/files.cfg',
      'datafiles/serialize/files.cfg',
      'datafiles/avx512-fp16/files.cfg',
      'datafiles/evex-map5-6/files.cfg',
    ]
  endif
  if get_option('emr')
    _cfgs += ['datafiles/emr/files.cfg', 'datafiles/tdx/files.cfg']
  endif
  if get_option('gnr')
    _cfgs += [
      'datafiles/gnr/files.cfg',
      'datafiles/amx-fp16/files.cfg',
      'datafiles/amx-complex/files.cfg',
      'datafiles/iprefetch/files.cfg',
    ]
  endif
  if get_option('srf')
    _cfgs += [
      'datafiles/srf/files.cfg',
      'datafiles/avx-ifma/files.cfg',
      'datafiles/avx-ne-convert/files.cfg',
      'datafiles/avx-vnni-int8/files.cfg',
      'datafiles/cmpccxadd/files.cfg',
      'datafiles/msrlist/files.cfg',
      'datafiles/wrmsrns/files.cfg',
      'datafiles/uintr/files.cfg',
      'datafiles/enqcmd/files.cfg',
    ]
  endif
  if get_option('cwf')
    _cfgs += [
      'datafiles/cwf/files.cfg',
      'datafiles/user-msr/files.cfg',
      'datafiles/iprefetch/files.cfg',
      'datafiles/avx-vnni-int16/files.cfg',
      'datafiles/sha512/files.cfg',
      'datafiles/sm3/files.cfg',
      'datafiles/sm4/files.cfg',
      'datafiles/vex-map7/files.cfg',
    ]
  endif
  if get_option('arl')
    _cfgs += [
      'datafiles/arrow-lake/files.cfg',
      'datafiles/uintr/files.cfg',
      'datafiles/avx-ifma/files.cfg',
      'datafiles/avx-ne-convert/files.cfg',
      'datafiles/avx-vnni-int8/files.cfg',
      'datafiles/cmpccxadd/files.cfg',
      'datafiles/avx-vnni-int16/files.cfg',
      'datafiles/sha512/files.cfg',
      'datafiles/sm3/files.cfg',
      'datafiles/sm4/files.cfg',
    ]
  endif
  if get_option('lnl')
    _cfgs += ['datafiles/lunar-lake/files.cfg', 'datafiles/pbndkb/files.cfg']
  endif
  if get_option('ptl')
    _cfgs += [
      'datafiles/ptl/files.cfg',
      'datafiles/iprefetch/files.cfg',
      'datafiles/msrlist/files.cfg',
      'datafiles/wrmsrns/files.cfg',
      'datafiles/fred/files.cfg',
    ]
  endif
  if get_option('future')
    _cfgs += [
      'datafiles/future/files.cfg',
      'datafiles/apx-f/files.cfg',
      'datafiles/rao-int/files.cfg',
    ]
  endif
endif

# Deduplicate any files that are repeated in the list
_cfgs_dedup = []
foreach cfg : _cfgs
  if cfg not in _cfgs_dedup
    _cfgs_dedup += [cfg]
  endif
endforeach
_cfgs = _cfgs_dedup

# Process the configuration files to collect together all the sources etc.
_files = {
  'dec-spine': [],
  'dec-instructions': [],
  'enc2-instructions': [],
  'enc-instructions': [],
  'dec-patterns': [],
  'enc-patterns': [],
  'enc-dec-patterns': [],  # decode patterns used for encode
  'fields': [],
  'state': [],
  'registers': [],
  'widths': [],
  'extra-widths': [],
  'pointer-names': [],
  'element-types': [],
  'element-type-base': [],
  'chip-models': [],
  'conversion-table': [],
  'cpuid': [],
  'map-descriptions': [],
  'errors': [],
}
_priority = {}
_enc2_exclude = []
_source_priority = {}
_sources_to_remove = []
_sources_dict = {}
foreach cfg : _cfgs
  debug(f'[EXIF PROCESSING] @0@'.format(cfg))

  foreach line : fs.read(cfg).split('\n')
    _rawwords = line.split('#')[0].strip().split(':')
    if _rawwords == ['']
      continue
    endif
    _words = []
    foreach rawword : _rawwords
      _words += [rawword.strip()]
    endforeach

    if _words[0] in [
      'clear',
      'replace-source',
      'remove',
      'add-tests',
      'enc2-instructions',
    ]
      assert(false, 'command @0@ unimplemented'.format(_words[0]))
    elif _words[0] == 'define'
      _cdata.set(_words[1].split('=')[0], _words[1].split('=').get(1, true))
      add_project_arguments('-D' + _words[1], language: 'c')
    elif _words[0] == 'remove-source'
      _sources_to_remove += _words[2]
    elif _words[0] == 'add-source'
      assert(_words[2].startswith('%(xed_dir)s/'))
      _src = files(_words[2].substring(12))
      debug('[CONSIDERING SOURCE] <PATH> @0@ @1@'.format(_words[1], _words.get(3, '1')))
      if _source_priority.get(_words[1], 0) < _words.get(3, '1').to_int()
        debug('[ADDING SOURCE] <PATH> @0@ @1@'.format(_words[1], _words.get(3, '1')))
        _sources_dict += {_words[1]: _src}
        _source_priority += {_words[1]: _words.get(3, '1').to_int()}
      endif
    elif _words[0] == 'add'
      if _priority.get(_words[1], 1) > _words.get(3, '1').to_int()
        debug(
          '[Skipping low priority file for type @0@:  @1@]'.format(_words[1], _words[2]),
        )
        continue
      endif
      if _priority.get(_words[1], 1) < _words.get(3, '1').to_int()
        debug('[Clearing file list for type @0@: [ <PATHS> ]]'.format(_words[1]))
        _files += {_words[1]: []}
        _priority += {_words[1]: _words.get(3, '1').to_int()}
      endif
      _files += {_words[1]: _files[_words[1]] + files(fs.parent(cfg) / _words[2])}
    elif _words[0] == 'no-enc2-instructions'
      _enc2_exclude += files(fs.parent(cfg) / _words[1])
    else
      debug(
        'Command from @0@: @1@, appending file @2@'.format(
          cfg,
          _words,
          fs.parent(cfg) / _words[1],
        ),
      )
      _files += {
        _words[0]: _files.get(_words[0], []) + files(fs.parent(cfg) / _words[1]),
      }
    endif
  endforeach
endforeach
# if get_option('enc2')
#   _enc2_files = []
#   foreach f : _files['dec-instructions']
#     if f not in _enc2_exclude
#       _enc2_files += [f]
#     endif
#   endforeach
#   _files += {'enc2-instructions': _files.get('enc2-instructions', []) + _enc2_files}
# endif


# Generate a header with all the configuration we've collected so far.
debug('[EMIT BUILD DEFINES HEADER FILE]')
_build_defs_h = configure_file(
  output: 'xed-build-defines.h',
  configuration: _cdata,
  output_format: 'c',
  install: true,
  install_dir: _public_incdir,
)


# Concatenate the input ISA files based on their group
_concat_files = {}
_concat = [python, files('concat.py')]
foreach group, isafiles : _files
  if isafiles.length() > 0
    _concat_files += {
      group: custom_target(
        output: f'all-@group@.txt',
        input: isafiles,
        command: [_concat, '@SOURCE_ROOT@', '@OUTPUT@', '@INPUT@'],
        build_by_default: true,
      ),
    }
  else
    _concat_files += {
      group: configure_file(output: f'all-@group@.txt', configuration: {}),
    }
  endif
endforeach


# Generate all the decoder code
_dec_cmd = [files('pysrc/generator.py')]
if get_option('limit_strings')
  _dec_cmd += ['--limit-enum-strings']
endif
_dec_cmd += [
  '--spine',
  _concat_files['dec-spine'],
  '--isa',
  _concat_files['dec-instructions'],
  '--patterns',
  _concat_files['dec-patterns'],
  '--input-fields',
  _concat_files['fields'],
  '--input-state',
  _concat_files['state'],
  '--chip-models',
  _concat_files['chip-models'],
  '--ctables',
  _concat_files['conversion-table'],
  '--input-regs',
  _concat_files['registers'],
  '--input-widths',
  _concat_files['widths'],
  '--input-extra-widths',
  _concat_files['extra-widths'],
  '--input-element-types',
  _concat_files['element-types'],
  '--input-element-type-base',
  _concat_files['element-type-base'],
  '--input-pointer-names',
  _concat_files['pointer-names'],
  '--cpuid',
  _concat_files['cpuid'],
  '--map-descriptions',
  _concat_files['map-descriptions'],
  '--input-errors',
  _concat_files['errors'],
  '--gendir',
  '@PRIVATE_DIR@',
  '--xeddir',
  meson.project_source_root(),
]
if get_option('compress_operands')
  _dec_cmd += ['--compress-operands']
endif
if get_option('add_orphan_inst_to_future_chip')
  _dec_cmd += ['--add-orphan-inst-to-future-chip']
endif
_dec_srcs = custom_target(
  'decoder',
  output: [
    'xed-operand-ctype-enum.c',
    'xed-operand-ctype-enum.h',
    'xed-operand-ctype-map.h',
    'xed-operand-ctype-map.c',
    'xed-reg-enum.c',
    'xed-reg-enum.h',
    'xed-reg-class-enum.c',
    'xed-reg-class-enum.h',
    'xed-init-reg-class.c',
    'xed-operand-width-enum.c',
    'xed-operand-width-enum.h',
    'xed-init-width.c',
    'xed-operand-element-type-enum.c',
    'xed-operand-element-type-enum.h',
    'xed-operand-element-xtype-enum.c',
    'xed-operand-element-xtype-enum.h',
    'xed-init-operand-type-mappings.c',
    'xed-init-pointer-names.h',
    'xed-init-pointer-names.c',
    'xed-error-enum.c',
    'xed-error-enum.h',
    'xed-operand-convert-init.c',
    'xed-iclass-enum.c',
    'xed-iclass-enum.h',
    'xed-rep-map.c',
    'xed-iform-enum.c',
    'xed-iform-enum.h',
    'xed-iformfl-enum.c',
    'xed-iformfl-enum.h',
    'xed-iform-max.c',
    'xed-iclass-string.c',
    'xed-iform-map-init.c',
    'xed-exception-enum.c',
    'xed-exception-enum.h',
    'xed-init-operand-data.c',
    'xed-init-operand-sequences.c',
    'xed-init-inst-table-data.c',
    'xed-init-inst-table-0.c',
    'xed-init-inst-table.c',
    'xed-attributes-list.c',
    'xed-nonterminal-enum.c',
    'xed-nonterminal-enum.h',
    'xed-operand-enum.c',
    'xed-operand-enum.h',
    'xed-operand-type-enum.c',
    'xed-operand-type-enum.h',
    'xed-attribute-enum.c',
    'xed-attribute-enum.h',
    'xed-category-enum.c',
    'xed-category-enum.h',
    'xed-extension-enum.c',
    'xed-extension-enum.h',
    'xed-machine-mode-enum.c',
    'xed-machine-mode-enum.h',
    'xed-syntax-enum.c',
    'xed-syntax-enum.h',
    'xed-operand-action-enum.c',
    'xed-operand-action-enum.h',
    'xed-operand-visibility-enum.c',
    'xed-operand-visibility-enum.h',
    'xed-address-width-enum.c',
    'xed-address-width-enum.h',
    'xed-flag-enum.c',
    'xed-flag-enum.h',
    'xed-reg-role-enum.c',
    'xed-reg-role-enum.h',
    'xed-flag-action-enum.c',
    'xed-flag-action-enum.h',
    'xed-attributes-init.c',
    'xed-gen-table-defs.h',
    'xed-chip-enum.c',
    'xed-chip-enum.h',
    'xed-isa-set-enum.c',
    'xed-isa-set-enum.h',
    'xed-chip-features-table.c',
    'xed-chip-features-table.h',
    'xed-operand-convert-enum.c',
    'xed-operand-convert-enum.h',
    'xed-convert-table-init.c',
    'xed-convert-table-init.h',
    'xed-operand-storage.h',
    'xed-classifiers.c',
    'xed-ild-modrm.h',
    'xed-ild-eosz.c',
    'xed-ild-eosz.h',
    'xed-ild-eosz-getters.h',
    'xed-ild-easz.c',
    'xed-ild-easz.h',
    'xed-ild-easz-getters.h',
    'xed-operand-accessors.h',
    'xed-operand-accessors.c',
    'xed-ild-imm-l3.c',
    'xed-ild-imm-l3.h',
    'xed-ild-imm-l2.h',
    'xed-ild-imm-l1.h',
    'xed-ild-imm-bytes.h',
    'xed-ild-disp-l3.c',
    'xed-ild-disp-l3.h',
    'xed-ild-disp-l2.h',
    'xed-ild-disp-l1.h',
    'xed-ild-disp-bytes.h',
    'xed3-phash-vv0.h',
    'xed3-phash-lu-vv0.c',
    'xed3-phash-lu-vv0.h',
    'xed3-phash-vv1.h',
    'xed3-phash-lu-vv1.c',
    'xed3-phash-lu-vv1.h',
    'xed3-phash-vv2.h',
    'xed3-phash-lu-vv2.c',
    'xed3-phash-lu-vv2.h',
    'xed3-phash-vv3.h',
    'xed3-phash-lu-vv3.c',
    'xed3-phash-lu-vv3.h',
    'xed3-operand-lu.h',
    'xed3-operand-lu.c',
    'xed3-phash.h',
    'xed3-nt-capture.h',
    'xed3-chain-capture.h',
    'xed3-chain-capture-lu.h',
    'xed3-op-chain-capture.h',
    'xed3-op-chain-capture-lu.h',
    'xed3-dynamic-part1-capture.h',
    'xed-ild-enum.c',
    'xed-ild-enum.h',
    'xed-mapu-enum.c',
    'xed-mapu-enum.h',
    'xed-map-feature-tables.h',
    'xed-cpuid-rec-enum.c',
    'xed-cpuid-rec-enum.h',
    'xed-cpuid-group-enum.c',
    'xed-cpuid-group-enum.h',
    'xed-cpuid-tables.c',
    'xed-flags-simple.c',
    'xed-flags-complex.c',
    'xed-flags-actions.c',
  ],
  install: true,
  install_dir: [
    false,
    _public_incdir,
    _public_incdir,
    false,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    false,
    _public_incdir,
    false,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    false,
    _public_incdir,
    false,
    false,
    _public_incdir,
    false,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    false,
    false,
    false,
    _public_incdir,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    false,
    false,
    _public_incdir,
    false,
    _public_incdir,
    _public_incdir,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    _public_incdir,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    false,
    _public_incdir,
    false,
    _public_incdir,
    false,
    false,
    false,
    false,
  ],
  depend_files: files(
    'datafiles/xed-addr-width-enum.txt',
    'datafiles/xed-flag-action-enum.txt',
    'datafiles/xed-flag-enum.txt',
    'datafiles/xed-machine-modes-enum.txt',
    'datafiles/xed-operand-action-enum.txt',
    'datafiles/xed-operand-visibility-enum.txt',
    'datafiles/xed-reg-role.enum.txt',
    'datafiles/xed-syntax-enum.txt',
    'pysrc/actions.py',
    'pysrc/actions_codegen.py',
    'pysrc/chipmodel.py',
    'pysrc/classifier.py',
    'pysrc/codegen.py',
    'pysrc/ctables.py',
    'pysrc/dec_dyn.py',
    'pysrc/encutil.py',
    'pysrc/enum_txt_writer.py',
    'pysrc/enumer.py',
    'pysrc/flag_gen.py',
    'pysrc/generator.py',
    'pysrc/genutil.py',
    'pysrc/hashfks.py',
    'pysrc/hashlin.py',
    'pysrc/hashmul.py',
    'pysrc/hlist.py',
    'pysrc/ild.py',
    'pysrc/ild_cdict.py',
    'pysrc/ild_codegen.py',
    'pysrc/ild_disp.py',
    'pysrc/ild_easz.py',
    'pysrc/ild_eosz.py',
    'pysrc/ild_imm.py',
    'pysrc/ild_info.py',
    'pysrc/ild_modrm.py',
    'pysrc/ild_nt.py',
    'pysrc/ild_phash.py',
    'pysrc/ild_storage.py',
    'pysrc/map_info_rdr.py',
    'pysrc/metaenum.py',
    'pysrc/operand_storage.py',
    'pysrc/opnd_types.py',
    'pysrc/opnds.py',
    'pysrc/patterns.py',
    'pysrc/refine_regs.py',
    'pysrc/slash_expand.py',
    'pysrc/tup2int.py',
    'pysrc/verbosity.py',
    'pysrc/xedhash.py',
  ),
  command: [
    python,
    files('generator-wrapper.py'),
    '@PRIVATE_DIR@',
    '@OUTDIR@',
    '--outputs',
    '@OUTPUT@',
    '--',
  ] + _dec_cmd,
)

_dec_headers = []
foreach src : _dec_srcs.to_list()
  if src.full_path().endswith('.h')
    _dec_headers += [src]
  endif
endforeach


# Generate all the encoder code
_enc_srcs = []
_enc_headers = []
if get_option('encoder')
  _enc_cmd = [
    files('pysrc/read-encfile.py'),
    '--isa',
    _concat_files['enc-instructions'],
    '--enc-patterns',
    _concat_files['enc-patterns'],
    '--enc-dec-patterns',
    _concat_files['enc-dec-patterns'],
    '--input-fields',
    _concat_files['fields'],
    '--input-state',
    _concat_files['state'],
    '--input-regs',
    _concat_files['registers'],
    '--map-descriptions',
    _concat_files['map-descriptions'],
    '--chip-models',
    _concat_files['chip-models'],
    '--chip',
    get_option('encoder_chip'),
    '--gendir',
    '@PRIVATE_DIR@',
    '--xeddir',
    meson.project_source_root(),
  ]
  if not get_option('amd_enabled')
    _enc_cmd += ['--no=amd']
  endif
  _enc_srcs = custom_target(
    'encoder',
    output: [
      'xed-enc-operand-lu.h',
      'xed-enc-operand-lu.c',
      'xed-encoder-order-init.c',
      'xed-enc-groups.h',
      'xed-enc-groups.c',
      'xed-enc-patterns.h',
      'xed-enc-patterns.c',
      'xed-encoder-pattern-lu.c',
      'xed-encoder-iforms-init.c',
      'xed-encoder-init.c',
      'xed-encoder.h',
      'xed-encoder-0.c',
      'xed-encoder-1.c',
      'xed-encoder-2.c',
      'xed-encoder-3.c',
      'xed-encoder-4.c',
      'xed-encoder-5.c',
      'xed-encoder-iforms.h',
      'xed-encoder-gen-defs.h',
    ],
    install: true,
    install_dir: [
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      _public_incdir,
      _public_incdir,
    ],
    depend_files: files(
      'pysrc/actions.py',
      'pysrc/actions.py',
      'pysrc/actions_codegen.py',
      'pysrc/chipmodel.py',
      'pysrc/codegen.py',
      'pysrc/constraint_vec_gen.py',
      'pysrc/encutil.py',
      'pysrc/func_gen.py',
      'pysrc/genutil.py',
      'pysrc/hashfks.py',
      'pysrc/hashlin.py',
      'pysrc/hashmul.py',
      'pysrc/ild_codegen.py',
      'pysrc/ild_info.py',
      'pysrc/ild_nt.py',
      'pysrc/ild_phash.py',
      'pysrc/ins_emit.py',
      'pysrc/map_info_rdr.py',
      'pysrc/nt_func_gen.py',
      'pysrc/operand_storage.py',
      'pysrc/opnds.py',
      'pysrc/patterns.py',
      'pysrc/read-encfile.py',
      'pysrc/refine_regs.py',
      'pysrc/scatter.py',
      'pysrc/slash_expand.py',
      'pysrc/tup2int.py',
      'pysrc/verbosity.py',
      'pysrc/xedhash.py',
    ),
    command: [
      python,
      files('generator-wrapper.py'),
      '@PRIVATE_DIR@',
      '@OUTDIR@',
      '--outputs',
      '@OUTPUT@',
      '--',
    ] + _enc_cmd,
  )

  foreach src : _enc_srcs.to_list()
    if src.full_path().endswith('.h')
      _enc_headers += [src]
    endif
  endforeach
endif


# Collect together the final batch of sources to include
_nongen_srcs = files(
  'src/common/xed-chip-modes-override.c',
  'src/common/xed-chip-modes.c',
  'src/common/xed-cpuid.c',
  'src/common/xed-flags.c',
  'src/common/xed-immdis.c',
  'src/common/xed-immed.c',
  'src/common/xed-init.c',
  'src/common/xed-isa-set.c',
  'src/common/xed-operand-action.c',
  'src/common/xed-operand-values-interface-repl.c',
  'src/common/xed-operand-values-interface.c',
  'src/common/xed-portability.c',
  'src/common/xed-reg-class.c',
  'src/common/xed-rep-prefix.c',
  'src/common/xed-state.c',
  'src/common/xed-table-sizes.c',
  'src/common/xed-util.c',
  'src/common/xed-version.c',
)
if get_option('decoder')
  _nongen_srcs += files(
    'src/dec/xed-agen.c',
    'src/dec/xed-chip-features.c',
    'src/dec/xed-decode.c',
    'src/dec/xed-decoded-init.c',
    'src/dec/xed-decoded-inst.c',
    'src/dec/xed-disas.c',
    'src/dec/xed-iform-map.c',
    'src/dec/xed-ild-extension.c',
    'src/dec/xed-ild.c',
    'src/dec/xed-inst.c',
    'src/dec/xed3-dynamic-decode.c',
    'src/dec/xed3-static-decode.c',
  )
else
  warning('Will compile xed-iform-map-init.c unnecessarily')
endif
if get_option('encoder')
  _nongen_srcs += files(
    'src/enc/xed-encode-isa-functions.c',
    'src/enc/xed-encode.c',
    'src/enc/xed-encoder-hl.c',
  )
endif
if get_option('encoder') and get_option('decoder')
  _nongen_srcs += files('src/encdec/xed-enc-dec.c', 'src/encdec/xed-patch.c')
endif

_new_nongen_srcs = []
foreach src : _nongen_srcs
  if fs.name(src) not in _sources_to_remove
    _new_nongen_srcs += [src]
  endif
endforeach
_nongen_srcs = _new_nongen_srcs
foreach _, src : _sources_dict
  _nongen_srcs += [src]
endforeach

# Install the headers
subdir('include/public/xed')

# Compile the libxed library
add_project_arguments('-DXED_BUILD', language: 'c')
libxed = library(
  'xed',
  _nongen_srcs,
  _dec_srcs,
  _enc_srcs,
  _build_defs_h,
  _gen_headers,
  include_directories: include_directories(
    'include/private',
    'include/public/xed',
    'include/public',
  ),
  install: true,
)
xed_dep = declare_dependency(
  sources: _gen_headers,
  link_with: libxed,
  include_directories: _incdirs,
)
meson.override_dependency('xed', xed_dep)

libxed_ild = library(
  'xed-ild',
  files(
    'src/common/xed-chip-modes-override.c',
    'src/common/xed-chip-modes.c',
    'src/common/xed-isa-set.c',
    'src/dec/xed-chip-features.c',
    'src/dec/xed-ild-extension.c',
    'src/dec/xed-ild.c',
    'src/ild/xed-ild-support.c',
    'src/ild/xed-init-ild.c',
  ),
  _dec_srcs[74],  # xed-chip-features-table.c
  _dec_srcs[96],  # xed-ild-disp-l3.c
  _dec_srcs[83],  # xed-ild-eosz.c
  _dec_srcs[86],  # xed-ild-easz.c
  _dec_srcs[91],  # xed-ild-imm-l3.c
  _dec_srcs[19],  # xed-error-enum.c
  _dec_headers,
  _enc_headers,
  _gen_headers,  # #included by other files
  include_directories: include_directories(
    'include/private',
    'include/public/xed',
    'include/public',
  ),
  install: true,
)
xed_ild_dep = declare_dependency(
  sources: _gen_headers,
  link_with: libxed_ild,
  include_directories: _incdirs,
)
meson.override_dependency('xed-ild', xed_ild_dep)
