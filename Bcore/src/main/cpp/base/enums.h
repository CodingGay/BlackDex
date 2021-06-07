/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ART_LIBARTBASE_BASE_ENUMS_H_
#define ART_LIBARTBASE_BASE_ENUMS_H_

#include <cstddef>
#include <ostream>

namespace art_lkchan {

enum class PointerSize : size_t {
  k32 = 4,
  k64 = 8
};

inline std::ostream& operator<<(std::ostream& os, const PointerSize& rhs) {
  switch (rhs) {
    case PointerSize::k32: os << "k32"; break;
    case PointerSize::k64: os << "k64"; break;
    default: os << "PointerSize[" << static_cast<int>(rhs) << "]"; break;
  }
  return os;
}

static constexpr PointerSize kRuntimePointerSize = sizeof(void*) == 8U
                                                       ? PointerSize::k64
                                                       : PointerSize::k32;

}  // namespace art_lkchan

#endif  // ART_LIBARTBASE_BASE_ENUMS_H_
