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
    return _("OTA compatible file copy.")


# This is going to be changed from various methods
status = pretty_name()


def pretty_status_message():
    return status


def ota(source, target):
    libcalamares.utils.host_env_process_output(
        ["/usr/share/calamares/scripts/ota", source, target], None
    )
    return None


def run():
    """
    Copy files
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

    if libcalamares.job.configuration.get("ota", None) is None:
        libcalamares.utils.warning("No *ota* key in job configuration.")
        return (
            _("Bad ota configuration"),
            _("There is no configuration information."),
        )

    # Bail out before we start when there are obvious problems
    #   - unsupported filesystems
    #   - non-existent sources
    #   - missing tools for specific FS
    for entry in libcalamares.job.configuration["ota"]:
        source = entry["source"]
        target = os.path.join(root_mount_point, entry["target"])

        if not os.path.exists(source):
            libcalamares.utils.warning(
                'The source filesystem "{}" does not exist'.format(source)
            )
            return (
                _("Bad ota configuration"),
                _('The source file "{}" does not exist').format(source),
            )

        if not os.path.exists(target):
            libcalamares.utils.warning(
                'The target file "{}" does not exist'.format(source)
            )
            return (
                _("Bad ota configuration"),
                _('The source file "{}" does not exist').format(source),
            )

        ota(source, target)

    return None
