#!/usr/bin/env bash
#
# SPDX-FileCopyrightText: 2021 René de Hesselle <dehesselle@web.de>
#
# SPDX-License-Identifier: GPL-2.0-or-later

### description ################################################################

# Install a pre-compiled version of our JHBuild-based toolset and all the
# required dependencies to build Inkscape.

### shellcheck #################################################################

# Nothing here.

### dependencies ###############################################################

source "$(dirname "${BASH_SOURCE[0]}")"/jhb-custom.conf.sh
source "$(dirname "${BASH_SOURCE[0]}")"/jhb/etc/jhb.conf.sh

### variables ##################################################################

SELF_DIR=$(dirname "${BASH_SOURCE[0]}")

### functions ##################################################################

# Nothing here.

### main #######################################################################

cp "$SELF_DIR"/jhb-custom.conf.sh "$SELF_DIR"/jhb/etc/jhb-custom.conf.sh

"$SELF_DIR"/jhb/usr/bin/archive install_dmg 3

if [ "$1" = "restore_overlay" ]; then
  # restore files fronm build stage
  gtar -C "$VER_DIR" -xpJf "$ARTIFACT_DIR"/toolset_overlay.tar.xz
fi

