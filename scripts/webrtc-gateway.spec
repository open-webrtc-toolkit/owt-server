
Name:    webrtc-gateway
Version: 0.11
Release: 0
Summary: WebRTC Gateway supporting WebRTC connections with ooVoo server
Group:   Development/Tools
License: Intel
URL:     www.intel.com
Source0: %{name}-%{version}.tar.gz
BuildRoot:%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Prefix: /opt
AutoReqProv: no
%define installpath %{prefix}/webrtc-gateway
Requires: libcap

%description
Only copy the dist directory to a system path

%prep
%setup -q

%install
mkdir -p %{buildroot}%{installpath}
cp -r * %{buildroot}%{installpath} > /dev/null
mkdir -p %{buildroot}/etc/init.d
cp bin/webrtc-gateway-init.sh %{buildroot}/etc/init.d/%{name}
mkdir -p %{buildroot}/etc/security/limits.d
cp etc/99-webrtc-gateway-limits.conf %{buildroot}/etc/security/limits.d/
mkdir -p %{buildroot}/etc/sysctl.d
cp etc/webrtc-gateway-sysctl.conf %{buildroot}/etc/sysctl.d/

%post
if [ $1 = 1 ]; then
  chkconfig --add %{name}
  echo "%{installpath}/lib" > /etc/ld.so.conf.d/%{name}.conf
  ldconfig
  setcap 'cap_net_bind_service=+ep' %{installpath}/sbin/webrtc_gateway
fi
rm -f %{installpath}/etc/99-webrtc-gateway-limits.conf
rm -f %{installpath}/etc/webrtc-gateway-sysctl.conf

%preun
service %{name} stop >/dev/null 2>&1
if [ $1 = 0 ]; then
  chkconfig --del %{name}
  rm -f /etc/ld.so.conf.d/%{name}.conf
  ldconfig
fi

%postun

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,777)
%{installpath}
/etc/init.d/%{name}
%config(noreplace) %{installpath}/etc/gateway_config.json
%config(noreplace) %{installpath}/etc/heartbeat_config.json
%config(noreplace) %{installpath}/etc/log4cxx.properties
%config(noreplace) %{installpath}/etc/linux_service_start_user
%config(noreplace) /etc/security/limits.d/99-webrtc-gateway-limits.conf
%config(noreplace) /etc/sysctl.d/webrtc-gateway-sysctl.conf
%doc

%changelog
