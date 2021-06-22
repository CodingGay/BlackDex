#include "platform_macro.h"
#if defined(TARGET_ARCH_ARM)

#include "InstructionRelocation/arm/ARMInstructionRelocation.h"

#include "dobby_internal.h"

#include "core/arch/arm/registers-arm.h"
#include "core/modules/assembler/assembler-arm.h"
#include "core/modules/codegen/codegen-arm.h"

using namespace zz;
using namespace zz::arm;

typedef struct ReloMapEntry {
  addr32_t orig_instr;
  addr32_t relocated_instr;
  int relocated_code_len;
} ReloMapEntry;

static bool is_thumb2(uint32_t instr) {
  uint16_t inst1, inst2;
  inst1 = instr & 0x0000ffff;
  inst2 = (instr & 0xffff0000) >> 16;
  // refer: Top level T32 instruction set encoding
  uint32_t op0 = bits(inst1, 13, 15);
  uint32_t op1 = bits(inst1, 11, 12);

  if (op0 == 0b111 && op1 != 0b00) {
    return true;
  }
  return false;
}

static void ARMRelocateSingleInstr(TurboAssembler *turbo_assembler, int32_t instr, addr32_t from_pc, addr32_t to_pc,
                                   addr32_t *execute_state_changed_pc_ptr) {
  bool is_instr_relocated = false;
#define _ turbo_assembler->
  // top level encoding
  uint32_t cond, op0, op1;
  cond = bits(instr, 28, 31);
  op0 = bits(instr, 25, 27);
  op1 = bit(instr, 4);
  // Load/Store Word, Unsigned byte (immediate, literal)
  if (cond != 0b1111 && op0 == 0b010) {
    uint32_t P, U, o2, W, o1, Rn, Rt, imm12;
    P = bit(instr, 24);
    U = bit(instr, 23);
    W = bit(instr, 21);
    imm12 = bits(instr, 0, 11);
    Rn = bits(instr, 16, 19);
    Rt = bits(instr, 12, 15);
    o1 = bit(instr, 20);
    o2 = bit(instr, 22);
    uint32_t P_W = (P << 1) | W;
    do {
      // LDR (literal)
      if (o1 == 1 && o2 == 0 && P_W != 0b01 && Rn == 0b1111) {
        goto load_literal_fix_scheme;
      }
      if (o1 == 1 && o2 == 1 && P_W != 0b01 && Rn == 0b1111) {
        goto load_literal_fix_scheme;
      }
      break;
    load_literal_fix_scheme:
      addr32_t target_address = 0;
      if (U == 0b1)
        target_address = from_pc + imm12;
      else
        target_address = from_pc - imm12;
      Register regRt = Register::R(Rt);

      RelocLabelEntry *pseudoDataLabel = new RelocLabelEntry(target_address);
      _ AppendRelocLabelEntry(pseudoDataLabel);

      // ===
      if (regRt.code() == pc.code()) {
        _ Ldr(VOLATILE_REGISTER, pseudoDataLabel);
        _ ldr(regRt, MemOperand(VOLATILE_REGISTER));
      } else {
        _ Ldr(regRt, pseudoDataLabel);
        _ ldr(regRt, MemOperand(regRt));
      }
      // ===
      is_instr_relocated = true;
    } while (0);
  }

  // Data-processing and miscellaneous instructions
  if (cond != 0b1111 && (op0 & 0b110) == 0b000) {
    uint32_t op0, op1, op2, op3, op4;
    op0 = bit(instr, 25);
    // Data-processing immediate
    if (op0 == 1) {
      uint32_t op0, op1;
      op0 = bits(instr, 23, 24);
      op1 = bits(instr, 20, 21);
      // Integer Data Processing (two register and immediate)
      if ((op0 & 0b10) == 0b00) {
        uint32_t opc, S, Rn;
        opc = bits(instr, 21, 23);
        S = bit(instr, 20);
        Rn = bits(instr, 16, 19);
        do {
          uint32_t target_address;
          int Rd = bits(instr, 12, 15);
          int imm12 = bits(instr, 0, 11);
          int label = imm12;
          if (opc == 0b010 && S == 0b0 && Rn == 0b1111) {
            // ADR - A2 variant
            // add = FALSE
            target_address = from_pc - imm12;
          } else if (opc == 0b100 && S == 0b0 && Rn == 0b1111) {
            // ADR - A1 variant
            // add = TRUE
            target_address = from_pc + imm12;
          } else
            break;

          Register regRd = Register::R(Rd);
          RelocLabelEntry *pseudoDataLabel = new RelocLabelEntry(target_address);
          _ AppendRelocLabelEntry(pseudoDataLabel);
          // ===
          _ Ldr(regRd, pseudoDataLabel);
          // ===
          is_instr_relocated = true;
        } while (0);

        // EXample
        if (opc == 0b111 && S == 0b1 && Rn == 0b1111) {
          // do something
        }
      }
    }
  }

  // Branch, branch with link, and block data transfer
  if ((op0 & 0b110) == 0b100) {
    uint32_t cond, op0;
    cond = bits(instr, 28, 31);
    op0 = bit(instr, 25);
    // Branch (immediate)
    if (op0 == 1) {
      uint32_t cond = 0, H = 0, imm24 = 0;
      bool flag_link;
      do {
        int imm24 = bits(instr, 0, 23);
        int label = imm24 << 2;
        uint32_t target_address = from_pc + label;
        if (cond != 0b1111 && H == 0) {
          // B
          flag_link = false;
        } else if (cond != 0b1111 && H == 1) {
          // BL, BLX (immediate) - A1 variant
          flag_link = true;
        } else if (cond == 0b1111) {
          // BL, BLX (immediate) - A2 variant
          flag_link = true;
        } else
          break;

        // ===
        // just modify orin instruction label bits, and keep the link and cond bits, the next instruction `b_imm` will
        // do the rest work.
        label = 0x4;
        imm24 = label >> 2;
        _ EmitARMInst((instr & 0xff000000) | imm24);
        if (flag_link) {
          _ bl(0);
          _ b(4);
        } else {
          _ b(4);
        }
        _ ldr(pc, MemOperand(pc, -4));
        _ EmitAddress(target_address);
        is_instr_relocated = true;
      } while (0);
    }
  }

  // if the instr do not needed relocate, just rewrite the origin
  if (!is_instr_relocated) {
    _ EmitARMInst(instr);
  }
}

