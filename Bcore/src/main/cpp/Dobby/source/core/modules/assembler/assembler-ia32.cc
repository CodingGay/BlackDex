#include "platform_macro.h"
#if TARGET_ARCH_IA32

#include "core/modules/assembler/assembler-ia32.h"

using namespace zz::x86;

void Assembler::jmp(Immediate imm) {
  buffer_->Emit8(0xE9);
  buffer_->Emit32((int)imm.value());
}

addr32_t TurboAssembler::CurrentIP() {
  return pc_offset() + (addr_t)realized_address_;
}

#endif