Name:           libsuperiormysqlpp-dev
Version:        0.5.2
Release:        1%{?dist}
Summary:        C++ mysql library development files

License:        LGPLv3+
URL:            http://ftp.gnu.org/gnu/%{name}
Source0:        http://ftp.gnu.org/gnu/%{name}/%{name}-%{version}.tar.gz

BuildRequires:  docker, gcc-c++, community-mysql-devel, hostname, libasan, libubsan, tree, boost-devel >= 1.49.0, socat

Requires(post): info
Requires(preun): info

Suggests: boost-devel >= 1.49.0

%description
C++ mysql library development files

%prep

%build

%check
make %{_smp_mflags} -B test

%install
make %{_smp_mflags} install VERSION=%{version} DESTDIR=%{buildroot} prefix=/usr

%clean
make %{_smp_mflags} clean package-fedora-22-clean

%post

%preun

%files
/usr/include/*
/usr/lib/pkgconfig/*

