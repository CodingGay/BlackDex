#ifndef CODE_BUFFER_X64_H
#define CODE_BUFFER_X64_H

#include "MemoryAllocator/CodeBuffer/CodeBufferBase.h"

class CodeBuffer : public CodeBufferBase {
public:
  CodeBuffer() : CodeBufferBase() {
  }

  CodeBuffer(int size) : CodeBufferBase(size) {
  }

public:
  void FixBindLabel(int offset, int32_t disp);

  void Emit32(int32_t data);

  void Emit64(int64_t data);
};

#endif