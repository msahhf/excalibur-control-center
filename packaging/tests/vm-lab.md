# Multi-distro VM test laboratory (plan / not executed)

Full GUI OS installs were **not** performed in this phase (no ISOs present; time
budget). Package build + install/remove + driver-absent GUI behaviour were instead
validated in **isolated containers** (Docker), which is reproducible and does not
touch the host. A full VM lab would add the real kernel + desktop integration.

## Target matrix (planned)

| Priority | Distro | Release (2026) | Desktop | Kernel note |
| --- | --- | --- | --- | --- |
| 1 | Debian | stable (13 / trixie) | none/headless | Debian kernel: WMI `wmidev_*` API presence **unverified** |
| 2 | Ubuntu | LTS (24.04) | none/headless | Ubuntu kernel: **unverified** |
| 3 | Fedora | stable | none/headless | Fedora kernel: **unverified** |

**Kernel API:** the driver builds only where `wmidev_set_block`/`wmidev_query_block`
exist (verified on 7.2.x; absent on 6.18.x-lts). Whether Debian/Ubuntu/Fedora
kernels provide it must be checked per kernel before any "supported" claim.

## What was actually tested (containers, isolated)

- `debian:stable` (Qt 6.8.2, kernel 6.12.111): `.deb` build; GUI + DKMS install;
  GUI starts with no driver; remove.
- `fedora:latest` (fc44): `.rpm` build; GUI + DKMS install; GUI starts with no
  driver; remove.
- DKMS behaviour (Arch container): supported 7.2.9 builds/installs; unsupported
  6.18.55-lts is excluded (exit 77, not built).

Containers have **no** physical CASPER WMI/EC hardware, so **live temperature/fan
telemetry is NOT tested** there — only build + lifecycle + driver-absent behaviour.

## Full VM runbook (not executed)

Use `VBoxManage` (headless). Do **not** touch the existing VMs ("Zorin OS",
"Pardus 25.2 GNOME").

1. Fetch official ISOs (netinst): Debian, Ubuntu LTS, Fedora.
2. `VBoxManage createvm --name <n> --ostype <Debian_64|Ubuntu_64|Fedora_64> --register`
3. `VBoxManage modifyvm <n> --memory 4096 --cpus 4 --nic1 nat --graphicscontroller vmsvga`
4. `VBoxManage createmedium disk --filename <n>.vdi --size 20480`
5. attach SATA controller + disk + DVD (ISO); start `--type headless` with an
   automated install (preseed/kickstart/autoinstall).
6. Snapshot `clean`.
7. Inside: install build deps, `makepkg`/`dpkg-buildpackage`/`rpmbuild`, install,
   check `dkms status`, run the GUI headless (`QT_QPA_PLATFORM=offscreen`), test
   settings/diagnostics/JSON export, remove, reinstall.
8. Record distro/kernel/Qt/package versions and logs per test.

See `vm-lab.sh` for a parameterised skeleton.
