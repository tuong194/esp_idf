# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/ESP_IDF/v5.3/esp-idf/components/bootloader/subproject"
  "C:/ESP_IDF/MY_CODE/station/build/bootloader"
  "C:/ESP_IDF/MY_CODE/station/build/bootloader-prefix"
  "C:/ESP_IDF/MY_CODE/station/build/bootloader-prefix/tmp"
  "C:/ESP_IDF/MY_CODE/station/build/bootloader-prefix/src/bootloader-stamp"
  "C:/ESP_IDF/MY_CODE/station/build/bootloader-prefix/src"
  "C:/ESP_IDF/MY_CODE/station/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/ESP_IDF/MY_CODE/station/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/ESP_IDF/MY_CODE/station/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
