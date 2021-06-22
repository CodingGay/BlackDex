#ifndef CORE_ASSEMBLER_ARM_H
#define CORE_ASSEMBLER_ARM_H

#include "common_header.h"

#include "core/arch/arm/constants-arm.h"
#include "core/arch/arm/registers-arm.h"
#include "core/modules/assembler/assembler.h"

#include "MemoryAllocator/CodeBuffer/code-buffer-arm.h"

#include "xnucxx/LiteMutableArray.h"
#include "xnucxx/LiteIterator.h"

#include <math.h>

namespace zz {
namespace arm {

// ARM design had a 3-stage pipeline (fetch-decode-execute)
#define ARM_PC_OFFSET 8
#define Thumb_PC_OFFSET 4

// define instruction length
#define ARM_INST_LEN 4
#define Thumb1_INST_LEN 2
#define Thumb2_INST_LEN 4

// Thumb instructions address is odd
#define THUMB_ADDRESS_FLAG 1

constexpr Register TMP_REG_0 = r12;

constexpr Register VOLATILE_REGISTER = r12;

#define Rd(rd) (rd.code() << kRdShift)
#define Rt(rt) (rt.code() << kRtShift)
#define Rn(rn) (rn.code() << kRnShift)
#define Rm(rm) (rm.code() << kRmShift)

// ===== PseudoLabel =====

class PseudoLabel : public Label {
public:
  enum PseudoLabelType { kLdrLiteral };

  typedef struct _PseudoLabelInstruction {
    int position_;
    int type_;
  } PseudoLabelInstruction;

public:
  PseudoLabel(void) : instructions_(8) {
  }

  ~PseudoLabel(void) {
    for (size_t i = 0; i < instructions_.getCount(); i++) {
      PseudoLabelInstruction *item = (PseudoLabelInstruction *)instructions_.getObject(i);
      delete item;
    }
  }

  bool has_confused_instructions() {
    return instructions_.getCount() > 0;
  }

  void link_confused_instructions(CodeBuffer *buffer = nullptr) {
    if (!buffer)
      UNREACHABLE();
    CodeBuffer *_buffer = buffer;

    for (size_t i = 0; i < instructions_.getCount(); i++) {
      PseudoLabelInstruction *instruction = (PseudoLabelInstruction *)instructions_.getObject(i);

      int32_t offset = pos() - instruction->position_;
      const int32_t inst32 = _buffer->LoadARMInst(instruction->position_);
      int32_t encoded = 0;

      switch (instruction->type_) {
      case kLdrLiteral: {
        encoded = inst32 & 0xfffff000;
        uint32_t imm12 = offset - ARM_PC_OFFSET;
        ASSERT(CheckSignLength(imm12));
        encoded = encoded | imm12;
      } break;
      default:
        UNREACHABLE();
        break;
      }
      _buffer->RewriteARMInst(instruction->position_, encoded);
    }
  };

  // compatible for thumb with int type
  void link_to(int pos, int type) {
    PseudoLabelInstruction *instruction = new PseudoLabelInstruction;
    instruction->position_ = pos;
    instruction->type_ = type;
    instructions_.pushObject((LiteObject *)instruction);
  }

protected:
  LiteMutableArray instructions_;
};

// reloc
class RelocLabelEntry : public PseudoLabel {
public:
  explicit RelocLabelEntry(uint32_t data) : data_size_(0) {
    data_ = data;
  }

  uint32_t data() {
    return data_;
  }

  void fixup_data(uint32_t data) {
    data_ = data;
  }

private:
  uint32_t data_;

  int data_size_;
};

// ================================================================
// Operand

class Operand {
  friend class OpEncode;

public:
  Operand(int immediate) : imm_(immediate), rm_(no_reg), shift_(LSL), shift_imm_(0), rs_(no_reg) {
  }

  Operand(Register rm) : imm_(0), rm_(rm), shift_(LSL), shift_imm_(0), rs_(no_reg) {
  }

  Operand(Register rm, Shift shift, uint32_t shift_imm)
      : imm_(0), rm_(rm), shift_(shift), shift_imm_(shift_imm), rs_(no_reg) {
  }

  Operand(Register rm, Shift shift, Register rs) : imm_(0), rm_(rm), shift_(shift), shift_imm_(0), rs_(rs) {
  }

public:
  int GetImmediate() const {
    return imm_;
  }

private:
  Register rm_;
  Register rs_;

  Shift shift_;
  int shift_imm_;

