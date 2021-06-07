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

#ifndef ART_LIBDEXFILE_DEX_DEX_FILE_H_
#define ART_LIBDEXFILE_DEX_DEX_FILE_H_

#include <memory>
#include <string>
#include <vector>

#include <android-base/logging.h>

#include "base/globals.h"
#include "base/iteration_range.h"
#include "base/macros.h"
#include "base/value_object.h"
#include "dex_file_types.h"
#include "dex_instruction_iterator.h"
#include "hidden_api_access_flags.h"
#include "jni.h"
#include "modifiers.h"

namespace art_lkchan {

class ClassDataItemIterator;
class CompactDexFile;
enum InvokeType : uint32_t;
class MemMap;
class OatDexFile;
class Signature;
class StandardDexFile;
class StringPiece;
class ZipArchive;

// Some instances of DexFile own the storage referred to by DexFile.  Clients who create
// such management do so by subclassing Container.
class DexFileContainer {
 public:
  DexFileContainer() { }
  virtual ~DexFileContainer() { }
  virtual int GetPermissions() = 0;
  virtual bool IsReadOnly() = 0;
  virtual bool EnableWrite() = 0;
  virtual bool DisableWrite() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(DexFileContainer);
};

// Dex file is the API that exposes native dex files (ordinary dex files) and CompactDex.
// Originally, the dex file format used by ART was mostly the same as APKs. The only change was
// quickened opcodes and layout optimizations.
// Since ART needs to support both native dex files and CompactDex files, the DexFile interface
// provides an abstraction to facilitate this.
class DexFile {
 public:
  // Number of bytes in the dex file magic.
  static constexpr size_t kDexMagicSize = 4;
  static constexpr size_t kDexVersionLen = 4;

  // First Dex format version enforcing class definition ordering rules.
  static const uint32_t kClassDefinitionOrderEnforcedVersion = 37;

  static constexpr size_t kSha1DigestSize = 20;
  static constexpr uint32_t kDexEndianConstant = 0x12345678;

  // The value of an invalid index.
  static const uint16_t kDexNoIndex16 = 0xFFFF;

  // Raw header_item.
  struct Header {
    uint8_t magic_[8] = {};
    uint32_t checksum_ = 0;  // See also location_checksum_
    uint8_t signature_[kSha1DigestSize] = {};
    uint32_t file_size_ = 0;  // size of entire file
    uint32_t header_size_ = 0;  // offset to start of next section
    uint32_t endian_tag_ = 0;
    uint32_t link_size_ = 0;  // unused
    uint32_t link_off_ = 0;  // unused
    uint32_t map_off_ = 0;  // unused
    uint32_t string_ids_size_ = 0;  // number of StringIds
    uint32_t string_ids_off_ = 0;  // file offset of StringIds array
    uint32_t type_ids_size_ = 0;  // number of TypeIds, we don't support more than 65535
    uint32_t type_ids_off_ = 0;  // file offset of TypeIds array
    uint32_t proto_ids_size_ = 0;  // number of ProtoIds, we don't support more than 65535
    uint32_t proto_ids_off_ = 0;  // file offset of ProtoIds array
    uint32_t field_ids_size_ = 0;  // number of FieldIds
    uint32_t field_ids_off_ = 0;  // file offset of FieldIds array
    uint32_t method_ids_size_ = 0;  // number of MethodIds
    uint32_t method_ids_off_ = 0;  // file offset of MethodIds array
    uint32_t class_defs_size_ = 0;  // number of ClassDefs
    uint32_t class_defs_off_ = 0;  // file offset of ClassDef array
    uint32_t data_size_ = 0;  // size of data section
    uint32_t data_off_ = 0;  // file offset of data section

    // Decode the dex magic version
    uint32_t GetVersion() const;
  };

  // Map item type codes.
  enum MapItemType : uint16_t {  // private
    kDexTypeHeaderItem               = 0x0000,
    kDexTypeStringIdItem             = 0x0001,
    kDexTypeTypeIdItem               = 0x0002,
    kDexTypeProtoIdItem              = 0x0003,
    kDexTypeFieldIdItem              = 0x0004,
    kDexTypeMethodIdItem             = 0x0005,
    kDexTypeClassDefItem             = 0x0006,
    kDexTypeCallSiteIdItem           = 0x0007,
    kDexTypeMethodHandleItem         = 0x0008,
    kDexTypeMapList                  = 0x1000,
    kDexTypeTypeList                 = 0x1001,
    kDexTypeAnnotationSetRefList     = 0x1002,
    kDexTypeAnnotationSetItem        = 0x1003,
    kDexTypeClassDataItem            = 0x2000,
    kDexTypeCodeItem                 = 0x2001,
    kDexTypeStringDataItem           = 0x2002,
    kDexTypeDebugInfoItem            = 0x2003,
    kDexTypeAnnotationItem           = 0x2004,
    kDexTypeEncodedArrayItem         = 0x2005,
    kDexTypeAnnotationsDirectoryItem = 0x2006,
  };

  struct MapItem {
    uint16_t type_;
    uint16_t unused_;
    uint32_t size_;
    uint32_t offset_;
  };

  struct MapList {
    uint32_t size_;
    MapItem list_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(MapList);
  };

  // Raw string_id_item.
  struct StringId {
    uint32_t string_data_off_;  // offset in bytes from the base address

   private:
    DISALLOW_COPY_AND_ASSIGN(StringId);
  };

  // Raw type_id_item.
  struct TypeId {
    dex::StringIndex descriptor_idx_;  // index into string_ids

   private:
    DISALLOW_COPY_AND_ASSIGN(TypeId);
  };

  // Raw field_id_item.
  struct FieldId {
    dex::TypeIndex class_idx_;   // index into type_ids_ array for defining class
    dex::TypeIndex type_idx_;    // index into type_ids_ array for field type
    dex::StringIndex name_idx_;  // index into string_ids_ array for field name

   private:
    DISALLOW_COPY_AND_ASSIGN(FieldId);
  };

  // Raw proto_id_item.
  struct ProtoId {
    dex::StringIndex shorty_idx_;     // index into string_ids array for shorty descriptor
    dex::TypeIndex return_type_idx_;  // index into type_ids array for return type
    uint16_t pad_;                    // padding = 0
    uint32_t parameters_off_;         // file offset to type_list for parameter types

   private:
    DISALLOW_COPY_AND_ASSIGN(ProtoId);
  };

