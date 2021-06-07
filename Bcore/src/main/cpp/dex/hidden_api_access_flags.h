/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_LIBDEXFILE_DEX_HIDDEN_API_ACCESS_FLAGS_H_
#define ART_LIBDEXFILE_DEX_HIDDEN_API_ACCESS_FLAGS_H_

#include "base/bit_utils.h"
#include "base/macros.h"
#include "dex/modifiers.h"

namespace art_lkchan {

/* This class is used for encoding and decoding access flags of class members
 * from the boot class path. These access flags might contain additional two bits
 * of information on whether the given class member should be hidden from apps
 * and under what circumstances.
 *
 * The encoding is different inside DexFile, where we are concerned with size,
 * and at runtime where we want to optimize for speed of access. The class
 * provides helper functions to decode/encode both of them.
 *
 * Encoding in DexFile
 * ===================
 *
 * First bit is encoded as inversion of visibility flags (public/private/protected).
 * At most one can be set for any given class member. If two or three are set,
 * this is interpreted as the first bit being set and actual visibility flags
 * being the complement of the encoded flags.
 *
 * Second bit is either encoded as bit 5 for fields and non-native methods, where
 * it carries no other meaning. If a method is native (bit 8 set), bit 9 is used.
 *
 * Bits were selected so that they never increase the length of unsigned LEB-128
 * encoding of the access flags.
 *
 * Encoding at runtime
 * ===================
 *
 * Two bits are set aside in the uint32_t access flags in the intrinsics ordinal
 * space (thus intrinsics need to be special-cased). These are two consecutive
 * bits and they are directly used to store the integer value of the ApiList
 * enum values.
 *
 */
class HiddenApiAccessFlags {
 public:
  enum ApiList {
    kWhitelist = 0,
    kLightGreylist,
    kDarkGreylist,
    kBlacklist,
  };

  static ALWAYS_INLINE ApiList DecodeFromDex(uint32_t dex_access_flags) {
    DexHiddenAccessFlags flags(dex_access_flags);
    uint32_t int_value = (flags.IsFirstBitSet() ? 1 : 0) + (flags.IsSecondBitSet() ? 2 : 0);
    return static_cast<ApiList>(int_value);
  }

  static ALWAYS_INLINE uint32_t RemoveFromDex(uint32_t dex_access_flags) {
    DexHiddenAccessFlags flags(dex_access_flags);
    flags.SetFirstBit(false);
    flags.SetSecondBit(false);
    return flags.GetEncoding();
  }

  static ALWAYS_INLINE uint32_t EncodeForDex(uint32_t dex_access_flags, ApiList value) {
    DexHiddenAccessFlags flags(RemoveFromDex(dex_access_flags));
    uint32_t int_value = static_cast<uint32_t>(value);
    flags.SetFirstBit((int_value & 1) != 0);
    flags.SetSecondBit((int_value & 2) != 0);
    return flags.GetEncoding();
  }

  static ALWAYS_INLINE ApiList DecodeFromRuntime(uint32_t runtime_access_flags) {
    // This is used in the fast path, only DCHECK here.
    DCHECK_EQ(runtime_access_flags & kAccIntrinsic, 0u);
    uint32_t int_value = (runtime_access_flags & kAccHiddenApiBits) >> kAccFlagsShift;
    return static_cast<ApiList>(int_value);
  }

  static ALWAYS_INLINE uint32_t EncodeForRuntime(uint32_t runtime_access_flags, ApiList value) {
    CHECK_EQ(runtime_access_flags & kAccIntrinsic, 0u);

    uint32_t hidden_api_flags = static_cast<uint32_t>(value) << kAccFlagsShift;
    CHECK_EQ(hidden_api_flags & ~kAccHiddenApiBits, 0u);

    runtime_access_flags &= ~kAccHiddenApiBits;
    return runtime_access_flags | hidden_api_flags;
  }

 private:
  static const int kAccFlagsShift = CTZ(kAccHiddenApiBits);
  static_assert(IsPowerOfTwo((kAccHiddenApiBits >> kAccFlagsShift) + 1),
                "kAccHiddenApiBits are not continuous");

  struct DexHiddenAccessFlags {
    explicit DexHiddenAccessFlags(uint32_t access_flags) : access_flags_(access_flags) {}

    ALWAYS_INLINE uint32_t GetSecondFlag() {
      return ((access_flags_ & kAccNative) != 0) ? kAccDexHiddenBitNative : kAccDexHiddenBit;
    }

    ALWAYS_INLINE bool IsFirstBitSet() {
      static_assert(IsPowerOfTwo(0u), "Following statement checks if *at most* one bit is set");
      return !IsPowerOfTwo(access_flags_ & kAccVisibilityFlags);
    }

    ALWAYS_INLINE void SetFirstBit(bool value) {
      if (IsFirstBitSet() != value) {
        access_flags_ ^= kAccVisibilityFlags;
      }
    }

    ALWAYS_INLINE bool IsSecondBitSet() {
      return (access_flags_ & GetSecondFlag()) != 0;
    }

    ALWAYS_INLINE void SetSecondBit(bool value) {
      if (value) {
        access_flags_ |= GetSecondFlag();
      } else {
        access_flags_ &= ~GetSecondFlag();
      }
    }

    ALWAYS_INLINE uint32_t GetEncoding() const {
      return access_flags_;
    }

    uint32_t access_flags_;
  };
};

inline std::ostream& operator<<(std::ostream& os, HiddenApiAccessFlags::ApiList value) {
  switch (value) {
    case HiddenApiAccessFlags::kWhitelist:
      os << "whitelist";
      break;
    case HiddenApiAccessFlags::kLightGreylist:
      os << "light greylist";
      break;
    case HiddenApiAccessFlags::kDarkGreylist:
      os << "dark greylist";
      break;
    case HiddenApiAccessFlags::kBlacklist:
      os << "blacklist";
      break;
  }
  return os;
}

}  // namespace art_lkchan


#endif  // ART_LIBDEXFILE_DEX_HIDDEN_API_ACCESS_FLAGS_H_
