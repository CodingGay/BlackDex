#include "platform_macro.h"
#if defined(TARGET_ARCH_ARM)

#include <string.h>

#include "MemoryAllocator/CodeBuffer/code-buffer-arm.h"

arm_inst_t CodeBuffer::LoadARMInst(int offset) {
  return *reinterpret_cast<arm_inst_t *>(buffer + offset);
}

thumb1_inst_t CodeBuffer::LoadThumb1Inst(int offset) {
  return *reinterpret_cast<thumb1_inst_t *>(buffer + offset);
};

thumb2_inst_t CodeBuffer::LoadThumb2Inst(int offset) {
  return *reinterpret_cast<thumb2_inst_t *>(buffer + offset);
};

void CodeBuffer::RewriteAddr(int offset, addr32_t addr) {
  memcpy(buffer + offset, &addr, sizeof(addr));
#if 0
  // SIGBUS (signal SIGBUS: illegal alignment)
  *reinterpret_cast<addr32_t *>(buffer + offset) = addr;
#endif
  return;
}

void CodeBuffer::RewriteARMInst(int offset, arm_inst_t instr) {
  *reinterpret_cast<arm_inst_t *>(buffer + offset) = instr;
  return;
}

void CodeBuffer::RewriteThumb1Inst(int offset, thumb1_inst_t instr) {
  *reinterpret_cast<thumb1_inst_t *>(buffer + offset) = instr;
  return;
}

void CodeBuffer::RewriteThumb2Inst(int offset, thumb2_inst_t instr) {
  memcpy(buffer + offset, &instr, sizeof(instr));
#if 0
  // SIGBUS (signal SIGBUS: illegal alignment)
  *reinterpret_cast<thumb2_inst_t *>(buffer + offset) = instr;
#endif
  return;
}

void CodeBuffer::EmitARMInst(arm_inst_t instr) {
  ensureCapacity(getSize() + sizeof(arm_inst_t));
  *reinterpret_cast<arm_inst_t *>(buffer_cursor) = instr;
  buffer_cursor += sizeof(arm_inst_t);
  return;
}

void CodeBuffer::EmitThumb1Inst(thumb1_inst_t instr) {
  ensureCapacity(getSize() + sizeof(thumb1_inst_t));
  *reinterpret_cast<thumb1_inst_t *>(buffer_cursor) = instr;
  buffer_cursor += sizeof(thumb1_inst_t);
  return;
}

void CodeBuffer::EmitThumb2Inst(thumb2_inst_t instr) {
  ensureCapacity(getSize() + sizeof(thumb2_inst_t));
  memcpy(buffer_cursor, &instr, sizeof(instr));
#if 0
  // SIGBUS (signal SIGBUS: illegal alignment)
  *reinterpret_cast<thumb2_inst_t *>(buffer_cursor) = instr;
#endif
  buffer_cursor += sizeof(thumb2_inst_t);
  return;
}

void CodeBuffer::Emit32(int32_t data) {
  ensureCapacity(getSize() + sizeof(int32_t));
  memcpy(buffer_cursor, &data, sizeof(data));
#if 0
  // SIGBUS (signal SIGBUS: illegal alignment)
  *reinterpret_cast<int32_t *>(buffer_cursor) = data;
#endif
  buffer_cursor += sizeof(int32_t);
  return;
}

#endif