  // Raw method_id_item.
  struct MethodId {
    dex::TypeIndex class_idx_;   // index into type_ids_ array for defining class
    uint16_t proto_idx_;         // index into proto_ids_ array for method prototype
    dex::StringIndex name_idx_;  // index into string_ids_ array for method name

   private:
    DISALLOW_COPY_AND_ASSIGN(MethodId);
  };

  // Base code_item, compact dex and standard dex have different code item layouts.
  struct CodeItem {
   protected:
    CodeItem() = default;

   private:
    DISALLOW_COPY_AND_ASSIGN(CodeItem);
  };

  // Raw class_def_item.
  struct ClassDef {
    dex::TypeIndex class_idx_;  // index into type_ids_ array for this class
    uint16_t pad1_;  // padding = 0
    uint32_t access_flags_;
    dex::TypeIndex superclass_idx_;  // index into type_ids_ array for superclass
    uint16_t pad2_;  // padding = 0
    uint32_t interfaces_off_;  // file offset to TypeList
    dex::StringIndex source_file_idx_;  // index into string_ids_ for source file name
    uint32_t annotations_off_;  // file offset to annotations_directory_item
    uint32_t class_data_off_;  // file offset to class_data_item
    uint32_t static_values_off_;  // file offset to EncodedArray

    // Returns the valid access flags, that is, Java modifier bits relevant to the ClassDef type
    // (class or interface). These are all in the lower 16b and do not contain runtime flags.
    uint32_t GetJavaAccessFlags() const {
      // Make sure that none of our runtime-only flags are set.
      static_assert((kAccValidClassFlags & kAccJavaFlagsMask) == kAccValidClassFlags,
                    "Valid class flags not a subset of Java flags");
      static_assert((kAccValidInterfaceFlags & kAccJavaFlagsMask) == kAccValidInterfaceFlags,
                    "Valid interface flags not a subset of Java flags");

      if ((access_flags_ & kAccInterface) != 0) {
        // Interface.
        return access_flags_ & kAccValidInterfaceFlags;
      } else {
        // Class.
        return access_flags_ & kAccValidClassFlags;
      }
    }

    template <typename Visitor>
    void VisitMethods(const DexFile* dex_file, const Visitor& visitor) const;

   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDef);
  };

  // Raw type_item.
  struct TypeItem {
    dex::TypeIndex type_idx_;  // index into type_ids section

   private:
    DISALLOW_COPY_AND_ASSIGN(TypeItem);
  };

  // Raw type_list.
  class TypeList {
   public:
    uint32_t Size() const {
      return size_;
    }

    const TypeItem& GetTypeItem(uint32_t idx) const {
      DCHECK_LT(idx, this->size_);
      return this->list_[idx];
    }

    // Size in bytes of the part of the list that is common.
    static constexpr size_t GetHeaderSize() {
      return 4U;
    }

    // Size in bytes of the whole type list including all the stored elements.
    static constexpr size_t GetListSize(size_t count) {
      return GetHeaderSize() + sizeof(TypeItem) * count;
    }

   private:
    uint32_t size_;  // size of the list, in entries
    TypeItem list_[1];  // elements of the list
    DISALLOW_COPY_AND_ASSIGN(TypeList);
  };

  // MethodHandle Types
  enum class MethodHandleType : uint16_t {  // private
    kStaticPut         = 0x0000,  // a setter for a given static field.
    kStaticGet         = 0x0001,  // a getter for a given static field.
    kInstancePut       = 0x0002,  // a setter for a given instance field.
    kInstanceGet       = 0x0003,  // a getter for a given instance field.
    kInvokeStatic      = 0x0004,  // an invoker for a given static method.
    kInvokeInstance    = 0x0005,  // invoke_instance : an invoker for a given instance method. This
                                  // can be any non-static method on any class (or interface) except
                                  // for “<init>”.
    kInvokeConstructor = 0x0006,  // an invoker for a given constructor.
    kInvokeDirect      = 0x0007,  // an invoker for a direct (special) method.
    kInvokeInterface   = 0x0008,  // an invoker for an interface method.
    kLast = kInvokeInterface
  };

  // raw method_handle_item
  struct MethodHandleItem {
    uint16_t method_handle_type_;
    uint16_t reserved1_;            // Reserved for future use.
    uint16_t field_or_method_idx_;  // Field index for accessors, method index otherwise.
    uint16_t reserved2_;            // Reserved for future use.
   private:
    DISALLOW_COPY_AND_ASSIGN(MethodHandleItem);
  };

  // raw call_site_id_item
  struct CallSiteIdItem {
    uint32_t data_off_;  // Offset into data section pointing to encoded array items.
   private:
    DISALLOW_COPY_AND_ASSIGN(CallSiteIdItem);
  };

  // Raw try_item.
  struct TryItem {
    static constexpr size_t kAlignment = sizeof(uint32_t);

    uint32_t start_addr_;
    uint16_t insn_count_;
    uint16_t handler_off_;

   private:
    TryItem() = default;
    friend class DexWriter;
    DISALLOW_COPY_AND_ASSIGN(TryItem);
  };

  // Annotation constants.
  enum {
    kDexVisibilityBuild         = 0x00,     /* annotation visibility */
    kDexVisibilityRuntime       = 0x01,
    kDexVisibilitySystem        = 0x02,

    kDexAnnotationByte          = 0x00,
    kDexAnnotationShort         = 0x02,
    kDexAnnotationChar          = 0x03,
    kDexAnnotationInt           = 0x04,
    kDexAnnotationLong          = 0x06,
    kDexAnnotationFloat         = 0x10,
    kDexAnnotationDouble        = 0x11,
    kDexAnnotationMethodType    = 0x15,
    kDexAnnotationMethodHandle  = 0x16,
    kDexAnnotationString        = 0x17,
    kDexAnnotationType          = 0x18,
    kDexAnnotationField         = 0x19,
    kDexAnnotationMethod        = 0x1a,
    kDexAnnotationEnum          = 0x1b,
    kDexAnnotationArray         = 0x1c,
    kDexAnnotationAnnotation    = 0x1d,
    kDexAnnotationNull          = 0x1e,
    kDexAnnotationBoolean       = 0x1f,

    kDexAnnotationValueTypeMask = 0x1f,     /* low 5 bits */
    kDexAnnotationValueArgShift = 5,
  };