  int imm_;

private:
  friend class EncodeUtility;
};

// ================================================================
// MemOperand

class MemOperand {
  friend class OpEncode;

public:
  MemOperand(Register rn, int32_t offset = 0, AddrMode addrmode = Offset)
      : rn_(rn), offset_(offset), rm_(no_reg), shift_(LSL), shift_imm_(0), addrmode_(addrmode) {
  }

  MemOperand(Register rn, Register rm, AddrMode addrmode = Offset)
      : rn_(rn), offset_(0), rm_(rm), shift_(LSL), shift_imm_(0), addrmode_(addrmode) {
  }

  MemOperand(Register rn, Register rm, Shift shift, uint32_t shift_imm, AddrMode addrmode = Offset)
      : rn_(rn), offset_(0), rm_(rm), shift_(shift), shift_imm_(shift_imm), addrmode_(addrmode) {
  }

  const Register &rn() const {
    return rn_;
  }
  const Register &rm() const {
    return rm_;
  }
  int32_t offset() const {
    return offset_;
  }

  bool IsImmediateOffset() const {
    return (addrmode_ == Offset);
  }
  bool IsRegisterOffset() const {
    return (addrmode_ == Offset);
  }
  bool IsPreIndex() const {
    return addrmode_ == PreIndex;
  }
  bool IsPostIndex() const {
    return addrmode_ == PostIndex;
  }

private:
  Register rn_; // base
  Register rm_; // register offset

  int32_t offset_; // valid if rm_ == no_reg

  Shift shift_;
  uint32_t shift_imm_; // valid if rm_ != no_reg && rs_ == no_reg

  AddrMode addrmode_; // bits P, U, and W
};

class OpEncode {
public:
  static uint32_t MemOperand(const MemOperand operand) {
    uint32_t encoding = 0;
    if (operand.rm_.IsValid()) {
      UNREACHABLE();
    }

    // sign
    uint32_t U = 0;
    if (operand.offset_ >= 0) {
      U = (1 << 23);
    }
    encoding |= U;

    // offset
    encoding |= bits(abs(operand.offset_), 0, 11);

    // addr mode
    uint32_t P, W;
    if (operand.addrmode_ == Offset) {
      P = 1;
      W = 0;
    } else if (operand.addrmode_ == PostIndex) {
      P = 0;
      W = 0;
    } else if (operand.addrmode_ == PreIndex) {
      P = 1;
      W = 1;
    }
    encoding |= ((P << 24) | (W << 21));

    // rn
    encoding |= Rn(operand.rn_);

    return encoding;
  }

  static uint32_t Operand(const Operand operand) {
    uint32_t encoding = 0;
    if (operand.rm_.IsValid()) {
      encoding = static_cast<uint32_t>(operand.rm_.code());
    } else {
      encoding = operand.GetImmediate();
    }

    return encoding;
  }
};

// ================================================================
// Assembler

enum ExecuteState { ARMExecuteState, ThumbExecuteState };

class Assembler : public AssemblerBase {
private:
  ExecuteState execute_state_;

public:
  Assembler(void *address) : AssemblerBase(address) {
    execute_state_ = ARMExecuteState;
    buffer_ = new CodeBuffer(64);
  }

  // shared_ptr is better choice
  // but we can't use it at kernelspace
  Assembler(void *address, CodeBuffer *buffer) : AssemblerBase(address) {
    execute_state_ = ARMExecuteState;
    buffer_ = buffer;
  }

  void ClearCodeBuffer() {
    buffer_ = NULL;
  }

public:
  void SetExecuteState(ExecuteState state) {
    execute_state_ = state;
  }
  ExecuteState GetExecuteState() {
    return execute_state_;
  }

  void SetRealizedAddress(void *address) {
    DCHECK_EQ(0, reinterpret_cast<uint64_t>(address) % 4);
    AssemblerBase::SetRealizedAddress(address);
  }

  void EmitARMInst(arm_inst_t instr);

  void EmitAddress(uint32_t value);

public:
  void sub(Register rd, Register rn, const Operand &operand) {
    uint32_t encoding = B25 | B22;
    add_sub(encoding, AL, rd, rn, operand);
  }

  void add(Register rd, Register rn, const Operand &operand) {
    uint32_t encoding = B25 | B23;
    add_sub(encoding, AL, rd, rn, operand);
  }

  void add_sub(uint32_t encoding, Condition cond, Register rd, Register rn, const Operand &operand) {
    encoding |= (cond << kConditionShift);

    uint32_t imm = operand.GetImmediate();
    encoding |= imm;

    encoding |= Rd(rd);

    encoding |= Rn(rn);

    buffer_->EmitARMInst(encoding);
  }

