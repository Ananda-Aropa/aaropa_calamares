#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# === This file is part of Calamares - <https://calamares.io> ===
#
#   SPDX-FileCopyrightText: 2024 Bùi Gia Viện (BlissLabs) <shadichy@blisslabs.org>
#   SPDX-License-Identifier: GPL-3.0-or-later
#
#   Calamares is Free Software: see the License-Identifier above.
#

import os

import libcalamares

import gettext

_ = gettext.translation(
    "calamares-python",
    localedir=libcalamares.utils.gettext_path(),
    languages=libcalamares.utils.gettext_languages(),
    fallback=True,
).gettext


def pretty_name():
    return _("Generating needed disk images.")


# This is going to be changed from various methods
status = pretty_name()


def pretty_status_message():
    return status


def run():
    """
    Create misc.img and data.img in cases
    """
    root_mount_point = libcalamares.globalstorage.value("rootMountPoint")

    if not root_mount_point:
        libcalamares.utils.warning("No mount point for root partition")
        return (
            _("No mount point for root partition"),
            _('globalstorage does not contain a "rootMountPoint" key.'),
        )
    if not os.path.exists(root_mount_point):
        libcalamares.utils.warning('Bad root mount point "{}"'.format(root_mount_point))
        return (
            _("Bad mount point for root partition"),
            _('rootMountPoint is "{}", which does not exist.'.format(root_mount_point)),
        )

    data_img = libcalamares.globalstorage.value("dataimg") or {}
    data_img_enabled = not bool(data_img.get("disabled", True))

    # Existing data/ or data.img on the target is kept unless the user asked to erase it
    # (existinginstall page, Fresh install + erase). See /usr/share/calamares/scripts/gen-img.
    upgrade = libcalamares.globalstorage.value("bassUpgrade") or {}
    wipe = bool(upgrade.get("wipeData", False)) and not bool(upgrade.get("enabled", False))

    size = ""
    if data_img_enabled:
        size = 0 if bool(data_img.get("useMaximum", True)) else data_img.get("dataSize", 0)

    libcalamares.utils.host_env_process_output(
        [
            "/usr/share/calamares/scripts/gen-img",
            root_mount_point,
            str(data_img_enabled),
            str(size),
            str(wipe),
        ],
        None,
    )
    libcalamares.job.setprogress(1.0)
    return None
