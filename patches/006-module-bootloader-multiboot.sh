#!/bin/bash
# Bootloader behaviour for machines that also start Windows or another Linux, and for
# upgrades of an existing Bass OS install (existinginstall page):
#  - reuse the existing EFI directory id (gs bassEfiBootloaderId, else on upgrade the
#    directory whose GRUB core was built for the root: "search.fs_uuid <uuid>", or
#    "(,gptN)/boot/grub" with the ESP on the same disk; the firmware's first entry wins)
#    instead of a new one, and look for ${SERIAL} id clashes in the target ESP's EFI/;
#  - upgrade: grub-install --no-nvram, keep the boot order, only add a missing entry at the end;
#  - upgrade: every other loader of the install gets the new GRUB core (old core + new
#    modules fails with "symbol ... not found");
#  - BIOS upgrade: keep the existing MBR boot code;
#  - BIOS fresh install: keep another system's MBR when the user declined to replace it
#    (gs bassAllowMbrReplace == false) and leave a GRUB entry to add to that loader;
#  - only write EFI/Boot/bootx64.efi when it is missing or was ours.
# Must run after 002-module-bootloader.sh.

python3 - src/modules/bootloader/main.py <<'EOF'
import sys

path = sys.argv[1]
src = open(path).read()


def sub(old, new):
    global src
    if src.count(old) != 1:
        sys.exit("006-module-bootloader-multiboot: anchor not found once: " + old.splitlines()[0])
    src = src.replace(old, new)


sub('''    if "efiBootloaderId" in libcalamares.job.configuration:
        efi_bootloader_id = change_efi_suffix(efi_directory, libcalamares.job.configuration["efiBootloaderId"])
''', '''    if libcalamares.globalstorage.value("bassEfiBootloaderId"):
        efi_bootloader_id = libcalamares.globalstorage.value("bassEfiBootloaderId")
    elif bass_upgrade() and bass_find_efi_id(efi_directory):
        efi_bootloader_id = bass_find_efi_id(efi_directory)
    elif "efiBootloaderId" in libcalamares.job.configuration:
        efi_bootloader_id = change_efi_suffix(bass_efi_firmware_dir(efi_directory),
                                              libcalamares.job.configuration["efiBootloaderId"])
''')

sub('''    is_zfs = any([is_zfs_root(partition) for partition in partitions])

    # zfs needs an environment variable set for grub
''', '''    is_zfs = any([is_zfs_root(partition) for partition in partitions])

    if bass_skip_grub_install(fw_type, installation_root_path, efi_directory):
        return

    # zfs needs an environment variable set for grub
''')

sub('''                                   "--efi-directory=" + installation_root_path + efi_directory,
                                   "--bootloader-id=" + efi_bootloader_id,
                                   "--force"])
''', '''                                   "--efi-directory=" + installation_root_path + efi_directory,
                                   "--bootloader-id=" + efi_bootloader_id,
                                   "--force"] + bass_grub_efi_extra_args())
            bass_ensure_efi_entry(efi_directory, efi_bootloader_id, efi_grub_file)
            bass_refresh_stale_loaders(efi_directory, efi_bootloader_id, efi_grub_file)
''')

sub('''        if libcalamares.job.configuration.get(fallback, True):
''', '''        if libcalamares.job.configuration.get(fallback, True) and \\
                bass_may_write_fallback(install_efi_boot_directory, efi_boot_file):
''')

