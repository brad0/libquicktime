Summary:	A library for manipulating QuickTime files
Name:		libquicktime
Version:	1.5.1.ap012502
Release:	1
License:	GPL
Group:		Libraries
Group(de):	Libraries
Group(es):	Bibliotecas
Group(fr):	Librairies
Group(pl):	Biblioteki
Group(pt_BR):	Bibliotecas
Group(ru):	‚…¬Ã…œ‘≈À…
Group(uk):	‚¶¬Ã¶œ‘≈À…
Source0:	%{name}-%{version}.tar.gz
# Source0	http://heroinewarrior.com/%{name}-%{version}.tar.gz
URL:		http://http://libquicktime.sourceforge.net/
BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	glib-devel
BuildRequires:	libpng-devel >= 1.0.8
BuildRequires:	libjpeg-devel
BuildRequires:	libdv-devel >= 0.9
BuildRequires:	libraw1394-devel
BuildRequires:	libogg-devel
BuildRequires:  libvorbis-devel
BuildRoot:	/tmp/libquicktime

%description
Libquicktime is a library for reading and writing QuickTime files
on UNIX systems. Video CODECs supported by this library are OpenDivX, MJPA,
JPEG Photo, PNG, RGB, YUV 4:2:2, and YUV 4:2:0 compression.  Supported
audio CODECs are Ogg Vorbis, IMA4, ulaw, and any linear PCM format.

Libquicktime is based on the quicktime4linux library.  Libquicktime add
features such as a GNU build tools-based build process and dynamically
loadable CODECs.

%package devel
Summary:	Header files and development documentation for libquicktime
Summary(pl):	Pliki nag≥Ûwkowe i dokumentacja do libquicktime
Group:		Development/Libraries
Group(de):	Entwicklung/Libraries
Group(es):	Desarrollo/Bibliotecas
Group(fr):	Development/Librairies
Group(pl):	Programowanie/Biblioteki
Group(pt_BR):	Desenvolvimento/Bibliotecas
Group(ru):	Ú¡⁄“¡¬œ‘À¡/‚…¬Ã…œ‘≈À…
Group(uk):	Úœ⁄“œ¬À¡/‚¶¬Ã¶œ‘≈À…
Requires:	%{name} = %{version}
Requires:	glib-devel
Requires:	libpng-devel >= 1.0.8
Requires:	libjpeg-devel
Requires:	libdv-devel
Requires:	libraw1394-devel
Requires:	libogg-devel
Requires:  	libvorbis-devel

%description devel
Header files and development documentation for libquicktime.

%description -l pl devel
Pliki nag≥Ûwkowe i dokumentacja do biblioteki libquicktime.

%package progs
Summary:	Useful tools to operate at QuickTime files
Summary(pl):	Poøyteczne narzÍdzia od operowania na plikach w formacie QuickTime
Group:		Applications/Graphics
Group(de):	Applikationen/Grafik
Group(pl):	Aplikacje/Grafika
Group(pt):	AplicaÁıes/Gr·ficos
Requires:	%{name} = %{version}

%description progs
Useful tools to operate on QuickTime files.

%description -l pl progs
Poøyteczne narzÍdzia od operowania na plikach w formacie QuickTime.

%package static
Summary:	Static libquicktime libraries
Summary(pl):	Biblioteki statyczne libquicktime
Group:		Development/Libraries
Group(de):	Entwicklung/Libraries
Group(es):	Desarrollo/Bibliotecas
Group(fr):	Development/Librairies
Group(pl):	Programowanie/Biblioteki
Group(pt_BR):	Desenvolvimento/Bibliotecas
Group(ru):	Ú¡⁄“¡¬œ‘À¡/‚…¬Ã…œ‘≈À…
Group(uk):	Úœ⁄“œ¬À¡/‚¶¬Ã¶œ‘≈À…
Requires:	%{name}-devel = %{version}

%description static
Static libquicktime libraries.

%description -l pl static
Biblioteki statyczne libquicktime.

%prep
%setup -q

%build
rm -f missing
aclocal
autoconf
automake -a -c
%configure

%{__make}

%install
rm -rf $RPM_BUILD_ROOT

%{__make} install DESTDIR=$RPM_BUILD_ROOT

gzip -9nf README

%clean
rm -rf $RPM_BUILD_ROOT

%post   -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(644,root,root,755)
%doc README.gz
%attr(755,root,root) %{_libdir}/lib*.so.*.*

%files progs
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/*

%files devel
%defattr(644,root,root,755)
%doc docs/*.html
%attr(755,root,root) %{_libdir}/lib*.so
%{_includedir}/quicktime

%files static
%defattr(644,root,root,755)
# FIXME: These are not being built for some reason: %{_libdir}/lib*.a

%define date	%(echo `LC_ALL="C" date +"%a %b %d %Y"`)
%changelog