// relocate thumb-1 instructions
static void Thumb1RelocateSingleInstr(ThumbTurboAssembler *turbo_assembler, LiteMutableArray *thumb_labels,
                                      int16_t instr, addr32_t from_pc, addr32_t to_pc,
                                      addr32_t *execute_state_changed_pc_ptr) {
  bool is_instr_relocated = false;

  _ AlignThumbNop();

  uint32_t val = 0, op = 0, rt = 0, rm = 0, rn = 0, rd = 0, shift = 0, cond = 0;
  int32_t offset = 0;

  int32_t op0 = 0, op1 = 0;
  op0 = bits(instr, 10, 15);
  // [F3.2.3 Special data instructions and branch and exchange]
  if (op0 == 0b010001) {
    op0 = bits(instr, 8, 9);
    // [Add, subtract, compare, move (two high registers)]
    if (op0 != 0b11) {
      int rs = bits(instr, 3, 6);
      // rs is PC register
      if (rs == 15) {
        val = from_pc;

        uint16_t rewrite_inst = 0;
        rewrite_inst = (instr & 0xff87) | LeftShift((VOLATILE_REGISTER.code()), 4, 3);

        ThumbRelocLabelEntry *label = new ThumbRelocLabelEntry(val, false);
        _ AppendRelocLabelEntry(label);

        _ T2_Ldr(VOLATILE_REGISTER, label);
        _ EmitInt16(rewrite_inst);

        is_instr_relocated = true;
      }
    }

    // Branch and exchange
    if (op0 == 0b11) {
      int32_t L = bit(instr, 7);
      // BX
      if (L == 0b0) {
        rm = bits(instr, 3, 6);
        if (rm == pc.code()) {
          val = from_pc;
          ThumbRelocLabelEntry *label = new ThumbRelocLabelEntry(val, true);
          _ AppendRelocLabelEntry(label);

          _ T2_Ldr(pc, label);

          *execute_state_changed_pc_ptr = val;
          is_instr_relocated = true;
        }
      }
      // BLX
      if (L == 0b1) {
        if (rm == pc.code()) {
          val = from_pc;
          ThumbRelocLabelEntry *label = new ThumbRelocLabelEntry(val, true);
          _ AppendRelocLabelEntry(label);

          int label_branch_off = 4, label_continue_off = 4;
          _ t2_bl(label_branch_off);
          _ t2_b(label_continue_off);
          /* Label: branch */
          _ T2_Ldr(pc, label);
          /* Label: continue */

          *execute_state_changed_pc_ptr = val;
          is_instr_relocated = true;
        }
      }
    }
  }

  // ldr literal
  if ((instr & 0xf800) == 0x4800) {
    int32_t imm8 = bits(instr, 0, 7);
    int32_t offset = imm8 << 2;
    val = from_pc + offset;
    val = ALIGN_FLOOR(val, 4);
    rt = bits(instr, 8, 10);

    ThumbRelocLabelEntry *label = new ThumbRelocLabelEntry(val, false);
    _ AppendRelocLabelEntry(label);

    _ T2_Ldr(Register::R(rt), label);
    _ t2_ldr(Register::R(rt), MemOperand(Register::R(rt), 0));

    is_instr_relocated = true;
  }

  // adr
  if ((instr & 0xf800) == 0xa000) {
    rd = bits(instr, 8, 10);
    uint16_t imm8 = bits(instr, 0, 7);
    val = from_pc + imm8;

    ThumbRelocLabelEntry *label = new ThumbRelocLabelEntry(val, false);
    _ AppendRelocLabelEntry(label);

    _ T2_Ldr(Register::R(rd), label);

    if (pc.code() == rd)
      val += 1;
    is_instr_relocated = true;
  }

  // b
  if ((instr & 0xf000) == 0xd000) {
    uint16_t cond = bits(instr, 8, 11);
    // cond != 111x
    if (cond >= 0b1110) {
      UNREACHABLE();
    }
    uint16_t imm8 = bits(instr, 0, 7);
    uint32_t offset = imm8 << 1;
    val = from_pc + offset;

    ThumbRelocLabelEntry *label = new ThumbRelocLabelEntry(val + 1, true);
    _ AppendRelocLabelEntry(label);

    // modify imm8 field
    imm8 = 0x4 >> 1;

    _ EmitInt16((instr & 0xfff0) | imm8);
    _ t1_nop(); // manual align
    _ t2_b(4);
    _ T2_Ldr(pc, label);

    is_instr_relocated = true;
  }

  // compare branch (cbz, cbnz)
  if ((instr & 0xf500) == 0xb100) {
    uint16_t imm5 = bits(instr, 3, 7);
    uint16_t i = bit(instr, 9);
    uint32_t offset = (i << 6) | (imm5 << 1);
    val = from_pc + offset;
    rn = bits(instr, 0, 2);

    ThumbRelocLabelEntry *label = new ThumbRelocLabelEntry(val + 1, true);
    _ AppendRelocLabelEntry(label);

    imm5 = bits(0x4 >> 1, 1, 5);
    i = bit(0x4 >> 1, 6);

    _ EmitInt16((instr & 0xfd07) | imm5 << 3 | i << 9);
    _ t1_nop(); // manual align
    _ t2_b(0);
    _ T2_Ldr(pc, label);

    is_instr_relocated = true;
  }

  // unconditional branch
  if ((instr & 0xf800) == 0xe000) {
    uint16_t imm11 = bits(instr, 0, 10);
    uint32_t offset = imm11 << 1;
    val = from_pc + offset;

    ThumbRelocLabelEntry *label = new ThumbRelocLabelEntry(val + 1, true);
    _ AppendRelocLabelEntry(label);

    _ T2_Ldr(pc, label);

    is_instr_relocated = true;
  }

  // if the instr do not needed relocate, just rewrite the origin
  if (!is_instr_relocated) {
#if 0
        if (from_pc % Thumb2_INST_LEN)
            _ t1_nop();
#endif
    _ EmitInt16(instr);
  }
}

