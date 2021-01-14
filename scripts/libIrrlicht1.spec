# Copyright (c) 2007-2011 oc2pus
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments to toni@links2linux.de

# Packmangroup: Libraries
# Packmanpackagename: Irrlicht
# Packman: Toni Graffy

# norootforbuild

Name:			libIrrlicht1
Version:		1.9.0
Release:		0.pm.1
Summary:		The Irrlicht Engine SDK
License:		see readme.txt
Group:			System/Libraries
URL:			http://irrlicht.sourceforge.net/
Source:			irrlicht-%{version}.tar.bz2
BuildRoot:		%{_tmppath}/%{name}-%{version}-build
BuildRequires:	freeglut-devel
BuildRequires:	ImageMagick
BuildRequires:	gcc-c++
%if %suse_version >= 1020
BuildRequires:	Mesa-devel
%else
BuildRequires:	xorg-x11-devel
%endif
BuildRequires:	update-desktop-files

%description
The Irrlicht Engine is an open source high performance realtime 3d engine
written and usable in C++. It is completely cross-platform, using D3D, OpenGL
and its own software renderer, and has all of the state-of-the-art features
which can be found in commercial 3d engines.

We've got a huge active community, and there are lots of projects in
development that use the engine. You can find enhancements for Irrlicht all
over the web, like alternative terrain renderers, portal renderers, exporters,
world layers, tutorials, editors, language bindings for .NET, Java, Perl, Ruby,
Basic, Python, Lua, and so on. And best of all: It's completely free.

%package -n libIrrlicht-devel
Summary:	Development package for the Irrlicht library
Group:		Development/Languages/C and C++
Requires:	libIrrlicht1 = %{version}
# Packmandepends: libIrrlicht1

%description -n libIrrlicht-devel
The Irrlicht Engine is an open source high performance realtime 3d engine
written and usable in C++. It is completely cross-platform, using D3D, OpenGL
and its own software renderer, and has all of the state-of-the-art features
which can be found in commercial 3d engines.

We've got a huge active community, and there are lots of projects in
development that use the engine. You can find enhancements for Irrlicht all
over the web, like alternative terrain renderers, portal renderers, exporters,
world layers, tutorials, editors, language bindings for .NET, Java, Perl, Ruby,
Basic, Python, Lua, and so on. And best of all: It's completely free.

%package -n Irrlicht-doc
Summary:	User documentation for the Irrlicht SDK.
Group:		Documentation/Other

%description -n Irrlicht-doc
User documentation for the Irrlicht SDK.

You need a chm-viewer to read the docs (e.g. kchmviewer).

%package -n Irrlicht-media
Summary:	Some media files for Irrlicht SDK
Group:		Development/Languages/C and C++

%description -n Irrlicht-media
Some media files for Irrlicht tools and demos.

%debug_package

%prep
%setup -q -n irrlicht-%{version}

%build
# create shared-lib first
pushd source/Irrlicht
%__make sharedlib %{?_smp_mflags}
popd

# create necessary links to avoid linker-error for tools/examples
pushd lib/Linux
ln -s libIrrlicht.so.%{version} libIrrlicht.so.1
ln -s libIrrlicht.so.%{version} libIrrlicht.so
popd

# build static lib
pushd source/Irrlicht
%__make %{?_smp_mflags}
popd

%install
%__install -dm 755 %{buildroot}%{_libdir}
%__install -m 644 lib/Linux/libIrrlicht.a \
	%{buildroot}%{_libdir}
%__install -m 644 lib/Linux/libIrrlicht.so.%{version} \
	%{buildroot}%{_libdir}

pushd %{buildroot}%{_libdir}
ln -s libIrrlicht.so.%{version} libIrrlicht.so.1
ln -s libIrrlicht.so.%{version} libIrrlicht.so
popd

# includes
%__install -dm 755 %{buildroot}%{_includedir}/irrlicht
%__install -m 644 include/*.h \
	%{buildroot}%{_includedir}/irrlicht

# media
%__install -dm 755 %{buildroot}%{_datadir}/irrlicht
%__install -m 755 media/* \
	%{buildroot}%{_datadir}/irrlicht

%clean
[ -d %{buildroot} -a "%{buildroot}" != "" ] && %__rm -rf %{buildroot}

%files
%defattr(-, root, root)
%doc *.txt
%{_libdir}/lib*.so.*

%files -n libIrrlicht-devel
%defattr(-, root, root)
%{_libdir}/lib*.so
%{_libdir}/lib*.a
%dir %{_includedir}/irrlicht
%{_includedir}/irrlicht/*.h

%files -n Irrlicht-doc
%defattr(-, root, root)
%doc doc/irrlicht.chm
%doc doc/*.txt

%files -n Irrlicht-media
%defattr(-, root, root)
%dir %{_datadir}/irrlicht
%{_datadir}/irrlicht/*

%changelog
* Wed Jun 20 2007 Toni Graffy <toni@links2linux.de> - 1.3.1-0.pm.1
- update to 1.3.1
* Sat Jun 16 2007 Toni Graffy <toni@links2linux.de> - 1.3-0.pm.1
- initial build 1.3
