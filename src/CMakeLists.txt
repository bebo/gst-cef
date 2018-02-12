# Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.


SET(GST_INSTALL_BASE "C:\\gstreamer\\1.0\\x86" CACHE PATH "Path to the GStreamer install base")

INCLUDE_DIRECTORIES(
  ${GST_INSTALL_BASE}/include
  ${GST_INSTALL_BASE}/include/gstreamer-1.0
  ${GST_INSTALL_BASE}/include/glib-2.0
  ${GST_INSTALL_BASE}/lib/glib-2.0/include
  ${SDK_INSTALL_BASE}/Samples/multimedia/directshow/baseclasses
  ${PROJECT_SOURCE_DIR}/../../win32/common
)

#
# Source files.
#
# Main executable sources.
set(EXAMPLE_SRCS
  app_browser_minimal.cc
  client_minimal.cc
  client_minimal.h
  main_minimal.cc
)

ADD_LIBRARY(libgstdshowdecwrapper SHARED
  gstdshowaudiodec.cpp
  gstdshowaudiodec.h
  gstdshowvideodec.cpp
  gstdshowvideodec.h
  gstdshowdecwrapper.cpp
  gstdshowdecwrapper.h
  gstdshowfakesrc.cpp
  gstdshowfakesrc.h
  gstdshowutil.cpp
  gstdshowutil.h
)
