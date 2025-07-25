///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB_ALNEWDEL
// (C) Copyright IBM Corp. 2025. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
//
// The code in this source file is based on
// https://github.com/llvm/llvm-project/blob/main/libcxx/src/new.cpp
// for the aligned new and delete.
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
///////////////////////////////////////////////////////////////////////////////

#if defined(__clang__) && !defined(__ibmxl__) && __cplusplus >= 201703L

#include <new>

#include "zos-base.h"


static void* operator_new_aligned_impl(std::size_t size, std::align_val_t al) {
  if (size == 0)
    size = 1;
  if (static_cast<size_t>(al) < sizeof(void*))
    al = std::align_val_t(sizeof(void*));

  void* p;
  while ((p =__aligned_malloc(size, static_cast<size_t>(al))) == nullptr) {
    // If malloc fails and there is a new_handler, call it to try free up memory
    std::new_handler nh = std::get_new_handler();
    if (nh)
      nh();
    else
      break;
  }
  return p;
}

extern "C" {

__Z_EXPORT void* zoslib_operator_new(std::size_t size, std::align_val_t al) _THROW_BAD_ALLOC {
  void *p = operator_new_aligned_impl(size, al);
  if (p == nullptr)
    std::__throw_bad_alloc();
  return p;
}

__Z_EXPORT void* zoslib_operator_new_not_noe(size_t size, std::align_val_t al, const std::nothrow_t&) _NOEXCEPT {
#if !__EXCEPTIONS
  return operator_new_aligned_impl(size, al);
#else
  void* p = nullptr;
  try {
    p = zoslib_operator_new(size, al);
  } catch (...) {
  }
  return p;
#endif
}

__Z_EXPORT void* zoslib_operator_new_ar(std::size_t size, std::align_val_t al) _THROW_BAD_ALLOC {
  return zoslib_operator_new(size, al);
}

__Z_EXPORT void* zoslib_operator_new_ar_not_noe(size_t size, std::align_val_t al, const std::nothrow_t&) _NOEXCEPT {
#if !__EXCEPTIONS
  return operator_new_aligned_impl(size, al);
#else
  void* p = nullptr;
  try {
    p = zoslib_operator_new_ar(size, al);
  } catch (...) {
  }
  return p;
#endif
}

__Z_EXPORT void zoslib_operator_delete_noe(void* ptr, std::align_val_t al) _NOEXCEPT {
  __aligned_free(ptr);
}

__Z_EXPORT void zoslib_operator_delete_not_noe(void* ptr, std::align_val_t al, const std::nothrow_t&) _NOEXCEPT {
  zoslib_operator_delete_noe(ptr, al);
}

__Z_EXPORT void zoslib_operator_delete_ar_noe(void* ptr, std::align_val_t al) _NOEXCEPT {
  zoslib_operator_delete_noe(ptr, al);
}

__Z_EXPORT void zoslib_operator_delete_ar_not_noe(void* ptr, std::align_val_t al, const std::nothrow_t&) _NOEXCEPT {
  zoslib_operator_delete_ar_noe(ptr, al);
}

} // extern "C"

#endif