src += '''

# --- Bass OS multi-boot / upgrade helpers (patches/006-module-bootloader-multiboot.sh) ---

BASS_EFI_DIR_PREFIXES = ("blissos", "bassos")
bass_own_loader_digests = set()


def bass_efi_firmware_dir(efi_directory):
    """
    <target ESP>/EFI, where the loader directories are. Without a chroot, efi_directory
    is relative to rootMountPoint; upstream looked for ${SERIAL} ids next to EFI/.
    """
    esp = (libcalamares.globalstorage.value("rootMountPoint") or "") + efi_directory
    if os.path.isdir(esp):
        return vfat_correct_case(esp, "EFI")
    return esp


def bass_upgrade():
    upgrade = libcalamares.globalstorage.value("bassUpgrade") or {}
    return bool(upgrade.get("enabled", False))


def bass_root_uuid():
    for partition in libcalamares.globalstorage.value("partitions") or []:
        if partition.get("mountPoint") == "/" and partition.get("uuid"):
            return partition["uuid"]
    return (libcalamares.globalstorage.value("bassUpgrade") or {}).get("uuid", "")


def bass_disk_and_number(device):
    """ (parent disk, partition number) of a partition device, or (None, None). """
    if not device:
        return None, None
    try:
        disk = subprocess.check_output(["lsblk", "-dnpo", "PKNAME", device],
                                       universal_newlines=True).strip()
        with open("/sys/class/block/" + os.path.basename(device) + "/partition") as f:
            return disk or None, f.read().strip() or None
    except (OSError, subprocess.CalledProcessError):
        return None, None


def bass_root_needles(efi_directory):
    """
    Byte strings found only in GRUB cores built for this root filesystem. With the ESP
    on another disk grub-install embeds "search.fs_uuid <uuid>"; on the same disk it
    hardcodes the partition instead: "(,gpt2)/boot/grub".
    """
    needles = []
    uuid = bass_root_uuid()
    if uuid:
        needles.append(uuid.casefold().encode())
    root_dev = esp_dev = None
    for partition in libcalamares.globalstorage.value("partitions") or []:
        if partition.get("mountPoint") == "/":
            root_dev = partition.get("device")
        elif partition.get("mountPoint") == efi_directory:
            esp_dev = partition.get("device")
    root_disk, number = bass_disk_and_number(root_dev)
    esp_disk, _ = bass_disk_and_number(esp_dev)
    if root_disk and number and root_disk == esp_disk:
        for table in ("gpt", "msdos"):
            needles.append("(,{}{})/boot/grub".format(table, number).encode())
    return needles


def bass_loaders_for_root(efi_firmware_dir, needles):
    """ (EFI dir name, path) of every loader on the ESP built for this root filesystem. """
    found = []
    if not needles or not os.path.isdir(efi_firmware_dir):
        return found
    for name in sorted(os.listdir(efi_firmware_dir)):
        sub_dir = os.path.join(efi_firmware_dir, name)
        if not os.path.isdir(sub_dir):
            continue
        for f in sorted(os.listdir(sub_dir)):
            path = os.path.join(sub_dir, f)
            if not f.casefold().endswith(".efi") or not os.path.isfile(path):
                continue
            try:
                with open(path, "rb") as fh:
                    data = fh.read().lower()
            except OSError:
                continue
            if any(n in data for n in needles):
                found.append((name, path))
    return found


def bass_boot_order_dirs():
    """ EFI directory names of the firmware boot entries, in boot order. """
    try:
        listing = subprocess.check_output(
            [libcalamares.job.configuration.get("efiBootMgr", "efibootmgr"), "-v"],
            universal_newlines=True)
    except (OSError, subprocess.CalledProcessError):
        return []
    entries = {}
    order = []
    for line in listing.splitlines():
        if line.startswith("BootOrder:"):
            order = [o.strip() for o in line.split(":", 1)[1].split(",") if o.strip()]
        m = re.match(r"Boot([0-9A-Fa-f]{4})\\*?\\s.*?\\\\EFI\\\\([^\\\\]+)\\\\", line, re.IGNORECASE)
        if m:
            entries[m.group(1).upper()] = m.group(2)
    return [entries[o.upper()] for o in order if o.upper() in entries]


def bass_find_efi_id(efi_directory):
    """
    Upgrade without a known EFI id: the directory whose loader starts this root,
    preferring the one the firmware boots first (older installs may have several).
    """
    names = [name for name, _ in bass_loaders_for_root(bass_efi_firmware_dir(efi_directory),
                                                       bass_root_needles(efi_directory))
             if name.casefold() != "boot"]
    for boot_dir in bass_boot_order_dirs():
        for name in names:
            if name.casefold() == boot_dir.casefold():
                return name
    return names[0] if names else None


def bass_refresh_stale_loaders(efi_directory, label, grub_file):
    """
    Upgrade: other loaders of this install (an older EFI id, the EFI/Boot copy) still hold
    the old GRUB core, which fails on the new modules in /boot/grub
    ("symbol ... not found"). Give them the new core.
    """
    if not bass_upgrade():
        return
    efi_firmware_dir = bass_efi_firmware_dir(efi_directory)
    new_loader = os.path.join(vfat_correct_case(efi_firmware_dir, label), grub_file)
    digest = bass_file_digest(new_loader)
    if not digest:
        libcalamares.utils.warning("No new loader at " + new_loader)
        return
    for _, path in bass_loaders_for_root(efi_firmware_dir, bass_root_needles(efi_directory)):
        if path == new_loader or bass_file_digest(path) == digest:
            continue
        libcalamares.utils.debug("Replacing the stale loader " + path)
        shutil.copy2(new_loader, path)


def bass_file_digest(path):
    import hashlib
    try:
        with open(path, "rb") as f:
            return hashlib.sha256(f.read()).hexdigest()
    except OSError:
        return None


def bass_remember_own_loaders(esp_root):
    """
    Hash every loader in our EFI directories before grub-install replaces them, so a
    fallback bootx64.efi copied from an older Bass install is still recognised as ours.
    """
    if not os.path.isdir(esp_root):
        return
    efi_firmware_dir = vfat_correct_case(esp_root, "EFI")
    if not os.path.isdir(efi_firmware_dir):
        return
    for name in os.listdir(efi_firmware_dir):
        sub_dir = os.path.join(efi_firmware_dir, name)
        if not name.casefold().startswith(BASS_EFI_DIR_PREFIXES) or not os.path.isdir(sub_dir):
            continue
        for f in os.listdir(sub_dir):
            if f.casefold().endswith(".efi"):
                digest = bass_file_digest(os.path.join(sub_dir, f))
                if digest:
                    bass_own_loader_digests.add(digest)


def bass_mbr_owner(disk):
    """ "empty" when the MBR has no boot code, "grub" or "other" otherwise. """
    try:
        with open(disk, "rb") as f:
            mbr = f.read(512)
    except OSError:
        return "other"
    if len(mbr) < 440 or not any(mbr[:440]):
        return "empty"
    if b"GRUB" in mbr[:440]:
        return "grub"
    return "other"


def bass_write_chainload_snippet(root):
    uuid = ""
    for partition in libcalamares.globalstorage.value("partitions") or []:
        if partition.get("mountPoint") == "/":
            uuid = partition.get("uuid", "")
    snippet = os.path.join(root, "boot/grub/bass-chainload.cfg")
    os.makedirs(os.path.dirname(snippet), exist_ok=True)
    with open(snippet, "w") as f:
        print("# Add this entry to the boot loader that starts this computer, for example", file=f)
        print("# /etc/grub.d/40_custom of another Linux, then run update-grub there.", file=f)
        print("menuentry \\"Bass OS\\" {", file=f)
        print("\\tinsmod part_gpt", file=f)
        print("\\tinsmod part_msdos", file=f)
        print("\\tinsmod ext2", file=f)
        print("\\tsearch --no-floppy --fs-uuid --set=root " + uuid, file=f)
        print("\\tconfigfile /boot/grub/grub.cfg", file=f)
        print("}", file=f)
    libcalamares.utils.warning("Kept the existing MBR boot code; see " + snippet)


def bass_skip_grub_install(fw_type, root, efi_directory):
    if fw_type == "efi":
        bass_remember_own_loaders(root + efi_directory)
        return False

    if bass_upgrade():
        libcalamares.utils.debug("Upgrade: keeping the existing BIOS boot code")
        return True

    gs = libcalamares.globalstorage
    if gs.contains("bassAllowMbrReplace") and not gs.value("bassAllowMbrReplace"):
        boot_loader = gs.value("bootLoader") or {}
        disk = boot_loader.get("installPath")
        if disk and bass_mbr_owner(disk) != "empty":
            bass_write_chainload_snippet(root)
            return True
    return False


def bass_grub_efi_extra_args():
    # An upgrade must not move Bass OS to the front of the firmware boot order.
    return ["--no-nvram"] if bass_upgrade() else []


def bass_ensure_efi_entry(efi_directory, label, grub_file):
    """
    After an upgrade with --no-nvram: add a firmware entry only when none points at our
    loader, appended to the end of BootOrder so the user's default OS stays first.
    """
    if not bass_upgrade():
        return
    boot_mgr = libcalamares.job.configuration.get("efiBootMgr", "efibootmgr")
    loader = "\\\\EFI\\\\" + label + "\\\\" + grub_file
    try:
        listing = subprocess.check_output([boot_mgr, "-v"], universal_newlines=True)
        # Match the directory: some efibootmgr versions cut the file name ("grubx64.").
        if ("\\\\EFI\\\\" + label + "\\\\").casefold() in listing.replace("/", "\\\\").casefold():
            return

        esp = None
        for partition in libcalamares.globalstorage.value("partitions") or []:
            if partition.get("mountPoint") == efi_directory:
                esp = partition.get("device")
        if not esp:
            libcalamares.utils.warning("No ESP device for a firmware boot entry")
            return
        disk = "/dev/" + subprocess.check_output(["lsblk", "-dno", "PKNAME", esp],
                                                 universal_newlines=True).strip()
        with open("/sys/class/block/" + os.path.basename(esp) + "/partition") as f:
            part_num = f.read().strip()

        order = []
        for line in listing.splitlines():
            if line.startswith("BootOrder:"):
                order = [o for o in line.split(":", 1)[1].strip().split(",") if o]
        created = subprocess.check_output(
            [boot_mgr, "-c", "-d", disk, "-p", part_num, "-L", label, "-l", loader],
            universal_newlines=True)
        new_entry = None
        for line in created.splitlines():
            words = line.split()
            if words and words[0].startswith("Boot") and words[0].rstrip("*")[4:].isalnum() \\
                    and len(words) > 1 and words[1] == label:
                new_entry = words[0].rstrip("*")[4:]
        if new_entry and order:
            subprocess.call([boot_mgr, "-o", ",".join([o for o in order if o != new_entry] + [new_entry])])
    except (OSError, subprocess.CalledProcessError) as e:
        libcalamares.utils.warning("Could not add a firmware boot entry: " + str(e))


def bass_may_write_fallback(boot_directory, boot_file):
    """ Another OS may rely on EFI/Boot/bootx64.efi: only replace a missing or Bass copy. """
    target = os.path.join(boot_directory, boot_file)
    if not os.path.exists(target):
        return True
    if bass_file_digest(target) in bass_own_loader_digests:
        return True
    libcalamares.utils.debug("Keeping the existing fallback loader " + target)
    return False
'''

open(path, "w").write(src)
EOF