static void Thumb2RelocateSingleInstr(ThumbTurboAssembler *turbo_assembler, LiteMutableArray *thumb_labels,
                                      thumb1_inst_t inst1, thumb1_inst_t inst2, addr32_t from_pc, addr32_t to_pc) {

  bool is_instr_relocated = false;

  // if (turbo_assembler->pc_offset() % 4) {
  //   _ t1_nop();
  // }

  _ AlignThumbNop();

  // Branches and miscellaneous control
  if ((inst1 & 0xf800) == 0xf000 && (inst2 & 0x8000) == 0x8000) {
    uint32_t op1 = 0, op3 = 0;
    op1 = bits(inst1, 6, 9);
    op3 = bits(inst2, 12, 14);

    // B-T3 AKA b.cond
    if (((op1 & 0b1110) != 0b1110) && ((op3 & 0b101) == 0b000)) {

      int S = sbits(inst1, 10, 10);
      int J1 = bit(inst2, 13);
      int J2 = bit(inst2, 11);
      int imm6 = bits(inst1, 0, 5);
      int imm11 = bits(inst2, 0, 10);

      int32_t label = (S << 20) | (J2 << 19) | (J1 << 18) | (imm6 << 12) | (imm11 << 1);
      addr32_t val = from_pc + label;

      // ===
      imm11 = 0x4 >> 1;
      _ EmitInt16(inst1 & 0xffc0);           // clear imm6
      _ EmitInt16((inst2 & 0xd000) | imm11); // 1. clear J1, J2, origin_imm12 2. set new imm11

      _ t2_b(4);
      _ t2_ldr(pc, MemOperand(pc, 0));
      _ EmitAddress(val + THUMB_ADDRESS_FLAG);
      // ===
      is_instr_relocated = true;
    }

    // B-T4 AKA b.w
    if ((op3 & 0b101) == 0b001) {
      int S = bit(inst1, 10);
      int J1 = bit(inst2, 13);
      int J2 = bit(inst2, 11);
      int imm10 = bits(inst1, 0, 9);
      int imm11 = bits(inst2, 0, 10);
      int i1 = !(J1 ^ S);
      int i2 = !(J2 ^ S);

      int32_t label = (-S << 24) | (i1 << 23) | (i2 << 22) | (imm10 << 12) | (imm11 << 1);
      addr32_t val = from_pc + label;

      _ t2_ldr(pc, MemOperand(pc, 0));
      _ EmitAddress(val + THUMB_ADDRESS_FLAG);
      // ===
      is_instr_relocated = true;
    }

    // BL, BLX (immediate) - T1 variant AKA bl
    if ((op3 & 0b101) == 0b101) {
      int S = bit(inst1, 10);
      int J1 = bit(inst2, 13);
      int J2 = bit(inst2, 11);
      int i1 = !(J1 ^ S);
      int i2 = !(J2 ^ S);
      int imm11 = bits(inst2, 0, 10);
      int imm10 = bits(inst1, 0, 9);
      // S is sign-bit, '-S' maybe not better
      int32_t label = (imm11 << 1) | (imm10 << 12) | (i2 << 22) | (i1 << 23) | (-S << 24);
      addr32_t val = from_pc + label;

      _ t2_bl(4);
      _ t2_b(8);
      _ t2_ldr(pc, MemOperand(pc, 0));
      _ EmitAddress(val + THUMB_ADDRESS_FLAG);
      // =====
      is_instr_relocated = true;
    }

    // BL, BLX (immediate) - T2 variant AKA blx
    if ((op3 & 0b101) == 0b100) {
      int S = bit(inst1, 10);
      int J1 = bit(inst2, 13);
      int J2 = bit(inst2, 11);
      int i1 = !(J1 ^ S);
      int i2 = !(J2 ^ S);
      int imm10h = bits(inst1, 0, 9);
      int imm10l = bits(inst2, 1, 10);
      // S is sign-bit, '-S' maybe not better
      int32_t label = (imm10l << 2) | (imm10h << 12) | (i2 << 22) | (i1 << 23) | (-S << 24);
      addr32_t val = ALIGN(from_pc, 4) + label;

      _ t2_bl(4);
      _ t2_b(8);
      _ t2_ldr(pc, MemOperand(pc, 0));
      _ EmitAddress(val);
      // =====
      is_instr_relocated = true;
    }
  }

  // Data-processing (plain binary immediate)
  if ((inst1 & (0xfa10)) == 0xf200 & (inst2 & 0x8000) == 0) {
    uint32_t op0 = 0, op1 = 0;
    op0 = bit(inst1, 8);
    op1 = bits(inst2, 5, 6);

    // Data-processing (simple immediate)
    if (op0 == 0 && (op1 & 0b10) == 0b00) {
      int o1 = bit(inst1, 7);
      int o2 = bit(inst1, 5);
      int rn = bits(inst1, 0, 3);

      // ADR
      if (((o1 == 0 && o2 == 0) || (o1 == 1 && o2 == 1)) && rn == 0b1111) {
        uint32_t i = bit(inst1, 10);
        uint32_t imm3 = bits(inst2, 12, 14);
        uint32_t imm8 = bits(inst2, 0, 7);
        uint32_t rd = bits(inst2, 8, 11);
        uint32_t label = imm8 | (imm3 << 8) | (i << 11);
        addr32_t val = 0;

        if (o1 == 0 && o2 == 0) { // ADR - T3
          // ADR - T3 variant
          // adr with add
          val = from_pc + label;
        } else if (o1 == 1 && o2 == 1) { // ADR - T2
          // ADR - T2 variant
          // adr with sub
          val = from_pc - label;
        } else {
          UNREACHABLE();
        }

        // ===
        _ t2_ldr(Register::R(rd), MemOperand(pc, 4));
        _ t2_b(0);
        _ EmitAddress(val);
        // ===
        is_instr_relocated = true;
      }
    }
  }

  // LDR literal (T2)
  if ((inst1 & 0xff7f) == 0xf85f) {
    uint32_t U = bit(inst1, 7);
    uint32_t imm12 = bits(inst2, 0, 11);
    uint16_t rt = bits(inst2, 12, 15);

    uint32_t label = imm12;
    addr32_t val = 0;
    if (U == 1) {
      val = from_pc + label;
    } else {
      val = from_pc - label;
    }

    val = ALIGN_FLOOR(val, 4);

    Register regRt = Register::R(rt);
    // =====
    _ t2_ldr(regRt, MemOperand(pc, 4));
    _ t2_b(4);
    _ EmitAddress(val);
    _ t2_ldr(regRt, MemOperand(regRt, 0));
    // =====
    is_instr_relocated = true;
  }

  // if the instr do not needed relocate, just rewrite the origin
  if (!is_instr_relocated) {
#if 0
        if (from_pc % Thumb2_INST_LEN)
            _ t1_nop();
#endif
    _ EmitInt16(inst1);
    _ EmitInt16(inst2);
  }
}

