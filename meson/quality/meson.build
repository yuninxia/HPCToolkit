# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

if not get_option('dev_quality').allowed()
  subdir_done()
endif

quality_collect_targets = []
quality_codeclimate_targets = []

subdir('codechecker')

# These quality targets are generic targets that run all quality tools
alias_target('quality-collect', quality_collect_targets)
alias_target('quality-codeclimate', quality_codeclimate_targets)
