#include "platform_macro.h"
#if defined(TARGET_ARCH_ARM64)

#include "InstructionRelocation/arm64/ARM64InstructionRelocation.h"

#include "dobby_internal.h"

#include "core/arch/arm64/registers-arm64.h"
#include "core/modules/assembler/assembler-arm64.h"
#include "core/modules/codegen/codegen-arm64.h"

using namespace zz::arm64;

// Compare and branch.
enum CompareBranchOp {
  CompareBranchFixed = 0x34000000,
  CompareBranchFixedMask = 0x7E000000,
  CompareBranchMask = 0xFF000000,
};

// Conditional branch.
enum ConditionalBranchOp {
  ConditionalBranchFixed = 0x54000000,
  ConditionalBranchFixedMask = 0xFE000000,
  ConditionalBranchMask = 0xFF000010,
};

// Test and branch.
enum TestBranchOp {
  TestBranchFixed = 0x36000000,
  TestBranchFixedMask = 0x7E000000,
  TestBranchMask = 0x7F000000,
  TBZ = TestBranchFixed | 0x00000000,
  TBNZ = TestBranchFixed | 0x01000000
};

static inline int64_t SignExtend(unsigned long x, int M, int N) {
#if 0
  char sign_bit      = bit(x, M - 1);
  unsigned long sign_mask = 0 - sign_bit;
  x |= ((sign_mask >> M) << M);
#else
  x = (long)(x << (N - M)) >> (N - M);
#endif
  return x;
}

#if 0
static inline unsigned long set_bit(obj, st, unsigned long bit) {
  return (((~(1 << st)) & obj) | (bit << st));
}
static inline unsigned long set_bits(obj, st, fn, unsigned long bits) {
  return (((~(submask(fn - st) << st)) & obj) | (bits << st));
}
#endif

static inline int64_t decode_imm14_offset(uint32_t instr) {
  int64_t offset;
  {
    int64_t imm19 = bits(instr, 5, 18);
    offset = (imm19 << 2);
  }
  offset = SignExtend(offset, 2 + 14, 64);
  return offset;
}

static inline int64_t decode_imm19_offset(uint32_t instr) {
  int64_t offset;
  {
    int64_t imm19 = bits(instr, 5, 23);
    offset = (imm19 << 2);
  }
  offset = SignExtend(offset, 2 + 19, 64);
  return offset;
}

static inline int64_t decode_imm26_offset(uint32_t instr) {
  int64_t offset;
  {
    int64_t imm26 = bits(instr, 0, 25);
    offset = (imm26 << 2);
  }
  offset = SignExtend(offset, 2 + 26, 64);
  return offset;
}

static inline int64_t decode_immhi_immlo_offset(uint32_t instr) {
  typedef uint32_t instr_t;
  struct {
    instr_t Rd : 5;      // Destination register
    instr_t immhi : 19;  // 19-bit upper immediate
    instr_t dummy_0 : 5; // Must be 10000 == 0x10
    instr_t immlo : 2;   // 2-bit lower immediate
    instr_t op : 1;      // 0 = ADR, 1 = ADRP
  } instr_decode;

  *(instr_t *)&instr_decode = instr;

  int64_t imm = instr_decode.immlo + (instr_decode.immhi << 2);
  imm = SignExtend(imm, 2 + 19, 64);
  return imm;
}

static inline int64_t decode_immhi_immlo_zero12_offset(uint32_t instr) {
  int64_t imm = decode_immhi_immlo_offset(instr);
  imm = imm << 12;
  return imm;
}

static inline int decode_rt(uint32_t instr) {
  return bits(instr, 0, 4);
}

static inline int decode_rd(uint32_t instr) {
  return bits(instr, 0, 4);
}

#if defined(DOBBY_DEBUG)
#define debug_nop() _ nop()
#else
#define debug_nop()
#endif

