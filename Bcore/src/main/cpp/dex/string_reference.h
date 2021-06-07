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

#ifndef ART_LIBDEXFILE_DEX_STRING_REFERENCE_H_
#define ART_LIBDEXFILE_DEX_STRING_REFERENCE_H_

#include <stdint.h>

#include <android-base/logging.h>

#include "dex/dex_file-inl.h"
#include "dex/dex_file_reference.h"
#include "dex/dex_file_types.h"
#include "dex/utf-inl.h"

namespace art_lkchan {

// A string is located by its DexFile and the string_ids_ table index into that DexFile.
class StringReference : public DexFileReference {
 public:
  StringReference(const DexFile* file, dex::StringIndex index)
     : DexFileReference(file, index.index_) {}

  dex::StringIndex StringIndex() const {
    return dex::StringIndex(index);
  }

  const char* GetStringData() const {
    return dex_file->GetStringData(dex_file->GetStringId(StringIndex()));
  }
};

// Compare the actual referenced string values. Used for string reference deduplication.
struct StringReferenceValueComparator {
  bool operator()(const StringReference& sr1, const StringReference& sr2) const {
    // Note that we want to deduplicate identical strings even if they are referenced
    // by different dex files, so we need some (any) total ordering of strings, rather
    // than references. However, the references should usually be from the same dex file,
    // so we choose the dex file string ordering so that we can simply compare indexes
    // and avoid the costly string comparison in the most common case.
    if (sr1.dex_file == sr2.dex_file) {
      // Use the string order enforced by the dex file verifier.
      DCHECK_EQ(
          sr1.index < sr2.index,
          CompareModifiedUtf8ToModifiedUtf8AsUtf16CodePointValues(sr1.GetStringData(),
                                                                  sr2.GetStringData()) < 0);
      return sr1.index < sr2.index;
    } else {
      // Cannot compare indexes, so do the string comparison.
      return CompareModifiedUtf8ToModifiedUtf8AsUtf16CodePointValues(sr1.GetStringData(),
                                                                     sr2.GetStringData()) < 0;
    }
  }
};

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_STRING_REFERENCE_H_
