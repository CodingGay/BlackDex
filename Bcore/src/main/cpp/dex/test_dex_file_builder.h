/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef ART_LIBDEXFILE_DEX_TEST_DEX_FILE_BUILDER_H_
#define ART_LIBDEXFILE_DEX_TEST_DEX_FILE_BUILDER_H_

#include <zlib.h>

#include <cstring>
#include <map>
#include <set>
#include <vector>

#include <android-base/logging.h>

#include "dex/dex_file_loader.h"
#include "dex/standard_dex_file.h"

namespace art_lkchan {

class TestDexFileBuilder {
 public:
  TestDexFileBuilder()
      : strings_(), types_(), fields_(), protos_(), dex_file_data_() {
  }

  void AddString(const std::string& str) {
    CHECK(dex_file_data_.empty());
    auto it = strings_.emplace(str, IdxAndDataOffset()).first;
    CHECK_LT(it->first.length(), 128u);  // Don't allow multi-byte length in uleb128.
  }

  void AddType(const std::string& descriptor) {
    CHECK(dex_file_data_.empty());
    AddString(descriptor);
    types_.emplace(descriptor, 0u);
  }

  void AddField(const std::string& class_descriptor, const std::string& type,
                const std::string& name) {
    CHECK(dex_file_data_.empty());
    AddType(class_descriptor);
    AddType(type);
    AddString(name);
    FieldKey key = { class_descriptor, type, name };
    fields_.emplace(key, 0u);
  }

  void AddMethod(const std::string& class_descriptor, const std::string& signature,
                 const std::string& name) {
    CHECK(dex_file_data_.empty());
    AddType(class_descriptor);
    AddString(name);

    ProtoKey proto_key = CreateProtoKey(signature);
    AddString(proto_key.shorty);
    AddType(proto_key.return_type);
    for (const auto& arg_type : proto_key.args) {
      AddType(arg_type);
    }
    auto it = protos_.emplace(proto_key, IdxAndDataOffset()).first;
    const ProtoKey* proto = &it->first;  // Valid as long as the element remains in protos_.

    MethodKey method_key = {
        class_descriptor, name, proto
    };
    methods_.emplace(method_key, 0u);
  }

