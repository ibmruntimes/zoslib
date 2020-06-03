add_library(zoslib STATIC
  zos-semaphore.cc
  zos.cc
  celquopt.s)
INSTALL(TARGETS zoslib DESTINATION "install/lib")
