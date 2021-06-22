#include "xnucxx/LiteMemOpt.h"
#include <stdlib.h>
#include <string.h>

#if 1
void *_memcpy(void *dest, const void *src, int len) {
  return memcpy(dest, src, len);
}

void *_memset(void *dest, int ch, int count) {
  return memset(dest, ch, count);
}
#endif

void *LiteMemOpt::alloc(int size) {
  void *result = malloc(size);
  return result;
}

void LiteMemOpt::free(void *address, int size) {
  return ::free(address);
}