  struct AnnotationsDirectoryItem {
    uint32_t class_annotations_off_;
    uint32_t fields_size_;
    uint32_t methods_size_;
    uint32_t parameters_size_;

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationsDirectoryItem);
  };

  struct FieldAnnotationsItem {
    uint32_t field_idx_;
    uint32_t annotations_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(FieldAnnotationsItem);
  };

  struct MethodAnnotationsItem {
    uint32_t method_idx_;
    uint32_t annotations_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(MethodAnnotationsItem);
  };

  struct ParameterAnnotationsItem {
    uint32_t method_idx_;
    uint32_t annotations_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(ParameterAnnotationsItem);
  };

  struct AnnotationSetRefItem {
    uint32_t annotations_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationSetRefItem);
  };

  struct AnnotationSetRefList {
    uint32_t size_;
    AnnotationSetRefItem list_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationSetRefList);
  };

  struct AnnotationSetItem {
    uint32_t size_;
    uint32_t entries_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationSetItem);
  };

  struct AnnotationItem {
    uint8_t visibility_;
    uint8_t annotation_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationItem);
  };

  enum AnnotationResultStyle {  // private
    kAllObjects,
    kPrimitivesOrObjects,
    kAllRaw
  };

  struct AnnotationValue;

  // Closes a .dex file.
  virtual ~DexFile();

  const std::string& GetLocation() const {
    return location_;
  }

  // For DexFiles directly from .dex files, this is the checksum from the DexFile::Header.
  // For DexFiles opened from a zip files, this will be the ZipEntry CRC32 of classes.dex.
  uint32_t GetLocationChecksum() const {
    return location_checksum_;
  }

  const Header& GetHeader() const {
    DCHECK(header_ != nullptr) << GetLocation();
    return *header_;
  }

  // Decode the dex magic version
  uint32_t GetDexVersion() const {
    return GetHeader().GetVersion();
  }

  // Returns true if the byte string points to the magic value.
  virtual bool IsMagicValid() const = 0;

  // Returns true if the byte string after the magic is the correct value.
  virtual bool IsVersionValid() const = 0;

  // Returns true if the dex file supports default methods.
  virtual bool SupportsDefaultMethods() const = 0;

  // Returns the maximum size in bytes needed to store an equivalent dex file strictly conforming to
  // the dex file specification. That is the size if we wanted to get rid of all the
  // quickening/compact-dexing/etc.
  //
  // TODO This should really be an exact size! b/72402467
  virtual size_t GetDequickenedSize() const = 0;

  // Returns the number of string identifiers in the .dex file.
  size_t NumStringIds() const {
    DCHECK(header_ != nullptr) << GetLocation();
    return header_->string_ids_size_;
  }

  // Returns the StringId at the specified index.
  const StringId& GetStringId(dex::StringIndex idx) const {
    DCHECK_LT(idx.index_, NumStringIds()) << GetLocation();
    return string_ids_[idx.index_];
  }

  dex::StringIndex GetIndexForStringId(const StringId& string_id) const {
    CHECK_GE(&string_id, string_ids_) << GetLocation();
    CHECK_LT(&string_id, string_ids_ + header_->string_ids_size_) << GetLocation();
    return dex::StringIndex(&string_id - string_ids_);
  }

  int32_t GetStringLength(const StringId& string_id) const;

  // Returns a pointer to the UTF-8 string data referred to by the given string_id as well as the
  // length of the string when decoded as a UTF-16 string. Note the UTF-16 length is not the same
  // as the string length of the string data.
  const char* GetStringDataAndUtf16Length(const StringId& string_id, uint32_t* utf16_length) const;

  const char* GetStringData(const StringId& string_id) const;

  // Index version of GetStringDataAndUtf16Length.
  const char* StringDataAndUtf16LengthByIdx(dex::StringIndex idx, uint32_t* utf16_length) const;

  const char* StringDataByIdx(dex::StringIndex idx) const;

  // Looks up a string id for a given modified utf8 string.
  const StringId* FindStringId(const char* string) const;

  const TypeId* FindTypeId(const char* string) const;

  // Looks up a string id for a given utf16 string.
  const StringId* FindStringId(const uint16_t* string, size_t length) const;

  // Returns the number of type identifiers in the .dex file.
  uint32_t NumTypeIds() const {
    DCHECK(header_ != nullptr) << GetLocation();
    return header_->type_ids_size_;
  }

  bool IsTypeIndexValid(dex::TypeIndex idx) const {
    return idx.IsValid() && idx.index_ < NumTypeIds();
  }

  // Returns the TypeId at the specified index.
  const TypeId& GetTypeId(dex::TypeIndex idx) const {
    DCHECK_LT(idx.index_, NumTypeIds()) << GetLocation();
    return type_ids_[idx.index_];
  }

  dex::TypeIndex GetIndexForTypeId(const TypeId& type_id) const {
    CHECK_GE(&type_id, type_ids_) << GetLocation();
    CHECK_LT(&type_id, type_ids_ + header_->type_ids_size_) << GetLocation();
    size_t result = &type_id - type_ids_;
    DCHECK_LT(result, 65536U) << GetLocation();
    return dex::TypeIndex(static_cast<uint16_t>(result));
  }

  // Get the descriptor string associated with a given type index.
  const char* StringByTypeIdx(dex::TypeIndex idx, uint32_t* unicode_length) const;

  const char* StringByTypeIdx(dex::TypeIndex idx) const;

  // Returns the type descriptor string of a type id.
  const char* GetTypeDescriptor(const TypeId& type_id) const;

  // Looks up a type for the given string index
  const TypeId* FindTypeId(dex::StringIndex string_idx) const;

  // Returns the number of field identifiers in the .dex file.
  size_t NumFieldIds() const {
    DCHECK(header_ != nullptr) << GetLocation();
    return header_->field_ids_size_;
  }

  // Returns the FieldId at the specified index.
  const FieldId& GetFieldId(uint32_t idx) const {
    DCHECK_LT(idx, NumFieldIds()) << GetLocation();
    return field_ids_[idx];
  }

  uint32_t GetIndexForFieldId(const FieldId& field_id) const {
    CHECK_GE(&field_id, field_ids_) << GetLocation();
    CHECK_LT(&field_id, field_ids_ + header_->field_ids_size_) << GetLocation();
    return &field_id - field_ids_;
  }

  // Looks up a field by its declaring class, name and type
  const FieldId* FindFieldId(const DexFile::TypeId& declaring_klass,
                             const DexFile::StringId& name,
                             const DexFile::TypeId& type) const;

  uint32_t FindCodeItemOffset(const DexFile::ClassDef& class_def,
                              uint32_t dex_method_idx) const;

  virtual uint32_t GetCodeItemSize(const DexFile::CodeItem& disk_code_item) const = 0;

  // Returns the declaring class descriptor string of a field id.
  const char* GetFieldDeclaringClassDescriptor(const FieldId& field_id) const {
    const DexFile::TypeId& type_id = GetTypeId(field_id.class_idx_);
    return GetTypeDescriptor(type_id);
  }

  // Returns the class descriptor string of a field id.
  const char* GetFieldTypeDescriptor(const FieldId& field_id) const;

  // Returns the name of a field id.
  const char* GetFieldName(const FieldId& field_id) const;

  // Returns the number of method identifiers in the .dex file.
  size_t NumMethodIds() const {
    DCHECK(header_ != nullptr) << GetLocation();
    return header_->method_ids_size_;
  }

  // Returns the MethodId at the specified index.
  const MethodId& GetMethodId(uint32_t idx) const {
    DCHECK_LT(idx, NumMethodIds()) << GetLocation();
    return method_ids_[idx];
  }

  uint32_t GetIndexForMethodId(const MethodId& method_id) const {
    CHECK_GE(&method_id, method_ids_) << GetLocation();
    CHECK_LT(&method_id, method_ids_ + header_->method_ids_size_) << GetLocation();
    return &method_id - method_ids_;
  }

  // Looks up a method by its declaring class, name and proto_id
  const MethodId* FindMethodId(const DexFile::TypeId& declaring_klass,
                               const DexFile::StringId& name,
                               const DexFile::ProtoId& signature) const;

  // Returns the declaring class descriptor string of a method id.
  const char* GetMethodDeclaringClassDescriptor(const MethodId& method_id) const;

  // Returns the prototype of a method id.
  const ProtoId& GetMethodPrototype(const MethodId& method_id) const {
    return GetProtoId(method_id.proto_idx_);
  }

  // Returns a representation of the signature of a method id.
  const Signature GetMethodSignature(const MethodId& method_id) const;

  // Returns a representation of the signature of a proto id.
  const Signature GetProtoSignature(const ProtoId& proto_id) const;

  // Returns the name of a method id.
  const char* GetMethodName(const MethodId& method_id) const;

  // Returns the shorty of a method by its index.
  const char* GetMethodShorty(uint32_t idx) const;

  // Returns the shorty of a method id.
  const char* GetMethodShorty(const MethodId& method_id) const;
  const char* GetMethodShorty(const MethodId& method_id, uint32_t* length) const;

  // Returns the number of class definitions in the .dex file.
  uint32_t NumClassDefs() const {
    DCHECK(header_ != nullptr) << GetLocation();
    return header_->class_defs_size_;
  }

  // Returns the ClassDef at the specified index.
  const ClassDef& GetClassDef(uint16_t idx) const {
    DCHECK_LT(idx, NumClassDefs()) << GetLocation();
    return class_defs_[idx];
  }

  uint16_t GetIndexForClassDef(const ClassDef& class_def) const {
    CHECK_GE(&class_def, class_defs_) << GetLocation();
    CHECK_LT(&class_def, class_defs_ + header_->class_defs_size_) << GetLocation();
    return &class_def - class_defs_;
  }

  // Returns the class descriptor string of a class definition.
  const char* GetClassDescriptor(const ClassDef& class_def) const;

  // Looks up a class definition by its type index.
  const ClassDef* FindClassDef(dex::TypeIndex type_idx) const;

  const TypeList* GetInterfacesList(const ClassDef& class_def) const {
    return DataPointer<TypeList>(class_def.interfaces_off_);
  }

  uint32_t NumMethodHandles() const {
    return num_method_handles_;
  }

  const MethodHandleItem& GetMethodHandle(uint32_t idx) const {
    CHECK_LT(idx, NumMethodHandles());
    return method_handles_[idx];
  }

  uint32_t NumCallSiteIds() const {
    return num_call_site_ids_;
  }

  const CallSiteIdItem& GetCallSiteId(uint32_t idx) const {
    CHECK_LT(idx, NumCallSiteIds());
    return call_site_ids_[idx];
  }

  // Returns a pointer to the raw memory mapped class_data_item
  const uint8_t* GetClassData(const ClassDef& class_def) const {
    return DataPointer<uint8_t>(class_def.class_data_off_);
  }

  // Return the code item for a provided offset.
  const CodeItem* GetCodeItem(const uint32_t code_off) const {
    // May be null for native or abstract methods.
    return DataPointer<CodeItem>(code_off);
  }

  const char* GetReturnTypeDescriptor(const ProtoId& proto_id) const;

  // Returns the number of prototype identifiers in the .dex file.
  size_t NumProtoIds() const {
    DCHECK(header_ != nullptr) << GetLocation();
    return header_->proto_ids_size_;
  }

  // Returns the ProtoId at the specified index.
  const ProtoId& GetProtoId(uint16_t idx) const {
    DCHECK_LT(idx, NumProtoIds()) << GetLocation();
    return proto_ids_[idx];
  }

  uint16_t GetIndexForProtoId(const ProtoId& proto_id) const {
    CHECK_GE(&proto_id, proto_ids_) << GetLocation();
    CHECK_LT(&proto_id, proto_ids_ + header_->proto_ids_size_) << GetLocation();
    return &proto_id - proto_ids_;
  }

  // Looks up a proto id for a given return type and signature type list
  const ProtoId* FindProtoId(dex::TypeIndex return_type_idx,
                             const dex::TypeIndex* signature_type_idxs,
                             uint32_t signature_length) const;
  const ProtoId* FindProtoId(dex::TypeIndex return_type_idx,
                             const std::vector<dex::TypeIndex>& signature_type_idxs) const {
    return FindProtoId(return_type_idx, &signature_type_idxs[0], signature_type_idxs.size());
  }

  // Given a signature place the type ids into the given vector, returns true on success
  bool CreateTypeList(const StringPiece& signature,
                      dex::TypeIndex* return_type_idx,
                      std::vector<dex::TypeIndex>* param_type_idxs) const;

  // Create a Signature from the given string signature or return Signature::NoSignature if not
  // possible.
  const Signature CreateSignature(const StringPiece& signature) const;

  // Returns the short form method descriptor for the given prototype.
  const char* GetShorty(uint32_t proto_idx) const;

  const TypeList* GetProtoParameters(const ProtoId& proto_id) const {
    return DataPointer<TypeList>(proto_id.parameters_off_);
  }

  const uint8_t* GetEncodedStaticFieldValuesArray(const ClassDef& class_def) const {
    return DataPointer<uint8_t>(class_def.static_values_off_);
  }

  const uint8_t* GetCallSiteEncodedValuesArray(const CallSiteIdItem& call_site_id) const {
    return DataBegin() + call_site_id.data_off_;
  }

  static const TryItem* GetTryItems(const DexInstructionIterator& code_item_end, uint32_t offset);

  // Get the base of the encoded data for the given DexCode.
  static const uint8_t* GetCatchHandlerData(const DexInstructionIterator& code_item_end,
                                            uint32_t tries_size,
                                            uint32_t offset);

  // Find which try region is associated with the given address (ie dex pc). Returns -1 if none.
  static int32_t FindTryItem(const TryItem* try_items, uint32_t tries_size, uint32_t address);

  // Get the pointer to the start of the debugging data
  const uint8_t* GetDebugInfoStream(uint32_t debug_info_off) const {
    // Check that the offset is in bounds.
    // Note that although the specification says that 0 should be used if there
    // is no debug information, some applications incorrectly use 0xFFFFFFFF.
    return (debug_info_off == 0 || debug_info_off >= data_size_)
        ? nullptr
        : DataBegin() + debug_info_off;
  }

  struct PositionInfo {
    PositionInfo() = default;

    uint32_t address_ = 0;  // In 16-bit code units.
    uint32_t line_ = 0;  // Source code line number starting at 1.
    const char* source_file_ = nullptr;  // nullptr if the file from ClassDef still applies.
    bool prologue_end_ = false;
    bool epilogue_begin_ = false;
  };

  struct LocalInfo {
    LocalInfo() = default;

    const char* name_ = nullptr;  // E.g., list.  It can be nullptr if unknown.
    const char* descriptor_ = nullptr;  // E.g., Ljava/util/LinkedList;
    const char* signature_ = nullptr;  // E.g., java.util.LinkedList<java.lang.Integer>
    uint32_t start_address_ = 0;  // PC location where the local is first defined.
    uint32_t end_address_ = 0;  // PC location where the local is no longer defined.
    uint16_t reg_ = 0;  // Dex register which stores the values.
    bool is_live_ = false;  // Is the local defined and live.
  };

  // Callback for "new locals table entry".
  typedef void (*DexDebugNewLocalCb)(void* context, const LocalInfo& entry);

  static bool LineNumForPcCb(void* context, const PositionInfo& entry);

  const AnnotationsDirectoryItem* GetAnnotationsDirectory(const ClassDef& class_def) const {
    return DataPointer<AnnotationsDirectoryItem>(class_def.annotations_off_);
  }

  const AnnotationSetItem* GetClassAnnotationSet(const AnnotationsDirectoryItem* anno_dir) const {
    return DataPointer<AnnotationSetItem>(anno_dir->class_annotations_off_);
  }

  const FieldAnnotationsItem* GetFieldAnnotations(const AnnotationsDirectoryItem* anno_dir) const {
    return (anno_dir->fields_size_ == 0)
         ? nullptr
         : reinterpret_cast<const FieldAnnotationsItem*>(&anno_dir[1]);
  }

  const MethodAnnotationsItem* GetMethodAnnotations(const AnnotationsDirectoryItem* anno_dir)
      const {
    if (anno_dir->methods_size_ == 0) {
      return nullptr;
    }
    // Skip past the header and field annotations.
    const uint8_t* addr = reinterpret_cast<const uint8_t*>(&anno_dir[1]);
    addr += anno_dir->fields_size_ * sizeof(FieldAnnotationsItem);
    return reinterpret_cast<const MethodAnnotationsItem*>(addr);
  }

  const ParameterAnnotationsItem* GetParameterAnnotations(const AnnotationsDirectoryItem* anno_dir)
      const {
    if (anno_dir->parameters_size_ == 0) {
      return nullptr;
    }
    // Skip past the header, field annotations, and method annotations.
    const uint8_t* addr = reinterpret_cast<const uint8_t*>(&anno_dir[1]);
    addr += anno_dir->fields_size_ * sizeof(FieldAnnotationsItem);
    addr += anno_dir->methods_size_ * sizeof(MethodAnnotationsItem);
    return reinterpret_cast<const ParameterAnnotationsItem*>(addr);
  }

  const AnnotationSetItem* GetFieldAnnotationSetItem(const FieldAnnotationsItem& anno_item) const {
    return DataPointer<AnnotationSetItem>(anno_item.annotations_off_);
  }

  const AnnotationSetItem* GetMethodAnnotationSetItem(const MethodAnnotationsItem& anno_item)
      const {
    return DataPointer<AnnotationSetItem>(anno_item.annotations_off_);
  }

  const AnnotationSetRefList* GetParameterAnnotationSetRefList(
      const ParameterAnnotationsItem* anno_item) const {
    return DataPointer<AnnotationSetRefList>(anno_item->annotations_off_);
  }

  ALWAYS_INLINE const AnnotationItem* GetAnnotationItemAtOffset(uint32_t offset) const {
    return DataPointer<AnnotationItem>(offset);
  }

  const AnnotationItem* GetAnnotationItem(const AnnotationSetItem* set_item, uint32_t index) const {
    DCHECK_LE(index, set_item->size_);
    return GetAnnotationItemAtOffset(set_item->entries_[index]);
  }

  const AnnotationSetItem* GetSetRefItemItem(const AnnotationSetRefItem* anno_item) const {
    return DataPointer<AnnotationSetItem>(anno_item->annotations_off_);
  }

  // Debug info opcodes and constants
  enum {
    DBG_END_SEQUENCE         = 0x00,
    DBG_ADVANCE_PC           = 0x01,
    DBG_ADVANCE_LINE         = 0x02,
    DBG_START_LOCAL          = 0x03,
    DBG_START_LOCAL_EXTENDED = 0x04,
    DBG_END_LOCAL            = 0x05,
    DBG_RESTART_LOCAL        = 0x06,
    DBG_SET_PROLOGUE_END     = 0x07,
    DBG_SET_EPILOGUE_BEGIN   = 0x08,
    DBG_SET_FILE             = 0x09,
    DBG_FIRST_SPECIAL        = 0x0a,
    DBG_LINE_BASE            = -4,
    DBG_LINE_RANGE           = 15,
  };

  struct LineNumFromPcContext {
    LineNumFromPcContext(uint32_t address, uint32_t line_num)
        : address_(address), line_num_(line_num) {}
    uint32_t address_;
    uint32_t line_num_;
   private:
    DISALLOW_COPY_AND_ASSIGN(LineNumFromPcContext);
  };

  // Returns false if there is no debugging information or if it cannot be decoded.
  template<typename NewLocalCallback, typename IndexToStringData, typename TypeIndexToStringData>
  static bool DecodeDebugLocalInfo(const uint8_t* stream,
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
                                   NewLocalCallback new_local,
                                   void* context);
  template<typename NewLocalCallback>
  bool DecodeDebugLocalInfo(uint32_t registers_size,
                            uint32_t ins_size,
                            uint32_t insns_size_in_code_units,
                            uint32_t debug_info_offset,
                            bool is_static,
                            uint32_t method_idx,
                            NewLocalCallback new_local,
                            void* context) const;

  // Returns false if there is no debugging information or if it cannot be decoded.
  template<typename DexDebugNewPosition, typename IndexToStringData>
  static bool DecodeDebugPositionInfo(const uint8_t* stream,
                                      IndexToStringData index_to_string_data,
                                      DexDebugNewPosition position_functor,
                                      void* context);
  template<typename DexDebugNewPosition>
  bool DecodeDebugPositionInfo(uint32_t debug_info_offset,
                               DexDebugNewPosition position_functor,
                               void* context) const;

  const char* GetSourceFile(const ClassDef& class_def) const {
    if (!class_def.source_file_idx_.IsValid()) {
      return nullptr;
    } else {
      return StringDataByIdx(class_def.source_file_idx_);
    }
  }

  int GetPermissions() const;

  bool IsReadOnly() const;

  bool EnableWrite() const;

  bool DisableWrite() const;

  const uint8_t* Begin() const {
    return begin_;
  }

  size_t Size() const {
    return size_;
  }

  const uint8_t* DataBegin() const {
    return data_begin_;
  }

  size_t DataSize() const {
    return data_size_;
  }

  template <typename T>
  const T* DataPointer(size_t offset) const {
    // DCHECK_LT(offset, DataSize()) << "Offset past end of data section";
    return (offset != 0u) ? reinterpret_cast<const T*>(DataBegin() + offset) : nullptr;
  }

  const OatDexFile* GetOatDexFile() const {
    return oat_dex_file_;
  }

  // Used by oat writer.
  void SetOatDexFile(OatDexFile* oat_dex_file) const {
    oat_dex_file_ = oat_dex_file;
  }

  // Read MapItems and validate/set remaining offsets.
  const DexFile::MapList* GetMapList() const {
    return reinterpret_cast<const DexFile::MapList*>(DataBegin() + header_->map_off_);
  }

  // Utility methods for reading integral values from a buffer.
  static int32_t ReadSignedInt(const uint8_t* ptr, int zwidth);
  static uint32_t ReadUnsignedInt(const uint8_t* ptr, int zwidth, bool fill_on_right);
  static int64_t ReadSignedLong(const uint8_t* ptr, int zwidth);
  static uint64_t ReadUnsignedLong(const uint8_t* ptr, int zwidth, bool fill_on_right);

  // Recalculates the checksum of the dex file. Does not use the current value in the header.
  virtual uint32_t CalculateChecksum() const;
  static uint32_t CalculateChecksum(const uint8_t* begin, size_t size);
  static uint32_t ChecksumMemoryRange(const uint8_t* begin, size_t size);

  // Returns a human-readable form of the method at an index.
  std::string PrettyMethod(uint32_t method_idx, bool with_signature = true) const;
  // Returns a human-readable form of the field at an index.
  std::string PrettyField(uint32_t field_idx, bool with_type = true) const;
  // Returns a human-readable form of the type at an index.
  std::string PrettyType(dex::TypeIndex type_idx) const;

  // Not virtual for performance reasons.
  ALWAYS_INLINE bool IsCompactDexFile() const {
    return is_compact_dex_;
  }
  ALWAYS_INLINE bool IsStandardDexFile() const {
    return !is_compact_dex_;
  }
  ALWAYS_INLINE const StandardDexFile* AsStandardDexFile() const;
  ALWAYS_INLINE const CompactDexFile* AsCompactDexFile() const;

  ALWAYS_INLINE bool IsPlatformDexFile() const {
    return is_platform_dex_;
  }

  ALWAYS_INLINE void SetIsPlatformDexFile() {
    is_platform_dex_ = true;
  }

  bool IsInMainSection(const void* addr) const {
    return Begin() <= addr && addr < Begin() + Size();
  }

  bool IsInDataSection(const void* addr) const {
    return DataBegin() <= addr && addr < DataBegin() + DataSize();
  }

  DexFileContainer* GetContainer() const {
    return container_.get();
  }

  // Changes the dex file pointed to by class_it to not have any hiddenapi flags.
  static void UnHideAccessFlags(ClassDataItemIterator& class_it);

 protected:
  // First Dex format version supporting default methods.
  static const uint32_t kDefaultMethodsVersion = 37;

  DexFile(const uint8_t* base,
          size_t size,
          const uint8_t* data_begin,
          size_t data_size,
          const std::string& location,
          uint32_t location_checksum,
          const OatDexFile* oat_dex_file,
          std::unique_ptr<DexFileContainer> container,
          bool is_compact_dex);

  // Top-level initializer that calls other Init methods.
  bool Init(std::string* error_msg);

  // Returns true if the header magic and version numbers are of the expected values.
  bool CheckMagicAndVersion(std::string* error_msg) const;

  // Initialize section info for sections only found in map. Returns true on success.
  void InitializeSectionsFromMapList();

  // The base address of the memory mapping.
  const uint8_t* const begin_;

  // The size of the underlying memory allocation in bytes.
  const size_t size_;

  // The base address of the data section (same as Begin() for standard dex).
  const uint8_t* const data_begin_;

  // The size of the data section.
  const size_t data_size_;

  // Typically the dex file name when available, alternatively some identifying string.
  //
  // The ClassLinker will use this to match DexFiles the boot class
  // path to DexCache::GetLocation when loading from an image.
  const std::string location_;

  const uint32_t location_checksum_;

  // Points to the header section.
  const Header* const header_;

  // Points to the base of the string identifier list.
  const StringId* const string_ids_;

  // Points to the base of the type identifier list.
  const TypeId* const type_ids_;

  // Points to the base of the field identifier list.
  const FieldId* const field_ids_;

  // Points to the base of the method identifier list.
  const MethodId* const method_ids_;

  // Points to the base of the prototype identifier list.
  const ProtoId* const proto_ids_;

  // Points to the base of the class definition list.
  const ClassDef* const class_defs_;

  // Points to the base of the method handles list.
  const MethodHandleItem* method_handles_;

  // Number of elements in the method handles list.
  size_t num_method_handles_;

  // Points to the base of the call sites id list.
  const CallSiteIdItem* call_site_ids_;

  // Number of elements in the call sites list.
  size_t num_call_site_ids_;

  // If this dex file was loaded from an oat file, oat_dex_file_ contains a
  // pointer to the OatDexFile it was loaded from. Otherwise oat_dex_file_ is
  // null.
  mutable const OatDexFile* oat_dex_file_;

  // Manages the underlying memory allocation.
  std::unique_ptr<DexFileContainer> container_;

  // If the dex file is a compact dex file. If false then the dex file is a standard dex file.
  const bool is_compact_dex_;

  // If the dex file is located in /system/framework/.
  bool is_platform_dex_;

  friend class DexFileLoader;
  friend class DexFileVerifierTest;
  friend class OatWriter;
};

