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
import subprocess
import re

import libcalamares

import gettext

_ = gettext.translation(
    "calamares-python",
    localedir=libcalamares.utils.gettext_path(),
    languages=libcalamares.utils.gettext_languages(),
    fallback=True,
).gettext


def pretty_name():
    return _("Generating fstab.")


# This is going to be changed from various methods
status = pretty_name()


def pretty_status_message():
    return status


FSTAB_HEADER = """# fstab.android: static file system information.
# FORMAT=0.2
#
# Use 'blkid' to print the universally unique identifier for a device; this may
# be used with UUID= as a more robust way to name devices that works even if
# disks are added and removed. See fstab(5).
#
# <src>    <mnt_point>    <type>    <mnt_flags and options>    <fs_mgr_flags>
$FS/system$SLOT.img						system$SLOT
$FS/kernel$SLOT							kernel$SLOT
$FS/initrd$SLOT.img						initrd$SLOT
$FS/recovery$SLOT.img					recovery$SLOT
$FS/misc.img							misc
"""


def userdata_is_image(root, data_img_enabled):
    """
    Must agree with scripts/gen-img, which ran before this job: a kept data/ directory
    wins over a stale data.img when the previous fstab.android mounted the directory.
    """
    data_dir = os.path.join(root, "data")
    has_img = os.path.isfile(os.path.join(root, "data.img"))
    has_dir = os.path.isdir(data_dir) and len(os.listdir(data_dir)) > 0
    prev_dir = False
    try:
        with open(os.path.join(root, "fstab.android")) as old:
            prev_dir = any(re.match(r"\$FS/data\s+userdata", line) for line in old)
    except OSError:
        pass

    if has_img and has_dir and prev_dir:
        return False
    if has_img:
        return True
    if has_dir:
        return False
    return data_img_enabled


def run():
    """
    Create fstab.android file
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

    genfstab_output = subprocess.run(
        ["/usr/share/calamares/scripts/genfstab", "-U", root_mount_point],
        stdout=subprocess.PIPE,
    ).stdout.decode()


    data_img = libcalamares.globalstorage.value("dataimg") or {}
    data_img_enabled = not bool(data_img.get("disabled", True))
    use_data_img = userdata_is_image(root_mount_point, data_img_enabled)

    with open(os.path.join(root_mount_point, "fstab.android"), "w") as fstab_file:
        print(FSTAB_HEADER, file=fstab_file)
        print(genfstab_output, file=fstab_file)

        if not re.search("/boot\\s", genfstab_output):
            print("$FS/boot bootloader", file=fstab_file)

        if not re.search("/data\\s", genfstab_output):
            if use_data_img:
                print("$FS/data.img userdata ext4 defaults defaults", file=fstab_file)
            else:
                print("$FS/data userdata", file=fstab_file)

        if os.path.isdir('/sys/firmware/efi'):
            print("none /sys/firmware/efi/efivars efivarfs defaults defaults", file=fstab_file)

    libcalamares.job.setprogress(1.0)
    return None
