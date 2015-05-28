# Generated automatically from gchartman.spec.in by configure.

Summary: GNOME program for technical stock analysis
Name: gchartman
Version: 0.1.2
Release: 1
Source0: gchartman-%{version}.tar.gz
Group: X11/Applications
Copyright: GNU GENERAL PUBLIC LICENSE
URL: http://gchartman.sourceforge.net
Vendor: Christian Glodt <cglodt@users.sourceforge.net>
BuildRoot: /var/tmp/gchartman-%{version}.root

%description
gChartman is a GNOME program for technical stock analysis. It uses MySQL as
quote storage backend and is able to capture quotes from videotext broadcasts
(bttv-compatible tv tuner card required). This version of gChartman is compiled
to work with the kernel-included bttv driver.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/opt/gnome --disable-debug --enable-optimize
make

%install

# build stripped executable

make ROOT="$RPM_BUILD_ROOT" SUID_ROOT="" install-strip

# need to move the libs because the compilation path is compiled
# both into the executable and the libs. Thus, they need to be
# compiled to /opt/gnome/lib/gchartman, and then moved to
# $RPM_BUILD_ROOT/opt/gnome/lib/gchartman
## use upx to compress the executable

./mkinstalldirs "$RPM_BUILD_ROOT//opt/gnome/bin"
./mkinstalldirs "$RPM_BUILD_ROOT//opt/gnome/lib/gchartman"
./mkinstalldirs "$RPM_BUILD_ROOT//opt/gnome/share/gnome/apps/Applications"
#upx --best /opt/gnome/bin/gchartman
cp -a /opt/gnome/bin/gchartman "$RPM_BUILD_ROOT/opt/gnome/bin"
cp -a /opt/gnome/lib/gchartman/*.so "$RPM_BUILD_ROOT/opt/gnome/lib/gchartman"
cp -a /opt/gnome/lib/gchartman/*.glade "$RPM_BUILD_ROOT/opt/gnome/lib/gchartman"
cp -a /opt/gnome/lib/gchartman/*.png "$RPM_BUILD_ROOT/opt/gnome/lib/gchartman"
cp -a /opt/gnome/share/gnome/apps/Applications/gchartman.desktop "$RPM_BUILD_ROOT/opt/gnome/share/gnome/apps/Applications"

# strip the libraries. Users don't need symbols, and developers
# will compile for themselves

strip -S "$RPM_BUILD_ROOT"/opt/gnome/lib/gchartman/*.so

%files
%defattr(-,root,root)
/opt/gnome/bin/gchartman
/opt/gnome/lib/gchartman/*
/opt/gnome/share/gnome/apps/Applications/gchartman.desktop
%doc README ChangeLog DIARY COPYING NEWS TODO BUGS INSTALL scripts

%clean
rm -rf $RPM_BUILD_ROOT

%post
