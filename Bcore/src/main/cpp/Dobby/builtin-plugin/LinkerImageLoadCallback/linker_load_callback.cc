#include "dobby.h"
#include "common_header.h"

#include <dlfcn.h>

#include <vector>

#define LOG_TAG "LinkerLoadCallback"

std::vector<linker_load_callback_t> *linker_load_callback_array;

static void *(*orig_dlopen)(const char *__file, int __mode);
static void *fake_dlopen(const char *__file, int __mode) {
  void *result = orig_dlopen(__file, __mode);
  if (result != NULL && __file) {
    for (auto &callback : *linker_load_callback_array) {
      callback(__file, result);
    }
  }
  return result;
}

static void *(*orig_loader_dlopen)(const char *filename, int flags, const void *caller_addr);
static void *fake_loader_dlopen(const char *filename, int flags, const void *caller_addr) {
  void *result = orig_loader_dlopen(filename, flags, caller_addr);
  if (result != NULL) {
    for (auto &callback : *linker_load_callback_array) {
      callback(filename, result);
    }
  }
  return result;
}

PUBLIC void dobby_register_image_load_callback(linker_load_callback_t func) {
  if (linker_load_callback_array == NULL)
    linker_load_callback_array = new std::vector<linker_load_callback_t>();
  linker_load_callback_array->push_back(func);
}

#if defined(DOBBY_DEBUG) && 1
static void monitor_linker_load(const char *image_name, void *handle) {
  LOG(1, "load %s at %p", image_name, handle);
}
#endif

__attribute__((constructor)) static void ctor() {
  if (linker_load_callback_array == NULL)
    linker_load_callback_array = new std::vector<linker_load_callback_t>();

#if defined(__ANDROID__)
  void *__loader_dlopen = DobbySymbolResolver(NULL, "__loader_dlopen");
  LOG(1, "__loader_dlopen: %p", __loader_dlopen);
  DobbyHook(__loader_dlopen, (void *)fake_loader_dlopen, (void **)&orig_loader_dlopen);
#else
  DobbyHook((void *)DobbySymbolResolver(NULL, "dlopen"), (void *)fake_dlopen, (void **)&orig_dlopen);
#endif

#if defined(DOBBY_DEBUG) && 1
  dobby_register_image_load_callback(monitor_linker_load);
#endif
}