  // NOTE: The builder holds the actual data, so it must live as long as the dex file.
  std::unique_ptr<const DexFile> Build(const std::string& dex_location) {
    CHECK(dex_file_data_.empty());
    union {
      uint8_t data[sizeof(DexFile::Header)];
      uint64_t force_alignment;
    } header_data;
    std::memset(header_data.data, 0, sizeof(header_data.data));
    DexFile::Header* header = reinterpret_cast<DexFile::Header*>(&header_data.data);
    std::copy_n(StandardDexFile::kDexMagic, 4u, header->magic_);
    std::copy_n(StandardDexFile::kDexMagicVersions[0], 4u, header->magic_ + 4u);
    header->header_size_ = sizeof(DexFile::Header);
    header->endian_tag_ = DexFile::kDexEndianConstant;
    header->link_size_ = 0u;  // Unused.
    header->link_off_ = 0u;  // Unused.
    header->map_off_ = 0u;  // Unused. TODO: This is wrong. Dex files created by this builder
                            //               cannot be verified. b/26808512

    uint32_t data_section_size = 0u;

    uint32_t string_ids_offset = sizeof(DexFile::Header);
    uint32_t string_idx = 0u;
    for (auto& entry : strings_) {
      entry.second.idx = string_idx;
      string_idx += 1u;
      entry.second.data_offset = data_section_size;
      data_section_size += entry.first.length() + 1u /* length */ + 1u /* null-terminator */;
    }
    header->string_ids_size_ = strings_.size();
    header->string_ids_off_ = strings_.empty() ? 0u : string_ids_offset;

    uint32_t type_ids_offset = string_ids_offset + strings_.size() * sizeof(DexFile::StringId);
    uint32_t type_idx = 0u;
    for (auto& entry : types_) {
      entry.second = type_idx;
      type_idx += 1u;
    }
    header->type_ids_size_ = types_.size();
    header->type_ids_off_ = types_.empty() ? 0u : type_ids_offset;

    uint32_t proto_ids_offset = type_ids_offset + types_.size() * sizeof(DexFile::TypeId);
    uint32_t proto_idx = 0u;
    for (auto& entry : protos_) {
      entry.second.idx = proto_idx;
      proto_idx += 1u;
      size_t num_args = entry.first.args.size();
      if (num_args != 0u) {
        entry.second.data_offset = RoundUp(data_section_size, 4u);
        data_section_size = entry.second.data_offset + 4u + num_args * sizeof(DexFile::TypeItem);
      } else {
        entry.second.data_offset = 0u;
      }
    }
    header->proto_ids_size_ = protos_.size();
    header->proto_ids_off_ = protos_.empty() ? 0u : proto_ids_offset;

    uint32_t field_ids_offset = proto_ids_offset + protos_.size() * sizeof(DexFile::ProtoId);
    uint32_t field_idx = 0u;
    for (auto& entry : fields_) {
      entry.second = field_idx;
      field_idx += 1u;
    }
    header->field_ids_size_ = fields_.size();
    header->field_ids_off_ = fields_.empty() ? 0u : field_ids_offset;

    uint32_t method_ids_offset = field_ids_offset + fields_.size() * sizeof(DexFile::FieldId);
    uint32_t method_idx = 0u;
    for (auto& entry : methods_) {
      entry.second = method_idx;
      method_idx += 1u;
    }
    header->method_ids_size_ = methods_.size();
    header->method_ids_off_ = methods_.empty() ? 0u : method_ids_offset;

    // No class defs.
    header->class_defs_size_ = 0u;
    header->class_defs_off_ = 0u;

    uint32_t data_section_offset = method_ids_offset + methods_.size() * sizeof(DexFile::MethodId);
    header->data_size_ = data_section_size;
    header->data_off_ = (data_section_size != 0u) ? data_section_offset : 0u;

    uint32_t total_size = data_section_offset + data_section_size;

    dex_file_data_.resize(total_size);

    for (const auto& entry : strings_) {
      CHECK_LT(entry.first.size(), 128u);
      uint32_t raw_offset = data_section_offset + entry.second.data_offset;
      dex_file_data_[raw_offset] = static_cast<uint8_t>(entry.first.size());
      std::memcpy(&dex_file_data_[raw_offset + 1], entry.first.c_str(), entry.first.size() + 1);
      Write32(string_ids_offset + entry.second.idx * sizeof(DexFile::StringId), raw_offset);
    }

    for (const auto& entry : types_) {
      Write32(type_ids_offset + entry.second * sizeof(DexFile::TypeId), GetStringIdx(entry.first));
      ++type_idx;
    }

    for (const auto& entry : protos_) {
      size_t num_args = entry.first.args.size();
      uint32_t type_list_offset =
          (num_args != 0u) ? data_section_offset + entry.second.data_offset : 0u;
      uint32_t raw_offset = proto_ids_offset + entry.second.idx * sizeof(DexFile::ProtoId);
      Write32(raw_offset + 0u, GetStringIdx(entry.first.shorty));
      Write16(raw_offset + 4u, GetTypeIdx(entry.first.return_type));
      Write32(raw_offset + 8u, type_list_offset);
      if (num_args != 0u) {
        CHECK_NE(entry.second.data_offset, 0u);
        Write32(type_list_offset, num_args);
        for (size_t i = 0; i != num_args; ++i) {
          Write16(type_list_offset + 4u + i * sizeof(DexFile::TypeItem),
                  GetTypeIdx(entry.first.args[i]));
        }
      }
    }

    for (const auto& entry : fields_) {
      uint32_t raw_offset = field_ids_offset + entry.second * sizeof(DexFile::FieldId);
      Write16(raw_offset + 0u, GetTypeIdx(entry.first.class_descriptor));
      Write16(raw_offset + 2u, GetTypeIdx(entry.first.type));
      Write32(raw_offset + 4u, GetStringIdx(entry.first.name));
    }

    for (const auto& entry : methods_) {
      uint32_t raw_offset = method_ids_offset + entry.second * sizeof(DexFile::MethodId);
      Write16(raw_offset + 0u, GetTypeIdx(entry.first.class_descriptor));
      auto it = protos_.find(*entry.first.proto);
      CHECK(it != protos_.end());
      Write16(raw_offset + 2u, it->second.idx);
      Write32(raw_offset + 4u, GetStringIdx(entry.first.name));
    }

    // Leave signature as zeros.

    header->file_size_ = dex_file_data_.size();

    // Write the complete header early, as part of it needs to be checksummed.
    std::memcpy(&dex_file_data_[0], header_data.data, sizeof(DexFile::Header));

    // Checksum starts after the checksum field.
    size_t skip = sizeof(header->magic_) + sizeof(header->checksum_);
    header->checksum_ = adler32(adler32(0L, Z_NULL, 0),
                                dex_file_data_.data() + skip,
                                dex_file_data_.size() - skip);

    // Write the complete header again, just simpler that way.
    std::memcpy(&dex_file_data_[0], header_data.data, sizeof(DexFile::Header));

    static constexpr bool kVerify = false;
    static constexpr bool kVerifyChecksum = false;
    std::string error_msg;
    const DexFileLoader dex_file_loader;
    std::unique_ptr<const DexFile> dex_file(dex_file_loader.Open(
        &dex_file_data_[0],
        dex_file_data_.size(),
        dex_location,
        0u,
        nullptr,
        kVerify,
        kVerifyChecksum,
        &error_msg));
    CHECK(dex_file != nullptr) << error_msg;
    return dex_file;
  }

  uint32_t GetStringIdx(const std::string& type) {
    auto it = strings_.find(type);
    CHECK(it != strings_.end());
    return it->second.idx;
  }

  uint32_t GetTypeIdx(const std::string& type) {
    auto it = types_.find(type);
    CHECK(it != types_.end());
    return it->second;
  }

