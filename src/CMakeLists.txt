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
  zos-locale.cc
  zos-semaphore.cc
  zos-spawn.cc
  zos-string.c
  zos-sys-info.cc
  zos-tls.cc
  zos.cc
  zos-mount.c
  zos-mkdtemp.c
)
set(libalnewdel alnewdel/zos-aligned-newdel.cc)
set(zoslib-help zoslib-help.cc)

set(CELQUOPT_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/celquopt.s.o")
set(CELQUOPT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/celquopt.s")
set_source_files_properties(${CELQUOPT_OBJECT}
  PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE
)
add_custom_command(OUTPUT ${CELQUOPT_OBJECT}
COMMAND /bin/as -mgoff -o ${CELQUOPT_OBJECT} ${CELQUOPT_SOURCE}
  DEPENDS ${CELQUOPT_SOURCE}
  COMMENT "Generate celquopt.s.o object"
  VERBATIM
)

add_library(libzoslib OBJECT ${libsrc})
add_library(libzoslib_alnewdel OBJECT ${libalnewdel})

add_library(zoslib SHARED $<TARGET_OBJECTS:libzoslib>)
add_library(zoslib_a STATIC $<TARGET_OBJECTS:libzoslib> ${CELQUOPT_OBJECT})
add_library(zoslib_alnewdel_a STATIC $<TARGET_OBJECTS:libzoslib_alnewdel>)
add_executable(zoslib-help ${zoslib-help})
target_link_libraries(zoslib-help libzoslib)
target_compile_options(zoslib_alnewdel_a PRIVATE ${zoslib_cflags})

set_target_properties(zoslib_a PROPERTIES OUTPUT_NAME zoslib)
set_target_properties(zoslib_alnewdel_a PROPERTIES OUTPUT_NAME zoslib_alnewdel)

install(
    DIRECTORY ${PROJECT_BINARY_DIR}/lib/
    DESTINATION "lib"
    FILES_MATCHING PATTERN "*.?")

install(
    DIRECTORY ${PROJECT_BINARY_DIR}/lib/
    DESTINATION "lib"
    FILES_MATCHING PATTERN "*.so"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(
    DIRECTORY ${PROJECT_BINARY_DIR}/src/
    DESTINATION "bin"
    FILES_MATCHING PATTERN "zoslib-help"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES ${CELQUOPT_OBJECT} DESTINATION "lib")

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION "include"
    FILES_MATCHING PATTERN "*")
