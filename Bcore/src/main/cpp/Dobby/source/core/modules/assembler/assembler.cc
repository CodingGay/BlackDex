#include "core/modules/assembler/assembler.h"
#include "logging/logging.h"

namespace zz {

// ===== Label =====

bool Label::is_bound() const {
  return pos_ < 0;
}
bool Label::is_unused() const {
  return pos_ == 0 && near_link_pos_ == 0;
}
bool Label::is_linked() const {
  return pos_ > 0;
}
bool Label::is_near_linked() const {
  return near_link_pos_ > 0;
}
int Label::pos() const {
  if (pos_ < 0)
    return -pos_ - 1;
  if (pos_ > 0)
    return pos_ - 1;
  return 0;
}
void Label::bind_to(int pos) {
  pos_ = -pos - 1;
}
void Label::link_to(int pos) {
  // for special condition: link_to(0)
  pos_ = pos + 1;
}

const void *ExternalReference::address() {
  return address_;
}

AssemblerBase::AssemblerBase(void *address) {
  realized_address_ = address;

  buffer_ = NULL;

  if (realized_address_ != NULL) {
    DLOG(0, "[assembler] Create fixed address at %p", realized_address_);
  }
}

AssemblerBase::~AssemblerBase() {
  buffer_ = NULL;
}

// TODO: mov to x64
int AssemblerBase::ip_offset() const {
  return reinterpret_cast<CodeBufferBase *>(buffer_)->getSize();
}

// TODO: mov to arm / arm64
int AssemblerBase::pc_offset() const {
  return reinterpret_cast<CodeBufferBase *>(buffer_)->getSize();
}

CodeBuffer *AssemblerBase::GetCodeBuffer() {
  return buffer_;
}

void AssemblerBase::SetRealizedAddress(void *address) {
  realized_address_ = address;
}

void *AssemblerBase::GetRealizedAddress() {
  return realized_address_;
}

void AssemblerBase::FlushICache(addr_t start, int size) {
}

void AssemblerBase::FlushICache(addr_t start, addr_t end) {
}

} // namespace zz
