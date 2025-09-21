# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/ESP32_IDF/Espressif/frameworks/esp-idf-v5.5/components/bootloader/subproject")
  file(MAKE_DIRECTORY "D:/ESP32_IDF/Espressif/frameworks/esp-idf-v5.5/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/build/bootloader"
  "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/build/bootloader-prefix"
  "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/build/bootloader-prefix/tmp"
  "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/build/bootloader-prefix/src/bootloader-stamp"
  "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/build/bootloader-prefix/src"
  "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
