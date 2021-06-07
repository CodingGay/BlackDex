#ifndef DLFCN_H
#define DLFCN_H

void *fake_dlopen(const char *libpath, int flags);

void *fake_dlsym(void *handle, const char *name);

int fake_dlclose(void *handle);

#endif