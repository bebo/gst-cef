# Note: plugindir is set in configure

plugin_LTLIBRARIES = libgstcef.la

# sources used to compile this plug-in
libgstcef_la_SOURCES = gstcef.c gstcef.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstcef_la_CFLAGS = $(GST_CFLAGS)
libgstcef_la_LIBADD = $(GST_LIBS)
libgstcef_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstcef_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstcef.h