void GenRelocateCodeAndBranch(void *buffer, AssemblyCodeChunk *origin, AssemblyCodeChunk *relocated) {
  TurboAssembler turbo_assembler_(0);
#define _ turbo_assembler_.

  uint64_t curr_orig_pc = origin->raw_instruction_start();
  uint64_t curr_relo_pc = relocated->raw_instruction_start();

  addr_t buffer_cursor = (addr_t)buffer;
  arm64_inst_t instr = *(arm64_inst_t *)buffer_cursor;

  int predefined_relocate_size = origin->raw_instruction_size();

  while (buffer_cursor < ((addr_t)buffer + predefined_relocate_size)) {
    int last_relo_offset = turbo_assembler_.GetCodeBuffer()->getSize();

    if ((instr & LoadRegLiteralFixedMask) == LoadRegLiteralFixed) { // ldr x0, #16
      int rt = decode_rt(instr);
      char opc = bits(instr, 30, 31);
      addr64_t memory_address = decode_imm19_offset(instr) + curr_orig_pc;

#define MEM(reg, offset) MemOperand(reg, offset)
      debug_nop(); // for debug
      {
        _ Mov(TMP_REG_0, memory_address); // should we replace with `Ldr` to set  X17 ?
        if (opc == 0b00)
          _ ldr(W(rt), MEM(TMP_REG_0, 0));
        else if (opc == 0b01)
          _ ldr(X(rt), MEM(TMP_REG_0, 0));
        else {
          UNIMPLEMENTED();
        }
      }
      debug_nop();
    } else if ((instr & PCRelAddressingFixedMask) == PCRelAddressingFixed) {
      int rd = decode_rd(instr);

      int64_t imm = 0;
      addr64_t runtime_address = 0;
      if ((instr & PCRelAddressingMask) == ADR) {
        imm = decode_immhi_immlo_offset(instr);
        runtime_address = curr_orig_pc + imm;
      } else {
        imm = decode_immhi_immlo_zero12_offset(instr);
        runtime_address = ALIGN_FLOOR(curr_orig_pc, (1 << 12)) + imm;
      }

      /* output Mov */
      debug_nop();
      {
        _ Mov(X(rd), runtime_address); // should we replace with `Ldr` to set  X17 ?
      }
      debug_nop();

    } else if ((instr & UnconditionalBranchFixedMask) == UnconditionalBranchFixed) { // b xxx
      addr_t branch_address = decode_imm26_offset(instr) + curr_orig_pc;
      RelocLabelEntry *branchAddressLabel = new RelocLabelEntry(branch_address);
      _ AppendRelocLabelEntry(branchAddressLabel);

      debug_nop();
      {
        _ Ldr(TMP_REG_0, branchAddressLabel); // should we replace with `Mov` to set  X17 ?
        if ((instr & UnconditionalBranchMask) == BL) {
          _ blr(TMP_REG_0);
        } else {
          _ br(TMP_REG_0);
        }
      }
      debug_nop();
    } else if ((instr & TestBranchFixedMask) == TestBranchFixed) { // tbz, tbnz
      addr64_t branch_address = decode_imm14_offset(instr) + curr_orig_pc;
      RelocLabelEntry *branchAddressLabel = new RelocLabelEntry(branch_address);
      _ AppendRelocLabelEntry(branchAddressLabel);

      arm64_inst_t branch_instr = instr;

      char op = bit(instr, 24);
      op = op ^ 1;
      set_bit(branch_instr, 24, op);

      int64_t offset = 4 * 3; // branch_instr; ldr x17, #label; br x17
      uint32_t imm14 = offset >> 2;
      set_bits(branch_instr, 5, 18, imm14);

      debug_nop();
      {
        _ Emit(branch_instr);
        {
          _ Ldr(TMP_REG_0, branchAddressLabel); // should we replace with `Mov` to set  X17 ?
          _ br(TMP_REG_0);
        }
      }
      debug_nop();

    } else if ((instr & CompareBranchFixedMask) == CompareBranchFixed) { // cbz cbnz
      addr64_t branch_address = decode_imm19_offset(instr) + curr_orig_pc;

      arm64_inst_t branch_instr = instr;

      char op = bit(instr, 24);
      op = op ^ 1;
      set_bit(branch_instr, 24, op);

      int64_t offset = 4 * 3;
      uint32_t imm19 = offset >> 2;
      set_bits(branch_instr, 5, 23, imm19);

      RelocLabelEntry *branchAddressLabel = new RelocLabelEntry(branch_address);
      _ AppendRelocLabelEntry(branchAddressLabel);

      debug_nop();
      {
        _ Emit(branch_instr);
        {
          _ Ldr(TMP_REG_0, branchAddressLabel); // should we replace with `Mov` to set  X17 ?
          _ br(TMP_REG_0);
        }
      }
      debug_nop();
    } else if ((instr & ConditionalBranchFixedMask) == ConditionalBranchFixed) { // b.cond
      addr64_t branch_address = decode_imm19_offset(instr) + curr_orig_pc;

      arm64_inst_t branch_instr = instr;

      char cond = bits(instr, 0, 3);
      cond = cond ^ 1;
      set_bits(branch_instr, 0, 3, cond);

      int64_t offset = 4 * 3;
      uint32_t imm19 = offset >> 2;
      set_bits(branch_instr, 5, 23, imm19);

      RelocLabelEntry *branchAddressLabel = new RelocLabelEntry(branch_address);
      _ AppendRelocLabelEntry(branchAddressLabel);

      debug_nop();
      {
        _ Emit(branch_instr);
        {
          _ Ldr(TMP_REG_0, branchAddressLabel); // should we replace with `Mov` to set  X17 ?
          _ br(TMP_REG_0);
        }
      }
      debug_nop();
    } else {
      // origin write the instruction bytes
      _ Emit(instr);
    }

    // Move to next instruction
    curr_orig_pc += 4;
    buffer_cursor += 4;

#if 0
    {
      // 1 orignal instrution => ? relocated instruction
      int relo_offset = turbo_assembler_.GetCodeBuffer()->getSize();
      int relo_len    = relo_offset - last_relo_offset;
      curr_relo_pc += relo_len;
    }
#endif

    curr_relo_pc = relocated->raw_instruction_start() + turbo_assembler_.pc_offset();

    instr = *(arm64_inst_t *)buffer_cursor;
  }

#if 0
     // check branch in relocate-code range
    {
      for (size_t i = 0; i < data_labels->getCount(); i++) {
        RelocLabelEntry *pseudoLabel = (RelocLabelEntry *)data_labels->getObject(i);
        if (pseudoLabel->address == curr_orig_pc) {
          FATAL("label(%p) in relo code %p, please enable b-xxx branch plugin.", pseudoLabel->address, curr_orig_pc);
        }
      }
    }
#endif

  // TODO: if last instr is unlink branch, skip
  // Branch to the rest of instructions
  CodeGen codegen(&turbo_assembler_);
  codegen.LiteralLdrBranch(curr_orig_pc);

  _ RelocBind();

  // Generate executable code
  {
    AssemblyCodeChunk *code = NULL;
    code = AssemblyCodeBuilder::FinalizeFromTurboAssembler(&turbo_assembler_);
    relocated->re_init_region_range(code->raw_instruction_start(), code->raw_instruction_size());
    delete code;
  }
}

#endif
