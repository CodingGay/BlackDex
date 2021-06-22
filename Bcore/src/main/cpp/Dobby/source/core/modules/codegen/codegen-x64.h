#ifndef CORE_CODEGEN_X64_H
#define CORE_CODEGEN_X64_H

#include "core/modules/codegen/codegen.h"
#include "core/modules/assembler/assembler.h"
#include "core/modules/assembler/assembler-x64.h"

namespace zz {
namespace x64 {

class CodeGen : public CodeGenBase {
public:
  CodeGen(TurboAssembler *turbo_assembler) : CodeGenBase(turbo_assembler) {
  }

  void JmpNearIndirect(uint64_t address);
};

} // namespace x64
} // namespace zz

#endif