  void ldr(Register rt, const MemOperand &operand) {
    uint32_t encoding = B20 | B26;
    load_store(encoding, AL, rt, operand);
  }

  void str(Register rt, const MemOperand &operand) {
    uint32_t encoding = B26;
    load_store(encoding, AL, rt, operand);
  }

  void load_store(uint32_t encoding, Condition cond, Register rt, const MemOperand &operand) {
    encoding |= (cond << kConditionShift);
    encoding |= Rt(rt) | OpEncode::MemOperand(operand);
    buffer_->EmitARMInst(encoding);
  }

  void mov(Register rd, const Operand &operand) {
    mov(AL, rd, operand);
  }

  void mov(Condition cond, Register rd, const Operand &operand) {
    uint32_t encoding = 0x01a00000;
    encoding |= (cond << kConditionShift);
    encoding |= Rd(rd) | OpEncode::Operand(operand);
    buffer_->EmitARMInst(encoding);
  }

  // Branch instructions.
  void b(int branch_offset) {
    b(AL, branch_offset);
  }
  void b(Condition cond, int branch_offset) {
    uint32_t encoding = 0xa000000;
    encoding |= (cond << kConditionShift);
    uint32_t imm24 = bits(branch_offset >> 2, 0, 23);
    encoding |= imm24;
    buffer_->EmitARMInst(encoding);
  }

  void bl(int branch_offset) {
    bl(AL, branch_offset);
  }
  void bl(Condition cond, int branch_offset) {
    uint32_t encoding = 0xb000000;
    encoding |= (cond << kConditionShift);
    uint32_t imm24 = bits(branch_offset >> 2, 0, 23);
    encoding |= imm24;
    buffer_->EmitARMInst(encoding);
  }

  void blx(int branch_offset) {
    UNIMPLEMENTED();
  }
  void blx(Register target, Condition cond = AL) {
    UNIMPLEMENTED();
  }
  void bx(Register target, Condition cond = AL) {
    UNIMPLEMENTED();
  }

}; // namespace arm

// ================================================================
// TurboAssembler

class TurboAssembler : public Assembler {
public:
  TurboAssembler(void *address) : Assembler(address) {
    data_labels_ = NULL;
  }

  ~TurboAssembler() {
    if (data_labels_) {
      for (size_t i = 0; i < data_labels_->getCount(); i++) {
        RelocLabelEntry *label = (RelocLabelEntry *)data_labels_->getObject(i);
        delete label;
      }

      delete data_labels_;
    }
  }

  TurboAssembler(void *address, CodeBuffer *buffer) : Assembler(address, buffer) {
    data_labels_ = NULL;
  }

  void Ldr(Register rt, PseudoLabel *label) {
    if (label->is_bound()) {
      int offset = label->pos() - buffer_->getSize();
      ldr(rt, MemOperand(pc, offset));
    } else {
      // record this ldr, and fix later.
      label->link_to(buffer_->getSize(), PseudoLabel::kLdrLiteral);
      ldr(rt, MemOperand(pc, 0));
    }
  }

  void CallFunction(ExternalReference function) {
    // trick: use bl to replace lr register
    bl(0);
    b(4);
    ldr(pc, MemOperand(pc, -4));
    buffer_->Emit32((uint32_t)function.address());
  }

  void Move32Immeidate(Register rd, const Operand &x, Condition cond = AL) {
  }

  // ================================================================
  // RelocLabelEntry

  void PseudoBind(PseudoLabel *label) {
    if (label->is_unused() == true) {
      const uint32_t bound_pc = buffer_->getSize();
      label->bind_to(bound_pc);
    }
    // If some instructions have been wrote, before the label bound, we need link these `confused` instructions
    if (label->has_confused_instructions()) {
      label->link_confused_instructions(this->GetCodeBuffer());
    }
  }

  void RelocBindFixup(RelocLabelEntry *label) {
    buffer_->RewriteAddr(label->pos(), label->data());
  }

  void RelocBind() {
    if (data_labels_ == NULL)
      return;
    for (size_t i = 0; i < data_labels_->getCount(); i++) {
      RelocLabelEntry *label = (RelocLabelEntry *)data_labels_->getObject(i);
      PseudoBind(label);
      EmitAddress(label->data());
    }
  }

  void AppendRelocLabelEntry(RelocLabelEntry *label) {
    if (data_labels_ == NULL) {
      data_labels_ = new LiteMutableArray(8);
    }
    data_labels_->pushObject((LiteObject *)label);
  }

  LiteMutableArray *GetLabels() {
    return data_labels_;
  }

private:
  LiteMutableArray *data_labels_;
};

} // namespace arm
} // namespace zz

#endif
