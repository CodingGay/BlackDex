#ifndef DOBBY_SYMBOL_RESOLVER_H
#define DOBBY_SYMBOL_RESOLVER_H

#ifdef __cplusplus
extern "C" {
#endif

void *DobbySymbolResolver(const char *image_name, const char *symbol_name);

#ifdef __cplusplus
}
#endif

#endif