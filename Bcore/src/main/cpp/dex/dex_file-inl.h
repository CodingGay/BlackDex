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

#ifndef ART_LIBDEXFILE_DEX_DEX_FILE_INL_H_
#define ART_LIBDEXFILE_DEX_DEX_FILE_INL_H_

#include "base/casts.h"
#include "base/leb128.h"
#include "base/stringpiece.h"
#include "compact_dex_file.h"
#include "dex_file.h"
#include "invoke_type.h"
#include "standard_dex_file.h"

namespace art_lkchan {

inline int32_t DexFile::GetStringLength(const StringId& string_id) const {
  const uint8_t* ptr = DataBegin() + string_id.string_data_off_;
  return DecodeUnsignedLeb128(&ptr);
}

inline const char* DexFile::GetStringDataAndUtf16Length(const StringId& string_id,
                                                        uint32_t* utf16_length) const {
  DCHECK(utf16_length != nullptr) << GetLocation();
  const uint8_t* ptr = DataBegin() + string_id.string_data_off_;
  *utf16_length = DecodeUnsignedLeb128(&ptr);
  return reinterpret_cast<const char*>(ptr);
}

inline const char* DexFile::GetStringData(const StringId& string_id) const {
  uint32_t ignored;
  return GetStringDataAndUtf16Length(string_id, &ignored);
}

inline const char* DexFile::StringDataAndUtf16LengthByIdx(dex::StringIndex idx,
                                                          uint32_t* utf16_length) const {
  if (!idx.IsValid()) {
    *utf16_length = 0;
    return nullptr;
  }
  const StringId& string_id = GetStringId(idx);
  return GetStringDataAndUtf16Length(string_id, utf16_length);
}

inline const char* DexFile::StringDataByIdx(dex::StringIndex idx) const {
  uint32_t unicode_length;
  return StringDataAndUtf16LengthByIdx(idx, &unicode_length);
}

inline const char* DexFile::StringByTypeIdx(dex::TypeIndex idx, uint32_t* unicode_length) const {
  if (!idx.IsValid()) {
    return nullptr;
  }
  const TypeId& type_id = GetTypeId(idx);
  return StringDataAndUtf16LengthByIdx(type_id.descriptor_idx_, unicode_length);
}

inline const char* DexFile::StringByTypeIdx(dex::TypeIndex idx) const {
  if (!idx.IsValid()) {
    return nullptr;
  }
  const TypeId& type_id = GetTypeId(idx);
  return StringDataByIdx(type_id.descriptor_idx_);
}

inline const char* DexFile::GetTypeDescriptor(const TypeId& type_id) const {
  return StringDataByIdx(type_id.descriptor_idx_);
}

inline const char* DexFile::GetFieldTypeDescriptor(const FieldId& field_id) const {
  const DexFile::TypeId& type_id = GetTypeId(field_id.type_idx_);
  return GetTypeDescriptor(type_id);
}

inline const char* DexFile::GetFieldName(const FieldId& field_id) const {
  return StringDataByIdx(field_id.name_idx_);
}

inline const char* DexFile::GetMethodDeclaringClassDescriptor(const MethodId& method_id) const {
  const DexFile::TypeId& type_id = GetTypeId(method_id.class_idx_);
  return GetTypeDescriptor(type_id);
}

inline const Signature DexFile::GetMethodSignature(const MethodId& method_id) const {
  return Signature(this, GetProtoId(method_id.proto_idx_));
}

inline const Signature DexFile::GetProtoSignature(const ProtoId& proto_id) const {
  return Signature(this, proto_id);
}

inline const char* DexFile::GetMethodName(const MethodId& method_id) const {
  return StringDataByIdx(method_id.name_idx_);
}

inline const char* DexFile::GetMethodShorty(uint32_t idx) const {
  return StringDataByIdx(GetProtoId(GetMethodId(idx).proto_idx_).shorty_idx_);
}

inline const char* DexFile::GetMethodShorty(const MethodId& method_id) const {
  return StringDataByIdx(GetProtoId(method_id.proto_idx_).shorty_idx_);
}

inline const char* DexFile::GetMethodShorty(const MethodId& method_id, uint32_t* length) const {
  // Using the UTF16 length is safe here as shorties are guaranteed to be ASCII characters.
  return StringDataAndUtf16LengthByIdx(GetProtoId(method_id.proto_idx_).shorty_idx_, length);
}

inline const char* DexFile::GetClassDescriptor(const ClassDef& class_def) const {
  return StringByTypeIdx(class_def.class_idx_);
}

inline const char* DexFile::GetReturnTypeDescriptor(const ProtoId& proto_id) const {
  return StringByTypeIdx(proto_id.return_type_idx_);
}

inline const char* DexFile::GetShorty(uint32_t proto_idx) const {
  const ProtoId& proto_id = GetProtoId(proto_idx);
  return StringDataByIdx(proto_id.shorty_idx_);
}

inline const DexFile::TryItem* DexFile::GetTryItems(const DexInstructionIterator& code_item_end,
                                                    uint32_t offset) {
  return reinterpret_cast<const TryItem*>
      (RoundUp(reinterpret_cast<uintptr_t>(&code_item_end.Inst()), TryItem::kAlignment)) + offset;
}

static inline bool DexFileStringEquals(const DexFile* df1, dex::StringIndex sidx1,
                                       const DexFile* df2, dex::StringIndex sidx2) {
  uint32_t s1_len;  // Note: utf16 length != mutf8 length.
  const char* s1_data = df1->StringDataAndUtf16LengthByIdx(sidx1, &s1_len);
  uint32_t s2_len;
  const char* s2_data = df2->StringDataAndUtf16LengthByIdx(sidx2, &s2_len);
  return (s1_len == s2_len) && (strcmp(s1_data, s2_data) == 0);
}

inline bool Signature::operator==(const Signature& rhs) const {
  if (dex_file_ == nullptr) {
    return rhs.dex_file_ == nullptr;
  }
  if (rhs.dex_file_ == nullptr) {
    return false;
  }
  if (dex_file_ == rhs.dex_file_) {
    return proto_id_ == rhs.proto_id_;
  }
  uint32_t lhs_shorty_len;  // For a shorty utf16 length == mutf8 length.
  const char* lhs_shorty_data = dex_file_->StringDataAndUtf16LengthByIdx(proto_id_->shorty_idx_,
                                                                         &lhs_shorty_len);
  StringPiece lhs_shorty(lhs_shorty_data, lhs_shorty_len);
  {
    uint32_t rhs_shorty_len;
    const char* rhs_shorty_data =
        rhs.dex_file_->StringDataAndUtf16LengthByIdx(rhs.proto_id_->shorty_idx_,
                                                     &rhs_shorty_len);
    StringPiece rhs_shorty(rhs_shorty_data, rhs_shorty_len);
    if (lhs_shorty != rhs_shorty) {
      return false;  // Shorty mismatch.
    }
  }
  if (lhs_shorty[0] == 'L') {
    const DexFile::TypeId& return_type_id = dex_file_->GetTypeId(proto_id_->return_type_idx_);
    const DexFile::TypeId& rhs_return_type_id =
        rhs.dex_file_->GetTypeId(rhs.proto_id_->return_type_idx_);
    if (!DexFileStringEquals(dex_file_, return_type_id.descriptor_idx_,
                             rhs.dex_file_, rhs_return_type_id.descriptor_idx_)) {
      return false;  // Return type mismatch.
    }
  }
  if (lhs_shorty.find('L', 1) != StringPiece::npos) {
    const DexFile::TypeList* params = dex_file_->GetProtoParameters(*proto_id_);
    const DexFile::TypeList* rhs_params = rhs.dex_file_->GetProtoParameters(*rhs.proto_id_);
    // We found a reference parameter in the matching shorty, so both lists must be non-empty.
    DCHECK(params != nullptr);
    DCHECK(rhs_params != nullptr);
    uint32_t params_size = params->Size();
    DCHECK_EQ(params_size, rhs_params->Size());  // Parameter list size must match.
    for (uint32_t i = 0; i < params_size; ++i) {
      const DexFile::TypeId& param_id = dex_file_->GetTypeId(params->GetTypeItem(i).type_idx_);
      const DexFile::TypeId& rhs_param_id =
          rhs.dex_file_->GetTypeId(rhs_params->GetTypeItem(i).type_idx_);
      if (!DexFileStringEquals(dex_file_, param_id.descriptor_idx_,
                               rhs.dex_file_, rhs_param_id.descriptor_idx_)) {
        return false;  // Parameter type mismatch.
      }
    }
  }
  return true;
}

inline
InvokeType ClassDataItemIterator::GetMethodInvokeType(const DexFile::ClassDef& class_def) const {
  if (HasNextDirectMethod()) {
    if ((GetRawMemberAccessFlags() & kAccStatic) != 0) {
      return kStatic;
    } else {
      return kDirect;
    }
  } else {
    DCHECK_EQ(GetRawMemberAccessFlags() & kAccStatic, 0U);
    if ((class_def.access_flags_ & kAccInterface) != 0) {
      return kInterface;
    } else if ((GetRawMemberAccessFlags() & kAccConstructor) != 0) {
      return kSuper;
    } else {
      return kVirtual;
    }
  }
}

template<typename NewLocalCallback, typename IndexToStringData, typename TypeIndexToStringData>
bool DexFile::DecodeDebugLocalInfo(const uint8_t* stream,
                                   const std::string& location,
                                   const char* declaring_class_descriptor,
                                   const std::vector<const char*>& arg_descriptors,
                                   const std::string& method_name,
                                   bool is_static,
                                   uint16_t registers_size,
                                   uint16_t ins_size,
                                   uint16_t insns_size_in_code_units,
                                   IndexToStringData index_to_string_data,
                                   TypeIndexToStringData type_index_to_string_data,
                                   NewLocalCallback new_local_callback,
                                   void* context) {
  if (stream == nullptr) {
    return false;
  }
  std::vector<LocalInfo> local_in_reg(registers_size);

  uint16_t arg_reg = registers_size - ins_size;
  if (!is_static) {
    const char* descriptor = declaring_class_descriptor;
    local_in_reg[arg_reg].name_ = "this";
    local_in_reg[arg_reg].descriptor_ = descriptor;
    local_in_reg[arg_reg].signature_ = nullptr;
    local_in_reg[arg_reg].start_address_ = 0;
    local_in_reg[arg_reg].reg_ = arg_reg;
    local_in_reg[arg_reg].is_live_ = true;
    arg_reg++;
  }

  DecodeUnsignedLeb128(&stream);  // Line.
  uint32_t parameters_size = DecodeUnsignedLeb128(&stream);
  uint32_t i;
  if (parameters_size != arg_descriptors.size()) {
    LOG(ERROR) << "invalid stream - problem with parameter iterator in " << location
               << " for method " << method_name;
    return false;
  }
  for (i = 0; i < parameters_size && i < arg_descriptors.size(); ++i) {
    if (arg_reg >= registers_size) {
      LOG(ERROR) << "invalid stream - arg reg >= reg size (" << arg_reg
                 << " >= " << registers_size << ") in " << location;
      return false;
    }
    uint32_t name_idx = DecodeUnsignedLeb128P1(&stream);
    const char* descriptor = arg_descriptors[i];
    local_in_reg[arg_reg].name_ = index_to_string_data(name_idx);
    local_in_reg[arg_reg].descriptor_ = descriptor;
    local_in_reg[arg_reg].signature_ = nullptr;
    local_in_reg[arg_reg].start_address_ = 0;
    local_in_reg[arg_reg].reg_ = arg_reg;
    local_in_reg[arg_reg].is_live_ = true;
    switch (*descriptor) {
      case 'D':
      case 'J':
        arg_reg += 2;
        break;
      default:
        arg_reg += 1;
        break;
    }
  }

  uint32_t address = 0;
  for (;;)  {
    uint8_t opcode = *stream++;
    switch (opcode) {
      case DBG_END_SEQUENCE:
        // Emit all variables which are still alive at the end of the method.
        for (uint16_t reg = 0; reg < registers_size; reg++) {
          if (local_in_reg[reg].is_live_) {
            local_in_reg[reg].end_address_ = insns_size_in_code_units;
            new_local_callback(context, local_in_reg[reg]);
          }
        }
        return true;
      case DBG_ADVANCE_PC:
        address += DecodeUnsignedLeb128(&stream);
        break;
      case DBG_ADVANCE_LINE:
        DecodeSignedLeb128(&stream);  // Line.
        break;
      case DBG_START_LOCAL:
      case DBG_START_LOCAL_EXTENDED: {
        uint16_t reg = DecodeUnsignedLeb128(&stream);
        if (reg >= registers_size) {
          LOG(ERROR) << "invalid stream - reg >= reg size (" << reg << " >= "
                     << registers_size << ") in " << location;
          return false;
        }

        uint32_t name_idx = DecodeUnsignedLeb128P1(&stream);
        uint16_t descriptor_idx = DecodeUnsignedLeb128P1(&stream);
        uint32_t signature_idx = dex::kDexNoIndex;
        if (opcode == DBG_START_LOCAL_EXTENDED) {
          signature_idx = DecodeUnsignedLeb128P1(&stream);
        }

        // Emit what was previously there, if anything
        if (local_in_reg[reg].is_live_) {
          local_in_reg[reg].end_address_ = address;
          new_local_callback(context, local_in_reg[reg]);
        }

        local_in_reg[reg].name_ = index_to_string_data(name_idx);
        local_in_reg[reg].descriptor_ = type_index_to_string_data(descriptor_idx);;
        local_in_reg[reg].signature_ = index_to_string_data(signature_idx);
        local_in_reg[reg].start_address_ = address;
        local_in_reg[reg].reg_ = reg;
        local_in_reg[reg].is_live_ = true;
        break;
      }
      case DBG_END_LOCAL: {
        uint16_t reg = DecodeUnsignedLeb128(&stream);
        if (reg >= registers_size) {
          LOG(ERROR) << "invalid stream - reg >= reg size (" << reg << " >= "
                     << registers_size << ") in " << location;
          return false;
        }
        // If the register is live, close it properly. Otherwise, closing an already
        // closed register is sloppy, but harmless if no further action is taken.
        if (local_in_reg[reg].is_live_) {
          local_in_reg[reg].end_address_ = address;
          new_local_callback(context, local_in_reg[reg]);
          local_in_reg[reg].is_live_ = false;
        }
        break;
      }
      case DBG_RESTART_LOCAL: {
        uint16_t reg = DecodeUnsignedLeb128(&stream);
        if (reg >= registers_size) {
          LOG(ERROR) << "invalid stream - reg >= reg size (" << reg << " >= "
                     << registers_size << ") in " << location;
          return false;
        }
        // If the register is live, the "restart" is superfluous,
        // and we don't want to mess with the existing start address.
        if (!local_in_reg[reg].is_live_) {
          local_in_reg[reg].start_address_ = address;
          local_in_reg[reg].is_live_ = true;
        }
        break;
      }
      case DBG_SET_PROLOGUE_END:
      case DBG_SET_EPILOGUE_BEGIN:
        break;
      case DBG_SET_FILE:
        DecodeUnsignedLeb128P1(&stream);  // name.
        break;
      default:
        address += (opcode - DBG_FIRST_SPECIAL) / DBG_LINE_RANGE;
        break;
    }
  }
}

template<typename NewLocalCallback>
bool DexFile::DecodeDebugLocalInfo(uint32_t registers_size,
                                   uint32_t ins_size,
                                   uint32_t insns_size_in_code_units,
                                   uint32_t debug_info_offset,
                                   bool is_static,
                                   uint32_t method_idx,
                                   NewLocalCallback new_local_callback,
                                   void* context) const {
  const uint8_t* const stream = GetDebugInfoStream(debug_info_offset);
  if (stream == nullptr) {
    return false;
  }
  std::vector<const char*> arg_descriptors;
  DexFileParameterIterator it(*this, GetMethodPrototype(GetMethodId(method_idx)));
  for (; it.HasNext(); it.Next()) {
    arg_descriptors.push_back(it.GetDescriptor());
  }
  return DecodeDebugLocalInfo(stream,
                              GetLocation(),
                              GetMethodDeclaringClassDescriptor(GetMethodId(method_idx)),
                              arg_descriptors,
                              this->PrettyMethod(method_idx),
                              is_static,
                              registers_size,
                              ins_size,
                              insns_size_in_code_units,
                              [this](uint32_t idx) {
                                return StringDataByIdx(dex::StringIndex(idx));
                              },
                              [this](uint32_t idx) {
                                return StringByTypeIdx(dex::TypeIndex(
                                    dchecked_integral_cast<uint16_t>(idx)));
                              },
                              new_local_callback,
                              context);
}

template<typename DexDebugNewPosition, typename IndexToStringData>
bool DexFile::DecodeDebugPositionInfo(const uint8_t* stream,
                                      IndexToStringData index_to_string_data,
                                      DexDebugNewPosition position_functor,
                                      void* context) {
  if (stream == nullptr) {
    return false;
  }

  PositionInfo entry = PositionInfo();
  entry.line_ = DecodeUnsignedLeb128(&stream);
  uint32_t parameters_size = DecodeUnsignedLeb128(&stream);
  for (uint32_t i = 0; i < parameters_size; ++i) {
    DecodeUnsignedLeb128P1(&stream);  // Parameter name.
  }

  for (;;)  {
    uint8_t opcode = *stream++;
    switch (opcode) {
      case DBG_END_SEQUENCE:
        return true;  // end of stream.
      case DBG_ADVANCE_PC:
        entry.address_ += DecodeUnsignedLeb128(&stream);
        break;
      case DBG_ADVANCE_LINE:
        entry.line_ += DecodeSignedLeb128(&stream);
        break;
      case DBG_START_LOCAL:
        DecodeUnsignedLeb128(&stream);  // reg.
        DecodeUnsignedLeb128P1(&stream);  // name.
        DecodeUnsignedLeb128P1(&stream);  // descriptor.
        break;
      case DBG_START_LOCAL_EXTENDED:
        DecodeUnsignedLeb128(&stream);  // reg.
        DecodeUnsignedLeb128P1(&stream);  // name.
        DecodeUnsignedLeb128P1(&stream);  // descriptor.
        DecodeUnsignedLeb128P1(&stream);  // signature.
        break;
      case DBG_END_LOCAL:
      case DBG_RESTART_LOCAL:
        DecodeUnsignedLeb128(&stream);  // reg.
        break;
      case DBG_SET_PROLOGUE_END:
        entry.prologue_end_ = true;
        break;
      case DBG_SET_EPILOGUE_BEGIN:
        entry.epilogue_begin_ = true;
        break;
      case DBG_SET_FILE: {
        uint32_t name_idx = DecodeUnsignedLeb128P1(&stream);
        entry.source_file_ = index_to_string_data(name_idx);
        break;
      }
      default: {
        int adjopcode = opcode - DBG_FIRST_SPECIAL;
        entry.address_ += adjopcode / DBG_LINE_RANGE;
        entry.line_ += DBG_LINE_BASE + (adjopcode % DBG_LINE_RANGE);
        if (position_functor(context, entry)) {
          return true;  // early exit.
        }
        entry.prologue_end_ = false;
        entry.epilogue_begin_ = false;
        break;
      }
    }
  }
}

template<typename DexDebugNewPosition>
bool DexFile::DecodeDebugPositionInfo(uint32_t debug_info_offset,
                                      DexDebugNewPosition position_functor,
                                      void* context) const {
  return DecodeDebugPositionInfo(GetDebugInfoStream(debug_info_offset),
                                 [this](uint32_t idx) {
                                   return StringDataByIdx(dex::StringIndex(idx));
                                 },
                                 position_functor,
                                 context);
}

inline const CompactDexFile* DexFile::AsCompactDexFile() const {
  DCHECK(IsCompactDexFile());
  return down_cast<const CompactDexFile*>(this);
}

inline const StandardDexFile* DexFile::AsStandardDexFile() const {
  DCHECK(IsStandardDexFile());
  return down_cast<const StandardDexFile*>(this);
}

// Get the base of the encoded data for the given DexCode.
inline const uint8_t* DexFile::GetCatchHandlerData(const DexInstructionIterator& code_item_end,
                                                   uint32_t tries_size,
                                                   uint32_t offset) {
  const uint8_t* handler_data =
      reinterpret_cast<const uint8_t*>(GetTryItems(code_item_end, tries_size));
  return handler_data + offset;
}

template <typename Visitor>
inline void DexFile::ClassDef::VisitMethods(const DexFile* dex_file, const Visitor& visitor) const {
  const uint8_t* class_data = dex_file->GetClassData(*this);
  if (class_data != nullptr) {
    ClassDataItemIterator it(*dex_file, class_data);
    it.SkipAllFields();
    for (; it.HasNext(); it.Next()) {
      visitor(it);
    }
  }
}

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_DEX_FILE_INL_H_
