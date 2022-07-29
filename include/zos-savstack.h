///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2022. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SAVSTACK_H_
#define ZOS_SAVSTACK_H_

#include "zos-macros.h"

#ifdef __cplusplus
#include "edcwccwi.h"

#include <map>
#include <mutex>

// ----------------------------------------------------------------------------
// LESavStackAsync
// https://www.ibm.com/docs/en/zos/2.4.0?topic=applications-saving-stack-pointer
//
// Before entry into JS code, save the current stack pointer SP that's stored
// at CEELCA_SAVSTACK_ASYNC, and replace it with a pointer to dynamic storage
// area DSA. After exit from JS, restore the stack pointer back into
// CEELCA_SAVSTACK_ASYNC. This class keeps track of the old and new SPs so
// they can be saved and restored, including when a signal occurs.
//
// save(x) is called before entry into JS to save x, the current SP at
// address __LE_SAVSTACK_ASYNC_ADDR, into map_'s key __LE_SAVSTACK_ASYNC_ADDR
//
// restore() is called after JS to restore into __LE_SAVSTACK_ASYNC_ADDR the
// SP saved in the map's key __LE_SAVSTACK_ASYNC_ADDR.
//
// restoreAll() is called from a signal handler to restore the SP from each
// thread into its corresponding __LE_SAVSTACK_ASYNC_ADDR.

class __Z_EXPORT LESavStackAsync {
  public:
    static LESavStackAsync& getInstance() {
      static LESavStackAsync instance;
      return instance;
    }

    void save(void* new_sp[1]) {
      std::lock_guard<std::mutex> lock(mutex_);
      void *old_sp = *__LE_SAVSTACK_ASYNC_ADDR;
      *__LE_SAVSTACK_ASYNC_ADDR = new_sp;
      // Store SP (r4) into new_sp[0]:
      asm(" lgr %0,4\n" : "=r"(new_sp[0])::);
      map_[__LE_SAVSTACK_ASYNC_ADDR] = old_sp;
    }

    void restore() {
      std::lock_guard<std::mutex> lock(mutex_);
      *__LE_SAVSTACK_ASYNC_ADDR = map_[__LE_SAVSTACK_ASYNC_ADDR];
      map_.erase(__LE_SAVSTACK_ASYNC_ADDR);
    }

    void restoreAll() {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto it = map_.cbegin();
           it != map_.cend(); ) {
        *(it->first) = it->second;
        it = map_.erase(it);
      }
    }

    LESavStackAsync(const LESavStackAsync&) = delete;
    LESavStackAsync& operator=(const LESavStackAsync&) = delete;
    LESavStackAsync(LESavStackAsync&&) = delete;
    LESavStackAsync& operator=(LESavStackAsync&&) = delete;

  private:
    LESavStackAsync() = default;

    std::map<void **, void *> map_;
    std::mutex mutex_;
};

#endif  // __cplusplus
#endif  // ZOS_SAVSTACK_H_
