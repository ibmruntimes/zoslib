###############################################################################
# Licensed Materials - Property of IBM
# ZOSLIB
# (C) Copyright IBM Corp. 2020. All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication
# or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
###############################################################################

set(libsrc
  zos-bpx.cc
  zos-char-util.cc
  zos-getentropy.cc
  zos-io.cc
  zos-semaphore.cc
  zos-sys-info.cc
  zos-tls.cc
  zos.cc
  celquopt.s
)

add_library(libzoslib OBJECT ${libsrc})

add_library(zoslib SHARED $<TARGET_OBJECTS:libzoslib>)
add_library(zoslib_a STATIC $<TARGET_OBJECTS:libzoslib>)

set_target_properties(zoslib_a PROPERTIES OUTPUT_NAME zoslib)

install(
    DIRECTORY ${PROJECT_BINARY_DIR}/lib/
    DESTINATION "lib"
    FILES_MATCHING PATTERN "*")

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION "include"
    FILES_MATCHING PATTERN "*")