void gen_arm_relocate_code(LiteMutableArray *relo_map, TurboAssembler *turbo_assembler_, void *buffer,
                           AssemblyCodeChunk *origin, AssemblyCodeChunk *relocated,
                           addr32_t *execute_state_changed_pc_ptr) {
#undef _
#define _ turbo_assembler_->
  addr32_t curr_orig_pc = origin->raw_instruction_start() + ARM_PC_OFFSET;
  addr32_t curr_relo_pc = relocated->raw_instruction_start() + ARM_PC_OFFSET + turbo_assembler_->pc_offset();

  addr_t buffer_cursor = (addr_t)buffer;
  arm_inst_t instr = *(arm_inst_t *)buffer_cursor;

  int predefined_relocate_size = origin->raw_instruction_size();

  addr32_t execute_state_changed_pc = 0;

  while (buffer_cursor < ((addr_t)buffer + predefined_relocate_size)) {
    int last_relo_offset = turbo_assembler_->GetCodeBuffer()->getSize();

    ARMRelocateSingleInstr(turbo_assembler_, instr, curr_orig_pc, curr_relo_pc, execute_state_changed_pc_ptr);
    DLOG(0, "[arm] Relocate arm instr: 0x%x", instr);

    {
      // 1 orignal instrution => ? relocated instruction
      int relo_offset = turbo_assembler_->GetCodeBuffer()->getSize();
      int relo_len = relo_offset - last_relo_offset;

      ReloMapEntry *map = new ReloMapEntry{.orig_instr = curr_orig_pc - ARM_PC_OFFSET,
                                           .relocated_instr = curr_relo_pc - ARM_PC_OFFSET,
                                           .relocated_code_len = relo_len};
      relo_map->pushObject(reinterpret_cast<LiteObject *>(map));
    }

    curr_relo_pc = relocated->raw_instruction_start() + ARM_PC_OFFSET + turbo_assembler_->pc_offset();

    // Move to next instruction
    curr_orig_pc += ARM_INST_LEN;
    buffer_cursor += ARM_INST_LEN;

    // execute state changed
    addr32_t next_instr_addr = curr_orig_pc - ARM_PC_OFFSET;
    if (execute_state_changed_pc != 0 && next_instr_addr == execute_state_changed_pc) {
      break;
    }

    instr = *(arm_inst_t *)buffer_cursor;
  }

  // update origin
  int new_origin_len = curr_orig_pc - origin->raw_instruction_start() - ARM_PC_OFFSET;
  origin->re_init_region_range(origin->raw_instruction_start(), new_origin_len);

  bool is_relocate_interrupted = buffer_cursor < ((addr_t)buffer + predefined_relocate_size);
  if (is_relocate_interrupted) {
    *execute_state_changed_pc_ptr = execute_state_changed_pc;
  }
}

