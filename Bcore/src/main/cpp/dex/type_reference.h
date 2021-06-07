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

#ifndef ART_LIBDEXFILE_DEX_TYPE_REFERENCE_H_
#define ART_LIBDEXFILE_DEX_TYPE_REFERENCE_H_

#include <stdint.h>

#include <android-base/logging.h>

#include "dex/dex_file_types.h"
#include "dex/string_reference.h"

namespace art_lkchan {

class DexFile;

// A type is located by its DexFile and the string_ids_ table index into that DexFile.
class TypeReference : public DexFileReference {
 public:
  TypeReference(const DexFile* file, dex::TypeIndex index)
      : DexFileReference(file, index.index_) {}

  dex::TypeIndex TypeIndex() const {
    return dex::TypeIndex(index);
  }
};

// Compare the actual referenced type names. Used for type reference deduplication.
struct TypeReferenceValueComparator {
  bool operator()(const TypeReference& tr1, const TypeReference& tr2) const {
    // Note that we want to deduplicate identical boot image types even if they are
    // referenced by different dex files, so we simply compare the descriptors.
    StringReference sr1(tr1.dex_file, tr1.dex_file->GetTypeId(tr1.TypeIndex()).descriptor_idx_);
    StringReference sr2(tr2.dex_file, tr2.dex_file->GetTypeId(tr2.TypeIndex()).descriptor_idx_);
    return StringReferenceValueComparator()(sr1, sr2);
  }
};

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_TYPE_REFERENCE_H_
