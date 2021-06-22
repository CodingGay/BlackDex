#ifndef LITE_MEM_OPT_H
#define LITE_MEM_OPT_H

extern void *_memcpy(void *, const void *, int);

extern void _bzero(void *, int);

extern void *_memset(void *, int, int);

class LiteMemOpt {
public:
  static void *alloc(int size);

  static void free(void *address, int size);
};

#endif
