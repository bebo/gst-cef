# Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.
cmake_minimum_required(VERSION 2.8.12.1)
set(CMAKE_CONFIGURATION_TYPES Release Debug)

project(gst-cef)

set(CEF_VERSION "3.3538.1849.g458cc98")

# Determine the platform.
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(CEF_PLATFORM "macosx64")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(CEF_PLATFORM "linux64")
  else()
    set(CEF_PLATFORM "linux32")
  endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(CEF_PLATFORM "windows64")
  else()
    set(CEF_PLATFORM "windows32")
  endif()
endif()


set(CEF_DOWNLOAD_BASE_URL "http://opensource.spotify.com/cefbuilds/")
# set(CEF_DOWNLOAD_URL "https://s3-us-west-2.amazonaws.com/bebo-files/darkclouds/cefbuilds/${CEF_DOWNLOAD_FILENAME}")


set_property(GLOBAL PROPERTY OS_FOLDERS ON)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

set(SUBPROCESS "gst_cef" CACHE STRING "subprocess name")

# set(GST_INSTALL_BASE "${CMAKE_SOURCE_DIR}/third_party" CACHE INTERNAL "Path to the GStreamer install base")

#
# CEF configuration.
#

# Specify the CEF distribution version.
#set(CEF_VERSION "3.3282.1742.g96f907e")

# Add this project's cmake/ directory to the module path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Download and extract the CEF binary distribution (executes DownloadCEF.cmake).
include(DownloadCEF)
DownloadCEF("${CEF_PLATFORM}" "${CEF_VERSION}" "${CMAKE_BINARY_DIR}/cef")
#DownloadCEF("windows64" "${CEF_VERSION}" "${CMAKE_SOURCE_DIR}/third_party/cef")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

# Download Gstreamer
# FIXME
# include(DownloadDependency)
# DownloadDependency("https://s3-us-west-1.amazonaws.com/bebo-app/repo/gst-bebo/gst-bebo_bebo_build-20180811012020_dev.zip" "gstreamer")

find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0
                  gstreamer-video-1.0
		          gio-2.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

# Load the CEF configuration (executes FindCEF.cmake).
find_package(CEF REQUIRED)


#
# Python configuration.
#

# Support specification of the Python executable path via the command-line.
if(DEFINED ENV{PYTHON_EXECUTABLE})
  file(TO_CMAKE_PATH "$ENV{PYTHON_EXECUTABLE}" PYTHON_EXECUTABLE)
endif()

if(NOT PYTHON_EXECUTABLE)
  unset(PYTHON_EXECUTABLE)

  # Find the python interpreter.
  find_package(PythonInterp 2)

  if(NOT ${PYTHONINTERP_FOUND})
    message(FATAL_ERROR "A Python installation is required. Set the "
                        "PYTHON_EXECUTABLE environment variable to explicitly "
                        "specify the Python executable path.")
  endif()
endif()


#
# Clang-format configuration.
#
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(GS_PLATFORM "win32")
  set(GS_HASHPATH "win/clang-format.exe.sha1")



  message(STATUS "Downloading clang-format from Google Storage...")
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}"
            "tools/buildtools/download_from_google_storage.py"
            "--no_resume"
            "--platform=${GS_PLATFORM}"
            "--no_auth"
            "--bucket" "chromium-clang-format"
            "-s" "tools/buildtools/${GS_HASHPATH}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE EXECUTE_RV
    )
  if(NOT EXECUTE_RV STREQUAL "0")
    message(FATAL_ERROR "Execution failed with unexpected result: ${EXECUTE_RV}")
  endif()
endif()

SET_CEF_TARGET_OUT_DIR()


#
# Target configuration.
#

# Allow includes relative to the current source directory.
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)


# Display configuration settings.
PRINT_CEF_CONFIG()
