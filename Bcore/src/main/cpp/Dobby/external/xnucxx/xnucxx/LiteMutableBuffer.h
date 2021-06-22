#ifndef LITE_MUTABLE_BUFFER_H
#define LITE_MUTABLE_BUFFER_H

#include "xnucxx/LiteObject.h"

class LiteMutableBuffer : public LiteObject {
protected:
  uint8_t *buffer;

  uint8_t *buffer_cursor;

  uint32_t buffer_capacity;

public:
  LiteMutableBuffer() {
    initWithCapacity(8);
  }

  LiteMutableBuffer(uint32_t size) {
    initWithCapacity(size);
  }

  ~LiteMutableBuffer() {
    release();
  }

  // === LiteObject override ==
  void release() override;

  virtual bool initWithCapacity(uint32_t initCapacity);

  virtual uint32_t ensureCapacity(uint32_t newCapacity);

  virtual inline uint32_t getSize() {
    return (uint32_t)(buffer_cursor - buffer);
  }

  virtual inline uint32_t getCapacity() {
    return buffer_capacity;
  }

  virtual inline void *getCursor() {
    return buffer_cursor;
  }

  virtual inline void *getRawBuffer() {
    return buffer;
  }
};

#endif
