/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_LIBDEXFILE_DEX_DEX_INSTRUCTION_UTILS_H_
#define ART_LIBDEXFILE_DEX_DEX_INSTRUCTION_UTILS_H_

#include "dex_instruction.h"

namespace art_lkchan {

// Dex invoke type corresponds to the ordering of INVOKE instructions;
// this order is the same for range and non-range invokes.
enum DexInvokeType : uint8_t {
  kDexInvokeVirtual = 0,  // invoke-virtual, invoke-virtual-range
  kDexInvokeSuper,        // invoke-super, invoke-super-range
  kDexInvokeDirect,       // invoke-direct, invoke-direct-range
  kDexInvokeStatic,       // invoke-static, invoke-static-range
  kDexInvokeInterface,    // invoke-interface, invoke-interface-range
  kDexInvokeTypeCount
};

// Dex instruction memory access types correspond to the ordering of GET/PUT instructions;
// this order is the same for IGET, IPUT, SGET, SPUT, AGET and APUT.
enum DexMemAccessType : uint8_t {
  kDexMemAccessWord = 0,  // op         0; int or float, the actual type is not encoded.
  kDexMemAccessWide,      // op_WIDE    1; long or double, the actual type is not encoded.
  kDexMemAccessObject,    // op_OBJECT  2; the actual reference type is not encoded.
  kDexMemAccessBoolean,   // op_BOOLEAN 3
  kDexMemAccessByte,      // op_BYTE    4
  kDexMemAccessChar,      // op_CHAR    5
  kDexMemAccessShort,     // op_SHORT   6
  kDexMemAccessTypeCount
};

std::ostream& operator<<(std::ostream& os, const DexMemAccessType& type);

// NOTE: The following functions disregard quickened instructions.

// By "direct" const we mean to exclude const-string and const-class
// which load data from somewhere else, i.e. indirectly.
constexpr bool IsInstructionDirectConst(Instruction::Code opcode) {
  return Instruction::CONST_4 <= opcode && opcode <= Instruction::CONST_WIDE_HIGH16;
}

constexpr bool IsInstructionConstWide(Instruction::Code opcode) {
  return Instruction::CONST_WIDE_16 <= opcode && opcode <= Instruction::CONST_WIDE_HIGH16;
}

constexpr bool IsInstructionReturn(Instruction::Code opcode) {
  return Instruction::RETURN_VOID <= opcode && opcode <= Instruction::RETURN_OBJECT;
}

constexpr bool IsInstructionInvoke(Instruction::Code opcode) {
  return Instruction::INVOKE_VIRTUAL <= opcode && opcode <= Instruction::INVOKE_INTERFACE_RANGE &&
      opcode != Instruction::RETURN_VOID_NO_BARRIER;
}

constexpr bool IsInstructionQuickInvoke(Instruction::Code opcode) {
  return opcode == Instruction::INVOKE_VIRTUAL_QUICK ||
      opcode == Instruction::INVOKE_VIRTUAL_RANGE_QUICK;
}

constexpr bool IsInstructionInvokeStatic(Instruction::Code opcode) {
  return opcode == Instruction::INVOKE_STATIC || opcode == Instruction::INVOKE_STATIC_RANGE;
}

constexpr bool IsInstructionGoto(Instruction::Code opcode) {
  return Instruction::GOTO <= opcode && opcode <= Instruction::GOTO_32;
}

constexpr bool IsInstructionIfCc(Instruction::Code opcode) {
  return Instruction::IF_EQ <= opcode && opcode <= Instruction::IF_LE;
}

constexpr bool IsInstructionIfCcZ(Instruction::Code opcode) {
  return Instruction::IF_EQZ <= opcode && opcode <= Instruction::IF_LEZ;
}

constexpr bool IsInstructionIGet(Instruction::Code code) {
  return Instruction::IGET <= code && code <= Instruction::IGET_SHORT;
}

constexpr bool IsInstructionIPut(Instruction::Code code) {
  return Instruction::IPUT <= code && code <= Instruction::IPUT_SHORT;
}

constexpr bool IsInstructionSGet(Instruction::Code code) {
  return Instruction::SGET <= code && code <= Instruction::SGET_SHORT;
}

constexpr bool IsInstructionSPut(Instruction::Code code) {
  return Instruction::SPUT <= code && code <= Instruction::SPUT_SHORT;
}

constexpr bool IsInstructionAGet(Instruction::Code code) {
  return Instruction::AGET <= code && code <= Instruction::AGET_SHORT;
}

constexpr bool IsInstructionAPut(Instruction::Code code) {
  return Instruction::APUT <= code && code <= Instruction::APUT_SHORT;
}

constexpr bool IsInstructionIGetOrIPut(Instruction::Code code) {
  return Instruction::IGET <= code && code <= Instruction::IPUT_SHORT;
}

constexpr bool IsInstructionIGetQuickOrIPutQuick(Instruction::Code code) {
  return (code >= Instruction::IGET_QUICK && code <= Instruction::IPUT_OBJECT_QUICK) ||
      (code >= Instruction::IPUT_BOOLEAN_QUICK && code <= Instruction::IGET_SHORT_QUICK);
}

constexpr bool IsInstructionSGetOrSPut(Instruction::Code code) {
  return Instruction::SGET <= code && code <= Instruction::SPUT_SHORT;
}

constexpr bool IsInstructionAGetOrAPut(Instruction::Code code) {
  return Instruction::AGET <= code && code <= Instruction::APUT_SHORT;
}

constexpr bool IsInstructionBinOp2Addr(Instruction::Code code) {
  return Instruction::ADD_INT_2ADDR <= code && code <= Instruction::REM_DOUBLE_2ADDR;
}

constexpr bool IsInvokeInstructionRange(Instruction::Code opcode) {
  DCHECK(IsInstructionInvoke(opcode));
  return opcode >= Instruction::INVOKE_VIRTUAL_RANGE;
}

constexpr DexInvokeType InvokeInstructionType(Instruction::Code opcode) {
  DCHECK(IsInstructionInvoke(opcode));
  return static_cast<DexInvokeType>(IsInvokeInstructionRange(opcode)
                                    ? (opcode - Instruction::INVOKE_VIRTUAL_RANGE)
                                    : (opcode - Instruction::INVOKE_VIRTUAL));
}

constexpr DexMemAccessType IGetMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionIGet(code));
  return static_cast<DexMemAccessType>(code - Instruction::IGET);
}