void gen_thumb_relocate_code(LiteMutableArray *relo_map, ThumbTurboAssembler *turbo_assembler_, void *buffer,
                             AssemblyCodeChunk *origin, AssemblyCodeChunk *relocated,
                             addr32_t *execute_state_changed_pc_ptr) {
  LiteMutableArray thumb_labels(8);
#define _ turbo_assembler_->

  addr32_t curr_orig_pc = origin->raw_instruction_start() + Thumb_PC_OFFSET;
  addr32_t curr_relo_pc = relocated->raw_instruction_start() + Thumb_PC_OFFSET;

  addr_t buffer_cursor = (addr_t)buffer;
  thumb2_inst_t instr = *(thumb2_inst_t *)buffer_cursor;

  int predefined_relocate_size = origin->raw_instruction_size();
  DLOG(0, "[arm] Thumb relocate %d start >>>>>", predefined_relocate_size);

  addr32_t execute_state_changed_pc = 0;

  while (buffer_cursor < ((addr_t)buffer + predefined_relocate_size)) {
    // align nop
    _ t1_nop();

    int last_relo_offset = turbo_assembler_->GetCodeBuffer()->getSize();
    if (is_thumb2(instr)) {
      Thumb2RelocateSingleInstr(turbo_assembler_, &thumb_labels, (uint16_t)instr, (uint16_t)(instr >> 16), curr_orig_pc,
                                curr_relo_pc);

      DLOG(0, "[arm] Relocate thumb2 instr: 0x%x", instr);
    } else {
      Thumb1RelocateSingleInstr(turbo_assembler_, &thumb_labels, (uint16_t)instr, curr_orig_pc, curr_relo_pc,
                                &execute_state_changed_pc);

      DLOG(0, "[arm] Relocate thumb1 instr: 0x%x", (uint16_t)instr);
    }

    {
      // 1 orignal instrution => ? relocated instruction
      int relo_offset = turbo_assembler_->GetCodeBuffer()->getSize();
      int relo_len = relo_offset - last_relo_offset;

      ReloMapEntry *map = new ReloMapEntry{.orig_instr = curr_orig_pc - Thumb_PC_OFFSET,
                                           .relocated_instr = curr_relo_pc - Thumb_PC_OFFSET,
                                           .relocated_code_len = relo_len};
      relo_map->pushObject(reinterpret_cast<LiteObject *>(map));
    }

    curr_relo_pc = relocated->raw_instruction_start() + Thumb_PC_OFFSET + turbo_assembler_->pc_offset();

    // Move to next instruction
    if (is_thumb2(instr)) {
      curr_orig_pc += Thumb2_INST_LEN;
      buffer_cursor += Thumb2_INST_LEN;
    } else {
      curr_orig_pc += Thumb1_INST_LEN;
      buffer_cursor += Thumb1_INST_LEN;
    }

    // execute state changed
    addr32_t next_instr_addr = curr_orig_pc - Thumb_PC_OFFSET;
    if (execute_state_changed_pc != 0 && next_instr_addr == execute_state_changed_pc) {
      break;
    }

    instr = *(thumb2_inst_t *)buffer_cursor;
  }

  // update origin
  int new_origin_len = curr_orig_pc - origin->raw_instruction_start() - Thumb_PC_OFFSET;
  origin->re_init_region_range(origin->raw_instruction_start(), new_origin_len);

  /*
  .thumb1 bx pc
  .thumb1 mov r8, r8
  .arm ldr pc, [pc, #-4]
   */

  bool is_relocate_interrupted = buffer_cursor < ((addr_t)buffer + predefined_relocate_size);
  if (is_relocate_interrupted) {
    *execute_state_changed_pc_ptr = execute_state_changed_pc;
    turbo_assembler_->SetExecuteState(ARMExecuteState);
  }
}

