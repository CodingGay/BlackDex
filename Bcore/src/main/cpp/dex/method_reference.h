/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ART_LIBDEXFILE_DEX_METHOD_REFERENCE_H_
#define ART_LIBDEXFILE_DEX_METHOD_REFERENCE_H_

#include <stdint.h>
#include <string>
#include "dex/dex_file.h"
#include "dex/dex_file_reference.h"

namespace art_lkchan {

// A method is uniquely located by its DexFile and the method_ids_ table index into that DexFile
class MethodReference : public DexFileReference {
 public:
  MethodReference(const DexFile* file, uint32_t index) : DexFileReference(file, index) {}
  std::string PrettyMethod(bool with_signature = true) const {
    return dex_file->PrettyMethod(index, with_signature);
  }
  const DexFile::MethodId& GetMethodId() const {
    return dex_file->GetMethodId(index);
  }
};

// Compare the actual referenced method signatures. Used for method reference deduplication.
struct MethodReferenceValueComparator {
  bool operator()(MethodReference mr1, MethodReference mr2) const {
    if (mr1.dex_file == mr2.dex_file) {
      DCHECK_EQ(mr1.index < mr2.index, SlowCompare(mr1, mr2));
      return mr1.index < mr2.index;
    } else {
      return SlowCompare(mr1, mr2);
    }
  }

  bool SlowCompare(MethodReference mr1, MethodReference mr2) const {
    // The order is the same as for method ids in a single dex file.
    // Compare the class descriptors first.
    const DexFile::MethodId& mid1 = mr1.GetMethodId();
    const DexFile::MethodId& mid2 = mr2.GetMethodId();
    int descriptor_diff = strcmp(mr1.dex_file->StringByTypeIdx(mid1.class_idx_),
                                 mr2.dex_file->StringByTypeIdx(mid2.class_idx_));
    if (descriptor_diff != 0) {
      return descriptor_diff < 0;
    }
    // Compare names second.
    int name_diff = strcmp(mr1.dex_file->GetMethodName(mid1), mr2.dex_file->GetMethodName(mid2));
    if (name_diff != 0) {
      return name_diff < 0;
    }
    // And then compare proto ids, starting with return type comparison.
    const DexFile::ProtoId& prid1 = mr1.dex_file->GetProtoId(mid1.proto_idx_);
    const DexFile::ProtoId& prid2 = mr2.dex_file->GetProtoId(mid2.proto_idx_);
    int return_type_diff = strcmp(mr1.dex_file->StringByTypeIdx(prid1.return_type_idx_),
                                  mr2.dex_file->StringByTypeIdx(prid2.return_type_idx_));
    if (return_type_diff != 0) {
      return return_type_diff < 0;
    }
    // And finishing with lexicographical parameter comparison.
    const DexFile::TypeList* params1 = mr1.dex_file->GetProtoParameters(prid1);
    size_t param1_size = (params1 != nullptr) ? params1->Size() : 0u;
    const DexFile::TypeList* params2 = mr2.dex_file->GetProtoParameters(prid2);
    size_t param2_size = (params2 != nullptr) ? params2->Size() : 0u;
    for (size_t i = 0, num = std::min(param1_size, param2_size); i != num; ++i) {
      int param_diff = strcmp(mr1.dex_file->StringByTypeIdx(params1->GetTypeItem(i).type_idx_),
                              mr2.dex_file->StringByTypeIdx(params2->GetTypeItem(i).type_idx_));
      if (param_diff != 0) {
        return param_diff < 0;
      }
    }
    return param1_size < param2_size;
  }
};

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_METHOD_REFERENCE_H_
