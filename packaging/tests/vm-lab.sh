#!/bin/sh
# Parameterised (NOT run in this phase) VBoxManage helper for the multi-distro lab.
# Usage: vm-lab.sh <name> <ostype> <iso>    e.g. vm-lab.sh deb13 Debian_64 ~/iso/debian.iso
set -eu
name="$1"; ostype="$2"; iso="$3"
vmdir="$HOME/VirtualBox VMs/$name"
echo "NOTE: this creates a NEW VM '$name'; it never touches existing VMs."
VBoxManage createvm --name "$name" --ostype "$ostype" --register
VBoxManage modifyvm "$name" --memory 4096 --cpus 4 --nic1 nat \
    --graphicscontroller vmsvga --audio none
VBoxManage createmedium disk --filename "$vmdir/$name.vdi" --size 20480 --format VDI
VBoxManage storagectl "$name" --name SATA --add sata --controller IntelAhci
VBoxManage storageattach "$name" --storagectl SATA --port 0 --device 0 --type hdd --medium "$vmdir/$name.vdi"
VBoxManage storageattach "$name" --storagectl SATA --port 1 --device 0 --type dvddrive --medium "$iso"
echo "Attach an automated install (preseed/kickstart/autoinstall) then:"
echo "  VBoxManage startvm '$name' --type headless"
echo "  # after install:  VBoxManage snapshot '$name' take clean"