std::ostream& operator<<(std::ostream& os, const DexFile& dex_file);

// Iterate over a dex file's ProtoId's paramters
class DexFileParameterIterator {
 public:
  DexFileParameterIterator(const DexFile& dex_file, const DexFile::ProtoId& proto_id)
      : dex_file_(dex_file) {
    type_list_ = dex_file_.GetProtoParameters(proto_id);
    if (type_list_ != nullptr) {
      size_ = type_list_->Size();
    }
  }
  bool HasNext() const { return pos_ < size_; }
  size_t Size() const { return size_; }
  void Next() { ++pos_; }
  dex::TypeIndex GetTypeIdx() {
    return type_list_->GetTypeItem(pos_).type_idx_;
  }
  const char* GetDescriptor() {
    return dex_file_.StringByTypeIdx(dex::TypeIndex(GetTypeIdx()));
  }
 private:
  const DexFile& dex_file_;
  const DexFile::TypeList* type_list_ = nullptr;
  uint32_t size_ = 0;
  uint32_t pos_ = 0;
  DISALLOW_IMPLICIT_CONSTRUCTORS(DexFileParameterIterator);
};

// Abstract the signature of a method.
class Signature : public ValueObject {
 public:
  std::string ToString() const;

  static Signature NoSignature() {
    return Signature();
  }

