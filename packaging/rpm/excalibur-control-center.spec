Name:           excalibur-control-center
Version:        0.5.5
Release:        1%{?dist}
Summary:        Read-only hardware telemetry for CASPER EXCALIBUR laptops
License:        GPL-2.0-only
URL:            https://github.com/msahhf/excalibur-control-center
Source0:        %{name}-%{version}.tar.gz

# Keep the RPM set to the same single, small artifact set as the other packages:
# no auto-generated -debuginfo/-debugsource subpackages.
%global debug_package %{nil}

BuildRequires:  cmake
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
Requires:       qt6-qtbase
Requires:       dkms

%description
Native Qt6 desktop application showing CPU/GPU temperature and fan RPM telemetry
exposed through the Linux hwmon interface for CASPER EXCALIBUR G870 laptops.
Strictly read-only; never requires root.

%package dkms
Summary:        excalibur-wmi read-only hwmon driver sources (DKMS)
Requires:       dkms
Requires:       %{name} = %{version}-%{release}
%description dkms
DKMS sources for the excalibur-wmi read-only hwmon kernel module. The dkms
framework builds the module for each installed kernel. Requires the 7.2.x kernel
WMI API (wmidev_set_block/query_block).

%prep
%autosetup -n %{name}-%{version}

%build
cmake -S gui -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{_prefix}
cmake --build build

%install
DESTDIR=%{buildroot} cmake --install build
mkdir -p %{buildroot}/usr/src/excalibur-wmi-%{version}
install -m644 driver/dkms.conf driver/Makefile analysis/phase3/excalibur-wmi.c LICENSE \
    %{buildroot}/usr/src/excalibur-wmi-%{version}/

%post dkms
for kver in $(ls /lib/modules 2>/dev/null); do
  dkms add -m excalibur-wmi -v %{version} >/dev/null 2>&1 || true
  dkms build -m excalibur-wmi -v %{version} -k "$kver" >/dev/null 2>&1 || true
  dkms install -m excalibur-wmi -v %{version} -k "$kver" >/dev/null 2>&1 || true
done

%preun dkms
if [ "$1" = 0 ]; then
  dkms remove -m excalibur-wmi -v %{version} --all >/dev/null 2>&1 || true
fi

%files
%{_bindir}/excalibur-control-center
%{_datadir}/applications/excalibur-control-center.desktop
%{_datadir}/icons/hicolor/*/apps/excalibur-control-center.*
%{_datadir}/metainfo/excalibur-control-center.metainfo.xml
%license LICENSE

%files dkms
/usr/src/excalibur-wmi-%{version}

%changelog
* Sun Oct 11 2026 Muhammedsah Fidan <muhammedsahfidan@gmail.com> - 0.5.5-1
- Track the 0.5.5 release (GUI + DKMS subpackage).

* Sat Oct 10 2026 Muhammedsah Fidan <muhammedsahfidan@gmail.com> - 0.5.3-1
- Initial RPM packaging prototype (GUI + DKMS subpackage).
