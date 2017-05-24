# Note: plugindir is set in configure

plugin_LTLIBRARIES = libgstcef.la

# sources used to compile this plug-in
libgstcef_la_SOURCES = gstcef.c cef.cpp cef.h gstcef.h cef_app.cpp cef_app.h cef_handler.cpp cef_handler_linux.cpp cef_handler.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstcef_la_CFLAGS = $(GST_CFLAGS)
libgstcef_la_CPPFLAGS = $(GTK_CFLAGS) -I/usr/local/cef
libgstcef_la_LIBADD = $(GST_LIBS) -L/usr/local/cef/lib
libgstcef_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -lcef -lcef_dll_wrapper
libgstcef_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstcef.h cef.h cef_app.h

bin_PROGRAMS = subprocess
subprocess_SOURCES = subprocess.cpp
subprocess_CFLAGS = $(GST_CFLAGS)
subprocess_CPPFLAGS = $(GTK_CFLAGS) -I/usr/local/cef
subprocess_LDADD = $(GST_LIBS) -L/usr/local/cef/lib
subprocess_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -lcef -lcef_dll_wrapper
subprocess_LIBTOOLFLAGS = --tag=disable-static
