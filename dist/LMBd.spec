Name: LMBd
Group: Productivity/Networking/Other
Summary: Led Message Board Driver Daemon
URL: https://github.com/Fishwaldo/LEDMessageBoard
License: GPL
Version: 1.0.1445180435.6a453c9
Release: 0
BuildRequires: gcc-c++ cmake boost-devel boost-thread boost-filesystem boost-program-options systemd

Source0: LMBd-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-root

%description
Led Message Board Driver 

%prep

%setup -q

%build
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_INSTALL_SYSCONFDIR=%{_sysconfdir} -G "Unix Makefiles" . 
make %{?_smp_mflags}

%install
make install DESTDIR=${RPM_BUILD_ROOT}
mkdir -p ${RPM_BUILD_ROOT}/var/spool/LMBd
mkdir -p ${RPM_BUILD_ROOT}/var/log/LMBd
%__install -D -m 444 scripts/zabbix-stats.service %{buildroot}%{_unitdir}/zabbix-stats.service
%__install -D -m 444 scripts/lmbd.service %{buildroot}%{_unitdir}/lmbd.service


%files
%defattr(-,root,root,-)
%{_prefix}/bin/LMBd
%{_prefix}/bin/zabbix_stats.py
%config /etc/LMBd.conf
%config /etc/zabbix_stats.cfg
%dir /var/spool/LMBd
%dir /var/log/LMBd
%{_unitdir}/*.service

#%doc README
#%config /etc/ozwwebapp/*

%pre


%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%changelog
