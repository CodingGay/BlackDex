#ifndef CORE_ASSEMBLER_H
#define CORE_ASSEMBLER_H

#include "MemoryAllocator/CodeBuffer/CodeBufferBase.h"

class CodeBuffer;

namespace zz {

class Label {
public:
  Label() : pos_(0), near_link_pos_(0) {
  }

public:
  bool is_bound() const;

  bool is_unused() const;

  bool is_linked() const;

  bool is_near_linked() const;

  int pos() const;

  void bind_to(int pos);

  void link_to(int pos);

private:
  // pos_: "< 0", indicate the Label is Binded, "> 0", indicate the Label is Linked, "= 0" indicate the Label is
  // iter-terminal or unused
  int pos_;
  int near_link_pos_;
};

class ExternalReference {
public:
  explicit ExternalReference(void *address) : address_(address) {
#if __APPLE__
#if __has_feature(ptrauth_calls)
    address_ = __builtin_ptrauth_strip(address, ptrauth_key_asia);
#endif
#endif
  }

  const void *address();

private:
  const void *address_;
};

class AssemblerBase {
public:
  explicit AssemblerBase(void *address);

  ~AssemblerBase();

  // === IP / PC register ===

  int ip_offset() const;

  int pc_offset() const;

  // === CodeBuffer ===

  CodeBuffer *GetCodeBuffer();

  // === Realized Address ===

  virtual void *GetRealizedAddress();

  virtual void SetRealizedAddress(void *address);

  // === CPU Cache ===

  static void FlushICache(addr_t start, int size);

  static void FlushICache(addr_t start, addr_t end);

protected:
  CodeBuffer *buffer_;

  void *realized_address_;
};

} // namespace zz

#if 0
#include "globals.h"
#if TARGET_ARCH_ARM
#include "core/modules/assembler/assembler-arm.h"
#elif TARGET_ARCH_ARM64
#include "core/modules/assembler/assembler-arm64.h"
#elif TARGET_ARCH_X64
#include "core/modules/assembler/assembler-x64.h"
#else
#error "unsupported architecture"
#endif
#endif

#endif
