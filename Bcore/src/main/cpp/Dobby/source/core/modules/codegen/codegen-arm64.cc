#include "platform_macro.h"
#if defined(TARGET_ARCH_ARM64)

#include "dobby_internal.h"
#include "core/modules/codegen/codegen-arm64.h"

namespace zz {
namespace arm64 {

void CodeGen::LiteralLdrBranch(uint64_t address) {
  TurboAssembler *turbo_assembler_ = reinterpret_cast<TurboAssembler *>(this->assembler_);
#define _ turbo_assembler_->
  PseudoLabel address_ptr;

  _ Ldr(TMP_REG_0, &address_ptr);
  _ br(TMP_REG_0);
  _ PseudoBind(&address_ptr);
  _ EmitInt64(address);
}

} // namespace arm64
} // namespace zz

#endif
