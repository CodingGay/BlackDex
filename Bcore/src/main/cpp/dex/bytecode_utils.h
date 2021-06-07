/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ART_LIBDEXFILE_DEX_BYTECODE_UTILS_H_
#define ART_LIBDEXFILE_DEX_BYTECODE_UTILS_H_

#include "base/value_object.h"
#include "dex/dex_file-inl.h"
#include "dex/dex_file.h"
#include "dex/dex_instruction-inl.h"

namespace art_lkchan {

class DexSwitchTable : public ValueObject {
 public:
  DexSwitchTable(const Instruction& instruction, uint32_t dex_pc)
      : instruction_(instruction),
        dex_pc_(dex_pc),
        sparse_(instruction.Opcode() == Instruction::SPARSE_SWITCH) {
    int32_t table_offset = instruction.VRegB_31t();
    const uint16_t* table = reinterpret_cast<const uint16_t*>(&instruction) + table_offset;
    DCHECK_EQ(table[0], sparse_ ? static_cast<uint16_t>(Instruction::kSparseSwitchSignature)
                                : static_cast<uint16_t>(Instruction::kPackedSwitchSignature));
    num_entries_ = table[1];
    values_ = reinterpret_cast<const int32_t*>(&table[2]);
  }

  uint16_t GetNumEntries() const {
    return num_entries_;
  }

  void CheckIndex(size_t index) const {
    if (sparse_) {
      // In a sparse table, we have num_entries_ keys and num_entries_ values, in that order.
      DCHECK_LT(index, 2 * static_cast<size_t>(num_entries_));
    } else {
      // In a packed table, we have the starting key and num_entries_ values.
      DCHECK_LT(index, 1 + static_cast<size_t>(num_entries_));
    }
  }

  int32_t GetEntryAt(size_t index) const {
    CheckIndex(index);
    return values_[index];
  }

  uint32_t GetDexPcForIndex(size_t index) const {
    CheckIndex(index);
    return dex_pc_ +
        (reinterpret_cast<const int16_t*>(values_ + index) -
         reinterpret_cast<const int16_t*>(&instruction_));
  }

  // Index of the first value in the table.
  size_t GetFirstValueIndex() const {
    if (sparse_) {
      // In a sparse table, we have num_entries_ keys and num_entries_ values, in that order.
      return num_entries_;
    } else {
      // In a packed table, we have the starting key and num_entries_ values.
      return 1;
    }
  }

  bool IsSparse() const { return sparse_; }

  bool ShouldBuildDecisionTree() {
    return IsSparse() || GetNumEntries() <= kSmallSwitchThreshold;
  }

 private:
  const Instruction& instruction_;
  const uint32_t dex_pc_;

  // Whether this is a sparse-switch table (or a packed-switch one).
  const bool sparse_;

  // This can't be const as it needs to be computed off of the given instruction, and complicated
  // expressions in the initializer list seemed very ugly.
  uint16_t num_entries_;

  const int32_t* values_;

  // The number of entries in a packed switch before we use a jump table or specified
  // compare/jump series.
  static constexpr uint16_t kSmallSwitchThreshold = 3;

  DISALLOW_COPY_AND_ASSIGN(DexSwitchTable);
};

class DexSwitchTableIterator {
 public:
  explicit DexSwitchTableIterator(const DexSwitchTable& table)
      : table_(table),
        num_entries_(static_cast<size_t>(table_.GetNumEntries())),
        first_target_offset_(table_.GetFirstValueIndex()),
        index_(0u) {}

  bool Done() const { return index_ >= num_entries_; }
  bool IsLast() const { return index_ == num_entries_ - 1; }

  void Advance() {
    DCHECK(!Done());
    index_++;
  }

  int32_t CurrentKey() const {
    return table_.IsSparse() ? table_.GetEntryAt(index_) : table_.GetEntryAt(0) + index_;
  }

  int32_t CurrentTargetOffset() const {
    return table_.GetEntryAt(index_ + first_target_offset_);
  }

  uint32_t GetDexPcForCurrentIndex() const { return table_.GetDexPcForIndex(index_); }

 private:
  const DexSwitchTable& table_;
  const size_t num_entries_;
  const size_t first_target_offset_;

  size_t index_;
};

inline bool IsThrowingDexInstruction(const Instruction& instruction) {
  // Special-case MONITOR_EXIT which is a throwing instruction but the verifier
  // guarantees that it will never throw. This is necessary to avoid rejecting
  // 'synchronized' blocks/methods.
  return instruction.IsThrow() && instruction.Opcode() != Instruction::MONITOR_EXIT;
}

}  // namespace art_lkchan

#endif  // ART_LIBDEXFILE_DEX_BYTECODE_UTILS_H_
