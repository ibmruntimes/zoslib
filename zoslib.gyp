###############################################################################
# Licensed Materials - Property of IBM
# ZOSLIB
# (C) Copyright IBM Corp. 2020. All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication
# or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
###############################################################################

{
  'target_defaults': {
  },
  'targets': [
    {
      'target_name': 'zoslib',
      'type': 'static_library',
      'include_dirs': [
        'include',
      ],
      'sources': [
        'src/celquopt.s',
        'src/zos.cc',
        'src/zos-bpx.cc',
        'src/zos-char-util.cc',
        'src/zos-getentropy.cc',
        'src/zos-io.cc',
        'src/zos-locale.cc',
        'src/zos-mkdtemp.c',
        'src/zos-mount.c',
        'src/zos-semaphore.cc',
        'src/zos-spawn.cc',
        'src/zos-string.c',
        'src/zos-sys-info.cc',
        'src/zos-tls.cc',
      ],
      # Undefine ZOSLIB_ALIGNED_NEWDEL so libzoslib.so isn't affected by the
      # overridden new and delete operators when zoslib is built as part of a
      # project that defines the macro.
      'defines!': [
        'ZOSLIB_ALIGNED_NEWDEL'
      ],
    },
    {
      'target_name': 'zoslib_alnewdel',
      'type': 'static_library',
      'sources': [
        'src/alnewdel/zos-aligned-newdel.cc',
      ],
      'include_dirs+': [
        'include-wrappers/c++',
      ],
    },
  ],
}
