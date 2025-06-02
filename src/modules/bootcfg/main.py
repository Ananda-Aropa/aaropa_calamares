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
    return _("Pre-config before installing bootloader.")


# This is going to be changed from various methods
status = pretty_name()


def pretty_status_message():
    return status


def mkdir_p(path):
    """
    Create directory.
    """
    if not os.path.exists(path):
        os.makedirs(path)

sys_prefix = "/usr/share"
calamares_shared = sys_prefix + "/calamares"
scriptdir = calamares_shared + "/scripts"

def run():
    """
    Pre-config before installing bootloader
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

    options = libcalamares.globalstorage.value("options")
    cmdline = (open("/cdrom/cmdline.txt", "r").readline() + " " + options).replace('\n', ' ')

    bootloader = os.environ.get("BOOTLOADER", "grub").lower()
    if bootloader == "grub":
        grubDir = os.path.join(root_mount_point, "boot/grub")
        mkdir_p(grubDir)

        libcalamares.utils.host_env_process_output(
            ["cp", "-r", sys_prefix + "/grub/themes/", grubDir], None
        )

        with open(os.path.join(grubDir, "android.cfg"), "w") as envCfg:
            print("SLOT=_a", file=envCfg)
            print("CMDLINE='" + cmdline + "'", file=envCfg)
            print("MODE=normal", file=envCfg)

        command = [scriptdir + "/grubcfg"]
    elif bootloader == "refind":
        command = [
            scriptdir + "/refind-conf",
            root_mount_point,
            cmdline,
        ]
    else:
        libcalamares.utils.warning("Unsupported bootloader: {}".format(bootloader))
        bootloader = "none"
        command = [
            scriptdir + "/no-bootloader",
            root_mount_point,
            cmdline,
        ]

    with open(os.path.abspath("/etc/default/grub"), "a") as grubConf:
        print("GRUB_TIMEOUT=10", file=grubConf)
        print("GRUB_TIMEOUT_STYLE=menu", file=grubConf)
        print("GRUB_DISTRIBUTOR=BlissLabs", file=grubConf)
        print("GRUB_GFXPAYLOAD_LINUX=keep", file=grubConf)
        print("GRUB_DISABLE_OS_PROBER=false", file=grubConf)
        print("GRUB_DEFAULT=saved", file=grubConf)
        print("GRUB_SAVEDEFAULT=true", file=grubConf)

        partitions = libcalamares.globalstorage.value("partitions")
        boot_device = ""
        for partition in partitions:
            dev = partition["device"]
            mntpt = partition["mountPoint"]
            if mntpt == "/":
                print("GRUB_DEVICE='" + dev + "'", file=grubConf)
                boot_device = dev
            elif mntpt == "/boot":
                boot_device = dev

        print("GRUB_DEVICE_BOOT='" + boot_device + "'", file=grubConf)

        print("SRC=", file=grubConf)

    # (Optional) Write CMDLINE to text file at $SRC for detection
    with open(os.path.join(root_mount_point, "cmdline.txt"), "w") as cmdlineFile:
        print(cmdline, file=cmdlineFile)

    libcalamares.utils.host_env_process_output(command, None)
    libcalamares.job.setprogress(1.0)
    return None
