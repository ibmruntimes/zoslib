###############################################################################
# Licensed Materials - Property of IBM
# ZOSLIB
# (C) Copyright IBM Corp. 2020. All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication
# or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
###############################################################################

{
  'targets': [
    {
      'target_name': 'zoslib',
      'type': 'static_library',
      'defines': [
        '_UNIX03_THREADS',
        '_UNIX03_SOURCE',
        '_UNIX03_WITHDRAWN',
        '_OPEN_SYS_IF_EXT',
        '_OPEN_SYS_SOCK_IPV6',
        '_OPEN_MSGQ_EXT',
        '_XOPEN_SOURCE_EXTENDED',
        '_ALL_SOURCE',
        '_LARGE_TIME_API',
        '_OPEN_SYS_FILE_EXT',
        '_AE_BIMODAL',
        'PATH_MAX=1023',
        '_ENHANCED_ASCII_EXT=0xFFFFFFFF',
      ],
      'cflags': ['-q64', '-qascii', '-qexportall', '-Wno-missing-field-initializers', '-qasmlib=//\\\'SYS1.MACLIB\\\''],
      'direct_dependent_settings': {
        'include_dirs': ['include'],
      },
      'include_dirs': ['.', 'include'],
      'sources': [
        'src/zos.cc',
        'src/zos-semaphore.cc',
        'src/celquopt.s'
      ],
    }
  ],
}