static addr32_t get_orig_instr_relocated_addr(LiteMutableArray *relo_map, addr32_t orig_pc) {
  for (size_t i = 0; i < relo_map->getCount(); i++) {
    ReloMapEntry *relo_entry = (ReloMapEntry *)relo_map->getObject(i);
    if (relo_entry->orig_instr == orig_pc) {
      return relo_entry->relocated_instr;
    }
  }
  return 0;
}

static void reloc_label_fixup(AssemblyCodeChunk *origin, LiteMutableArray *relo_map,
                              ThumbTurboAssembler *thumb_turbo_assembler, TurboAssembler *arm_turbo_assembler) {
  addr32_t origin_instr_start = origin->raw_instruction_start();
  addr32_t origin_instr_end = origin_instr_start + origin->raw_instruction_size();

  LiteMutableArray *labels = NULL;
  labels = thumb_turbo_assembler->GetLabels();
  if (labels) {
    for (size_t i = 0; i < labels->getCount(); i++) {
      ThumbRelocLabelEntry *label = (ThumbRelocLabelEntry *)labels->getObject(i);
      if (label->used_for_branch() == false)
        continue;
      addr32_t val = label->data();

      if (val >= origin_instr_start && val < origin_instr_end) {
        DLOG(0, "[reloc label fixup warning] found thumb instr branch / access in origin code range");
        addr32_t fixup_val = get_orig_instr_relocated_addr(relo_map, val);
        fixup_val += (addr_t)thumb_turbo_assembler->GetRealizedAddress();
        label->fixup_data(fixup_val);
        thumb_turbo_assembler->RelocBindFixup(label);
      }
    }
  }

  labels = arm_turbo_assembler->GetLabels();
  if (labels) {
    for (size_t i = 0; i < labels->getCount(); i++) {
      RelocLabelEntry *label = (RelocLabelEntry *)labels->getObject(i);
      addr32_t val = label->data();

      if (val >= origin_instr_start && val < origin_instr_end) {
        DLOG(0, "[reloc label fixup warning]found thumb instr branch / access in origin code range");
        addr32_t fixup_val = get_orig_instr_relocated_addr(relo_map, val);
        fixup_val += (addr_t)arm_turbo_assembler->GetRealizedAddress();
        label->fixup_data(fixup_val);
        arm_turbo_assembler->RelocBindFixup(label);
      }
    }
  }
}

