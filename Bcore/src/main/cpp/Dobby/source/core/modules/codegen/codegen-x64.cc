#include "platform_macro.h"
#if defined(TARGET_ARCH_X64)

#include "core/modules/codegen/codegen-x64.h"

namespace zz {
namespace x64 {

void CodeGen::JmpNearIndirect(uint64_t address) {
  TurboAssembler *turbo_assembler_ = reinterpret_cast<TurboAssembler *>(this->assembler_);
#define _ turbo_assembler_->
#define __ turbo_assembler_->GetCodeBuffer()->
  uint64_t currIP = turbo_assembler_->CurrentIP() + 6;
  dword offset = (dword)(address - currIP);

  // RIP-relative addressing
  __ Emit8(0xFF);
  __ Emit8(0x25);
  __ Emit32(offset);
}

} // namespace x64
} // namespace zz

#endif