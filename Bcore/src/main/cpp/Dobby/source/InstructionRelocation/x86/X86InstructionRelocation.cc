#include "platform_macro.h"
#if defined(TARGET_ARCH_IA32)

#include "InstructionRelocation/x86//X86InstructionRelocation.h"

#include <string.h>

#include "dobby_internal.h"

#include "InstructionRelocation/x86/x86_insn_decode/x86_insn_decode.h"

#include "core/arch/x86/registers-x86.h"
#include "core/modules/assembler/assembler-ia32.h"
#include "core/modules/codegen/codegen-ia32.h"

using namespace zz::x86;

static int GenRelocateCodeFixed(void *buffer, AssemblyCodeChunk *origin, AssemblyCodeChunk *relocated) {
  TurboAssembler turbo_assembler_(0);
  // Set fixed executable code chunk address
  turbo_assembler_.SetRealizedAddress((void *)relocated->raw_instruction_start());
#define _ turbo_assembler_.
#define __ turbo_assembler_.GetCodeBuffer()->

  addr64_t curr_orig_ip = origin->raw_instruction_start();
  addr64_t curr_relo_ip = relocated->raw_instruction_start();

  addr_t buffer_cursor = (addr_t)buffer;

  x86_options_t conf = {0};
  conf.mode = 32;

  int predefined_relocate_size = origin->raw_instruction_size();

  while ((buffer_cursor < ((addr_t)buffer + predefined_relocate_size))) {
    int last_relo_offset = turbo_assembler_.GetCodeBuffer()->getSize();

    x86_insn_decode_t insn = {0};
    memset(&insn, 0, sizeof(insn));
    // decode x86 insn
    x86_insn_decode(&insn, (uint8_t *)buffer_cursor, &conf);

    if (insn.primary_opcode >= 0x70 && insn.primary_opcode <= 0x7F) { // jc rel8
      DLOG(0, "[x86 relo] jc rel8, %p", buffer_cursor);

      int8_t orig_offset = insn.immediate;
      int new_offset = (int)(curr_orig_ip + orig_offset - curr_relo_ip);
      uint8_t opcode = 0x80 | (insn.primary_opcode & 0x0f);

      __ Emit8(0x0F);
      __ Emit8(opcode);
      __ Emit32(new_offset);
    } else if (insn.primary_opcode == 0xEB) { // jmp rel8
      DLOG(0, "[x86 relo] jmp rel8, %p", buffer_cursor);

      int8_t orig_offset = insn.immediate;
      int8_t new_offset = (int8_t)(curr_orig_ip + orig_offset - curr_relo_ip);

      __ Emit8(0xE9);
      __ Emit32(new_offset);
    } else if (insn.primary_opcode == 0xE8 || insn.primary_opcode == 0xE9) { // call or jmp rel32
      DLOG(0, "[x86 relo] jmp or call rel32, %p", buffer_cursor);

      dword orig_offset = insn.immediate;
      dword offset = (dword)(curr_orig_ip + orig_offset - curr_relo_ip);

      __ EmitBuffer((void *)buffer_cursor, insn.immediate_offset);
      __ Emit32(offset);
    } else if (insn.primary_opcode >= 0xE0 && insn.primary_opcode <= 0xE2) { // LOOPNZ/LOOPZ/LOOP/JECXZ
      // LOOP/LOOPcc
      UNIMPLEMENTED();
    } else if (insn.primary_opcode == 0xE3) {
      // JCXZ JCEXZ JCRXZ
      UNIMPLEMENTED();
    } else {
      // Emit the origin instrution
      __ EmitBuffer((void *)buffer_cursor, insn.length);
    }

    // go next
    curr_orig_ip += insn.length;
    buffer_cursor += insn.length;

#if 0
    {
      // 1 orignal instrution => ? relocated instruction
      int relo_offset = turbo_assembler_.GetCodeBuffer()->getSize();
      int relo_len    = relo_offset - last_relo_offset;
      curr_relo_ip += relo_len;
    }
#endif
    curr_relo_ip = relocated->raw_instruction_start() + turbo_assembler_.ip_offset();
  }

  // jmp to the origin rest instructions
  CodeGen codegen(&turbo_assembler_);
  addr64_t stub_addr = curr_relo_ip + 6;
  codegen.JmpNear(curr_orig_ip);

  // update origin
  int new_origin_len = curr_orig_ip - origin->raw_instruction_start();
  origin->re_init_region_range(origin->raw_instruction_start(), new_origin_len);

  int relo_len = turbo_assembler_.GetCodeBuffer()->getSize();
  if (relo_len > relocated->raw_instruction_size()) {
    DLOG(0, "pre-alloc code chunk not enough");
    return RT_FAILED;
  }

  // Generate executable code
  {
    AssemblyCodeChunk *code = NULL;
    code = AssemblyCodeBuilder::FinalizeFromTurboAssembler(&turbo_assembler_);
    delete code;
  }

  return RT_SUCCESS;
}

void GenRelocateCodeAndBranch(void *buffer, AssemblyCodeChunk *origin, AssemblyCodeChunk *relocated) {
  // pre-alloc code chunk
  AssemblyCodeChunk *cchunk = NULL;

  int relo_code_chunk_size = 32;
  const int chunk_size_step = 16;

x86_try_again:
  if (relocated->raw_instruction_start() == 0) {
    cchunk = MemoryArena::AllocateCodeChunk(relo_code_chunk_size);
    if (cchunk == nullptr) {
      return;
    }
    relocated->re_init_region_range((addr_t)cchunk->address, (int)cchunk->length);
  }

  int ret = GenRelocateCodeFixed(buffer, origin, relocated);
  if (ret != RT_SUCCESS) {
    // free the cchunk
    MemoryArena::Destroy(cchunk);

    relo_code_chunk_size += chunk_size_step;
    relocated->re_init_region_range(0, 0);

    goto x86_try_again;
  }
}

#endif
