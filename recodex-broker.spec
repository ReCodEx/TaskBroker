%define name recodex-broker
%define version 1.0.0
%define unmangled_version 1.0.0
%define release 1

Summary: ReCodEx broker component
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{unmangled_version}.tar.gz
License: MIT
Group: Development/Libraries
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix: %{_prefix}
Vendor: Petr Stefan <UNKNOWN>
Url: https://github.com/ReCodEx/broker
BuildRequires: systemd cmake
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd

%description
Backend part of ReCodEx programmer testing solution.

%prep
%setup -n %{name}-%{unmangled_version}

%build
%cmake -DDISABLE_TESTS=true .
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}/var/log/recodex

%clean


%post
%systemd_post 'recodex-broker.service'

%postun
%systemd_postun_with_restart 'recodex-broker.service'

%pre
getent group recodex >/dev/null || groupadd -r recodex
getent passwd recodex >/dev/null || useradd -r -g recodex -d %{_sysconfdir}/recodex -s /sbin/nologin -c "ReCodEx Code Examiner" recodex
exit 0

%preun
%systemd_preun 'recodex-broker.service'

%files
%defattr(-,root,root)
%dir %attr(-,recodex,recodex) %{_sysconfdir}/recodex/broker
%dir %attr(-,recodex,recodex) /var/log/recodex

%{_bindir}/recodex-broker
%config(noreplace) %attr(-,recodex,recodex) %{_sysconfdir}/recodex/broker/config.yml

#%{_unitdir}/recodex-broker.service
/lib/systemd/system/recodex-broker.service

%changelog
