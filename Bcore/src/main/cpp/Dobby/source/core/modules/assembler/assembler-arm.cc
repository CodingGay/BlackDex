#include "platform_macro.h"
#if TARGET_ARCH_ARM

#include "core/modules/assembler/assembler-arm.h"

namespace zz {
namespace arm {

void Assembler::EmitARMInst(arm_inst_t instr) {
  buffer_->EmitARMInst(instr);
}

void Assembler::EmitAddress(uint32_t value) {
  buffer_->Emit32(value);
}

} // namespace arm
} // namespace zz

#endif