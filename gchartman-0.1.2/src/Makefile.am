SUBDIRS = elements
bin_PROGRAMS = gchartman
gchartman_SOURCES = gchartman.c gchartman.h \
		  connectdb.c connectdb.h \
		  about.c about.h \
		  chartwindow.c chartwindow.h \
		  getvtquotes.c getvtquotes.h module.h

EXTRA_DIST = gtk.png zoomxplus.png zoomxminus.png zoomyplus.png zoomyminus.png


gchartman_LDFLAGS = -module -export-dynamic
CFLAGS = @CFLAGS@ -Wall -DPLUGINDIR=\"@plugindir@\"

iconsdir = @plugindir@
icons_DATA = gtk.png zoomxplus.png zoomxminus.png zoomyplus.png zoomyminus.png
