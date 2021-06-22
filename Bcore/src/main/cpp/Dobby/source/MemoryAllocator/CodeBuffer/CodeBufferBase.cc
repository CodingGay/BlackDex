#include "MemoryAllocator/CodeBuffer/CodeBufferBase.h"

#include "xnucxx/LiteMemOpt.h"

CodeBufferBase *CodeBufferBase::Copy() {
  CodeBufferBase *result = new CodeBufferBase(this->getCapacity());
  result->EmitBuffer(this->getRawBuffer(), this->getSize());
  return result;
}

void CodeBufferBase::Emit8(uint8_t value) {
  // Ensure the free space enough for the template T value
  this->ensureCapacity(sizeof(uint8_t) + this->getSize());

  *reinterpret_cast<uint8_t *>(buffer_cursor) = value;
  buffer_cursor += sizeof(uint8_t);
}

void CodeBufferBase::Emit16(uint16_t value) {
  // Ensure the free space enough for the template T value
  this->ensureCapacity(sizeof(uint16_t) + this->getSize());

  *reinterpret_cast<uint16_t *>(buffer_cursor) = value;
  buffer_cursor += sizeof(uint16_t);
}

void CodeBufferBase::Emit32(uint32_t value) {
  // Ensure the free space enough for the template T value
  this->ensureCapacity(sizeof(uint32_t) + this->getSize());

  *reinterpret_cast<uint32_t *>(buffer_cursor) = value;
  buffer_cursor += sizeof(uint32_t);
}

void CodeBufferBase::Emit64(uint64_t value) {
  // Ensure the free space enough for the template T value
  this->ensureCapacity(sizeof(uint64_t) + this->getSize());

  *reinterpret_cast<uint64_t *>(buffer_cursor) = value;
  buffer_cursor += sizeof(uint64_t);
}

void CodeBufferBase::EmitBuffer(void *buffer, int buffer_size) {
  // Ensure the free space enough for the template T value
  this->ensureCapacity(buffer_size + this->getSize());

  _memcpy(buffer_cursor, buffer, buffer_size);

  buffer_cursor += buffer_size;
}

#if 0 // Template Advanced won't enable even in userspace
template <typename T> T CodeBufferBase::Load(int offset) {
  return *reinterpret_cast<T *>(buffer + offset);
}

template <typename T> void CodeBufferBase::Store(int offset, T value) {
  *reinterpret_cast<T *>(buffer + offset) = value;
}

template <typename T> void CodeBufferBase::Emit(T value) {
  // Ensure the free space enough for the template T value
  ensureCapacity(sizeof(T) + getSize());

  *reinterpret_cast<T *>(buffer_cursor) = value;
  buffer_cursor += sizeof(T);
}
#endif
