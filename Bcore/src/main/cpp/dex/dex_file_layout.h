/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ART_LIBDEXFILE_DEX_DEX_FILE_LAYOUT_H_
#define ART_LIBDEXFILE_DEX_DEX_FILE_LAYOUT_H_

#include <algorithm>
#include <cstdint>
#include <iosfwd>

#include <android-base/logging.h>

namespace art_lkchan {

class DexFile;

enum class LayoutType : uint8_t {
  // Layout of things that are hot (commonly accessed), these should be pinned or madvised will
  // need.
  kLayoutTypeHot,
  // Layout of things that are randomly used. These should be advised to random access.
  // Without layout, this is the default mode when loading a dex file.
  kLayoutTypeSometimesUsed,
  // Layout of things that are only used during startup, these can be madvised after launch.
  kLayoutTypeStartupOnly,
  // Layout of things that are needed probably only once (class initializers). These can be
  // madvised during trim events.
  kLayoutTypeUsedOnce,
  // Layout of things that are thought to be unused. These things should be advised to random
  // access.
  kLayoutTypeUnused,
  // Unused value, just the number of elements in the enum.
  kLayoutTypeCount,
};
std::ostream& operator<<(std::ostream& os, const LayoutType& collector_type);

// Return the "best" layout option if the same item has multiple different layouts.
static inline LayoutType MergeLayoutType(LayoutType a, LayoutType b) {
  return std::min(a, b);
}

enum class MadviseState : uint8_t {
  // Madvise based on a file that was just loaded.
  kMadviseStateAtLoad,
  // Madvise based after launch is finished.
  kMadviseStateFinishedLaunch,
  // Trim by madvising code that is unlikely to be too important in the future.
  kMadviseStateFinishedTrim,
};
std::ostream& operator<<(std::ostream& os, const MadviseState& collector_type);

// A dex layout section such as code items or strings. Each section is composed of subsections
// that are laid out adjacently to each other such as (hot, unused, startup, etc...).
class DexLayoutSection {
 public:
  // A subsection is a a continuous range of dex file that is all part of the same layout hint.
  class Subsection {
   public:
    // Use uint32_t to handle 32/64 bit cross compilation.
    uint32_t start_offset_ = 0u;
    uint32_t end_offset_ = 0u;

    bool Contains(uint32_t offset) const {
      return start_offset_ <= offset && offset < end_offset_;
    }

    bool Size() const {
      DCHECK_LE(start_offset_, end_offset_);
      return end_offset_ - start_offset_;
    }

    void CombineSection(uint32_t start_offset, uint32_t end_offset) {
      DCHECK_LE(start_offset, end_offset);
      if (start_offset_ == end_offset_) {
        start_offset_ = start_offset;
        end_offset_ = end_offset;
      } else  {
        start_offset_ = std::min(start_offset_, start_offset);
        end_offset_ = std::max(end_offset_, end_offset);
      }
    }

    void Madvise(const DexFile* dex_file, int advice) const;
  };

  // Madvise the largest page-aligned region contained in [begin, end).
  static int MadviseLargestPageAlignedRegion(const uint8_t* begin, const uint8_t* end, int advice);

  Subsection parts_[static_cast<size_t>(LayoutType::kLayoutTypeCount)];
};

// A set of dex layout sections, currently there is only one section for code and one for strings.
class DexLayoutSections {
 public:
  enum class SectionType : uint8_t {
    kSectionTypeCode,
    kSectionTypeStrings,
    kSectionCount,
  };

  // Advise access about the dex file based on layout. The caller is expected to have already
  // madvised to MADV_RANDOM.
  void Madvise(const DexFile* dex_file, MadviseState state) const;

  DexLayoutSection sections_[static_cast<size_t>(SectionType::kSectionCount)];
};

std::ostream& operator<<(std::ostream& os, const DexLayoutSections::SectionType& collector_type);
std::ostream& operator<<(std::ostream& os, const DexLayoutSection& section);
std::ostream& operator<<(std::ostream& os, const DexLayoutSections& sections);

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_DEX_FILE_LAYOUT_H_