  bool IsVoid() const;
  uint32_t GetNumberOfParameters() const;

  bool operator==(const Signature& rhs) const;
  bool operator!=(const Signature& rhs) const {
    return !(*this == rhs);
  }

  bool operator==(const StringPiece& rhs) const;

 private:
  Signature(const DexFile* dex, const DexFile::ProtoId& proto) : dex_file_(dex), proto_id_(&proto) {
  }

  Signature() = default;

  friend class DexFile;

  const DexFile* const dex_file_ = nullptr;
  const DexFile::ProtoId* const proto_id_ = nullptr;
};
std::ostream& operator<<(std::ostream& os, const Signature& sig);

// Iterate and decode class_data_item
class ClassDataItemIterator {
 public:
  ClassDataItemIterator(const DexFile& dex_file, const uint8_t* raw_class_data_item)
      : dex_file_(dex_file), pos_(0), ptr_pos_(raw_class_data_item), last_idx_(0) {
    ReadClassDataHeader();
    if (EndOfInstanceFieldsPos() > 0) {
      ReadClassDataField();
    } else if (EndOfVirtualMethodsPos() > 0) {
      ReadClassDataMethod();
    }
  }
  uint32_t NumStaticFields() const {
    return header_.static_fields_size_;
  }
  uint32_t NumInstanceFields() const {
    return header_.instance_fields_size_;
  }
  uint32_t NumDirectMethods() const {
    return header_.direct_methods_size_;
  }
  uint32_t NumVirtualMethods() const {
    return header_.virtual_methods_size_;
  }
  bool IsAtMethod() const {
    return pos_ >= EndOfInstanceFieldsPos();
  }
  bool HasNextStaticField() const {
    return pos_ < EndOfStaticFieldsPos();
  }
  bool HasNextInstanceField() const {
    return pos_ >= EndOfStaticFieldsPos() && pos_ < EndOfInstanceFieldsPos();
  }
  bool HasNextDirectMethod() const {
    return pos_ >= EndOfInstanceFieldsPos() && pos_ < EndOfDirectMethodsPos();
  }
  bool HasNextVirtualMethod() const {
    return pos_ >= EndOfDirectMethodsPos() && pos_ < EndOfVirtualMethodsPos();
  }
  bool HasNextMethod() const {
    const bool result = pos_ >= EndOfInstanceFieldsPos() && pos_ < EndOfVirtualMethodsPos();
    DCHECK_EQ(result, HasNextDirectMethod() || HasNextVirtualMethod());
    return result;
  }
  void SkipStaticFields() {
    while (HasNextStaticField()) {
      Next();
    }
  }
  void SkipInstanceFields() {
    while (HasNextInstanceField()) {
      Next();
    }
  }
  void SkipAllFields() {
    SkipStaticFields();
    SkipInstanceFields();
  }
  void SkipDirectMethods() {
    while (HasNextDirectMethod()) {
      Next();
    }
  }
  void SkipVirtualMethods() {
    while (HasNextVirtualMethod()) {
      Next();
    }
  }
  bool HasNext() const {
    return pos_ < EndOfVirtualMethodsPos();
  }
  inline void Next() {
    pos_++;
    if (pos_ < EndOfStaticFieldsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataField();
    } else if (pos_ == EndOfStaticFieldsPos() && NumInstanceFields() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataField();
    } else if (pos_ < EndOfInstanceFieldsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataField();
    } else if (pos_ == EndOfInstanceFieldsPos() && NumDirectMethods() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataMethod();
    } else if (pos_ < EndOfDirectMethodsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataMethod();
    } else if (pos_ == EndOfDirectMethodsPos() && NumVirtualMethods() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataMethod();
    } else if (pos_ < EndOfVirtualMethodsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataMethod();
    } else {
      DCHECK(!HasNext());
    }
  }
  uint32_t GetMemberIndex() const {
    if (pos_ < EndOfInstanceFieldsPos()) {
      return last_idx_ + field_.field_idx_delta_;
    } else {
      DCHECK_LT(pos_, EndOfVirtualMethodsPos());
      return last_idx_ + method_.method_idx_delta_;
    }
  }
  uint32_t GetRawMemberAccessFlags() const {
    if (pos_ < EndOfInstanceFieldsPos()) {
      return field_.access_flags_;
    } else {
      DCHECK_LT(pos_, EndOfVirtualMethodsPos());
      return method_.access_flags_;
    }
  }
  uint32_t GetFieldAccessFlags() const {
    return GetMemberAccessFlags() & kAccValidFieldFlags;
  }
  uint32_t GetMethodAccessFlags() const {
    return GetMemberAccessFlags() & kAccValidMethodFlags;
  }
  uint32_t GetMemberAccessFlags() const {
    return HiddenApiAccessFlags::RemoveFromDex(GetRawMemberAccessFlags());
  }
  HiddenApiAccessFlags::ApiList DecodeHiddenAccessFlags() const {
    return HiddenApiAccessFlags::DecodeFromDex(GetRawMemberAccessFlags());
  }
  bool MemberIsNative() const {
    return GetRawMemberAccessFlags() & kAccNative;
  }
  bool MemberIsFinal() const {
    return GetRawMemberAccessFlags() & kAccFinal;
  }
  ALWAYS_INLINE InvokeType GetMethodInvokeType(const DexFile::ClassDef& class_def) const;
  const DexFile::CodeItem* GetMethodCodeItem() const {
    return dex_file_.GetCodeItem(method_.code_off_);
  }
  uint32_t GetMethodCodeItemOffset() const {
    return method_.code_off_;
  }
  const uint8_t* DataPointer() const {
    return ptr_pos_;
  }
  const uint8_t* EndDataPointer() const {
    CHECK(!HasNext());
    return ptr_pos_;
  }

