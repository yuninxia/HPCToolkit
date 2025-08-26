#!/bin/sh

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

if ! grep -q "^$_REMOTE_USER:" /etc/passwd; then
    if ! command -v useradd > /dev/null; then
        echo "useradd not found, unable to create new users"
        exit 127
    fi
    echo "Creating new user $_REMOTE_USER"
    useradd -m "$_REMOTE_USER" || exit $?
else
    echo "User already exists in /etc/passwd"
fi

# Update sudoers to allow access to root commands
echo "# Allow $_REMOTE_USER to run all sudo commands without a password" >> /etc/sudoers
echo "$_REMOTE_USER ALL=(root) NOPASSWD:ALL" >> /etc/sudoers
