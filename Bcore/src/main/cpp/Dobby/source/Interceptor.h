#ifndef INTERCEPTOR_H
#define INTERCEPTOR_H

#include "dobby_internal.h"

#include "include/list_structure.h"

typedef struct {
  struct list_head list_node;
  HookEntry *entry;
} HookEntryNode;

class Interceptor {
public:
  static Interceptor *SharedInstance();

  HookEntry *FindHookEntry(void *address);

  void AddHookEntry(HookEntry *entry);

  void RemoveHookEntry(void *address);

  int GetHookEntryCount();

private:
  Interceptor() {
  }

  HookEntryNode *find_hook_entry_node(void *address);

private:
  struct list_head hook_entry_list_;

  static Interceptor *priv_interceptor_;
};

#endif
