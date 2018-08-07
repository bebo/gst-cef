function(DownloadDependency url filename)
  set(CEF_DOWNLOAD_DIR "${CMAKE_SOURCE_DIR}/third_party")
  set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/third_party/${filename}")

  # Download and/or extract the binary distribution if necessary.
  if(NOT IS_DIRECTORY "${EXTRACT_DIR}")
    set(CEF_DOWNLOAD_PATH "${CEF_DOWNLOAD_DIR}/${filename}.tar.bz2")
    if(NOT EXISTS "${CEF_DOWNLOAD_PATH}")
      set(CEF_DOWNLOAD_URL "${url}")

      # Download the binary distribution and verify the hash.
      # message(STATUS "Downloading ${CEF_DOWNLOAD_PATH}...")
      file(
        DOWNLOAD "${CEF_DOWNLOAD_URL}" "${CEF_DOWNLOAD_PATH}"
      #   EXPECTED_HASH SHA1=${CEF_SHA1}
        SHOW_PROGRESS
        )
    endif()

    # Extract the binary distribution.
    message(STATUS "Extracting ${CEF_DOWNLOAD_PATH}...")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xzf "${CEF_DOWNLOAD_DIR}/${filename}.tar.bz2"
      WORKING_DIRECTORY ${CEF_DOWNLOAD_DIR}
      )
  endif()
endfunction()