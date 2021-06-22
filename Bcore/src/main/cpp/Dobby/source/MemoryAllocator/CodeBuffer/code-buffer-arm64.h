#ifndef CODE_BUFFER_ARM64_H
#define CODE_BUFFER_ARM64_H

#include "MemoryAllocator/CodeBuffer/CodeBufferBase.h"

typedef int32_t arm64_inst_t;

class CodeBuffer : public CodeBufferBase {

public:
  CodeBuffer() : CodeBufferBase() {
  }

  CodeBuffer(int size) : CodeBufferBase(size) {
  }

public:
  arm64_inst_t LoadInst(int offset);

  void FixBindLabel(int offset, arm64_inst_t instr);

  void EmitInst(arm64_inst_t instr);

  void Emit64(int64_t data);
};

#endif
