add_library(zoslib STATIC
  zos-semaphore.cc
  zos.cc)
INSTALL(TARGETS zoslib DESTINATION "install/lib")
