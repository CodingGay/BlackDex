#include "xnucxx/LiteMutableBuffer.h"
#include "xnucxx/LiteMemOpt.h"

bool LiteMutableBuffer::initWithCapacity(uint32_t initCapacity) {
  if (initCapacity <= 0)
    return false;

  this->buffer = (uint8_t *)LiteMemOpt::alloc(initCapacity);
  this->buffer_cursor = buffer;
  this->buffer_capacity = initCapacity;
  return true;
}

uint32_t LiteMutableBuffer::ensureCapacity(uint32_t newCapacity) {
  if (newCapacity <= buffer_capacity)
    return buffer_capacity;

  // or use step
  newCapacity = newCapacity + (uint32_t)newCapacity / 2;

  // alloc new buffer
  uint8_t *newBuffer;
  newBuffer = (uint8_t *)LiteMemOpt::alloc(newCapacity);
  if (newBuffer == nullptr) {
    return 0;
  }

  // clear buffer content
  _memset(newBuffer, 'A', newCapacity);

  // copy the origin content
  uint32_t originContentSize = (uint32_t)(buffer_cursor - buffer);
  _memcpy(newBuffer, buffer, originContentSize);

  // free the origin
  uint32_t originBufferSize = buffer_capacity;
  LiteMemOpt::free(buffer, originBufferSize);

  // update info
  this->buffer = newBuffer;
  this->buffer_cursor = newBuffer + originContentSize;
  this->buffer_capacity = newCapacity;

  return newCapacity;
}

void LiteMutableBuffer::release() {
  if (buffer != NULL) {
    uint32_t originBufferSize = buffer_capacity;
    LiteMemOpt::free(buffer, originBufferSize);

    buffer = NULL;
  }
}