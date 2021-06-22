#ifndef CODE_BUFFER_ARM_H
#define CODE_BUFFER_ARM_H

#include "MemoryAllocator/CodeBuffer/CodeBufferBase.h"

typedef int32_t arm_inst_t;
typedef int16_t thumb1_inst_t;
typedef int32_t thumb2_inst_t;

class CodeBuffer : public CodeBufferBase {
  enum ExecuteState { ARMExecuteState, ThumbExecuteState };

public:
  CodeBuffer() : CodeBufferBase() {
  }

  CodeBuffer(int size) : CodeBufferBase(size) {
  }

public:
  arm_inst_t LoadARMInst(int offset);

  thumb1_inst_t LoadThumb1Inst(int offset);

  thumb2_inst_t LoadThumb2Inst(int offset);

  void RewriteAddr(int offset, addr32_t addr);

  void RewriteARMInst(int offset, arm_inst_t instr);

  void RewriteThumb1Inst(int offset, thumb1_inst_t instr);

  void RewriteThumb2Inst(int offset, thumb2_inst_t instr);

  void EmitARMInst(arm_inst_t instr);

  void EmitThumb1Inst(thumb1_inst_t instr);

  void EmitThumb2Inst(thumb2_inst_t instr);

  void Emit32(int32_t data);
};

#endif