 private:
  // A dex file's class_data_item is leb128 encoded, this structure holds a decoded form of the
  // header for a class_data_item
  struct ClassDataHeader {
    uint32_t static_fields_size_;  // the number of static fields
    uint32_t instance_fields_size_;  // the number of instance fields
    uint32_t direct_methods_size_;  // the number of direct methods
    uint32_t virtual_methods_size_;  // the number of virtual methods
  } header_;

  // Read and decode header from a class_data_item stream into header
  void ReadClassDataHeader();

  uint32_t EndOfStaticFieldsPos() const {
    return header_.static_fields_size_;
  }
  uint32_t EndOfInstanceFieldsPos() const {
    return EndOfStaticFieldsPos() + header_.instance_fields_size_;
  }
  uint32_t EndOfDirectMethodsPos() const {
    return EndOfInstanceFieldsPos() + header_.direct_methods_size_;
  }
  uint32_t EndOfVirtualMethodsPos() const {
    return EndOfDirectMethodsPos() + header_.virtual_methods_size_;
  }

  // A decoded version of the field of a class_data_item
  struct ClassDataField {
    uint32_t field_idx_delta_;  // delta of index into the field_ids array for FieldId
    uint32_t access_flags_;  // access flags for the field
    ClassDataField() :  field_idx_delta_(0), access_flags_(0) {}

   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDataField);
  };
  ClassDataField field_;

  // Read and decode a field from a class_data_item stream into field
  void ReadClassDataField();

  // A decoded version of the method of a class_data_item
  struct ClassDataMethod {
    uint32_t method_idx_delta_;  // delta of index into the method_ids array for MethodId
    uint32_t access_flags_;
    uint32_t code_off_;
    ClassDataMethod() : method_idx_delta_(0), access_flags_(0), code_off_(0) {}

   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDataMethod);
  };
  ClassDataMethod method_;

  // Read and decode a method from a class_data_item stream into method
  void ReadClassDataMethod();

  const DexFile& dex_file_;
  size_t pos_;  // integral number of items passed
  const uint8_t* ptr_pos_;  // pointer into stream of class_data_item
  uint32_t last_idx_;  // last read field or method index to apply delta to
  DISALLOW_IMPLICIT_CONSTRUCTORS(ClassDataItemIterator);
};