constexpr DexMemAccessType IPutMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionIPut(code));
  return static_cast<DexMemAccessType>(code - Instruction::IPUT);
}

constexpr DexMemAccessType SGetMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionSGet(code));
  return static_cast<DexMemAccessType>(code - Instruction::SGET);
}

constexpr DexMemAccessType SPutMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionSPut(code));
  return static_cast<DexMemAccessType>(code - Instruction::SPUT);
}

constexpr DexMemAccessType AGetMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionAGet(code));
  return static_cast<DexMemAccessType>(code - Instruction::AGET);
}

constexpr DexMemAccessType APutMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionAPut(code));
  return static_cast<DexMemAccessType>(code - Instruction::APUT);
}

constexpr DexMemAccessType IGetOrIPutMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionIGetOrIPut(code));
  return (code >= Instruction::IPUT) ? IPutMemAccessType(code) : IGetMemAccessType(code);
}

inline DexMemAccessType IGetQuickOrIPutQuickMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionIGetQuickOrIPutQuick(code));
  switch (code) {
    case Instruction::IGET_QUICK: case Instruction::IPUT_QUICK:
      return kDexMemAccessWord;
    case Instruction::IGET_WIDE_QUICK: case Instruction::IPUT_WIDE_QUICK:
      return kDexMemAccessWide;
    case Instruction::IGET_OBJECT_QUICK: case Instruction::IPUT_OBJECT_QUICK:
      return kDexMemAccessObject;
    case Instruction::IGET_BOOLEAN_QUICK: case Instruction::IPUT_BOOLEAN_QUICK:
      return kDexMemAccessBoolean;
    case Instruction::IGET_BYTE_QUICK: case Instruction::IPUT_BYTE_QUICK:
      return kDexMemAccessByte;
    case Instruction::IGET_CHAR_QUICK: case Instruction::IPUT_CHAR_QUICK:
      return kDexMemAccessChar;
    case Instruction::IGET_SHORT_QUICK: case Instruction::IPUT_SHORT_QUICK:
      return kDexMemAccessShort;
    default:
      LOG(FATAL) << code;
      UNREACHABLE();
  }
}

constexpr DexMemAccessType SGetOrSPutMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionSGetOrSPut(code));
  return (code >= Instruction::SPUT) ? SPutMemAccessType(code) : SGetMemAccessType(code);
}

constexpr DexMemAccessType AGetOrAPutMemAccessType(Instruction::Code code) {
  DCHECK(IsInstructionAGetOrAPut(code));
  return (code >= Instruction::APUT) ? APutMemAccessType(code) : AGetMemAccessType(code);
}

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_DEX_INSTRUCTION_UTILS_H_
