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
    return _("Post-config after installing bootloader.")


# This is going to be changed from various methods
status = pretty_name()


def pretty_status_message():
    return status


def mkdir_p(path):
    """Create directory.

    :param path:
    """
    if not os.path.exists(path):
        os.makedirs(path)

sys_prefix = "/usr/share"
calamares_shared = sys_prefix + "/calamares"
scriptdir = calamares_shared + "/scripts"

def run():
    """
    Post-config after installing bootloader
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

    sys_prefix = "/usr/share"
    calamares_shared = sys_prefix + "/calamares"
    scriptdir = calamares_shared + "/scripts"

    partitions = libcalamares.globalstorage.value("partitions")

    filesystems = []

    for partition in partitions:
        if partition["fs"] == "linuxswap":
            continue
        filesystems.append(partition["fs"])

    bootloader = os.environ.get("BOOTLOADER", "grub").lower()
    if bootloader == "refind":
        command = [
            scriptdir + "/refind-postconf",
            root_mount_point,
            " ".join(filesystems),
        ]
    else:
        bootloader = "none"
        command = ["echo", "Skipping"]


    libcalamares.utils.host_env_process_output(command, None)
    libcalamares.job.setprogress(1.0)
    return None
