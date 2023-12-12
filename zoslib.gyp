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
        '_AE_BIMODAL=1',
        '_ALL_SOURCE',
        '_ENHANCED_ASCII_EXT=0xFFFFFFFF',
        '_LARGE_TIME_API',
        '_OPEN_MSGQ_EXT',
        '_OPEN_SYS_FILE_EXT=1',
        '_OPEN_SYS_SOCK_IPV6',
        'PATH_MAX=1024',
        '_UNIX03_SOURCE',
        '_UNIX03_THREADS',
        '_UNIX03_WITHDRAWN',
        '_XOPEN_SOURCE=600',
        '_XOPEN_SOURCE_EXTENDED',
      ],
      'conditions': [
        [ '"<!(echo $CC)" == "xlclang"', {
          'cflags': ['-q64', '-qascii', '-qexportall', '-Wno-missing-field-initializers', '-qasmlib=//\\\'SYS1.MACLIB\\\''],
          'include_dirs': ['include'],
        }, {
          'cflags': ['-fzos-le-char-mode=ascii', '-fgnu-keywords', '-fno-short-enums', '-mzos-target=zosv2r4'],
          'include_dirs': ['include', 'include/c++/v1'],
        }],
      ],
      'sources': [
        'src/zos.cc',
        'src/zos-bpx.cc',
        'src/zos-char-util.cc',
        'src/zos-getentropy.cc',
        'src/zos-io.cc',
        'src/zos-mount.c',
        'src/zos-semaphore.cc',
        'src/zos-spawn.cc',
        'src/zos-string.c',
        'src/zos-sys-info.cc',
        'src/zos-tls.cc',
        'src/celquopt.s',
      ],
    }
  ],
}