class EncodedArrayValueIterator {
 public:
  EncodedArrayValueIterator(const DexFile& dex_file, const uint8_t* array_data);

  bool HasNext() const { return pos_ < array_size_; }

  void Next();

  enum ValueType {
    kByte         = 0x00,
    kShort        = 0x02,
    kChar         = 0x03,
    kInt          = 0x04,
    kLong         = 0x06,
    kFloat        = 0x10,
    kDouble       = 0x11,
    kMethodType   = 0x15,
    kMethodHandle = 0x16,
    kString       = 0x17,
    kType         = 0x18,
    kField        = 0x19,
    kMethod       = 0x1a,
    kEnum         = 0x1b,
    kArray        = 0x1c,
    kAnnotation   = 0x1d,
    kNull         = 0x1e,
    kBoolean      = 0x1f,
  };

  ValueType GetValueType() const { return type_; }
  const jvalue& GetJavaValue() const { return jval_; }

 protected:
  static constexpr uint8_t kEncodedValueTypeMask = 0x1f;  // 0b11111
  static constexpr uint8_t kEncodedValueArgShift = 5;

  const DexFile& dex_file_;
  size_t array_size_;  // Size of array.
  size_t pos_;  // Current position.
  const uint8_t* ptr_;  // Pointer into encoded data array.
  ValueType type_;  // Type of current encoded value.
  jvalue jval_;  // Value of current encoded value.

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(EncodedArrayValueIterator);
};
std::ostream& operator<<(std::ostream& os, const EncodedArrayValueIterator::ValueType& code);

class EncodedStaticFieldValueIterator : public EncodedArrayValueIterator {
 public:
  EncodedStaticFieldValueIterator(const DexFile& dex_file,
                                  const DexFile::ClassDef& class_def)
      : EncodedArrayValueIterator(dex_file,
                                  dex_file.GetEncodedStaticFieldValuesArray(class_def))
  {}

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(EncodedStaticFieldValueIterator);
};
std::ostream& operator<<(std::ostream& os, const EncodedStaticFieldValueIterator::ValueType& code);

class CallSiteArrayValueIterator : public EncodedArrayValueIterator {
 public:
  CallSiteArrayValueIterator(const DexFile& dex_file,
                             const DexFile::CallSiteIdItem& call_site_id)
      : EncodedArrayValueIterator(dex_file,
                                  dex_file.GetCallSiteEncodedValuesArray(call_site_id))
  {}

  uint32_t Size() const { return array_size_; }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CallSiteArrayValueIterator);
};
std::ostream& operator<<(std::ostream& os, const CallSiteArrayValueIterator::ValueType& code);

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_DEX_FILE_H_
