###############################################################################
# Licensed Materials - Property of IBM
# ZOSLIB
# (C) Copyright IBM Corp. 2020. All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication
# or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
###############################################################################

add_library(zoslib STATIC
  zos-semaphore.cc
  zos-sys-info.cc
  zos-bpx.cc
  zos-char-util.cc
  zos-io.cc
  zos-tls.cc
  zos.cc
  celquopt.s)
INSTALL(TARGETS zoslib DESTINATION "install/lib")
