# Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

# See the included README.md file for usage instructions.

cmake_minimum_required(VERSION 2.8.12.1)

# Only generate Debug and Release configuration types.
set(CMAKE_CONFIGURATION_TYPES Release Debug)

# Project name.
project(gst-cef)

# Use folders in the resulting project files.
set_property(GLOBAL PROPERTY OS_FOLDERS ON)
set(GST_INSTALL_BASE "${CMAKE_SOURCE_DIR}/third_party" CACHE INTERNAL "Path to the GStreamer install base")

#
# CEF configuration.
#

# Specify the CEF distribution version.
set(CEF_VERSION "3.3282.1742.g96f907e")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Zi")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

# Add this project's cmake/ directory to the module path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Download and extract the CEF binary distribution (executes DownloadCEF.cmake).
include(DownloadCEF)
DownloadCEF("windows64" "${CEF_VERSION}" "${CMAKE_SOURCE_DIR}/third_party/cef")

# Download Gstreamer
include(DownloadDependency)
DownloadDependency("https://s3-us-west-1.amazonaws.com/bebo-app/repo/gst-bebo/gst-bebo_bebo_build-20180811012020_dev.zip" "gstreamer")

# Add the CEF binary distribution's cmake/ directory to the module path.
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


#
# Target configuration.
#

# Allow includes relative to the current source directory.
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)


# Display configuration settings.
PRINT_CEF_CONFIG()
