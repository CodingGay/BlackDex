#include "Interceptor.h"

#include "dobby_internal.h"

Interceptor *Interceptor::priv_interceptor_ = nullptr;

Interceptor *Interceptor::SharedInstance() {
  if (Interceptor::priv_interceptor_ == nullptr) {
    Interceptor::priv_interceptor_ = new Interceptor();
    INIT_LIST_HEAD(&Interceptor::priv_interceptor_->hook_entry_list_);
  }
  return Interceptor::priv_interceptor_;
}

HookEntryNode *Interceptor::find_hook_entry_node(void *address) {
  HookEntryNode *entry_node = nullptr;
#if defined(_MSC_VER)
#if 0 // only valid if offsetof(HookEntryNode, list_node) == 0
  for(entry_node = (HookEntryNode *)hook_entry_list_.next; &entry_node->list_node != &hook_entry_list_; entry_node = (HookEntryNode *)entry_node->list_node.next);
#endif
  struct list_head *list_node = nullptr;
  for(list_node = hook_entry_list_.next; list_node != &hook_entry_list_; list_node = list_node->next) {
    entry_node = (HookEntryNode *)((char *)list_node - offsetof(HookEntryNode, list_node));
#else
    list_for_each_entry(entry_node, &hook_entry_list_, list_node) {
#endif
    HookEntry *entry = entry_node->entry;
    if (entry->instruction_address == address) {
      return entry_node;
    }
  }
  return nullptr;
}

HookEntry *Interceptor::FindHookEntry(void *address) {
  HookEntryNode *entry_node = nullptr;
  entry_node = find_hook_entry_node(address);
  if (entry_node) {
    return entry_node->entry;
  }

  return nullptr;
}

void Interceptor::AddHookEntry(HookEntry *entry) {
  HookEntryNode *entry_node = new HookEntryNode;
  entry_node->entry = entry;

  list_add(&entry_node->list_node, &hook_entry_list_);
}

void Interceptor::RemoveHookEntry(void *address) {
  if (HookEntryNode *entry_node = find_hook_entry_node(address)) {
    list_del(&entry_node->list_node);
  }
}

int Interceptor::GetHookEntryCount() {
  int count = 0;
  HookEntryNode *entry_node = nullptr;
#if defined(_MSC_VER)
  struct list_head *list_node = nullptr;
  for(list_node = hook_entry_list_.next; list_node != &hook_entry_list_; list_node = list_node->next) {
    entry_node = (HookEntryNode *)((char *)list_node - offsetof(HookEntryNode, list_node));
#else
    list_for_each_entry(entry_node, &hook_entry_list_, list_node) {
#endif
    count += 1;
  }
  return count;
}