void GenRelocateCodeAndBranch(void *buffer, AssemblyCodeChunk *origin, AssemblyCodeChunk *relocated) {
  CodeBuffer *code_buffer = new CodeBuffer(64);

  ThumbTurboAssembler thumb_turbo_assembler_(0, code_buffer);
#define thumb_ thumb_turbo_assembler_.
  TurboAssembler arm_turbo_assembler_(0, code_buffer);
#define arm_ arm_turbo_assembler_.

  Assembler *curr_assembler_ = NULL;

  AssemblyCodeChunk origin_chunk;
  origin_chunk.init_region_range(origin->raw_instruction_start(), origin->raw_instruction_size());

  bool entry_is_thumb = origin->raw_instruction_start() % 2;
  if (entry_is_thumb) {
    origin->re_init_region_range(origin->raw_instruction_start() - THUMB_ADDRESS_FLAG, origin->raw_instruction_size());
  }

  LiteMutableArray relo_map(8);

relocate_remain:
  addr32_t execute_state_changed_pc = 0;

  bool is_thumb = origin_chunk.raw_instruction_start() % 2;
  if (is_thumb) {
    curr_assembler_ = &thumb_turbo_assembler_;

    buffer = (void *)((addr_t)buffer - THUMB_ADDRESS_FLAG);

    addr32_t origin_code_start_aligned = origin_chunk.raw_instruction_start() - THUMB_ADDRESS_FLAG;
    // remove thumb address flag
    origin_chunk.re_init_region_range(origin_code_start_aligned, origin_chunk.raw_instruction_size());

    gen_thumb_relocate_code(&relo_map, &thumb_turbo_assembler_, buffer, &origin_chunk, relocated,
                            &execute_state_changed_pc);
    if (thumb_turbo_assembler_.GetExecuteState() == ARMExecuteState) {
      // relocate interrupt as execute state changed
      if (execute_state_changed_pc < origin_chunk.raw_instruction_start() + origin_chunk.raw_instruction_size()) {
        // re-init the origin
        int relocate_remain_size =
            origin_chunk.raw_instruction_start() + origin_chunk.raw_instruction_size() - execute_state_changed_pc;
        // current execute state is ARMExecuteState, so not need `+ THUMB_ADDRESS_FLAG`
        origin_chunk.re_init_region_range(execute_state_changed_pc, relocate_remain_size);

        // update buffer
        buffer = (void *)((addr_t)buffer + (execute_state_changed_pc - origin_code_start_aligned));

        // add nop to align ARM
        if (thumb_turbo_assembler_.pc_offset() % 4)
          thumb_turbo_assembler_.t1_nop();
        goto relocate_remain;
      }
    }
  } else {
    curr_assembler_ = &arm_turbo_assembler_;

    gen_arm_relocate_code(&relo_map, &arm_turbo_assembler_, buffer, &origin_chunk, relocated,
                          &execute_state_changed_pc);
    if (arm_turbo_assembler_.GetExecuteState() == ThumbExecuteState) {
      // relocate interrupt as execute state changed
      if (execute_state_changed_pc < origin_chunk.raw_instruction_start() + origin_chunk.raw_instruction_size()) {
        // re-init the origin
        int relocate_remain_size =
            origin_chunk.raw_instruction_start() + origin_chunk.raw_instruction_size() - execute_state_changed_pc;
        // current execute state is ThumbExecuteState, add THUMB_ADDRESS_FLAG
        origin_chunk.re_init_region_range(execute_state_changed_pc + THUMB_ADDRESS_FLAG, relocate_remain_size);

        // update buffer
        buffer = (void *)((addr_t)buffer + (execute_state_changed_pc - origin_chunk.raw_instruction_start()));
        goto relocate_remain;
      }
    }
  }

  // TODO:
  // if last instr is unlink branch, skip
  addr32_t rest_instr_addr = origin_chunk.raw_instruction_start() + origin_chunk.raw_instruction_size();
  if (curr_assembler_ == &thumb_turbo_assembler_) {
    // Branch to the rest of instructions
    thumb_ AlignThumbNop();
    thumb_ t2_ldr(pc, MemOperand(pc, 0));
    // Get the real branch address
    thumb_ EmitAddress(rest_instr_addr + THUMB_ADDRESS_FLAG);
  } else {
    // Branch to the rest of instructions
    CodeGen codegen(&arm_turbo_assembler_);
    // Get the real branch address
    codegen.LiteralLdrBranch(rest_instr_addr);
  }

  // Realize all the Pseudo-Label-Data
  thumb_turbo_assembler_.RelocBind();

  // Realize all the Pseudo-Label-Data
  arm_turbo_assembler_.RelocBind();

  // Generate executable code
  {
    // assembler without specific memory address
    AssemblyCodeChunk *cchunk;
    cchunk = MemoryArena::AllocateCodeChunk(code_buffer->getSize());
    if (cchunk == nullptr)
      return;

    thumb_turbo_assembler_.SetRealizedAddress(cchunk->address);
    arm_turbo_assembler_.SetRealizedAddress(cchunk->address);

    // fixup the instr branch into trampoline(has been modified)
    reloc_label_fixup(origin, &relo_map, &thumb_turbo_assembler_, &arm_turbo_assembler_);

    AssemblyCodeChunk *code = NULL;
    code = AssemblyCodeBuilder::FinalizeFromTurboAssembler(curr_assembler_);
    relocated->re_init_region_range(code->raw_instruction_start(), code->raw_instruction_size());
    delete code;
  }

  // thumb
  if (entry_is_thumb) {
    // add thumb address flag
    relocated->re_init_region_range(relocated->raw_instruction_start() + THUMB_ADDRESS_FLAG,
                                    relocated->raw_instruction_size());
  }

  // clean
  {
    thumb_turbo_assembler_.ClearCodeBuffer();
    arm_turbo_assembler_.ClearCodeBuffer();

    delete code_buffer;
  }
}

#endif
