Name: LMBd
Group: Productivity/Networking/Other
Summary: Led Message Board Driver Daemon
URL: https://github.com/Fishwaldo/LEDMessageBoard
License: GPL
Version: 1.0.1445180435.6a453c9
Release: 0
BuildRequires: gcc-c++ cmake boost-devel boost-thread boost-log boost-filesystem

Source0: LMBd-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-root

%description
Led Message Board Driver 

%prep

%setup -q

%build
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -Dlib_dir=%{_libdir} -G "Unix Makefiles" . 
make %{?_smp_mflags}

%install
make install DESTDIR=${RPM_BUILD_ROOT} && cd ..


%files
%defattr(-,root,root,-)
%{_prefix}/bin/LMBd
#%doc README
#%config /etc/ozwwebapp/*

%pre


%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%changelog
