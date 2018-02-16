# Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.10)

SET(GST_INSTALL_BASE "C:\\gstreamer\\1.0\\x86_64" CACHE PATH "Path to the GStreamer install base")

SET_PROPERTY(
  DIRECTORY
  APPEND PROPERTY COMPILE_DEFINITIONS
)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

INCLUDE_DIRECTORIES(
  ${GST_INSTALL_BASE}/include
  ${GST_INSTALL_BASE}/include/gstreamer-1.0
  ${GST_INSTALL_BASE}/include/glib-2.0
  ${GST_INSTALL_BASE}/lib/glib-2.0/include
  ${PROJECT_SOURCE_DIR}/../../win32/common
  ${CMAKE_SOURCE_DIR}/third_party/cef/cef_binary_${CEF_VERSION}_windows64
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
  rpcrt4
  uuid
  winmm
  dmoguids
  wmcodecdspuuid
  msdmo
  libcef_lib
  libcef_dll_wrapper
)

if(USE_SANDBOX)
  # Logical target used to link the cef_sandbox library.
  ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
  TARGET_LINK_LIBRARIES(libgstcef
    cef_sandbox_lib
    ${CEF_SANDBOX_STANDARD_LIBS}
  )
endif()

ADD_EXECUTABLE(subprocess WIN32
  subprocess.cpp
  cef_app.h
)

TARGET_LINK_LIBRARIES(subprocess
  libcef_lib
  libcef_dll_wrapper
)

SET_TARGET_PROPERTIES(subprocess PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

# Copy binary and resource files to the target output directory.
set(DIST_DIR "${CMAKE_BINARY_DIR}/dist")
file(MAKE_DIRECTORY ${DIST_DIR}/locales)
COPY_FILES(libgstcef "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${DIST_DIR}")

set(RESOURCE_FILES 
  cef.pak
  cef_100_percent.pak
  cef_200_percent.pak
  cef_extensions.pak
  devtools_resources.pak
  icudtl.dat
)
COPY_FILES(libgstcef "${RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${DIST_DIR}")

set(ENGLISH_FILES 
  en-US.pak
  en-US.pak.info
)
COPY_FILES(libgstcef "${ENGLISH_FILES}" "${CEF_RESOURCE_DIR}/locales" "${DIST_DIR}/locales")
COPY_FILES(libgstcef "libgstcef.dll" "${CMAKE_BINARY_DIR}/src/$<CONFIGURATION>" "${DIST_DIR}")
COPY_FILES(subprocess "subprocess.exe" "${CMAKE_BINARY_DIR}/src/$<CONFIGURATION>" "${DIST_DIR}")