  uint32_t GetFieldIdx(const std::string& class_descriptor, const std::string& type,
                       const std::string& name) {
    FieldKey key = { class_descriptor, type, name };
    auto it = fields_.find(key);
    CHECK(it != fields_.end());
    return it->second;
  }

  uint32_t GetMethodIdx(const std::string& class_descriptor, const std::string& signature,
                        const std::string& name) {
    ProtoKey proto_key = CreateProtoKey(signature);
    MethodKey method_key = { class_descriptor, name, &proto_key };
    auto it = methods_.find(method_key);
    CHECK(it != methods_.end());
    return it->second;
  }

 private:
  struct IdxAndDataOffset {
    uint32_t idx;
    uint32_t data_offset;
  };

  struct FieldKey {
    const std::string class_descriptor;
    const std::string type;
    const std::string name;
  };
  struct FieldKeyComparator {
    bool operator()(const FieldKey& lhs, const FieldKey& rhs) const {
      if (lhs.class_descriptor != rhs.class_descriptor) {
        return lhs.class_descriptor < rhs.class_descriptor;
      }
      if (lhs.name != rhs.name) {
        return lhs.name < rhs.name;
      }
      return lhs.type < rhs.type;
    }
  };

  struct ProtoKey {
    std::string shorty;
    std::string return_type;
    std::vector<std::string> args;
  };
  struct ProtoKeyComparator {
    bool operator()(const ProtoKey& lhs, const ProtoKey& rhs) const {
      if (lhs.return_type != rhs.return_type) {
        return lhs.return_type < rhs.return_type;
      }
      size_t min_args = std::min(lhs.args.size(), rhs.args.size());
      for (size_t i = 0; i != min_args; ++i) {
        if (lhs.args[i] != rhs.args[i]) {
          return lhs.args[i] < rhs.args[i];
        }
      }
      return lhs.args.size() < rhs.args.size();
    }
  };

  struct MethodKey {
    std::string class_descriptor;
    std::string name;
    const ProtoKey* proto;
  };
  struct MethodKeyComparator {
    bool operator()(const MethodKey& lhs, const MethodKey& rhs) const {
      if (lhs.class_descriptor != rhs.class_descriptor) {
        return lhs.class_descriptor < rhs.class_descriptor;
      }
      if (lhs.name != rhs.name) {
        return lhs.name < rhs.name;
      }
      return ProtoKeyComparator()(*lhs.proto, *rhs.proto);
    }
  };

  ProtoKey CreateProtoKey(const std::string& signature) {
    CHECK_EQ(signature[0], '(');
    const char* args = signature.c_str() + 1;
    const char* args_end = std::strchr(args, ')');
    CHECK(args_end != nullptr);
    const char* return_type = args_end + 1;

    ProtoKey key = {
        std::string() + ((*return_type == '[') ? 'L' : *return_type),
        return_type,
        std::vector<std::string>()
    };
    while (args != args_end) {
      key.shorty += (*args == '[') ? 'L' : *args;
      const char* arg_start = args;
      while (*args == '[') {
        ++args;
      }
      if (*args == 'L') {
        do {
          ++args;
          CHECK_NE(args, args_end);
        } while (*args != ';');
      }
      ++args;
      key.args.emplace_back(arg_start, args);
    }
    return key;
  }

  void Write32(size_t offset, uint32_t value) {
    CHECK_LE(offset + 4u, dex_file_data_.size());
    CHECK_EQ(dex_file_data_[offset + 0], 0u);
    CHECK_EQ(dex_file_data_[offset + 1], 0u);
    CHECK_EQ(dex_file_data_[offset + 2], 0u);
    CHECK_EQ(dex_file_data_[offset + 3], 0u);
    dex_file_data_[offset + 0] = static_cast<uint8_t>(value >> 0);
    dex_file_data_[offset + 1] = static_cast<uint8_t>(value >> 8);
    dex_file_data_[offset + 2] = static_cast<uint8_t>(value >> 16);
    dex_file_data_[offset + 3] = static_cast<uint8_t>(value >> 24);
  }

  void Write16(size_t offset, uint32_t value) {
    CHECK_LE(value, 0xffffu);
    CHECK_LE(offset + 2u, dex_file_data_.size());
    CHECK_EQ(dex_file_data_[offset + 0], 0u);
    CHECK_EQ(dex_file_data_[offset + 1], 0u);
    dex_file_data_[offset + 0] = static_cast<uint8_t>(value >> 0);
    dex_file_data_[offset + 1] = static_cast<uint8_t>(value >> 8);
  }

  std::map<std::string, IdxAndDataOffset> strings_;
  std::map<std::string, uint32_t> types_;
  std::map<FieldKey, uint32_t, FieldKeyComparator> fields_;
  std::map<ProtoKey, IdxAndDataOffset, ProtoKeyComparator> protos_;
  std::map<MethodKey, uint32_t, MethodKeyComparator> methods_;

  std::vector<uint8_t> dex_file_data_;
};

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_TEST_DEX_FILE_BUILDER_H_
