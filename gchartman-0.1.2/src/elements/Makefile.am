lib_LTLIBRARIES = libmain_chart.la libx-axis.la liby-axis.la libtrendline.la libmovavg.la \
		  libchannel.la librsi.la libmomentum.la libbuysell.la libcomment.la

CFLAGS = @CFLAGS@ -Wall -DPLUGINDIR=\"@plugindir@\"
LDFLAGS =

libdir = $(plugindir)

EXTRA_DIST = x-axisprop.glade main_chartprop.glade movavgprop.glade rsiprop.glade \
	     momentumprop.glade buysellprop.glade trendline.png movavg.png \
	     channel.png rsi.png momentum.png buysell.png comment.png \
	     commentprop.glade

mainchartpropdir = @plugindir@
mainchartprop_DATA = main_chartprop.glade
libmain_chart_la_LDFLAGS = -module -export-dynamic -avoid-version
libmain_chart_la_SOURCES = main_chart.c

xaxispropdir = @plugindir@
xaxisprop_DATA = x-axisprop.glade
libx_axis_la_LDFLAGS = -module -export-dynamic -avoid-version
libx_axis_la_SOURCES = x-axis.c

liby_axis_la_LDFLAGS = -module -export-dynamic -avoid-version
liby_axis_la_SOURCES = y-axis.c

trendlinepropdir = @plugindir@
trendlineprop_DATA = trendline.png
libtrendline_la_LDFLAGS = -module -export-dynamic -avoid-version
libtrendline_la_SOURCES = trendline.c

movavgpropdir = @plugindir@
movavgprop_DATA = movavgprop.glade movavg.png
libmovavg_la_LDFLAGS = -module -export-dynamic -avoid-version
libmovavg_la_SOURCES = movavg.c

libchannelpropdir = @plugindir@
libchannelprop_DATA = channel.png
libchannel_la_LDFLAGS = -module -export-dynamic -avoid-version
libchannel_la_SOURCES = channel.c

rsipropdir = @plugindir@
rsiprop_DATA = rsiprop.glade rsi.png
librsi_la_LDFLAGS = -module -export-dynamic -avoid-version
librsi_la_SOURCES = rsi.c

momentumpropdir = @plugindir@
momentumprop_DATA = momentumprop.glade momentum.png
libmomentum_la_LDFLAGS = -module -export-dynamic -avoid-version
libmomentum_la_SOURCES = momentum.c

buysellpropdir = @plugindir@
buysellprop_DATA = buysellprop.glade buysell.png
libbuysell_la_LDFLAGS = -module -export-dynamic -avoid-version
libbuysell_la_SOURCES = buysell.c

commentpropdir = @plugindir@
commentprop_DATA = commentprop.glade comment.png
libcomment_la_LDFLAGS = -module -export-dynamic -avoid-version
libcomment_la_SOURCES = comment.c
