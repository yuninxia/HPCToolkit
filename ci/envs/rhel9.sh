#!/bin/bash

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

dnf_packages=(
  binutils-devel
  boost-devel
  bzip2-devel
  ccache
  clang
  clang-tools-extra
  cmake
  elfutils-devel
  gcc
  gcc-c++
  git
  gmock-devel
  gtest-devel
  libpfm-devel
  libunwind-devel
  make # FIXME: See https://gitlab.com/hpctoolkit/hpctoolkit/-/issues/704
  mvapich2-devel
  ninja-build
  ocl-icd-devel
  papi-devel
  pkg-config
  python3
  python3-devel
  python3-docutils
  python3-pip
  redhat-rpm-config
  tbb-devel
  xerces-c-devel
  xxhash-devel
  xz-devel
  yaml-cpp-devel
  zlib-devel
)

# Install the required OS packages
if ! (for pkg in "${dnf_packages[@]}"; do rpm -q "$pkg" || exit 1; done) &>/dev/null; then
  echo -e "\e[0Ksection_start:$(date +%s):dnf_install[collapsed=true]\r\e[0KInstalling required packages with dnf..."

  # We need the EPEL and CRB repositories to install the packages, so do them first
  dnf install -y epel-release || exit $?
  dnf config-manager --enable crb || exit $?

  # Now install the packages
  dnf install -y "${dnf_packages[@]}" || exit $?

  echo -e "\e[0Ksection_end:$(date +%s):dnf_install\r\e[0K"
fi

# Install a up-to-date versions of Meson, using a suitable Python
echo -e "\e[0Ksection_start:$(date +%s):pip_install[collapsed=true]\r\e[0KInstalling required packages with pip..."
python3 -m pip install 'meson>=1.1.0,<2' || exit $?
echo -e "\e[0Ksection_end:$(date +%s):pip_install\r\e[0K"

# Install Sphinx and necessary dependencies, in a venv to keep from tangling the system install
if ! command -v sphinx-build >/dev/null; then
  echo -e "\e[0Ksection_start:$(date +%s):sphinx_install[collapsed=true]\r\e[0KInstalling Sphinx to /opt/sphinx..."
  rm -rf /opt/sphinx
  python3 -m venv /opt/sphinx || exit $?
  /opt/sphinx/bin/python -m pip install \
    sphinx \
    myst-parser ||
    exit $?
  ln -s /opt/sphinx/bin/sphinx-build /usr/bin/sphinx-build || exit $?
  echo -e "\e[0Ksection_end:$(date +%s):sphinx_install\r\e[0K"
fi
