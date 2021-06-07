/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "dex_file_verifier.h"

#include <inttypes.h>

#include <limits>
#include <memory>

#include "android-base/stringprintf.h"

#include "base/leb128.h"
#include "code_item_accessors-inl.h"
#include "descriptors_names.h"
#include "dex_file-inl.h"
#include "modifiers.h"
#include "utf-inl.h"

namespace art_lkchan {

using android_lkchan::base::StringAppendV;
using android_lkchan::base::StringPrintf;

static constexpr uint32_t kTypeIdLimit = std::numeric_limits<uint16_t>::max();

static bool IsValidOrNoTypeId(uint16_t low, uint16_t high) {
  return (high == 0) || ((high == 0xffffU) && (low == 0xffffU));
}

static bool IsValidTypeId(uint16_t low ATTRIBUTE_UNUSED, uint16_t high) {
  return (high == 0);
}

static uint32_t MapTypeToBitMask(DexFile::MapItemType map_item_type) {
  switch (map_item_type) {
    case DexFile::kDexTypeHeaderItem:               return 1 << 0;
    case DexFile::kDexTypeStringIdItem:             return 1 << 1;
    case DexFile::kDexTypeTypeIdItem:               return 1 << 2;
    case DexFile::kDexTypeProtoIdItem:              return 1 << 3;
    case DexFile::kDexTypeFieldIdItem:              return 1 << 4;
    case DexFile::kDexTypeMethodIdItem:             return 1 << 5;
    case DexFile::kDexTypeClassDefItem:             return 1 << 6;
    case DexFile::kDexTypeCallSiteIdItem:           return 1 << 7;
    case DexFile::kDexTypeMethodHandleItem:         return 1 << 8;
    case DexFile::kDexTypeMapList:                  return 1 << 9;
    case DexFile::kDexTypeTypeList:                 return 1 << 10;
    case DexFile::kDexTypeAnnotationSetRefList:     return 1 << 11;
    case DexFile::kDexTypeAnnotationSetItem:        return 1 << 12;
    case DexFile::kDexTypeClassDataItem:            return 1 << 13;
    case DexFile::kDexTypeCodeItem:                 return 1 << 14;
    case DexFile::kDexTypeStringDataItem:           return 1 << 15;
    case DexFile::kDexTypeDebugInfoItem:            return 1 << 16;
    case DexFile::kDexTypeAnnotationItem:           return 1 << 17;
    case DexFile::kDexTypeEncodedArrayItem:         return 1 << 18;
    case DexFile::kDexTypeAnnotationsDirectoryItem: return 1 << 19;
  }
  return 0;
}

static bool IsDataSectionType(DexFile::MapItemType map_item_type) {
  switch (map_item_type) {
    case DexFile::kDexTypeHeaderItem:
    case DexFile::kDexTypeStringIdItem:
    case DexFile::kDexTypeTypeIdItem:
    case DexFile::kDexTypeProtoIdItem:
    case DexFile::kDexTypeFieldIdItem:
    case DexFile::kDexTypeMethodIdItem:
    case DexFile::kDexTypeClassDefItem:
      return false;
    case DexFile::kDexTypeCallSiteIdItem:
    case DexFile::kDexTypeMethodHandleItem:
    case DexFile::kDexTypeMapList:
    case DexFile::kDexTypeTypeList:
    case DexFile::kDexTypeAnnotationSetRefList:
    case DexFile::kDexTypeAnnotationSetItem:
    case DexFile::kDexTypeClassDataItem:
    case DexFile::kDexTypeCodeItem:
    case DexFile::kDexTypeStringDataItem:
    case DexFile::kDexTypeDebugInfoItem:
    case DexFile::kDexTypeAnnotationItem:
    case DexFile::kDexTypeEncodedArrayItem:
    case DexFile::kDexTypeAnnotationsDirectoryItem:
      return true;
  }
  return true;
}

const char* DexFileVerifier::CheckLoadStringByIdx(dex::StringIndex idx, const char* error_string) {
  if (UNLIKELY(!CheckIndex(idx.index_, dex_file_->NumStringIds(), error_string))) {
    return nullptr;
  }
  return dex_file_->StringDataByIdx(idx);
}

// Try to find the name of the method with the given index. We do not want to rely on DexFile
// infrastructure at this point, so do it all by hand. begin and header correspond to begin_ and
// header_ of the DexFileVerifier. str will contain the pointer to the method name on success
// (flagged by the return value), otherwise error_msg will contain an error string.
static bool FindMethodName(uint32_t method_index,
                           const uint8_t* begin,
                           const DexFile::Header* header,
                           const char** str,
                           std::string* error_msg) {
  if (method_index >= header->method_ids_size_) {
    *error_msg = "Method index not available for method flags verification";
    return false;
  }
  uint32_t string_idx =
      (reinterpret_cast<const DexFile::MethodId*>(begin + header->method_ids_off_) +
          method_index)->name_idx_.index_;
  if (string_idx >= header->string_ids_size_) {
    *error_msg = "String index not available for method flags verification";
    return false;
  }
  uint32_t string_off =
      (reinterpret_cast<const DexFile::StringId*>(begin + header->string_ids_off_) + string_idx)->
          string_data_off_;
  if (string_off >= header->file_size_) {
    *error_msg = "String offset out of bounds for method flags verification";
    return false;
  }
  const uint8_t* str_data_ptr = begin + string_off;
  uint32_t dummy;
  if (!DecodeUnsignedLeb128Checked(&str_data_ptr, begin + header->file_size_, &dummy)) {
    *error_msg = "String size out of bounds for method flags verification";
    return false;
  }
  *str = reinterpret_cast<const char*>(str_data_ptr);
  return true;
}

// Gets constructor flags based on the |method_name|. Returns true if
// method_name is either <clinit> or <init> and sets
// |constructor_flags_by_name| appropriately. Otherwise set
// |constructor_flags_by_name| to zero and returns whether
// |method_name| is valid.
bool GetConstructorFlagsForMethodName(const char* method_name,
                                      uint32_t* constructor_flags_by_name) {
  if (method_name[0] != '<') {
    *constructor_flags_by_name = 0;
    return true;
  }
  if (strcmp(method_name + 1, "clinit>") == 0) {
    *constructor_flags_by_name = kAccStatic | kAccConstructor;
    return true;
  }
  if (strcmp(method_name + 1, "init>") == 0) {
    *constructor_flags_by_name = kAccConstructor;
    return true;
  }
  *constructor_flags_by_name = 0;
  return false;
}

const char* DexFileVerifier::CheckLoadStringByTypeIdx(dex::TypeIndex type_idx,
                                                      const char* error_string) {
  if (UNLIKELY(!CheckIndex(type_idx.index_, dex_file_->NumTypeIds(), error_string))) {
    return nullptr;
  }
  return CheckLoadStringByIdx(dex_file_->GetTypeId(type_idx).descriptor_idx_, error_string);
}

const DexFile::FieldId* DexFileVerifier::CheckLoadFieldId(uint32_t idx, const char* error_string) {
  if (UNLIKELY(!CheckIndex(idx, dex_file_->NumFieldIds(), error_string))) {
    return nullptr;
  }
  return &dex_file_->GetFieldId(idx);
}

const DexFile::MethodId* DexFileVerifier::CheckLoadMethodId(uint32_t idx, const char* err_string) {
  if (UNLIKELY(!CheckIndex(idx, dex_file_->NumMethodIds(), err_string))) {
    return nullptr;
  }
  return &dex_file_->GetMethodId(idx);
}

const DexFile::ProtoId* DexFileVerifier::CheckLoadProtoId(uint32_t idx, const char* err_string) {
  if (UNLIKELY(!CheckIndex(idx, dex_file_->NumProtoIds(), err_string))) {
    return nullptr;
  }
  return &dex_file_->GetProtoId(idx);
}

// Helper macro to load string and return false on error.
#define LOAD_STRING(var, idx, error)                    \
  const char* (var) = CheckLoadStringByIdx(idx, error); \
  if (UNLIKELY((var) == nullptr)) {                     \
    return false;                                       \
  }

// Helper macro to load string by type idx and return false on error.
#define LOAD_STRING_BY_TYPE(var, type_idx, error)                \
  const char* (var) = CheckLoadStringByTypeIdx(type_idx, error); \
  if (UNLIKELY((var) == nullptr)) {                              \
    return false;                                                \
  }

// Helper macro to load method id. Return last parameter on error.
#define LOAD_METHOD(var, idx, error_string, error_stmt)                   \
  const DexFile::MethodId* (var)  = CheckLoadMethodId(idx, error_string); \
  if (UNLIKELY((var) == nullptr)) {                                       \
    error_stmt;                                                           \
  }

// Helper macro to load method id. Return last parameter on error.
#define LOAD_FIELD(var, idx, fmt, error_stmt)                 \
  const DexFile::FieldId* (var) = CheckLoadFieldId(idx, fmt); \
  if (UNLIKELY((var) == nullptr)) {                           \
    error_stmt;                                               \
  }

bool DexFileVerifier::Verify(const DexFile* dex_file,
                             const uint8_t* begin,
                             size_t size,
                             const char* location,
                             bool verify_checksum,
                             std::string* error_msg) {
  std::unique_ptr<DexFileVerifier> verifier(
      new DexFileVerifier(dex_file, begin, size, location, verify_checksum));
  if (!verifier->Verify()) {
    *error_msg = verifier->FailureReason();
    return false;
  }
  return true;
}

bool DexFileVerifier::CheckShortyDescriptorMatch(char shorty_char, const char* descriptor,
                                                bool is_return_type) {
  switch (shorty_char) {
    case 'V':
      if (UNLIKELY(!is_return_type)) {
        ErrorStringPrintf("Invalid use of void");
        return false;
      }
      FALLTHROUGH_INTENDED;
    case 'B':
    case 'C':
    case 'D':
    case 'F':
    case 'I':
    case 'J':
    case 'S':
    case 'Z':
      if (UNLIKELY((descriptor[0] != shorty_char) || (descriptor[1] != '\0'))) {
        ErrorStringPrintf("Shorty vs. primitive type mismatch: '%c', '%s'",
                          shorty_char, descriptor);
        return false;
      }
      break;
    case 'L':
      if (UNLIKELY((descriptor[0] != 'L') && (descriptor[0] != '['))) {
        ErrorStringPrintf("Shorty vs. type mismatch: '%c', '%s'", shorty_char, descriptor);
        return false;
      }
      break;
    default:
      ErrorStringPrintf("Bad shorty character: '%c'", shorty_char);
      return false;
  }
  return true;
}

bool DexFileVerifier::CheckListSize(const void* start, size_t count, size_t elem_size,
                                    const char* label) {
  // Check that size is not 0.
  CHECK_NE(elem_size, 0U);

  const uint8_t* range_start = reinterpret_cast<const uint8_t*>(start);
  const uint8_t* file_start = reinterpret_cast<const uint8_t*>(begin_);

  // Check for overflow.
  uintptr_t max = 0 - 1;
  size_t available_bytes_till_end_of_mem = max - reinterpret_cast<uintptr_t>(start);
  size_t max_count = available_bytes_till_end_of_mem / elem_size;
  if (max_count < count) {
    ErrorStringPrintf("Overflow in range for %s: %zx for %zu@%zu", label,
                      static_cast<size_t>(range_start - file_start),
                      count, elem_size);
    return false;
  }

  const uint8_t* range_end = range_start + count * elem_size;
  const uint8_t* file_end = file_start + size_;
  if (UNLIKELY((range_start < file_start) || (range_end > file_end))) {
    // Note: these two tests are enough as we make sure above that there's no overflow.
    ErrorStringPrintf("Bad range for %s: %zx to %zx", label,
                      static_cast<size_t>(range_start - file_start),
                      static_cast<size_t>(range_end - file_start));
    return false;
  }
  return true;
}

bool DexFileVerifier::CheckList(size_t element_size, const char* label, const uint8_t* *ptr) {
  // Check that the list is available. The first 4B are the count.
  if (!CheckListSize(*ptr, 1, 4U, label)) {
    return false;
  }

  uint32_t count = *reinterpret_cast<const uint32_t*>(*ptr);
  if (count > 0) {
    if (!CheckListSize(*ptr + 4, count, element_size, label)) {
      return false;
    }
  }

  *ptr += 4 + count * element_size;
  return true;
}

bool DexFileVerifier::CheckIndex(uint32_t field, uint32_t limit, const char* label) {
  if (UNLIKELY(field >= limit)) {
    ErrorStringPrintf("Bad index for %s: %x >= %x", label, field, limit);
    return false;
  }
  return true;
}

bool DexFileVerifier::CheckValidOffsetAndSize(uint32_t offset,
                                              uint32_t size,
                                              size_t alignment,
                                              const char* label) {
  if (size == 0) {
    if (offset != 0) {
      ErrorStringPrintf("Offset(%d) should be zero when size is zero for %s.", offset, label);
      return false;
    }
  }
  if (size_ <= offset) {
    ErrorStringPrintf("Offset(%d) should be within file size(%zu) for %s.", offset, size_, label);
    return false;
  }
  if (alignment != 0 && !IsAlignedParam(offset, alignment)) {
    ErrorStringPrintf("Offset(%d) should be aligned by %zu for %s.", offset, alignment, label);
    return false;
  }
  return true;
}

bool DexFileVerifier::CheckSizeLimit(uint32_t size, uint32_t limit, const char* label) {
  if (size > limit) {
    ErrorStringPrintf("Size(%u) should not exceed limit(%u) for %s.", size, limit, label);
    return false;
  }
  return true;
}

bool DexFileVerifier::CheckHeader() {
  // Check file size from the header.
  uint32_t expected_size = header_->file_size_;
  if (size_ != expected_size) {
    ErrorStringPrintf("Bad file size (%zd, expected %u)", size_, expected_size);
    return false;
  }

  uint32_t adler_checksum = dex_file_->CalculateChecksum();
  // Compute and verify the checksum in the header.
  if (adler_checksum != header_->checksum_) {
    if (verify_checksum_) {
      ErrorStringPrintf("Bad checksum (%08x, expected %08x)", adler_checksum, header_->checksum_);
      return false;
    } else {
      LOG(WARNING) << StringPrintf(
          "Ignoring bad checksum (%08x, expected %08x)", adler_checksum, header_->checksum_);
    }
  }

  // Check the contents of the header.
  if (header_->endian_tag_ != DexFile::kDexEndianConstant) {
    ErrorStringPrintf("Unexpected endian_tag: %x", header_->endian_tag_);
    return false;
  }

  const uint32_t expected_header_size = dex_file_->IsCompactDexFile()
      ? sizeof(CompactDexFile::Header)
      : sizeof(StandardDexFile::Header);

  if (header_->header_size_ != expected_header_size) {
    ErrorStringPrintf("Bad header size: %ud expected %ud",
                      header_->header_size_,
                      expected_header_size);
    return false;
  }

  // Check that all offsets are inside the file.
  bool result =
      CheckValidOffsetAndSize(header_->link_off_,
                              header_->link_size_,
                              0 /* unaligned */,
                              "link") &&
      CheckValidOffsetAndSize(header_->map_off_,
                              header_->map_off_,
                              4,
                              "map") &&
      CheckValidOffsetAndSize(header_->string_ids_off_,
                              header_->string_ids_size_,
                              4,
                              "string-ids") &&
      CheckValidOffsetAndSize(header_->type_ids_off_,
                              header_->type_ids_size_,
                              4,
                              "type-ids") &&
      CheckSizeLimit(header_->type_ids_size_, DexFile::kDexNoIndex16, "type-ids") &&
      CheckValidOffsetAndSize(header_->proto_ids_off_,
                              header_->proto_ids_size_,
                              4,
                              "proto-ids") &&
      CheckSizeLimit(header_->proto_ids_size_, DexFile::kDexNoIndex16, "proto-ids") &&
      CheckValidOffsetAndSize(header_->field_ids_off_,
                              header_->field_ids_size_,
                              4,
                              "field-ids") &&
      CheckValidOffsetAndSize(header_->method_ids_off_,
                              header_->method_ids_size_,
                              4,
                              "method-ids") &&
      CheckValidOffsetAndSize(header_->class_defs_off_,
                              header_->class_defs_size_,
                              4,
                              "class-defs") &&
      CheckValidOffsetAndSize(header_->data_off_,
                              header_->data_size_,
                              0,  // Unaligned, spec doesn't talk about it, even though size
                                  // is supposed to be a multiple of 4.
                              "data");
  return result;
}

bool DexFileVerifier::CheckMap() {
  const DexFile::MapList* map = reinterpret_cast<const DexFile::MapList*>(begin_ +
                                                                          header_->map_off_);
  // Check that map list content is available.
  if (!CheckListSize(map, 1, sizeof(DexFile::MapList), "maplist content")) {
    return false;
  }

  const DexFile::MapItem* item = map->list_;

  uint32_t count = map->size_;
  uint32_t last_offset = 0;
  uint32_t last_type = 0;
  uint32_t data_item_count = 0;
  uint32_t data_items_left = header_->data_size_;
  uint32_t used_bits = 0;

  // Sanity check the size of the map list.
  if (!CheckListSize(item, count, sizeof(DexFile::MapItem), "map size")) {
    return false;
  }

  // Check the items listed in the map.
  for (uint32_t i = 0; i < count; i++) {
    if (UNLIKELY(last_offset >= item->offset_ && i != 0)) {
      ErrorStringPrintf("Out of order map item: %x then %x for type %x last type was %x",
                        last_offset,
                        item->offset_,
                        static_cast<uint32_t>(item->type_),
                        last_type);
      return false;
    }
    if (UNLIKELY(item->offset_ >= header_->file_size_)) {
      ErrorStringPrintf("Map item after end of file: %x, size %x",
                        item->offset_, header_->file_size_);
      return false;
    }

    DexFile::MapItemType item_type = static_cast<DexFile::MapItemType>(item->type_);
    if (IsDataSectionType(item_type)) {
      uint32_t icount = item->size_;
      if (UNLIKELY(icount > data_items_left)) {
        ErrorStringPrintf("Too many items in data section: %ud item_type %zx",
                          data_item_count + icount,
                          static_cast<size_t>(item_type));
        return false;
      }
      data_items_left -= icount;
      data_item_count += icount;
    }

    uint32_t bit = MapTypeToBitMask(item_type);

    if (UNLIKELY(bit == 0)) {
      ErrorStringPrintf("Unknown map section type %x", item->type_);
      return false;
    }

    if (UNLIKELY((used_bits & bit) != 0)) {
      ErrorStringPrintf("Duplicate map section of type %x", item->type_);
      return false;
    }

    used_bits |= bit;
    last_offset = item->offset_;
    last_type = item->type_;
    item++;
  }

  // Check for missing sections in the map.
  if (UNLIKELY((used_bits & MapTypeToBitMask(DexFile::kDexTypeHeaderItem)) == 0)) {
    ErrorStringPrintf("Map is missing header entry");
    return false;
  }
  if (UNLIKELY((used_bits & MapTypeToBitMask(DexFile::kDexTypeMapList)) == 0)) {
    ErrorStringPrintf("Map is missing map_list entry");
    return false;
  }
  if (UNLIKELY((used_bits & MapTypeToBitMask(DexFile::kDexTypeStringIdItem)) == 0 &&
               ((header_->string_ids_off_ != 0) || (header_->string_ids_size_ != 0)))) {
    ErrorStringPrintf("Map is missing string_ids entry");
    return false;
  }
  if (UNLIKELY((used_bits & MapTypeToBitMask(DexFile::kDexTypeTypeIdItem)) == 0 &&
               ((header_->type_ids_off_ != 0) || (header_->type_ids_size_ != 0)))) {
    ErrorStringPrintf("Map is missing type_ids entry");
    return false;
  }
  if (UNLIKELY((used_bits & MapTypeToBitMask(DexFile::kDexTypeProtoIdItem)) == 0 &&
               ((header_->proto_ids_off_ != 0) || (header_->proto_ids_size_ != 0)))) {
    ErrorStringPrintf("Map is missing proto_ids entry");
    return false;
  }
  if (UNLIKELY((used_bits & MapTypeToBitMask(DexFile::kDexTypeFieldIdItem)) == 0 &&
               ((header_->field_ids_off_ != 0) || (header_->field_ids_size_ != 0)))) {
    ErrorStringPrintf("Map is missing field_ids entry");
    return false;
  }
  if (UNLIKELY((used_bits & MapTypeToBitMask(DexFile::kDexTypeMethodIdItem)) == 0 &&
               ((header_->method_ids_off_ != 0) || (header_->method_ids_size_ != 0)))) {
    ErrorStringPrintf("Map is missing method_ids entry");
    return false;
  }
  if (UNLIKELY((used_bits & MapTypeToBitMask(DexFile::kDexTypeClassDefItem)) == 0 &&
               ((header_->class_defs_off_ != 0) || (header_->class_defs_size_ != 0)))) {
    ErrorStringPrintf("Map is missing class_defs entry");
    return false;
  }
  return true;
}

uint32_t DexFileVerifier::ReadUnsignedLittleEndian(uint32_t size) {
  uint32_t result = 0;
  if (LIKELY(CheckListSize(ptr_, size, sizeof(uint8_t), "encoded_value"))) {
    for (uint32_t i = 0; i < size; i++) {
      result |= ((uint32_t) *(ptr_++)) << (i * 8);
    }
  }
  return result;
}


#define DECODE_UNSIGNED_CHECKED_FROM_WITH_ERROR_VALUE(ptr, var, error_value)  \
  uint32_t var;                                                               \
  if (!DecodeUnsignedLeb128Checked(&(ptr), begin_ + size_, &(var))) {         \
    return error_value;                                                       \
  }

#define DECODE_UNSIGNED_CHECKED_FROM(ptr, var)                        \
  uint32_t var;                                                       \
  if (!DecodeUnsignedLeb128Checked(&(ptr), begin_ + size_, &(var))) { \
    ErrorStringPrintf("Read out of bounds");                          \
    return false;                                                     \
  }

#define DECODE_SIGNED_CHECKED_FROM(ptr, var)                        \
  int32_t var;                                                      \
  if (!DecodeSignedLeb128Checked(&(ptr), begin_ + size_, &(var))) { \
    ErrorStringPrintf("Read out of bounds");                        \
    return false;                                                   \
  }

bool DexFileVerifier::CheckAndGetHandlerOffsets(const DexFile::CodeItem* code_item,
                                                uint32_t* handler_offsets, uint32_t handlers_size) {
  CodeItemDataAccessor accessor(*dex_file_, code_item);
  const uint8_t* handlers_base = accessor.GetCatchHandlerData();

  for (uint32_t i = 0; i < handlers_size; i++) {
    bool catch_all;
    size_t offset = ptr_ - handlers_base;
    DECODE_SIGNED_CHECKED_FROM(ptr_, size);

    if (UNLIKELY((size < -65536) || (size > 65536))) {
      ErrorStringPrintf("Invalid exception handler size: %d", size);
      return false;
    }

    if (size <= 0) {
      catch_all = true;
      size = -size;
    } else {
      catch_all = false;
    }

    handler_offsets[i] = static_cast<uint32_t>(offset);

    while (size-- > 0) {
      DECODE_UNSIGNED_CHECKED_FROM(ptr_, type_idx);
      if (!CheckIndex(type_idx, header_->type_ids_size_, "handler type_idx")) {
        return false;
      }

      DECODE_UNSIGNED_CHECKED_FROM(ptr_, addr);
      if (UNLIKELY(addr >= accessor.InsnsSizeInCodeUnits())) {
        ErrorStringPrintf("Invalid handler addr: %x", addr);
        return false;
      }
    }

    if (catch_all) {
      DECODE_UNSIGNED_CHECKED_FROM(ptr_, addr);
      if (UNLIKELY(addr >= accessor.InsnsSizeInCodeUnits())) {
        ErrorStringPrintf("Invalid handler catch_all_addr: %x", addr);
        return false;
      }
    }
  }

  return true;
}

bool DexFileVerifier::CheckClassDataItemField(uint32_t idx,
                                              uint32_t access_flags,
                                              uint32_t class_access_flags,
                                              dex::TypeIndex class_type_index,
                                              bool expect_static) {
  // Check for overflow.
  if (!CheckIndex(idx, header_->field_ids_size_, "class_data_item field_idx")) {
    return false;
  }

  // Check that it's the right class.
  dex::TypeIndex my_class_index =
      (reinterpret_cast<const DexFile::FieldId*>(begin_ + header_->field_ids_off_) + idx)->
          class_idx_;
  if (class_type_index != my_class_index) {
    ErrorStringPrintf("Field's class index unexpected, %" PRIu16 "vs %" PRIu16,
                      my_class_index.index_,
                      class_type_index.index_);
    return false;
  }

  // Check that it falls into the right class-data list.
  bool is_static = (access_flags & kAccStatic) != 0;
  if (UNLIKELY(is_static != expect_static)) {
    ErrorStringPrintf("Static/instance field not in expected list");
    return false;
  }

  // Check field access flags.
  std::string error_msg;
  if (!CheckFieldAccessFlags(idx, access_flags, class_access_flags, &error_msg)) {
    ErrorStringPrintf("%s", error_msg.c_str());
    return false;
  }

  return true;
}

bool DexFileVerifier::CheckClassDataItemMethod(uint32_t idx,
                                               uint32_t access_flags,
                                               uint32_t class_access_flags,
                                               dex::TypeIndex class_type_index,
                                               uint32_t code_offset,
                                               std::unordered_set<uint32_t>* direct_method_indexes,
                                               bool expect_direct) {
  DCHECK(direct_method_indexes != nullptr);
  // Check for overflow.
  if (!CheckIndex(idx, header_->method_ids_size_, "class_data_item method_idx")) {
    return false;
  }

  // Check that it's the right class.
  dex::TypeIndex my_class_index =
      (reinterpret_cast<const DexFile::MethodId*>(begin_ + header_->method_ids_off_) + idx)->
          class_idx_;
  if (class_type_index != my_class_index) {
    ErrorStringPrintf("Method's class index unexpected, %" PRIu16 " vs %" PRIu16,
                      my_class_index.index_,
                      class_type_index.index_);
    return false;
  }

  // Check that it's not defined as both direct and virtual.
  if (expect_direct) {
    direct_method_indexes->insert(idx);
  } else if (direct_method_indexes->find(idx) != direct_method_indexes->end()) {
    ErrorStringPrintf("Found virtual method with same index as direct method: %d", idx);
    return false;
  }

  std::string error_msg;
  const char* method_name;
  if (!FindMethodName(idx, begin_, header_, &method_name, &error_msg)) {
    ErrorStringPrintf("%s", error_msg.c_str());
    return false;
  }

  uint32_t constructor_flags_by_name = 0;
  if (!GetConstructorFlagsForMethodName(method_name, &constructor_flags_by_name)) {
    ErrorStringPrintf("Bad method name: %s", method_name);
    return false;
  }

  bool has_code = (code_offset != 0);
  if (!CheckMethodAccessFlags(idx,
                              access_flags,
                              class_access_flags,
                              constructor_flags_by_name,
                              has_code,
                              expect_direct,
                              &error_msg)) {
    ErrorStringPrintf("%s", error_msg.c_str());
    return false;
  }

  if (constructor_flags_by_name != 0) {
    if (!CheckConstructorProperties(idx, constructor_flags_by_name)) {
      DCHECK(FailureReasonIsSet());
      return false;
    }
  }

  return true;
}

bool DexFileVerifier::CheckPadding(size_t offset,
                                   uint32_t aligned_offset,
                                   DexFile::MapItemType type) {
  if (offset < aligned_offset) {
    if (!CheckListSize(begin_ + offset, aligned_offset - offset, sizeof(uint8_t), "section")) {
      return false;
    }
    while (offset < aligned_offset) {
      if (UNLIKELY(*ptr_ != '\0')) {
        ErrorStringPrintf("Non-zero padding %x before section of type %zu at offset 0x%zx",
                          *ptr_,
                          static_cast<size_t>(type),
                          offset);
        return false;
      }
      ptr_++;
      offset++;
    }
  }
  return true;
}

bool DexFileVerifier::CheckEncodedValue() {
  if (!CheckListSize(ptr_, 1, sizeof(uint8_t), "encoded_value header")) {
    return false;
  }

  uint8_t header_byte = *(ptr_++);
  uint32_t value_type = header_byte & DexFile::kDexAnnotationValueTypeMask;
  uint32_t value_arg = header_byte >> DexFile::kDexAnnotationValueArgShift;

  switch (value_type) {
    case DexFile::kDexAnnotationByte:
      if (UNLIKELY(value_arg != 0)) {
        ErrorStringPrintf("Bad encoded_value byte size %x", value_arg);
        return false;
      }
      ptr_++;
      break;
    case DexFile::kDexAnnotationShort:
    case DexFile::kDexAnnotationChar:
      if (UNLIKELY(value_arg > 1)) {
        ErrorStringPrintf("Bad encoded_value char/short size %x", value_arg);
        return false;
      }
      ptr_ += value_arg + 1;
      break;
    case DexFile::kDexAnnotationInt:
    case DexFile::kDexAnnotationFloat:
      if (UNLIKELY(value_arg > 3)) {
        ErrorStringPrintf("Bad encoded_value int/float size %x", value_arg);
        return false;
      }
      ptr_ += value_arg + 1;
      break;
    case DexFile::kDexAnnotationLong:
    case DexFile::kDexAnnotationDouble:
      ptr_ += value_arg + 1;
      break;
    case DexFile::kDexAnnotationString: {
      if (UNLIKELY(value_arg > 3)) {
        ErrorStringPrintf("Bad encoded_value string size %x", value_arg);
        return false;
      }
      uint32_t idx = ReadUnsignedLittleEndian(value_arg + 1);
      if (!CheckIndex(idx, header_->string_ids_size_, "encoded_value string")) {
        return false;
      }
      break;
    }
    case DexFile::kDexAnnotationType: {
      if (UNLIKELY(value_arg > 3)) {
        ErrorStringPrintf("Bad encoded_value type size %x", value_arg);
        return false;
      }
      uint32_t idx = ReadUnsignedLittleEndian(value_arg + 1);
      if (!CheckIndex(idx, header_->type_ids_size_, "encoded_value type")) {
        return false;
      }
      break;
    }
    case DexFile::kDexAnnotationField:
    case DexFile::kDexAnnotationEnum: {
      if (UNLIKELY(value_arg > 3)) {
        ErrorStringPrintf("Bad encoded_value field/enum size %x", value_arg);
        return false;
      }
      uint32_t idx = ReadUnsignedLittleEndian(value_arg + 1);
      if (!CheckIndex(idx, header_->field_ids_size_, "encoded_value field")) {
        return false;
      }
      break;
    }
    case DexFile::kDexAnnotationMethod: {
      if (UNLIKELY(value_arg > 3)) {
        ErrorStringPrintf("Bad encoded_value method size %x", value_arg);
        return false;
      }
      uint32_t idx = ReadUnsignedLittleEndian(value_arg + 1);
      if (!CheckIndex(idx, header_->method_ids_size_, "encoded_value method")) {
        return false;
      }
      break;
    }
    case DexFile::kDexAnnotationArray:
      if (UNLIKELY(value_arg != 0)) {
        ErrorStringPrintf("Bad encoded_value array value_arg %x", value_arg);
        return false;
      }
      if (!CheckEncodedArray()) {
        return false;
      }
      break;
    case DexFile::kDexAnnotationAnnotation:
      if (UNLIKELY(value_arg != 0)) {
        ErrorStringPrintf("Bad encoded_value annotation value_arg %x", value_arg);
        return false;
      }
      if (!CheckEncodedAnnotation()) {
        return false;
      }
      break;
    case DexFile::kDexAnnotationNull:
      if (UNLIKELY(value_arg != 0)) {
        ErrorStringPrintf("Bad encoded_value null value_arg %x", value_arg);
        return false;
      }
      break;
    case DexFile::kDexAnnotationBoolean:
      if (UNLIKELY(value_arg > 1)) {
        ErrorStringPrintf("Bad encoded_value boolean size %x", value_arg);
        return false;
      }
      break;
    case DexFile::kDexAnnotationMethodType: {
      if (UNLIKELY(value_arg > 3)) {
        ErrorStringPrintf("Bad encoded_value method type size %x", value_arg);
        return false;
      }
      uint32_t idx = ReadUnsignedLittleEndian(value_arg + 1);
      if (!CheckIndex(idx, header_->proto_ids_size_, "method_type value")) {
        return false;
      }
      break;
    }
    case DexFile::kDexAnnotationMethodHandle: {
      if (UNLIKELY(value_arg > 3)) {
        ErrorStringPrintf("Bad encoded_value method handle size %x", value_arg);
        return false;
      }
      uint32_t idx = ReadUnsignedLittleEndian(value_arg + 1);
      if (!CheckIndex(idx, dex_file_->NumMethodHandles(), "method_handle value")) {
        return false;
      }
      break;
    }
    default:
      ErrorStringPrintf("Bogus encoded_value value_type %x", value_type);
      return false;
  }

  return true;
}

bool DexFileVerifier::CheckEncodedArray() {
  DECODE_UNSIGNED_CHECKED_FROM(ptr_, size);

  while (size--) {
    if (!CheckEncodedValue()) {
      failure_reason_ = StringPrintf("Bad encoded_array value: %s", failure_reason_.c_str());
      return false;
    }
  }
  return true;
}

bool DexFileVerifier::CheckEncodedAnnotation() {
  DECODE_UNSIGNED_CHECKED_FROM(ptr_, anno_idx);
  if (!CheckIndex(anno_idx, header_->type_ids_size_, "encoded_annotation type_idx")) {
    return false;
  }

  DECODE_UNSIGNED_CHECKED_FROM(ptr_, size);
  uint32_t last_idx = 0;

  for (uint32_t i = 0; i < size; i++) {
    DECODE_UNSIGNED_CHECKED_FROM(ptr_, idx);
    if (!CheckIndex(idx, header_->string_ids_size_, "annotation_element name_idx")) {
      return false;
    }

    if (UNLIKELY(last_idx >= idx && i != 0)) {
      ErrorStringPrintf("Out-of-order annotation_element name_idx: %x then %x",
                        last_idx, idx);
      return false;
    }

    if (!CheckEncodedValue()) {
      return false;
    }

    last_idx = idx;
  }
  return true;
}

bool DexFileVerifier::FindClassIndexAndDef(uint32_t index,
                                           bool is_field,
                                           dex::TypeIndex* class_type_index,
                                           const DexFile::ClassDef** output_class_def) {
  DCHECK(class_type_index != nullptr);
  DCHECK(output_class_def != nullptr);

  // First check if the index is valid.
  if (index >= (is_field ? header_->field_ids_size_ : header_->method_ids_size_)) {
    return false;
  }

  // Next get the type index.
  if (is_field) {
    *class_type_index =
        (reinterpret_cast<const DexFile::FieldId*>(begin_ + header_->field_ids_off_) + index)->
            class_idx_;
  } else {
    *class_type_index =
        (reinterpret_cast<const DexFile::MethodId*>(begin_ + header_->method_ids_off_) + index)->
            class_idx_;
  }

  // Check if that is valid.
  if (class_type_index->index_ >= header_->type_ids_size_) {
    return false;
  }

  // Now search for the class def. This is basically a specialized version of the DexFile code, as
  // we should not trust that this is a valid DexFile just yet.
  const DexFile::ClassDef* class_def_begin =
      reinterpret_cast<const DexFile::ClassDef*>(begin_ + header_->class_defs_off_);
  for (size_t i = 0; i < header_->class_defs_size_; ++i) {
    const DexFile::ClassDef* class_def = class_def_begin + i;
    if (class_def->class_idx_ == *class_type_index) {
      *output_class_def = class_def;
      return true;
    }
  }

  // Didn't find the class-def, not defined here...
  return false;
}

bool DexFileVerifier::CheckOrderAndGetClassDef(bool is_field,
                                               const char* type_descr,
                                               uint32_t curr_index,
                                               uint32_t prev_index,
                                               bool* have_class,
                                               dex::TypeIndex* class_type_index,
                                               const DexFile::ClassDef** class_def) {
  if (curr_index < prev_index) {
    ErrorStringPrintf("out-of-order %s indexes %" PRIu32 " and %" PRIu32,
                      type_descr,
                      prev_index,
                      curr_index);
    return false;
  }

  if (!*have_class) {
    *have_class = FindClassIndexAndDef(curr_index, is_field, class_type_index, class_def);
    if (!*have_class) {
      // Should have really found one.
      ErrorStringPrintf("could not find declaring class for %s index %" PRIu32,
                        type_descr,
                        curr_index);
      return false;
    }
  }
  return true;
}

bool DexFileVerifier::CheckStaticFieldTypes(const DexFile::ClassDef* class_def) {
  if (class_def == nullptr) {
    return true;
  }

  ClassDataItemIterator field_it(*dex_file_, ptr_);
  EncodedStaticFieldValueIterator array_it(*dex_file_, *class_def);

  for (; field_it.HasNextStaticField() && array_it.HasNext(); field_it.Next(), array_it.Next()) {
    uint32_t index = field_it.GetMemberIndex();
    const DexFile::TypeId& type_id = dex_file_->GetTypeId(dex_file_->GetFieldId(index).type_idx_);
    const char* field_type_name =
        dex_file_->GetStringData(dex_file_->GetStringId(type_id.descriptor_idx_));
    Primitive::Type field_type = Primitive::GetType(field_type_name[0]);
    EncodedArrayValueIterator::ValueType array_type = array_it.GetValueType();
    // Ensure this matches RuntimeEncodedStaticFieldValueIterator.
    switch (array_type) {
      case EncodedArrayValueIterator::ValueType::kBoolean:
        if (field_type != Primitive::kPrimBoolean) {
          ErrorStringPrintf("unexpected static field initial value type: 'Z' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      case EncodedArrayValueIterator::ValueType::kByte:
        if (field_type != Primitive::kPrimByte) {
          ErrorStringPrintf("unexpected static field initial value type: 'B' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      case EncodedArrayValueIterator::ValueType::kShort:
        if (field_type != Primitive::kPrimShort) {
          ErrorStringPrintf("unexpected static field initial value type: 'S' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      case EncodedArrayValueIterator::ValueType::kChar:
        if (field_type != Primitive::kPrimChar) {
          ErrorStringPrintf("unexpected static field initial value type: 'C' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      case EncodedArrayValueIterator::ValueType::kInt:
        if (field_type != Primitive::kPrimInt) {
          ErrorStringPrintf("unexpected static field initial value type: 'I' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      case EncodedArrayValueIterator::ValueType::kLong:
        if (field_type != Primitive::kPrimLong) {
          ErrorStringPrintf("unexpected static field initial value type: 'J' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      case EncodedArrayValueIterator::ValueType::kFloat:
        if (field_type != Primitive::kPrimFloat) {
          ErrorStringPrintf("unexpected static field initial value type: 'F' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      case EncodedArrayValueIterator::ValueType::kDouble:
        if (field_type != Primitive::kPrimDouble) {
          ErrorStringPrintf("unexpected static field initial value type: 'D' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      case EncodedArrayValueIterator::ValueType::kNull:
      case EncodedArrayValueIterator::ValueType::kString:
      case EncodedArrayValueIterator::ValueType::kType:
        if (field_type != Primitive::kPrimNot) {
          ErrorStringPrintf("unexpected static field initial value type: 'L' vs '%c'",
                            field_type_name[0]);
          return false;
        }
        break;
      default:
        ErrorStringPrintf("unexpected static field initial value type: %x", array_type);
        return false;
    }
  }

  if (array_it.HasNext()) {
    ErrorStringPrintf("too many static field initial values");
    return false;
  }
  return true;
}

template <bool kStatic>
bool DexFileVerifier::CheckIntraClassDataItemFields(ClassDataItemIterator* it,
                                                    bool* have_class,
                                                    dex::TypeIndex* class_type_index,
                                                    const DexFile::ClassDef** class_def) {
  DCHECK(it != nullptr);
  // These calls use the raw access flags to check whether the whole dex field is valid.
  uint32_t prev_index = 0;
  for (; kStatic ? it->HasNextStaticField() : it->HasNextInstanceField(); it->Next()) {
    uint32_t curr_index = it->GetMemberIndex();
    if (!CheckOrderAndGetClassDef(true,
                                  kStatic ? "static field" : "instance field",
                                  curr_index,
                                  prev_index,
                                  have_class,
                                  class_type_index,
                                  class_def)) {
      return false;
    }
    DCHECK(class_def != nullptr);
    if (!CheckClassDataItemField(curr_index,
                                 it->GetRawMemberAccessFlags(),
                                 (*class_def)->access_flags_,
                                 *class_type_index,
                                 kStatic)) {
      return false;
    }

    prev_index = curr_index;
  }

  return true;
}

template <bool kDirect>
bool DexFileVerifier::CheckIntraClassDataItemMethods(
    ClassDataItemIterator* it,
    std::unordered_set<uint32_t>* direct_method_indexes,
    bool* have_class,
    dex::TypeIndex* class_type_index,
    const DexFile::ClassDef** class_def) {
  uint32_t prev_index = 0;
  for (; kDirect ? it->HasNextDirectMethod() : it->HasNextVirtualMethod(); it->Next()) {
    uint32_t curr_index = it->GetMemberIndex();
    if (!CheckOrderAndGetClassDef(false,
                                  kDirect ? "direct method" : "virtual method",
                                  curr_index,
                                  prev_index,
                                  have_class,
                                  class_type_index,
                                  class_def)) {
      return false;
    }
    DCHECK(class_def != nullptr);
    if (!CheckClassDataItemMethod(curr_index,
                                  it->GetRawMemberAccessFlags(),
                                  (*class_def)->access_flags_,
                                  *class_type_index,
                                  it->GetMethodCodeItemOffset(),
                                  direct_method_indexes,
                                  kDirect)) {
      return false;
    }

    prev_index = curr_index;
  }

  return true;
}

bool DexFileVerifier::CheckIntraClassDataItem() {
  ClassDataItemIterator it(*dex_file_, ptr_);
  std::unordered_set<uint32_t> direct_method_indexes;

  // This code is complicated by the fact that we don't directly know which class this belongs to.
  // So we need to explicitly search with the first item we find (either field or method), and then,
  // as the lookup is expensive, cache the result.
  bool have_class = false;
  dex::TypeIndex class_type_index;
  const DexFile::ClassDef* class_def = nullptr;

  // Check fields.
  if (!CheckIntraClassDataItemFields<true>(&it,
                                           &have_class,
                                           &class_type_index,
                                           &class_def)) {
    return false;
  }
  if (!CheckIntraClassDataItemFields<false>(&it,
                                            &have_class,
                                            &class_type_index,
                                            &class_def)) {
    return false;
  }

  // Check methods.
  if (!CheckIntraClassDataItemMethods<true>(&it,
                                            &direct_method_indexes,
                                            &have_class,
                                            &class_type_index,
                                            &class_def)) {
    return false;
  }
  if (!CheckIntraClassDataItemMethods<false>(&it,
                                             &direct_method_indexes,
                                             &have_class,
                                             &class_type_index,
                                             &class_def)) {
    return false;
  }

  const uint8_t* end_ptr = it.EndDataPointer();

  // Check static field types against initial static values in encoded array.
  if (!CheckStaticFieldTypes(class_def)) {
    return false;
  }

  ptr_ = end_ptr;
  return true;
}

bool DexFileVerifier::CheckIntraCodeItem() {
  const DexFile::CodeItem* code_item = reinterpret_cast<const DexFile::CodeItem*>(ptr_);
  if (!CheckListSize(code_item, 1, sizeof(DexFile::CodeItem), "code")) {
    return false;
  }

  CodeItemDataAccessor accessor(*dex_file_, code_item);
  if (UNLIKELY(accessor.InsSize() > accessor.RegistersSize())) {
    ErrorStringPrintf("ins_size (%ud) > registers_size (%ud)",
                      accessor.InsSize(), accessor.RegistersSize());
    return false;
  }

  if (UNLIKELY(accessor.OutsSize() > 5 && accessor.OutsSize() > accessor.RegistersSize())) {
    /*
     * outs_size can be up to 5, even if registers_size is smaller, since the
     * short forms of method invocation allow repetitions of a register multiple
     * times within a single parameter list. However, longer parameter lists
     * need to be represented in-order in the register file.
     */
    ErrorStringPrintf("outs_size (%ud) > registers_size (%ud)",
                      accessor.OutsSize(), accessor.RegistersSize());
    return false;
  }

  const uint16_t* insns = accessor.Insns();
  uint32_t insns_size = accessor.InsnsSizeInCodeUnits();
  if (!CheckListSize(insns, insns_size, sizeof(uint16_t), "insns size")) {
    return false;
  }

  // Grab the end of the insns if there are no try_items.
  uint32_t try_items_size = accessor.TriesSize();
  if (try_items_size == 0) {
    ptr_ = reinterpret_cast<const uint8_t*>(&insns[insns_size]);
    return true;
  }

  // try_items are 4-byte aligned. Verify the spacer is 0.
  if (((reinterpret_cast<uintptr_t>(&insns[insns_size]) & 3) != 0) && (insns[insns_size] != 0)) {
    ErrorStringPrintf("Non-zero padding: %x", insns[insns_size]);
    return false;
  }

  const DexFile::TryItem* try_items = accessor.TryItems().begin();
  if (!CheckListSize(try_items, try_items_size, sizeof(DexFile::TryItem), "try_items size")) {
    return false;
  }

  ptr_ = accessor.GetCatchHandlerData();
  DECODE_UNSIGNED_CHECKED_FROM(ptr_, handlers_size);

  if (UNLIKELY((handlers_size == 0) || (handlers_size >= 65536))) {
    ErrorStringPrintf("Invalid handlers_size: %ud", handlers_size);
    return false;
  }

  std::unique_ptr<uint32_t[]> handler_offsets(new uint32_t[handlers_size]);
  if (!CheckAndGetHandlerOffsets(code_item, &handler_offsets[0], handlers_size)) {
    return false;
  }

  uint32_t last_addr = 0;
  while (try_items_size--) {
    if (UNLIKELY(try_items->start_addr_ < last_addr)) {
      ErrorStringPrintf("Out-of_order try_item with start_addr: %x", try_items->start_addr_);
      return false;
    }

    if (UNLIKELY(try_items->start_addr_ >= insns_size)) {
      ErrorStringPrintf("Invalid try_item start_addr: %x", try_items->start_addr_);
      return false;
    }

    uint32_t i;
    for (i = 0; i < handlers_size; i++) {
      if (try_items->handler_off_ == handler_offsets[i]) {
        break;
      }
    }

    if (UNLIKELY(i == handlers_size)) {
      ErrorStringPrintf("Bogus handler offset: %x", try_items->handler_off_);
      return false;
    }

    last_addr = try_items->start_addr_ + try_items->insn_count_;
    if (UNLIKELY(last_addr > insns_size)) {
      ErrorStringPrintf("Invalid try_item insn_count: %x", try_items->insn_count_);
      return false;
    }

    try_items++;
  }

  return true;
}

bool DexFileVerifier::CheckIntraStringDataItem() {
  DECODE_UNSIGNED_CHECKED_FROM(ptr_, size);
  const uint8_t* file_end = begin_ + size_;

  for (uint32_t i = 0; i < size; i++) {
    CHECK_LT(i, size);  // b/15014252 Prevents hitting the impossible case below
    if (UNLIKELY(ptr_ >= file_end)) {
      ErrorStringPrintf("String data would go beyond end-of-file");
      return false;
    }

    uint8_t byte = *(ptr_++);

    // Switch on the high 4 bits.
    switch (byte >> 4) {
      case 0x00:
        // Special case of bit pattern 0xxx.
        if (UNLIKELY(byte == 0)) {
          CHECK_LT(i, size);  // b/15014252 Actually hit this impossible case with clang
          ErrorStringPrintf("String data shorter than indicated utf16_size %x", size);
          return false;
        }
        break;
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
        // No extra checks necessary for bit pattern 0xxx.
        break;
      case 0x08:
      case 0x09:
      case 0x0a:
      case 0x0b:
      case 0x0f:
        // Illegal bit patterns 10xx or 1111.
        // Note: 1111 is valid for normal UTF-8, but not here.
        ErrorStringPrintf("Illegal start byte %x in string data", byte);
        return false;
      case 0x0c:
      case 0x0d: {
        // Bit pattern 110x has an additional byte.
        uint8_t byte2 = *(ptr_++);
        if (UNLIKELY((byte2 & 0xc0) != 0x80)) {
          ErrorStringPrintf("Illegal continuation byte %x in string data", byte2);
          return false;
        }
        uint16_t value = ((byte & 0x1f) << 6) | (byte2 & 0x3f);
        if (UNLIKELY((value != 0) && (value < 0x80))) {
          ErrorStringPrintf("Illegal representation for value %x in string data", value);
          return false;
        }
        break;
      }
      case 0x0e: {
        // Bit pattern 1110 has 2 additional bytes.
        uint8_t byte2 = *(ptr_++);
        if (UNLIKELY((byte2 & 0xc0) != 0x80)) {
          ErrorStringPrintf("Illegal continuation byte %x in string data", byte2);
          return false;
        }
        uint8_t byte3 = *(ptr_++);
        if (UNLIKELY((byte3 & 0xc0) != 0x80)) {
          ErrorStringPrintf("Illegal continuation byte %x in string data", byte3);
          return false;
        }
        uint16_t value = ((byte & 0x0f) << 12) | ((byte2 & 0x3f) << 6) | (byte3 & 0x3f);
        if (UNLIKELY(value < 0x800)) {
          ErrorStringPrintf("Illegal representation for value %x in string data", value);
          return false;
        }
        break;
      }
    }
  }

  if (UNLIKELY(*(ptr_++) != '\0')) {
    ErrorStringPrintf("String longer than indicated size %x", size);
    return false;
  }

  return true;
}

bool DexFileVerifier::CheckIntraDebugInfoItem() {
  DECODE_UNSIGNED_CHECKED_FROM(ptr_, dummy);
  DECODE_UNSIGNED_CHECKED_FROM(ptr_, parameters_size);
  if (UNLIKELY(parameters_size > 65536)) {
    ErrorStringPrintf("Invalid parameters_size: %x", parameters_size);
    return false;
  }

  for (uint32_t j = 0; j < parameters_size; j++) {
    DECODE_UNSIGNED_CHECKED_FROM(ptr_, parameter_name);
    if (parameter_name != 0) {
      parameter_name--;
      if (!CheckIndex(parameter_name, header_->string_ids_size_, "debug_info_item parameter_name")) {
        return false;
      }
    }
  }

  while (true) {
    uint8_t opcode = *(ptr_++);
    switch (opcode) {
      case DexFile::DBG_END_SEQUENCE: {
        return true;
      }
      case DexFile::DBG_ADVANCE_PC: {
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, advance_pc_dummy);
        break;
      }
      case DexFile::DBG_ADVANCE_LINE: {
        DECODE_SIGNED_CHECKED_FROM(ptr_, advance_line_dummy);
        break;
      }
      case DexFile::DBG_START_LOCAL: {
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, reg_num);
        if (UNLIKELY(reg_num >= 65536)) {
          ErrorStringPrintf("Bad reg_num for opcode %x", opcode);
          return false;
        }
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, name_idx);
        if (name_idx != 0) {
          name_idx--;
          if (!CheckIndex(name_idx, header_->string_ids_size_, "DBG_START_LOCAL name_idx")) {
            return false;
          }
        }
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, type_idx);
        if (type_idx != 0) {
          type_idx--;
          if (!CheckIndex(type_idx, header_->type_ids_size_, "DBG_START_LOCAL type_idx")) {
            return false;
          }
        }
        break;
      }
      case DexFile::DBG_END_LOCAL:
      case DexFile::DBG_RESTART_LOCAL: {
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, reg_num);
        if (UNLIKELY(reg_num >= 65536)) {
          ErrorStringPrintf("Bad reg_num for opcode %x", opcode);
          return false;
        }
        break;
      }
      case DexFile::DBG_START_LOCAL_EXTENDED: {
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, reg_num);
        if (UNLIKELY(reg_num >= 65536)) {
          ErrorStringPrintf("Bad reg_num for opcode %x", opcode);
          return false;
        }
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, name_idx);
        if (name_idx != 0) {
          name_idx--;
          if (!CheckIndex(name_idx, header_->string_ids_size_, "DBG_START_LOCAL_EXTENDED name_idx")) {
            return false;
          }
        }
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, type_idx);
        if (type_idx != 0) {
          type_idx--;
          if (!CheckIndex(type_idx, header_->type_ids_size_, "DBG_START_LOCAL_EXTENDED type_idx")) {
            return false;
          }
        }
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, sig_idx);
        if (sig_idx != 0) {
          sig_idx--;
          if (!CheckIndex(sig_idx, header_->string_ids_size_, "DBG_START_LOCAL_EXTENDED sig_idx")) {
            return false;
          }
        }
        break;
      }
      case DexFile::DBG_SET_FILE: {
        DECODE_UNSIGNED_CHECKED_FROM(ptr_, name_idx);
        if (name_idx != 0) {
          name_idx--;
          if (!CheckIndex(name_idx, header_->string_ids_size_, "DBG_SET_FILE name_idx")) {
            return false;
          }
        }
        break;
      }
    }
  }
}

bool DexFileVerifier::CheckIntraAnnotationItem() {
  if (!CheckListSize(ptr_, 1, sizeof(uint8_t), "annotation visibility")) {
    return false;
  }

  // Check visibility
  switch (*(ptr_++)) {
    case DexFile::kDexVisibilityBuild:
    case DexFile::kDexVisibilityRuntime:
    case DexFile::kDexVisibilitySystem:
      break;
    default:
      ErrorStringPrintf("Bad annotation visibility: %x", *ptr_);
      return false;
  }

  if (!CheckEncodedAnnotation()) {
    return false;
  }

  return true;
}

bool DexFileVerifier::CheckIntraAnnotationsDirectoryItem() {
  const DexFile::AnnotationsDirectoryItem* item =
      reinterpret_cast<const DexFile::AnnotationsDirectoryItem*>(ptr_);
  if (!CheckListSize(item, 1, sizeof(DexFile::AnnotationsDirectoryItem), "annotations_directory")) {
    return false;
  }

  // Field annotations follow immediately after the annotations directory.
  const DexFile::FieldAnnotationsItem* field_item =
      reinterpret_cast<const DexFile::FieldAnnotationsItem*>(item + 1);
  uint32_t field_count = item->fields_size_;
  if (!CheckListSize(field_item, field_count, sizeof(DexFile::FieldAnnotationsItem), "field_annotations list")) {
    return false;
  }

  uint32_t last_idx = 0;
  for (uint32_t i = 0; i < field_count; i++) {
    if (UNLIKELY(last_idx >= field_item->field_idx_ && i != 0)) {
      ErrorStringPrintf("Out-of-order field_idx for annotation: %x then %x", last_idx, field_item->field_idx_);
      return false;
    }
    last_idx = field_item->field_idx_;
    field_item++;
  }

  // Method annotations follow immediately after field annotations.
  const DexFile::MethodAnnotationsItem* method_item =
      reinterpret_cast<const DexFile::MethodAnnotationsItem*>(field_item);
  uint32_t method_count = item->methods_size_;
  if (!CheckListSize(method_item, method_count, sizeof(DexFile::MethodAnnotationsItem), "method_annotations list")) {
    return false;
  }

  last_idx = 0;
  for (uint32_t i = 0; i < method_count; i++) {
    if (UNLIKELY(last_idx >= method_item->method_idx_ && i != 0)) {
      ErrorStringPrintf("Out-of-order method_idx for annotation: %x then %x",
                       last_idx, method_item->method_idx_);
      return false;
    }
    last_idx = method_item->method_idx_;
    method_item++;
  }

  // Parameter annotations follow immediately after method annotations.
  const DexFile::ParameterAnnotationsItem* parameter_item =
      reinterpret_cast<const DexFile::ParameterAnnotationsItem*>(method_item);
  uint32_t parameter_count = item->parameters_size_;
  if (!CheckListSize(parameter_item, parameter_count, sizeof(DexFile::ParameterAnnotationsItem),
                     "parameter_annotations list")) {
    return false;
  }

  last_idx = 0;
  for (uint32_t i = 0; i < parameter_count; i++) {
    if (UNLIKELY(last_idx >= parameter_item->method_idx_ && i != 0)) {
      ErrorStringPrintf("Out-of-order method_idx for annotation: %x then %x",
                        last_idx, parameter_item->method_idx_);
      return false;
    }
    last_idx = parameter_item->method_idx_;
    parameter_item++;
  }

  // Return a pointer to the end of the annotations.
  ptr_ = reinterpret_cast<const uint8_t*>(parameter_item);
  return true;
}

bool DexFileVerifier::CheckIntraSectionIterate(size_t offset, uint32_t section_count,
                                               DexFile::MapItemType type) {
  // Get the right alignment mask for the type of section.
  size_t alignment_mask;
  switch (type) {
    case DexFile::kDexTypeClassDataItem:
    case DexFile::kDexTypeStringDataItem:
    case DexFile::kDexTypeDebugInfoItem:
    case DexFile::kDexTypeAnnotationItem:
    case DexFile::kDexTypeEncodedArrayItem:
      alignment_mask = sizeof(uint8_t) - 1;
      break;
    default:
      alignment_mask = sizeof(uint32_t) - 1;
      break;
  }

  // Iterate through the items in the section.
  for (uint32_t i = 0; i < section_count; i++) {
    size_t aligned_offset = (offset + alignment_mask) & ~alignment_mask;

    // Check the padding between items.
    if (!CheckPadding(offset, aligned_offset, type)) {
      return false;
    }

    // Check depending on the section type.
    const uint8_t* start_ptr = ptr_;
    switch (type) {
      case DexFile::kDexTypeStringIdItem: {
        if (!CheckListSize(ptr_, 1, sizeof(DexFile::StringId), "string_ids")) {
          return false;
        }
        ptr_ += sizeof(DexFile::StringId);
        break;
      }
      case DexFile::kDexTypeTypeIdItem: {
        if (!CheckListSize(ptr_, 1, sizeof(DexFile::TypeId), "type_ids")) {
          return false;
        }
        ptr_ += sizeof(DexFile::TypeId);
        break;
      }
      case DexFile::kDexTypeProtoIdItem: {
        if (!CheckListSize(ptr_, 1, sizeof(DexFile::ProtoId), "proto_ids")) {
          return false;
        }
        ptr_ += sizeof(DexFile::ProtoId);
        break;
      }
      case DexFile::kDexTypeFieldIdItem: {
        if (!CheckListSize(ptr_, 1, sizeof(DexFile::FieldId), "field_ids")) {
          return false;
        }
        ptr_ += sizeof(DexFile::FieldId);
        break;
      }
      case DexFile::kDexTypeMethodIdItem: {
        if (!CheckListSize(ptr_, 1, sizeof(DexFile::MethodId), "method_ids")) {
          return false;
        }
        ptr_ += sizeof(DexFile::MethodId);
        break;
      }
      case DexFile::kDexTypeClassDefItem: {
        if (!CheckListSize(ptr_, 1, sizeof(DexFile::ClassDef), "class_defs")) {
          return false;
        }
        ptr_ += sizeof(DexFile::ClassDef);
        break;
      }
      case DexFile::kDexTypeCallSiteIdItem: {
        if (!CheckListSize(ptr_, 1, sizeof(DexFile::CallSiteIdItem), "call_site_ids")) {
          return false;
        }
        ptr_ += sizeof(DexFile::CallSiteIdItem);
        break;
      }
      case DexFile::kDexTypeMethodHandleItem: {
        if (!CheckListSize(ptr_, 1, sizeof(DexFile::MethodHandleItem), "method_handles")) {
          return false;
        }
        ptr_ += sizeof(DexFile::MethodHandleItem);
        break;
      }
      case DexFile::kDexTypeTypeList: {
        if (!CheckList(sizeof(DexFile::TypeItem), "type_list", &ptr_)) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeAnnotationSetRefList: {
        if (!CheckList(sizeof(DexFile::AnnotationSetRefItem), "annotation_set_ref_list", &ptr_)) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeAnnotationSetItem: {
        if (!CheckList(sizeof(uint32_t), "annotation_set_item", &ptr_)) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeClassDataItem: {
        if (!CheckIntraClassDataItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeCodeItem: {
        if (!CheckIntraCodeItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeStringDataItem: {
        if (!CheckIntraStringDataItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeDebugInfoItem: {
        if (!CheckIntraDebugInfoItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeAnnotationItem: {
        if (!CheckIntraAnnotationItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeEncodedArrayItem: {
        if (!CheckEncodedArray()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeAnnotationsDirectoryItem: {
        if (!CheckIntraAnnotationsDirectoryItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeHeaderItem:
      case DexFile::kDexTypeMapList:
        break;
    }

    if (start_ptr == ptr_) {
      ErrorStringPrintf("Unknown map item type %x", type);
      return false;
    }

    if (IsDataSectionType(type)) {
      if (aligned_offset == 0u) {
        ErrorStringPrintf("Item %d offset is 0", i);
        return false;
      }
      DCHECK(offset_to_type_map_.Find(aligned_offset) == offset_to_type_map_.end());
      offset_to_type_map_.Insert(std::pair<uint32_t, uint16_t>(aligned_offset, type));
    }

    aligned_offset = ptr_ - begin_;
    if (UNLIKELY(aligned_offset > size_)) {
      ErrorStringPrintf("Item %d at ends out of bounds", i);
      return false;
    }

    offset = aligned_offset;
  }

  return true;
}

bool DexFileVerifier::CheckIntraIdSection(size_t offset,
                                          uint32_t count,
                                          DexFile::MapItemType type) {
  uint32_t expected_offset;
  uint32_t expected_size;

  // Get the expected offset and size from the header.
  switch (type) {
    case DexFile::kDexTypeStringIdItem:
      expected_offset = header_->string_ids_off_;
      expected_size = header_->string_ids_size_;
      break;
    case DexFile::kDexTypeTypeIdItem:
      expected_offset = header_->type_ids_off_;
      expected_size = header_->type_ids_size_;
      break;
    case DexFile::kDexTypeProtoIdItem:
      expected_offset = header_->proto_ids_off_;
      expected_size = header_->proto_ids_size_;
      break;
    case DexFile::kDexTypeFieldIdItem:
      expected_offset = header_->field_ids_off_;
      expected_size = header_->field_ids_size_;
      break;
    case DexFile::kDexTypeMethodIdItem:
      expected_offset = header_->method_ids_off_;
      expected_size = header_->method_ids_size_;
      break;
    case DexFile::kDexTypeClassDefItem:
      expected_offset = header_->class_defs_off_;
      expected_size = header_->class_defs_size_;
      break;
    default:
      ErrorStringPrintf("Bad type for id section: %x", type);
      return false;
  }

  // Check that the offset and size are what were expected from the header.
  if (UNLIKELY(offset != expected_offset)) {
    ErrorStringPrintf("Bad offset for section: got %zx, expected %x", offset, expected_offset);
    return false;
  }
  if (UNLIKELY(count != expected_size)) {
    ErrorStringPrintf("Bad size for section: got %x, expected %x", count, expected_size);
    return false;
  }

  return CheckIntraSectionIterate(offset, count, type);
}

bool DexFileVerifier::CheckIntraDataSection(size_t offset,
                                            uint32_t count,
                                            DexFile::MapItemType type) {
  size_t data_start = header_->data_off_;
  size_t data_end = data_start + header_->data_size_;

  // Sanity check the offset of the section.
  if (UNLIKELY((offset < data_start) || (offset > data_end))) {
    ErrorStringPrintf("Bad offset for data subsection: %zx", offset);
    return false;
  }

  if (!CheckIntraSectionIterate(offset, count, type)) {
    return false;
  }

  size_t next_offset = ptr_ - begin_;
  if (next_offset > data_end) {
    ErrorStringPrintf("Out-of-bounds end of data subsection: %zu data_off=%u data_size=%u",
                      next_offset,
                      header_->data_off_,
                      header_->data_size_);
    return false;
  }

  return true;
}

bool DexFileVerifier::CheckIntraSection() {
  const DexFile::MapList* map = reinterpret_cast<const DexFile::MapList*>(begin_ + header_->map_off_);
  const DexFile::MapItem* item = map->list_;
  size_t offset = 0;
  uint32_t count = map->size_;
  ptr_ = begin_;

  // Check the items listed in the map.
  while (count--) {
    const size_t current_offset = offset;
    uint32_t section_offset = item->offset_;
    uint32_t section_count = item->size_;
    DexFile::MapItemType type = static_cast<DexFile::MapItemType>(item->type_);

    // Check for padding and overlap between items.
    if (!CheckPadding(offset, section_offset, type)) {
      return false;
    } else if (UNLIKELY(offset > section_offset)) {
      ErrorStringPrintf("Section overlap or out-of-order map: %zx, %x", offset, section_offset);
      return false;
    }

    // Check each item based on its type.
    switch (type) {
      case DexFile::kDexTypeHeaderItem:
        if (UNLIKELY(section_count != 1)) {
          ErrorStringPrintf("Multiple header items");
          return false;
        }
        if (UNLIKELY(section_offset != 0)) {
          ErrorStringPrintf("Header at %x, not at start of file", section_offset);
          return false;
        }
        ptr_ = begin_ + header_->header_size_;
        offset = header_->header_size_;
        break;
      case DexFile::kDexTypeStringIdItem:
      case DexFile::kDexTypeTypeIdItem:
      case DexFile::kDexTypeProtoIdItem:
      case DexFile::kDexTypeFieldIdItem:
      case DexFile::kDexTypeMethodIdItem:
      case DexFile::kDexTypeClassDefItem:
        if (!CheckIntraIdSection(section_offset, section_count, type)) {
          return false;
        }
        offset = ptr_ - begin_;
        break;
      case DexFile::kDexTypeMapList:
        if (UNLIKELY(section_count != 1)) {
          ErrorStringPrintf("Multiple map list items");
          return false;
        }
        if (UNLIKELY(section_offset != header_->map_off_)) {
          ErrorStringPrintf("Map not at header-defined offset: %x, expected %x",
                            section_offset, header_->map_off_);
          return false;
        }
        ptr_ += sizeof(uint32_t) + (map->size_ * sizeof(DexFile::MapItem));
        offset = section_offset + sizeof(uint32_t) + (map->size_ * sizeof(DexFile::MapItem));
        break;
      case DexFile::kDexTypeMethodHandleItem:
      case DexFile::kDexTypeCallSiteIdItem:
        CheckIntraSectionIterate(section_offset, section_count, type);
        offset = ptr_ - begin_;
        break;
      case DexFile::kDexTypeTypeList:
      case DexFile::kDexTypeAnnotationSetRefList:
      case DexFile::kDexTypeAnnotationSetItem:
      case DexFile::kDexTypeClassDataItem:
      case DexFile::kDexTypeCodeItem:
      case DexFile::kDexTypeStringDataItem:
      case DexFile::kDexTypeDebugInfoItem:
      case DexFile::kDexTypeAnnotationItem:
      case DexFile::kDexTypeEncodedArrayItem:
      case DexFile::kDexTypeAnnotationsDirectoryItem:
        if (!CheckIntraDataSection(section_offset, section_count, type)) {
          return false;
        }
        offset = ptr_ - begin_;
        break;
    }

    if (offset == current_offset) {
        ErrorStringPrintf("Unknown map item type %x", type);
        return false;
    }

    item++;
  }

  return true;
}

bool DexFileVerifier::CheckOffsetToTypeMap(size_t offset, uint16_t type) {
  DCHECK_NE(offset, 0u);
  auto it = offset_to_type_map_.Find(offset);
  if (UNLIKELY(it == offset_to_type_map_.end())) {
    ErrorStringPrintf("No data map entry found @ %zx; expected %x", offset, type);
    return false;
  }
  if (UNLIKELY(it->second != type)) {
    ErrorStringPrintf("Unexpected data map entry @ %zx; expected %x, found %x",
                      offset, type, it->second);
    return false;
  }
  return true;
}

dex::TypeIndex DexFileVerifier::FindFirstClassDataDefiner(const uint8_t* ptr, bool* success) {
  ClassDataItemIterator it(*dex_file_, ptr);
  *success = true;

  if (it.HasNextStaticField() || it.HasNextInstanceField()) {
    LOAD_FIELD(field, it.GetMemberIndex(), "first_class_data_definer field_id",
               *success = false; return dex::TypeIndex(DexFile::kDexNoIndex16))
    return field->class_idx_;
  }

  if (it.HasNextMethod()) {
    LOAD_METHOD(method, it.GetMemberIndex(), "first_class_data_definer method_id",
                *success = false; return dex::TypeIndex(DexFile::kDexNoIndex16))
    return method->class_idx_;
  }

  return dex::TypeIndex(DexFile::kDexNoIndex16);
}

dex::TypeIndex DexFileVerifier::FindFirstAnnotationsDirectoryDefiner(const uint8_t* ptr,
                                                                     bool* success) {
  const DexFile::AnnotationsDirectoryItem* item =
      reinterpret_cast<const DexFile::AnnotationsDirectoryItem*>(ptr);
  *success = true;

  if (item->fields_size_ != 0) {
    DexFile::FieldAnnotationsItem* field_items = (DexFile::FieldAnnotationsItem*) (item + 1);
    LOAD_FIELD(field, field_items[0].field_idx_, "first_annotations_dir_definer field_id",
               *success = false; return dex::TypeIndex(DexFile::kDexNoIndex16))
    return field->class_idx_;
  }

  if (item->methods_size_ != 0) {
    DexFile::MethodAnnotationsItem* method_items = (DexFile::MethodAnnotationsItem*) (item + 1);
    LOAD_METHOD(method, method_items[0].method_idx_, "first_annotations_dir_definer method id",
                *success = false; return dex::TypeIndex(DexFile::kDexNoIndex16))
    return method->class_idx_;
  }

  if (item->parameters_size_ != 0) {
    DexFile::ParameterAnnotationsItem* parameter_items = (DexFile::ParameterAnnotationsItem*) (item + 1);
    LOAD_METHOD(method, parameter_items[0].method_idx_, "first_annotations_dir_definer method id",
                *success = false; return dex::TypeIndex(DexFile::kDexNoIndex16))
    return method->class_idx_;
  }

  return dex::TypeIndex(DexFile::kDexNoIndex16);
}

bool DexFileVerifier::CheckInterStringIdItem() {
  const DexFile::StringId* item = reinterpret_cast<const DexFile::StringId*>(ptr_);

  // Check the map to make sure it has the right offset->type.
  if (!CheckOffsetToTypeMap(item->string_data_off_, DexFile::kDexTypeStringDataItem)) {
    return false;
  }

  // Check ordering between items.
  if (previous_item_ != nullptr) {
    const DexFile::StringId* prev_item = reinterpret_cast<const DexFile::StringId*>(previous_item_);
    const char* prev_str = dex_file_->GetStringData(*prev_item);
    const char* str = dex_file_->GetStringData(*item);
    if (UNLIKELY(CompareModifiedUtf8ToModifiedUtf8AsUtf16CodePointValues(prev_str, str) >= 0)) {
      ErrorStringPrintf("Out-of-order string_ids: '%s' then '%s'", prev_str, str);
      return false;
    }
  }

  ptr_ += sizeof(DexFile::StringId);
  return true;
}

bool DexFileVerifier::CheckInterTypeIdItem() {
  const DexFile::TypeId* item = reinterpret_cast<const DexFile::TypeId*>(ptr_);

  LOAD_STRING(descriptor, item->descriptor_idx_, "inter_type_id_item descriptor_idx")

  // Check that the descriptor is a valid type.
  if (UNLIKELY(!IsValidDescriptor(descriptor))) {
    ErrorStringPrintf("Invalid type descriptor: '%s'", descriptor);
    return false;
  }

  // Check ordering between items.
  if (previous_item_ != nullptr) {
    const DexFile::TypeId* prev_item = reinterpret_cast<const DexFile::TypeId*>(previous_item_);
    if (UNLIKELY(prev_item->descriptor_idx_ >= item->descriptor_idx_)) {
      ErrorStringPrintf("Out-of-order type_ids: %x then %x",
                        prev_item->descriptor_idx_.index_,
                        item->descriptor_idx_.index_);
      return false;
    }
  }

  ptr_ += sizeof(DexFile::TypeId);
  return true;
}

bool DexFileVerifier::CheckInterProtoIdItem() {
  const DexFile::ProtoId* item = reinterpret_cast<const DexFile::ProtoId*>(ptr_);

  LOAD_STRING(shorty, item->shorty_idx_, "inter_proto_id_item shorty_idx")

  if (item->parameters_off_ != 0 &&
      !CheckOffsetToTypeMap(item->parameters_off_, DexFile::kDexTypeTypeList)) {
    return false;
  }

  // Check that return type is representable as a uint16_t;
  if (UNLIKELY(!IsValidOrNoTypeId(item->return_type_idx_.index_, item->pad_))) {
    ErrorStringPrintf("proto with return type idx outside uint16_t range '%x:%x'",
                      item->pad_, item->return_type_idx_.index_);
    return false;
  }
  // Check the return type and advance the shorty.
  LOAD_STRING_BY_TYPE(return_type, item->return_type_idx_, "inter_proto_id_item return_type_idx")
  if (!CheckShortyDescriptorMatch(*shorty, return_type, true)) {
    return false;
  }
  shorty++;

  DexFileParameterIterator it(*dex_file_, *item);
  while (it.HasNext() && *shorty != '\0') {
    if (!CheckIndex(it.GetTypeIdx().index_,
                    dex_file_->NumTypeIds(),
                    "inter_proto_id_item shorty type_idx")) {
      return false;
    }
    const char* descriptor = it.GetDescriptor();
    if (!CheckShortyDescriptorMatch(*shorty, descriptor, false)) {
      return false;
    }
    it.Next();
    shorty++;
  }
  if (UNLIKELY(it.HasNext() || *shorty != '\0')) {
    ErrorStringPrintf("Mismatched length for parameters and shorty");
    return false;
  }

  // Check ordering between items. This relies on type_ids being in order.
  if (previous_item_ != nullptr) {
    const DexFile::ProtoId* prev = reinterpret_cast<const DexFile::ProtoId*>(previous_item_);
    if (UNLIKELY(prev->return_type_idx_ > item->return_type_idx_)) {
      ErrorStringPrintf("Out-of-order proto_id return types");
      return false;
    } else if (prev->return_type_idx_ == item->return_type_idx_) {
      DexFileParameterIterator curr_it(*dex_file_, *item);
      DexFileParameterIterator prev_it(*dex_file_, *prev);

      while (curr_it.HasNext() && prev_it.HasNext()) {
        dex::TypeIndex prev_idx = prev_it.GetTypeIdx();
        dex::TypeIndex curr_idx = curr_it.GetTypeIdx();
        DCHECK_NE(prev_idx, dex::TypeIndex(DexFile::kDexNoIndex16));
        DCHECK_NE(curr_idx, dex::TypeIndex(DexFile::kDexNoIndex16));

        if (prev_idx < curr_idx) {
          break;
        } else if (UNLIKELY(prev_idx > curr_idx)) {
          ErrorStringPrintf("Out-of-order proto_id arguments");
          return false;
        }

        prev_it.Next();
        curr_it.Next();
      }
      if (!curr_it.HasNext()) {
        // Either a duplicate ProtoId or a ProtoId with a shorter argument list follows
        // a ProtoId with a longer one. Both cases are forbidden by the specification.
        ErrorStringPrintf("Out-of-order proto_id arguments");
        return false;
      }
    }
  }

  ptr_ += sizeof(DexFile::ProtoId);
  return true;
}

bool DexFileVerifier::CheckInterFieldIdItem() {
  const DexFile::FieldId* item = reinterpret_cast<const DexFile::FieldId*>(ptr_);

  // Check that the class descriptor is valid.
  LOAD_STRING_BY_TYPE(class_descriptor, item->class_idx_, "inter_field_id_item class_idx")
  if (UNLIKELY(!IsValidDescriptor(class_descriptor) || class_descriptor[0] != 'L')) {
    ErrorStringPrintf("Invalid descriptor for class_idx: '%s'", class_descriptor);
    return false;
  }

  // Check that the type descriptor is a valid field name.
  LOAD_STRING_BY_TYPE(type_descriptor, item->type_idx_, "inter_field_id_item type_idx")
  if (UNLIKELY(!IsValidDescriptor(type_descriptor) || type_descriptor[0] == 'V')) {
    ErrorStringPrintf("Invalid descriptor for type_idx: '%s'", type_descriptor);
    return false;
  }

  // Check that the name is valid.
  LOAD_STRING(descriptor, item->name_idx_, "inter_field_id_item name_idx")
  if (UNLIKELY(!IsValidMemberName(descriptor))) {
    ErrorStringPrintf("Invalid field name: '%s'", descriptor);
    return false;
  }

  // Check ordering between items. This relies on the other sections being in order.
  if (previous_item_ != nullptr) {
    const DexFile::FieldId* prev_item = reinterpret_cast<const DexFile::FieldId*>(previous_item_);
    if (UNLIKELY(prev_item->class_idx_ > item->class_idx_)) {
      ErrorStringPrintf("Out-of-order field_ids");
      return false;
    } else if (prev_item->class_idx_ == item->class_idx_) {
      if (UNLIKELY(prev_item->name_idx_ > item->name_idx_)) {
        ErrorStringPrintf("Out-of-order field_ids");
        return false;
      } else if (prev_item->name_idx_ == item->name_idx_) {
        if (UNLIKELY(prev_item->type_idx_ >= item->type_idx_)) {
          ErrorStringPrintf("Out-of-order field_ids");
          return false;
        }
      }
    }
  }

  ptr_ += sizeof(DexFile::FieldId);
  return true;
}

bool DexFileVerifier::CheckInterMethodIdItem() {
  const DexFile::MethodId* item = reinterpret_cast<const DexFile::MethodId*>(ptr_);

  // Check that the class descriptor is a valid reference name.
  LOAD_STRING_BY_TYPE(class_descriptor, item->class_idx_, "inter_method_id_item class_idx")
  if (UNLIKELY(!IsValidDescriptor(class_descriptor) || (class_descriptor[0] != 'L' &&
                                                        class_descriptor[0] != '['))) {
    ErrorStringPrintf("Invalid descriptor for class_idx: '%s'", class_descriptor);
    return false;
  }

  // Check that the name is valid.
  LOAD_STRING(descriptor, item->name_idx_, "inter_method_id_item name_idx")
  if (UNLIKELY(!IsValidMemberName(descriptor))) {
    ErrorStringPrintf("Invalid method name: '%s'", descriptor);
    return false;
  }

  // Check that the proto id is valid.
  if (UNLIKELY(!CheckIndex(item->proto_idx_, dex_file_->NumProtoIds(),
                           "inter_method_id_item proto_idx"))) {
    return false;
  }

  // Check ordering between items. This relies on the other sections being in order.
  if (previous_item_ != nullptr) {
    const DexFile::MethodId* prev_item = reinterpret_cast<const DexFile::MethodId*>(previous_item_);
    if (UNLIKELY(prev_item->class_idx_ > item->class_idx_)) {
      ErrorStringPrintf("Out-of-order method_ids");
      return false;
    } else if (prev_item->class_idx_ == item->class_idx_) {
      if (UNLIKELY(prev_item->name_idx_ > item->name_idx_)) {
        ErrorStringPrintf("Out-of-order method_ids");
        return false;
      } else if (prev_item->name_idx_ == item->name_idx_) {
        if (UNLIKELY(prev_item->proto_idx_ >= item->proto_idx_)) {
          ErrorStringPrintf("Out-of-order method_ids");
          return false;
        }
      }
    }
  }

  ptr_ += sizeof(DexFile::MethodId);
  return true;
}

bool DexFileVerifier::CheckInterClassDefItem() {
  const DexFile::ClassDef* item = reinterpret_cast<const DexFile::ClassDef*>(ptr_);

  // Check that class_idx_ is representable as a uint16_t;
  if (UNLIKELY(!IsValidTypeId(item->class_idx_.index_, item->pad1_))) {
    ErrorStringPrintf("class with type idx outside uint16_t range '%x:%x'", item->pad1_,
                      item->class_idx_.index_);
    return false;
  }
  // Check that superclass_idx_ is representable as a uint16_t;
  if (UNLIKELY(!IsValidOrNoTypeId(item->superclass_idx_.index_, item->pad2_))) {
    ErrorStringPrintf("class with superclass type idx outside uint16_t range '%x:%x'", item->pad2_,
                      item->superclass_idx_.index_);
    return false;
  }
  // Check for duplicate class def.
  if (defined_classes_.find(item->class_idx_) != defined_classes_.end()) {
    ErrorStringPrintf("Redefinition of class with type idx: '%d'", item->class_idx_.index_);
    return false;
  }
  defined_classes_.insert(item->class_idx_);

  LOAD_STRING_BY_TYPE(class_descriptor, item->class_idx_, "inter_class_def_item class_idx")
  if (UNLIKELY(!IsValidDescriptor(class_descriptor) || class_descriptor[0] != 'L')) {
    ErrorStringPrintf("Invalid class descriptor: '%s'", class_descriptor);
    return false;
  }

  // Only allow non-runtime modifiers.
  if ((item->access_flags_ & ~kAccJavaFlagsMask) != 0) {
    ErrorStringPrintf("Invalid class flags: '%d'", item->access_flags_);
    return false;
  }

  if (item->interfaces_off_ != 0 &&
      !CheckOffsetToTypeMap(item->interfaces_off_, DexFile::kDexTypeTypeList)) {
    return false;
  }
  if (item->annotations_off_ != 0 &&
      !CheckOffsetToTypeMap(item->annotations_off_, DexFile::kDexTypeAnnotationsDirectoryItem)) {
    return false;
  }
  if (item->class_data_off_ != 0 &&
      !CheckOffsetToTypeMap(item->class_data_off_, DexFile::kDexTypeClassDataItem)) {
    return false;
  }
  if (item->static_values_off_ != 0 &&
      !CheckOffsetToTypeMap(item->static_values_off_, DexFile::kDexTypeEncodedArrayItem)) {
    return false;
  }

  if (item->superclass_idx_.IsValid()) {
    if (header_->GetVersion() >= DexFile::kClassDefinitionOrderEnforcedVersion) {
      // Check that a class does not inherit from itself directly (by having
      // the same type idx as its super class).
      if (UNLIKELY(item->superclass_idx_ == item->class_idx_)) {
        ErrorStringPrintf("Class with same type idx as its superclass: '%d'",
                          item->class_idx_.index_);
        return false;
      }

      // Check that a class is defined after its super class (if the
      // latter is defined in the same Dex file).
      const DexFile::ClassDef* superclass_def = dex_file_->FindClassDef(item->superclass_idx_);
      if (superclass_def != nullptr) {
        // The superclass is defined in this Dex file.
        if (superclass_def > item) {
          // ClassDef item for super class appearing after the class' ClassDef item.
          ErrorStringPrintf("Invalid class definition ordering:"
                            " class with type idx: '%d' defined before"
                            " superclass with type idx: '%d'",
                            item->class_idx_.index_,
                            item->superclass_idx_.index_);
          return false;
        }
      }
    }

    LOAD_STRING_BY_TYPE(superclass_descriptor, item->superclass_idx_,
                        "inter_class_def_item superclass_idx")
    if (UNLIKELY(!IsValidDescriptor(superclass_descriptor) || superclass_descriptor[0] != 'L')) {
      ErrorStringPrintf("Invalid superclass: '%s'", superclass_descriptor);
      return false;
    }
  }

  // Check interfaces.
  const DexFile::TypeList* interfaces = dex_file_->GetInterfacesList(*item);
  if (interfaces != nullptr) {
    uint32_t size = interfaces->Size();
    for (uint32_t i = 0; i < size; i++) {
      if (header_->GetVersion() >= DexFile::kClassDefinitionOrderEnforcedVersion) {
        // Check that a class does not implement itself directly (by having the
        // same type idx as one of its immediate implemented interfaces).
        if (UNLIKELY(interfaces->GetTypeItem(i).type_idx_ == item->class_idx_)) {
          ErrorStringPrintf("Class with same type idx as implemented interface: '%d'",
                            item->class_idx_.index_);
          return false;
        }

        // Check that a class is defined after the interfaces it implements
        // (if they are defined in the same Dex file).
        const DexFile::ClassDef* interface_def =
            dex_file_->FindClassDef(interfaces->GetTypeItem(i).type_idx_);
        if (interface_def != nullptr) {
          // The interface is defined in this Dex file.
          if (interface_def > item) {
            // ClassDef item for interface appearing after the class' ClassDef item.
            ErrorStringPrintf("Invalid class definition ordering:"
                              " class with type idx: '%d' defined before"
                              " implemented interface with type idx: '%d'",
                              item->class_idx_.index_,
                              interfaces->GetTypeItem(i).type_idx_.index_);
            return false;
          }
        }
      }

      // Ensure that the interface refers to a class (not an array nor a primitive type).
      LOAD_STRING_BY_TYPE(inf_descriptor, interfaces->GetTypeItem(i).type_idx_,
                          "inter_class_def_item interface type_idx")
      if (UNLIKELY(!IsValidDescriptor(inf_descriptor) || inf_descriptor[0] != 'L')) {
        ErrorStringPrintf("Invalid interface: '%s'", inf_descriptor);
        return false;
      }
    }

    /*
     * Ensure that there are no duplicates. This is an O(N^2) test, but in
     * practice the number of interfaces implemented by any given class is low.
     */
    for (uint32_t i = 1; i < size; i++) {
      dex::TypeIndex idx1 = interfaces->GetTypeItem(i).type_idx_;
      for (uint32_t j =0; j < i; j++) {
        dex::TypeIndex idx2 = interfaces->GetTypeItem(j).type_idx_;
        if (UNLIKELY(idx1 == idx2)) {
          ErrorStringPrintf("Duplicate interface: '%s'", dex_file_->StringByTypeIdx(idx1));
          return false;
        }
      }
    }
  }

  // Check that references in class_data_item are to the right class.
  if (item->class_data_off_ != 0) {
    const uint8_t* data = begin_ + item->class_data_off_;
    bool success;
    dex::TypeIndex data_definer = FindFirstClassDataDefiner(data, &success);
    if (!success) {
      return false;
    }
    if (UNLIKELY((data_definer != item->class_idx_) &&
                 (data_definer != dex::TypeIndex(DexFile::kDexNoIndex16)))) {
      ErrorStringPrintf("Invalid class_data_item");
      return false;
    }
  }

  // Check that references in annotations_directory_item are to right class.
  if (item->annotations_off_ != 0) {
    // annotations_off_ is supposed to be aligned by 4.
    if (!IsAlignedParam(item->annotations_off_, 4)) {
      ErrorStringPrintf("Invalid annotations_off_, not aligned by 4");
      return false;
    }
    const uint8_t* data = begin_ + item->annotations_off_;
    bool success;
    dex::TypeIndex annotations_definer = FindFirstAnnotationsDirectoryDefiner(data, &success);
    if (!success) {
      return false;
    }
    if (UNLIKELY((annotations_definer != item->class_idx_) &&
                 (annotations_definer != dex::TypeIndex(DexFile::kDexNoIndex16)))) {
      ErrorStringPrintf("Invalid annotations_directory_item");
      return false;
    }
  }

  ptr_ += sizeof(DexFile::ClassDef);
  return true;
}

bool DexFileVerifier::CheckInterCallSiteIdItem() {
  const DexFile::CallSiteIdItem* item = reinterpret_cast<const DexFile::CallSiteIdItem*>(ptr_);

  // Check call site referenced by item is in encoded array section.
  if (!CheckOffsetToTypeMap(item->data_off_, DexFile::kDexTypeEncodedArrayItem)) {
    ErrorStringPrintf("Invalid offset in CallSideIdItem");
    return false;
  }

  CallSiteArrayValueIterator it(*dex_file_, *item);

  // Check Method Handle
  if (!it.HasNext() || it.GetValueType() != EncodedArrayValueIterator::ValueType::kMethodHandle) {
    ErrorStringPrintf("CallSiteArray missing method handle");
    return false;
  }

  uint32_t handle_index = static_cast<uint32_t>(it.GetJavaValue().i);
  if (handle_index >= dex_file_->NumMethodHandles()) {
    ErrorStringPrintf("CallSite has bad method handle id: %x", handle_index);
    return false;
  }

  // Check target method name.
  it.Next();
  if (!it.HasNext() ||
      it.GetValueType() != EncodedArrayValueIterator::ValueType::kString) {
    ErrorStringPrintf("CallSiteArray missing target method name");
    return false;
  }

  uint32_t name_index = static_cast<uint32_t>(it.GetJavaValue().i);
  if (name_index >= dex_file_->NumStringIds()) {
    ErrorStringPrintf("CallSite has bad method name id: %x", name_index);
    return false;
  }

  // Check method type.
  it.Next();
  if (!it.HasNext() ||
      it.GetValueType() != EncodedArrayValueIterator::ValueType::kMethodType) {
    ErrorStringPrintf("CallSiteArray missing method type");
    return false;
  }

  uint32_t proto_index = static_cast<uint32_t>(it.GetJavaValue().i);
  if (proto_index >= dex_file_->NumProtoIds()) {
    ErrorStringPrintf("CallSite has bad method type: %x", proto_index);
    return false;
  }

  ptr_ += sizeof(DexFile::CallSiteIdItem);
  return true;
}

bool DexFileVerifier::CheckInterMethodHandleItem() {
  const DexFile::MethodHandleItem* item = reinterpret_cast<const DexFile::MethodHandleItem*>(ptr_);

  DexFile::MethodHandleType method_handle_type =
      static_cast<DexFile::MethodHandleType>(item->method_handle_type_);
  if (method_handle_type > DexFile::MethodHandleType::kLast) {
    ErrorStringPrintf("Bad method handle type %x", item->method_handle_type_);
    return false;
  }

  uint32_t index = item->field_or_method_idx_;
  switch (method_handle_type) {
    case DexFile::MethodHandleType::kStaticPut:
    case DexFile::MethodHandleType::kStaticGet:
    case DexFile::MethodHandleType::kInstancePut:
    case DexFile::MethodHandleType::kInstanceGet: {
      LOAD_FIELD(field, index, "method_handle_item field_idx", return false);
      break;
    }
    case DexFile::MethodHandleType::kInvokeStatic:
    case DexFile::MethodHandleType::kInvokeInstance:
    case DexFile::MethodHandleType::kInvokeConstructor:
    case DexFile::MethodHandleType::kInvokeDirect:
    case DexFile::MethodHandleType::kInvokeInterface: {
      LOAD_METHOD(method, index, "method_handle_item method_idx", return false);
      break;
    }
  }

  ptr_ += sizeof(DexFile::MethodHandleItem);
  return true;
}

bool DexFileVerifier::CheckInterAnnotationSetRefList() {
  const DexFile::AnnotationSetRefList* list =
      reinterpret_cast<const DexFile::AnnotationSetRefList*>(ptr_);
  const DexFile::AnnotationSetRefItem* item = list->list_;
  uint32_t count = list->size_;

  while (count--) {
    if (item->annotations_off_ != 0 &&
        !CheckOffsetToTypeMap(item->annotations_off_, DexFile::kDexTypeAnnotationSetItem)) {
      return false;
    }
    item++;
  }

  ptr_ = reinterpret_cast<const uint8_t*>(item);
  return true;
}

bool DexFileVerifier::CheckInterAnnotationSetItem() {
  const DexFile::AnnotationSetItem* set = reinterpret_cast<const DexFile::AnnotationSetItem*>(ptr_);
  const uint32_t* offsets = set->entries_;
  uint32_t count = set->size_;
  uint32_t last_idx = 0;

  for (uint32_t i = 0; i < count; i++) {
    if (*offsets != 0 && !CheckOffsetToTypeMap(*offsets, DexFile::kDexTypeAnnotationItem)) {
      return false;
    }

    // Get the annotation from the offset and the type index for the annotation.
    const DexFile::AnnotationItem* annotation =
        reinterpret_cast<const DexFile::AnnotationItem*>(begin_ + *offsets);
    const uint8_t* data = annotation->annotation_;
    DECODE_UNSIGNED_CHECKED_FROM(data, idx);

    if (UNLIKELY(last_idx >= idx && i != 0)) {
      ErrorStringPrintf("Out-of-order entry types: %x then %x", last_idx, idx);
      return false;
    }

    last_idx = idx;
    offsets++;
  }

  ptr_ = reinterpret_cast<const uint8_t*>(offsets);
  return true;
}

bool DexFileVerifier::CheckInterClassDataItem() {
  ClassDataItemIterator it(*dex_file_, ptr_);
  bool success;
  dex::TypeIndex defining_class = FindFirstClassDataDefiner(ptr_, &success);
  if (!success) {
    return false;
  }

  for (; it.HasNextStaticField() || it.HasNextInstanceField(); it.Next()) {
    LOAD_FIELD(field, it.GetMemberIndex(), "inter_class_data_item field_id", return false)
    if (UNLIKELY(field->class_idx_ != defining_class)) {
      ErrorStringPrintf("Mismatched defining class for class_data_item field");
      return false;
    }
  }
  for (; it.HasNextMethod(); it.Next()) {
    uint32_t code_off = it.GetMethodCodeItemOffset();
    if (code_off != 0 && !CheckOffsetToTypeMap(code_off, DexFile::kDexTypeCodeItem)) {
      return false;
    }
    LOAD_METHOD(method, it.GetMemberIndex(), "inter_class_data_item method_id", return false)
    if (UNLIKELY(method->class_idx_ != defining_class)) {
      ErrorStringPrintf("Mismatched defining class for class_data_item method");
      return false;
    }
  }

  ptr_ = it.EndDataPointer();
  return true;
}

bool DexFileVerifier::CheckInterAnnotationsDirectoryItem() {
  const DexFile::AnnotationsDirectoryItem* item =
      reinterpret_cast<const DexFile::AnnotationsDirectoryItem*>(ptr_);
  bool success;
  dex::TypeIndex defining_class = FindFirstAnnotationsDirectoryDefiner(ptr_, &success);
  if (!success) {
    return false;
  }

  if (item->class_annotations_off_ != 0 &&
      !CheckOffsetToTypeMap(item->class_annotations_off_, DexFile::kDexTypeAnnotationSetItem)) {
    return false;
  }

  // Field annotations follow immediately after the annotations directory.
  const DexFile::FieldAnnotationsItem* field_item =
      reinterpret_cast<const DexFile::FieldAnnotationsItem*>(item + 1);
  uint32_t field_count = item->fields_size_;
  for (uint32_t i = 0; i < field_count; i++) {
    LOAD_FIELD(field, field_item->field_idx_, "inter_annotations_directory_item field_id",
               return false)
    if (UNLIKELY(field->class_idx_ != defining_class)) {
      ErrorStringPrintf("Mismatched defining class for field_annotation");
      return false;
    }
    if (!CheckOffsetToTypeMap(field_item->annotations_off_, DexFile::kDexTypeAnnotationSetItem)) {
      return false;
    }
    field_item++;
  }

  // Method annotations follow immediately after field annotations.
  const DexFile::MethodAnnotationsItem* method_item =
      reinterpret_cast<const DexFile::MethodAnnotationsItem*>(field_item);
  uint32_t method_count = item->methods_size_;
  for (uint32_t i = 0; i < method_count; i++) {
    LOAD_METHOD(method, method_item->method_idx_, "inter_annotations_directory_item method_id",
                return false)
    if (UNLIKELY(method->class_idx_ != defining_class)) {
      ErrorStringPrintf("Mismatched defining class for method_annotation");
      return false;
    }
    if (!CheckOffsetToTypeMap(method_item->annotations_off_, DexFile::kDexTypeAnnotationSetItem)) {
      return false;
    }
    method_item++;
  }

  // Parameter annotations follow immediately after method annotations.
  const DexFile::ParameterAnnotationsItem* parameter_item =
      reinterpret_cast<const DexFile::ParameterAnnotationsItem*>(method_item);
  uint32_t parameter_count = item->parameters_size_;
  for (uint32_t i = 0; i < parameter_count; i++) {
    LOAD_METHOD(parameter_method, parameter_item->method_idx_,
                "inter_annotations_directory_item parameter method_id", return false)
    if (UNLIKELY(parameter_method->class_idx_ != defining_class)) {
      ErrorStringPrintf("Mismatched defining class for parameter_annotation");
      return false;
    }
    if (!CheckOffsetToTypeMap(parameter_item->annotations_off_,
        DexFile::kDexTypeAnnotationSetRefList)) {
      return false;
    }
    parameter_item++;
  }

  ptr_ = reinterpret_cast<const uint8_t*>(parameter_item);
  return true;
}

bool DexFileVerifier::CheckInterSectionIterate(size_t offset,
                                               uint32_t count,
                                               DexFile::MapItemType type) {
  // Get the right alignment mask for the type of section.
  size_t alignment_mask;
  switch (type) {
    case DexFile::kDexTypeClassDataItem:
      alignment_mask = sizeof(uint8_t) - 1;
      break;
    default:
      alignment_mask = sizeof(uint32_t) - 1;
      break;
  }

  // Iterate through the items in the section.
  previous_item_ = nullptr;
  for (uint32_t i = 0; i < count; i++) {
    uint32_t new_offset = (offset + alignment_mask) & ~alignment_mask;
    ptr_ = begin_ + new_offset;
    const uint8_t* prev_ptr = ptr_;

    if (MapTypeToBitMask(type) == 0) {
      ErrorStringPrintf("Unknown map item type %x", type);
      return false;
    }

    // Check depending on the section type.
    switch (type) {
      case DexFile::kDexTypeHeaderItem:
      case DexFile::kDexTypeMapList:
      case DexFile::kDexTypeTypeList:
      case DexFile::kDexTypeCodeItem:
      case DexFile::kDexTypeStringDataItem:
      case DexFile::kDexTypeDebugInfoItem:
      case DexFile::kDexTypeAnnotationItem:
      case DexFile::kDexTypeEncodedArrayItem:
        break;
      case DexFile::kDexTypeStringIdItem: {
        if (!CheckInterStringIdItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeTypeIdItem: {
        if (!CheckInterTypeIdItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeProtoIdItem: {
        if (!CheckInterProtoIdItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeFieldIdItem: {
        if (!CheckInterFieldIdItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeMethodIdItem: {
        if (!CheckInterMethodIdItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeClassDefItem: {
        // There shouldn't be more class definitions than type ids allow.
        // This check should be redundant, since there are checks that the
        // class_idx_ is within range and that there is only one definition
        // for a given type id.
        if (i > kTypeIdLimit) {
          ErrorStringPrintf("Too many class definition items");
          return false;
        }
        if (!CheckInterClassDefItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeCallSiteIdItem: {
        if (!CheckInterCallSiteIdItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeMethodHandleItem: {
        if (!CheckInterMethodHandleItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeAnnotationSetRefList: {
        if (!CheckInterAnnotationSetRefList()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeAnnotationSetItem: {
        if (!CheckInterAnnotationSetItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeClassDataItem: {
        // There shouldn't be more class data than type ids allow.
        // This check should be redundant, since there are checks that the
        // class_idx_ is within range and that there is only one definition
        // for a given type id.
        if (i > kTypeIdLimit) {
          ErrorStringPrintf("Too many class data items");
          return false;
        }
        if (!CheckInterClassDataItem()) {
          return false;
        }
        break;
      }
      case DexFile::kDexTypeAnnotationsDirectoryItem: {
        if (!CheckInterAnnotationsDirectoryItem()) {
          return false;
        }
        break;
      }
    }

    previous_item_ = prev_ptr;
    offset = ptr_ - begin_;
  }

  return true;
}

bool DexFileVerifier::CheckInterSection() {
  const DexFile::MapList* map = reinterpret_cast<const DexFile::MapList*>(begin_ + header_->map_off_);
  const DexFile::MapItem* item = map->list_;
  uint32_t count = map->size_;

  // Cross check the items listed in the map.
  while (count--) {
    uint32_t section_offset = item->offset_;
    uint32_t section_count = item->size_;
    DexFile::MapItemType type = static_cast<DexFile::MapItemType>(item->type_);
    bool found = false;

    switch (type) {
      case DexFile::kDexTypeHeaderItem:
      case DexFile::kDexTypeMapList:
      case DexFile::kDexTypeTypeList:
      case DexFile::kDexTypeCodeItem:
      case DexFile::kDexTypeStringDataItem:
      case DexFile::kDexTypeDebugInfoItem:
      case DexFile::kDexTypeAnnotationItem:
      case DexFile::kDexTypeEncodedArrayItem:
        found = true;
        break;
      case DexFile::kDexTypeStringIdItem:
      case DexFile::kDexTypeTypeIdItem:
      case DexFile::kDexTypeProtoIdItem:
      case DexFile::kDexTypeFieldIdItem:
      case DexFile::kDexTypeMethodIdItem:
      case DexFile::kDexTypeClassDefItem:
      case DexFile::kDexTypeCallSiteIdItem:
      case DexFile::kDexTypeMethodHandleItem:
      case DexFile::kDexTypeAnnotationSetRefList:
      case DexFile::kDexTypeAnnotationSetItem:
      case DexFile::kDexTypeClassDataItem:
      case DexFile::kDexTypeAnnotationsDirectoryItem: {
        if (!CheckInterSectionIterate(section_offset, section_count, type)) {
          return false;
        }
        found = true;
        break;
      }
    }

    if (!found) {
      ErrorStringPrintf("Unknown map item type %x", item->type_);
      return false;
    }

    item++;
  }

  return true;
}

bool DexFileVerifier::Verify() {
  // Check the header.
  if (!CheckHeader()) {
    return false;
  }

  // Check the map section.
  if (!CheckMap()) {
    return false;
  }

  // Check structure within remaining sections.
  if (!CheckIntraSection()) {
    return false;
  }

  // Check references from one section to another.
  if (!CheckInterSection()) {
    return false;
  }

  return true;
}

void DexFileVerifier::ErrorStringPrintf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  DCHECK(failure_reason_.empty()) << failure_reason_;
  failure_reason_ = StringPrintf("Failure to verify dex file '%s': ", location_);
  StringAppendV(&failure_reason_, fmt, ap);
  va_end(ap);
}

// Fields and methods may have only one of public/protected/private.
static bool CheckAtMostOneOfPublicProtectedPrivate(uint32_t flags) {
  size_t count = (((flags & kAccPublic) == 0) ? 0 : 1) +
                 (((flags & kAccProtected) == 0) ? 0 : 1) +
                 (((flags & kAccPrivate) == 0) ? 0 : 1);
  return count <= 1;
}

// Helper functions to retrieve names from the dex file. We do not want to rely on DexFile
// functionality, as we're still verifying the dex file. begin and header correspond to the
// underscored variants in the DexFileVerifier.

static std::string GetStringOrError(const uint8_t* const begin,
                                    const DexFile::Header* const header,
                                    dex::StringIndex string_idx) {
  // The `string_idx` is not guaranteed to be valid yet.
  if (header->string_ids_size_ <= string_idx.index_) {
    return "(error)";
  }

  const DexFile::StringId* string_id =
      reinterpret_cast<const DexFile::StringId*>(begin + header->string_ids_off_)
          + string_idx.index_;

  // Assume that the data is OK at this point. String data has been checked at this point.

  const uint8_t* ptr = begin + string_id->string_data_off_;
  uint32_t dummy;
  if (!DecodeUnsignedLeb128Checked(&ptr, begin + header->file_size_, &dummy)) {
    return "(error)";
  }
  return reinterpret_cast<const char*>(ptr);
}

static std::string GetClassOrError(const uint8_t* const begin,
                                   const DexFile::Header* const header,
                                   dex::TypeIndex class_idx) {
  // The `class_idx` is either `FieldId::class_idx_` or `MethodId::class_idx_` and
  // it has already been checked in `DexFileVerifier::CheckClassDataItemField()`
  // or `DexFileVerifier::CheckClassDataItemMethod()`, respectively, to match
  // a valid defining class.
  CHECK_LT(class_idx.index_, header->type_ids_size_);

  const DexFile::TypeId* type_id =
      reinterpret_cast<const DexFile::TypeId*>(begin + header->type_ids_off_) + class_idx.index_;

  // Assume that the data is OK at this point. Type id offsets have been checked at this point.

  return GetStringOrError(begin, header, type_id->descriptor_idx_);
}

static std::string GetFieldDescriptionOrError(const uint8_t* const begin,
                                              const DexFile::Header* const header,
                                              uint32_t idx) {
  // The `idx` has already been checked in `DexFileVerifier::CheckClassDataItemField()`.
  CHECK_LT(idx, header->field_ids_size_);

  const DexFile::FieldId* field_id =
      reinterpret_cast<const DexFile::FieldId*>(begin + header->field_ids_off_) + idx;

  // Assume that the data is OK at this point. Field id offsets have been checked at this point.

  std::string class_name = GetClassOrError(begin, header, field_id->class_idx_);
  std::string field_name = GetStringOrError(begin, header, field_id->name_idx_);

  return class_name + "." + field_name;
}

static std::string GetMethodDescriptionOrError(const uint8_t* const begin,
                                               const DexFile::Header* const header,
                                               uint32_t idx) {
  // The `idx` has already been checked in `DexFileVerifier::CheckClassDataItemMethod()`.
  CHECK_LT(idx, header->method_ids_size_);

  const DexFile::MethodId* method_id =
      reinterpret_cast<const DexFile::MethodId*>(begin + header->method_ids_off_) + idx;

  // Assume that the data is OK at this point. Method id offsets have been checked at this point.

  std::string class_name = GetClassOrError(begin, header, method_id->class_idx_);
  std::string method_name = GetStringOrError(begin, header, method_id->name_idx_);

  return class_name + "." + method_name;
}

bool DexFileVerifier::CheckFieldAccessFlags(uint32_t idx,
                                            uint32_t field_access_flags,
                                            uint32_t class_access_flags,
                                            std::string* error_msg) {
  // Generally sort out >16-bit flags.
  if ((field_access_flags & ~kAccJavaFlagsMask) != 0) {
    *error_msg = StringPrintf("Bad field access_flags for %s: %x(%s)",
                              GetFieldDescriptionOrError(begin_, header_, idx).c_str(),
                              field_access_flags,
                              PrettyJavaAccessFlags(field_access_flags).c_str());
    return false;
  }

  // Flags allowed on fields, in general. Other lower-16-bit flags are to be ignored.
  constexpr uint32_t kFieldAccessFlags = kAccPublic |
                                         kAccPrivate |
                                         kAccProtected |
                                         kAccStatic |
                                         kAccFinal |
                                         kAccVolatile |
                                         kAccTransient |
                                         kAccSynthetic |
                                         kAccEnum;

  // Fields may have only one of public/protected/final.
  if (!CheckAtMostOneOfPublicProtectedPrivate(field_access_flags)) {
    *error_msg = StringPrintf("Field may have only one of public/protected/private, %s: %x(%s)",
                              GetFieldDescriptionOrError(begin_, header_, idx).c_str(),
                              field_access_flags,
                              PrettyJavaAccessFlags(field_access_flags).c_str());
    return false;
  }

  // Interfaces have a pretty restricted list.
  if ((class_access_flags & kAccInterface) != 0) {
    // Interface fields must be public final static.
    constexpr uint32_t kPublicFinalStatic = kAccPublic | kAccFinal | kAccStatic;
    if ((field_access_flags & kPublicFinalStatic) != kPublicFinalStatic) {
      *error_msg = StringPrintf("Interface field is not public final static, %s: %x(%s)",
                                GetFieldDescriptionOrError(begin_, header_, idx).c_str(),
                                field_access_flags,
                                PrettyJavaAccessFlags(field_access_flags).c_str());
      if (dex_file_->SupportsDefaultMethods()) {
        return false;
      } else {
        // Allow in older versions, but warn.
        LOG(WARNING) << "This dex file is invalid and will be rejected in the future. Error is: "
                     << *error_msg;
      }
    }
    // Interface fields may be synthetic, but may not have other flags.
    constexpr uint32_t kDisallowed = ~(kPublicFinalStatic | kAccSynthetic);
    if ((field_access_flags & kFieldAccessFlags & kDisallowed) != 0) {
      *error_msg = StringPrintf("Interface field has disallowed flag, %s: %x(%s)",
                                GetFieldDescriptionOrError(begin_, header_, idx).c_str(),
                                field_access_flags,
                                PrettyJavaAccessFlags(field_access_flags).c_str());
      if (dex_file_->SupportsDefaultMethods()) {
        return false;
      } else {
        // Allow in older versions, but warn.
        LOG(WARNING) << "This dex file is invalid and will be rejected in the future. Error is: "
                     << *error_msg;
      }
    }
    return true;
  }

  // Volatile fields may not be final.
  constexpr uint32_t kVolatileFinal = kAccVolatile | kAccFinal;
  if ((field_access_flags & kVolatileFinal) == kVolatileFinal) {
    *error_msg = StringPrintf("Fields may not be volatile and final: %s",
                              GetFieldDescriptionOrError(begin_, header_, idx).c_str());
    return false;
  }

  return true;
}

bool DexFileVerifier::CheckMethodAccessFlags(uint32_t method_index,
                                             uint32_t method_access_flags,
                                             uint32_t class_access_flags,
                                             uint32_t constructor_flags_by_name,
                                             bool has_code,
                                             bool expect_direct,
                                             std::string* error_msg) {
  // Generally sort out >16-bit flags, except dex knows Constructor and DeclaredSynchronized.
  constexpr uint32_t kAllMethodFlags =
      kAccJavaFlagsMask | kAccConstructor | kAccDeclaredSynchronized;
  if ((method_access_flags & ~kAllMethodFlags) != 0) {
    *error_msg = StringPrintf("Bad method access_flags for %s: %x",
                              GetMethodDescriptionOrError(begin_, header_, method_index).c_str(),
                              method_access_flags);
    return false;
  }

  // Flags allowed on fields, in general. Other lower-16-bit flags are to be ignored.
  constexpr uint32_t kMethodAccessFlags = kAccPublic |
                                          kAccPrivate |
                                          kAccProtected |
                                          kAccStatic |
                                          kAccFinal |
                                          kAccSynthetic |
                                          kAccSynchronized |
                                          kAccBridge |
                                          kAccVarargs |
                                          kAccNative |
                                          kAccAbstract |
                                          kAccStrict;

  // Methods may have only one of public/protected/final.
  if (!CheckAtMostOneOfPublicProtectedPrivate(method_access_flags)) {
    *error_msg = StringPrintf("Method may have only one of public/protected/private, %s: %x",
                              GetMethodDescriptionOrError(begin_, header_, method_index).c_str(),
                              method_access_flags);
    return false;
  }

  constexpr uint32_t kConstructorFlags = kAccStatic | kAccConstructor;
  const bool is_constructor_by_name = (constructor_flags_by_name & kConstructorFlags) != 0;
  const bool is_clinit_by_name = constructor_flags_by_name == kConstructorFlags;

  // Only methods named "<clinit>" or "<init>" may be marked constructor. Note: we cannot enforce
  // the reverse for backwards compatibility reasons.
  if (((method_access_flags & kAccConstructor) != 0) && !is_constructor_by_name) {
    *error_msg =
        StringPrintf("Method %" PRIu32 "(%s) is marked constructor, but doesn't match name",
                      method_index,
                      GetMethodDescriptionOrError(begin_, header_, method_index).c_str());
    return false;
  }

  if (is_constructor_by_name) {
    // Check that the static constructor (= static initializer) is named "<clinit>" and that the
    // instance constructor is called "<init>".
    bool is_static = (method_access_flags & kAccStatic) != 0;
    if (is_static ^ is_clinit_by_name) {
      *error_msg = StringPrintf("Constructor %" PRIu32 "(%s) is not flagged correctly wrt/ static.",
                                method_index,
                                GetMethodDescriptionOrError(begin_, header_, method_index).c_str());
      if (dex_file_->SupportsDefaultMethods()) {
        return false;
      } else {
        // Allow in older versions, but warn.
        LOG(WARNING) << "This dex file is invalid and will be rejected in the future. Error is: "
                     << *error_msg;
      }
    }
  }

  // Check that static and private methods, as well as constructors, are in the direct methods list,
  // and other methods in the virtual methods list.
  bool is_direct = ((method_access_flags & (kAccStatic | kAccPrivate)) != 0) ||
                   is_constructor_by_name;
  if (is_direct != expect_direct) {
    *error_msg = StringPrintf("Direct/virtual method %" PRIu32 "(%s) not in expected list %d",
                              method_index,
                              GetMethodDescriptionOrError(begin_, header_, method_index).c_str(),
                              expect_direct);
    return false;
  }

  // From here on out it is easier to mask out the bits we're supposed to ignore.
  method_access_flags &= kMethodAccessFlags;

  // Interfaces are special.
  if ((class_access_flags & kAccInterface) != 0) {
    // Non-static interface methods must be public or private.
    uint32_t desired_flags = (kAccPublic | kAccStatic);
    if (dex_file_->SupportsDefaultMethods()) {
      desired_flags |= kAccPrivate;
    }
    if ((method_access_flags & desired_flags) == 0) {
      *error_msg = StringPrintf("Interface virtual method %" PRIu32 "(%s) is not public",
          method_index,
          GetMethodDescriptionOrError(begin_, header_, method_index).c_str());
      if (dex_file_->SupportsDefaultMethods()) {
        return false;
      } else {
        // Allow in older versions, but warn.
        LOG(WARNING) << "This dex file is invalid and will be rejected in the future. Error is: "
                      << *error_msg;
      }
    }
  }

  // If there aren't any instructions, make sure that's expected.
  if (!has_code) {
    // Only native or abstract methods may not have code.
    if ((method_access_flags & (kAccNative | kAccAbstract)) == 0) {
      *error_msg = StringPrintf("Method %" PRIu32 "(%s) has no code, but is not marked native or "
                                "abstract",
                                method_index,
                                GetMethodDescriptionOrError(begin_, header_, method_index).c_str());
      return false;
    }
    // Constructors must always have code.
    if (is_constructor_by_name) {
      *error_msg = StringPrintf("Constructor %u(%s) must not be abstract or native",
                                method_index,
                                GetMethodDescriptionOrError(begin_, header_, method_index).c_str());
      if (dex_file_->SupportsDefaultMethods()) {
        return false;
      } else {
        // Allow in older versions, but warn.
        LOG(WARNING) << "This dex file is invalid and will be rejected in the future. Error is: "
                      << *error_msg;
      }
    }
    if ((method_access_flags & kAccAbstract) != 0) {
      // Abstract methods are not allowed to have the following flags.
      constexpr uint32_t kForbidden =
          kAccPrivate | kAccStatic | kAccFinal | kAccNative | kAccStrict | kAccSynchronized;
      if ((method_access_flags & kForbidden) != 0) {
        *error_msg = StringPrintf("Abstract method %" PRIu32 "(%s) has disallowed access flags %x",
            method_index,
            GetMethodDescriptionOrError(begin_, header_, method_index).c_str(),
            method_access_flags);
        return false;
      }
      // Abstract methods should be in an abstract class or interface.
      if ((class_access_flags & (kAccInterface | kAccAbstract)) == 0) {
        LOG(WARNING) << "Method " << GetMethodDescriptionOrError(begin_, header_, method_index)
                     << " is abstract, but the declaring class is neither abstract nor an "
                     << "interface in dex file "
                     << dex_file_->GetLocation();
      }
    }
    // Interfaces are special.
    if ((class_access_flags & kAccInterface) != 0) {
      // Interface methods without code must be abstract.
      if ((method_access_flags & (kAccPublic | kAccAbstract)) != (kAccPublic | kAccAbstract)) {
        *error_msg = StringPrintf("Interface method %" PRIu32 "(%s) is not public and abstract",
            method_index,
            GetMethodDescriptionOrError(begin_, header_, method_index).c_str());
        if (dex_file_->SupportsDefaultMethods()) {
          return false;
        } else {
          // Allow in older versions, but warn.
          LOG(WARNING) << "This dex file is invalid and will be rejected in the future. Error is: "
                       << *error_msg;
        }
      }
      // At this point, we know the method is public and abstract. This means that all the checks
      // for invalid combinations above applies. In addition, interface methods must not be
      // protected. This is caught by the check for only-one-of-public-protected-private.
    }
    return true;
  }

  // When there's code, the method must not be native or abstract.
  if ((method_access_flags & (kAccNative | kAccAbstract)) != 0) {
    *error_msg = StringPrintf("Method %" PRIu32 "(%s) has code, but is marked native or abstract",
                              method_index,
                              GetMethodDescriptionOrError(begin_, header_, method_index).c_str());
    return false;
  }

  // Instance constructors must not be synchronized and a few other flags.
  if (constructor_flags_by_name == kAccConstructor) {
    static constexpr uint32_t kInitAllowed =
        kAccPrivate | kAccProtected | kAccPublic | kAccStrict | kAccVarargs | kAccSynthetic;
    if ((method_access_flags & ~kInitAllowed) != 0) {
      *error_msg = StringPrintf("Constructor %" PRIu32 "(%s) flagged inappropriately %x",
                                method_index,
                                GetMethodDescriptionOrError(begin_, header_, method_index).c_str(),
                                method_access_flags);
      return false;
    }
  }

  return true;
}

bool DexFileVerifier::CheckConstructorProperties(
      uint32_t method_index,
      uint32_t constructor_flags) {
  DCHECK(constructor_flags == kAccConstructor ||
         constructor_flags == (kAccConstructor | kAccStatic));

  // Check signature matches expectations.
  const DexFile::MethodId* const method_id = CheckLoadMethodId(method_index,
                                                               "Bad <init>/<clinit> method id");
  if (method_id == nullptr) {
    return false;
  }

  // Check the ProtoId for the corresponding method.
  //
  // TODO(oth): the error message here is to satisfy the MethodId test
  // in the DexFileVerifierTest. The test is checking that the error
  // contains this string if the index is out of range.
  const DexFile::ProtoId* const proto_id = CheckLoadProtoId(method_id->proto_idx_,
                                                            "inter_method_id_item proto_idx");
  if (proto_id == nullptr) {
    return false;
  }

  Signature signature = dex_file_->GetMethodSignature(*method_id);
  if (constructor_flags == (kAccStatic | kAccConstructor)) {
    if (!signature.IsVoid() || signature.GetNumberOfParameters() != 0) {
      ErrorStringPrintf("<clinit> must have descriptor ()V");
      return false;
    }
  } else if (!signature.IsVoid()) {
    ErrorStringPrintf("Constructor %u(%s) must be void",
                      method_index,
                      GetMethodDescriptionOrError(begin_, header_, method_index).c_str());
    return false;
  }

  return true;
}

}  // namespace art_lkchan
