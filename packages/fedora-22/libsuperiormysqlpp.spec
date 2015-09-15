Name:           libsuperiormysqlpp-dev
Version:        0.1.1
Release:        development%{?dist}
Summary:        C++ mysql library development files

License:        LGPLv3+
URL:            http://ftp.gnu.org/gnu/%{name}
Source0:        http://ftp.gnu.org/gnu/%{name}/%{name}-%{version}.tar.gz

BuildRequires:  docker, gcc-c++, community-mysql-devel, hostname, libasan, libubsan, tree
      
Requires(post): info
Requires(preun): info

%description 
C++ mysql library development files

%prep

%build

%check
make -j -B test

%install
make -j install VERSION=%{version} DESTDIR=%{buildroot} prefix=/usr

%clean
make -j clean package-fedora-22-clean

%post

%preun

%files 
/usr/include/*

%changelog
* Tue Sep 15 2015 Tomáš Nožička - 0.1.1-development
- New development version 0.1.1

* Tue Sep 15 2015 Tomáš Nožička - 0.1.0-1
- Release version 0.1.0 (going open-source)
