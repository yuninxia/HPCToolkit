#!/bin/bash

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

apt_packages=(
  ccache
  clang
  clang-11
  clang-12
  clang-13
  clang-14
  clang-15
  clang-format
  clang-tidy
  cmake
  g++
  g++-10
  g++-11
  g++-12
  gcc
  gcc-10
  gcc-11
  gcc-12
  git
  libboost-atomic-dev
  libboost-chrono-dev
  libboost-date-time-dev
  libboost-filesystem-dev
  libboost-graph-dev
  libboost-system-dev
  libboost-thread-dev
  libboost-timer-dev
  libbz2-dev
  libdw-dev
  libelf-dev
  libgmock-dev
  libgtest-dev
  libiberty-dev
  liblzma-dev
  libmpich-dev
  libomp-dev
  libpapi-dev
  libpfm4-dev
  libtbb-dev
  libxerces-c-dev
  libxxhash-dev
  libyaml-cpp-dev
  libzstd-dev
  make # FIXME: See https://gitlab.com/hpctoolkit/hpctoolkit/-/issues/704
  mawk # FIXME: See https://gitlab.com/hpctoolkit/hpctoolkit/-/issues/704
  ninja-build
  ocl-icd-opencl-dev
  pkg-config
  python3
  python3-dev
  python3-docutils
  python3-myst-parser
  python3-pip
  python3-sphinx
  python3-venv
  sed # FIXME: See https://gitlab.com/hpctoolkit/hpctoolkit/-/issues/704
  zlib1g-dev
)

case "$(dpkg --print-architecture)" in
amd64 | arm64) apt_packages+=(nvidia-cuda-toolkit libnvidia-compute-550) ;;
esac

# Install the required OS packages
if ! (for pkg in "${apt_packages[@]}"; do dpkg-query -s "$pkg" || exit 1; done) &>/dev/null; then
  echo -e "\e[0Ksection_start:$(date +%s):apt_install[collapsed=true]\r\e[0KInstalling required packages with apt-get..."
  DEBIAN_FRONTEND=noninteractive apt-get update -yqq || exit $?
  DEBIAN_FRONTEND=noninteractive apt-get upgrade --with-new-pkgs --yes --no-install-recommends "${apt_packages[@]}" || exit $?
  echo -e "\e[0Ksection_end:$(date +%s):apt_install\r\e[0K"
fi

# Install a up-to-date versions of Meson, using a suitable Python
echo -e "\e[0Ksection_start:$(date +%s):pip_install[collapsed=true]\r\e[0KInstalling required packages with pip..."
python3 -m pip install 'meson>=1.1.0,<2' || exit $?
echo -e "\e[0Ksection_end:$(date +%s):pip_install\r\e[0K"
