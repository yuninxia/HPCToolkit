# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

testdata_dbase = {}

_databases = {
  'small': {'yaml': true},
  'small.nostruct': {'yaml': true, 'args': ['--ignore-structs']},
  'loops-cputime-t': {},
  'loops-cputime-t.nostruct': {'args': ['--ignore-structs']},
  'loops-perf': {},
  'loops-perf.nostruct': {'args': ['--ignore-structs']},
  'loops-cuda-nvidia-t': {},
  'loops-cuda-nvidia-t.nostruct': {'args': ['--ignore-structs']},
  'loops-cuda-nvidiapc-t': {},
  'loops-cuda-nvidiapc-t.nostruct': {'args': ['--ignore-structs']},
  'divergence': {},
  'divergence.nostruct': {'args': ['--ignore-structs']},
  'divergence.M_none': {'stats': 'none'},
  'divergence.M_sum': {'stats': 'sum'},
  'divergence.M_normal': {'stats': 'normal'},
  'divergence.M_extrema': {'stats': 'extrema'},
  'divergence.M_normal_extrema': {'stats': 'normal,extrema'},
  'recursion-cuda-nvidiapc-t': {},
  'recursion-cuda-nvidiapc-t.nostruct': {'args': ['--ignore-structs']},
  'loops-hip-amd-t': {},
  'loops-hip-amd-t.nostruct': {'args': ['--ignore-structs']},
}

_prof = [find_program(files('prof')), hpcprof]

foreach _name, _features : _databases
  _meas = testdata_meas[_name.split('.')[0]]
  _common_args = ['--foreign', '-M', _features.get('stats', 'stats')] + _features.get(
    'args',
    [],
  )
  _args = _common_args

  _stamp = custom_target(
    output: f'@_name@.d.fresh',
    build_by_default: false,
    console: true,
    command: [_prof, _meas['freshstamp'], '@OUTPUT@', _common_args],
  )
  _dbase = {
    'measurements': _meas,
    'dir': meson.current_source_dir() / f'@_name@.d',
    'freshstamp': _stamp,
    'args': _args,
  }
  testdata_files += {
    _dbase['dir']: {
      'tag': _meas['tag'],
      'src': meson.current_build_dir() / f'@_name@.d',
      'depends': [_stamp],
    },
  }

  if _features.get('yaml', false)
    _yaml = custom_target(
      output: f'@_name@.yaml',
      build_by_default: false,
      depends: [_stamp],
      command: [
        hpctesttool,
        'yaml',
        meson.current_build_dir() / f'@_name@.d',
        '@OUTPUT@',
      ],
    )
    _dbase += {'yaml': meson.current_source_dir() / f'@_name@.yaml', 'freshyaml': _yaml}
    testdata_files += {_dbase['yaml']: {'src': _yaml, 'tag': _meas['tag']}}
  endif

  testdata_dbase += {_name: _dbase}
endforeach
