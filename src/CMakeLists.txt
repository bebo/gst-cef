# Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.10)

SET(GST_INSTALL_BASE "C:\\gstreamer\\1.0\\x86_64" CACHE PATH "Path to the GStreamer install base")

SET_PROPERTY(
  DIRECTORY
  APPEND PROPERTY COMPILE_DEFINITIONS
)

INCLUDE_DIRECTORIES(
  ${GST_INSTALL_BASE}/include
  ${GST_INSTALL_BASE}/include/gstreamer-1.0
  ${GST_INSTALL_BASE}/include/glib-2.0
  ${GST_INSTALL_BASE}/lib/glib-2.0/include
  ${PROJECT_SOURCE_DIR}/../../win32/common
  ${CMAKE_SOURCE_DIR}/third_party/cef/cef_binary_3.3282.1732.g973997f_windows64
)

LINK_DIRECTORIES(
  ${GST_INSTALL_BASE}/lib
)

ADD_LIBRARY(libgstcef SHARED
  gstcef.c
  cef.cpp
  cef.h
  gstcef.h
  cef_app.cpp
  cef_app.h
  cef_handler.cpp
  cef_handler_linux.cpp
  cef_handler.h
)

# We probably don't need most of these libraries.
TARGET_LINK_LIBRARIES(libgstcef
  gstreamer-1.0
  gstaudio-1.0
  gstvideo-1.0
  gstbase-1.0
  glib-2.0
  gobject-2.0
  debug strmbasd
  rpcrt4
  uuid
  winmm
  dmoguids
  wmcodecdspuuid
  msdmo
)
