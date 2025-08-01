# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

_tst = find_program(files('tst-flags-effective'))
foreach name, meas : testdata_meas
  test(
    f'Flags on @name@ are effective',
    _tst,
    args: [hpctesttool, hpcprof, meas['dir']],
    suite: 'hpcprof',
  )
endforeach

_tst = find_program(files('tst-accuracy'))
foreach name, dbase : testdata_dbase
  foreach threads : [1, 3]
    test(
      f'Database from @name@ is accurate (-j@threads@)',
      _tst,
      args: [
        hpctesttool,
        dbase['dir'],
        hpcprof,
        f'-j@threads@',
        dbase['args'],
        dbase['measurements']['dir'],
      ],
      suite: 'hpcprof',
    )
  endforeach
endforeach
