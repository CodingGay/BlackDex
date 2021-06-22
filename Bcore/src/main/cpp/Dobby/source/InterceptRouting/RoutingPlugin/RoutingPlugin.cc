#include "InterceptRouting/RoutingPlugin/RoutingPlugin.h"

LiteMutableArray *RoutingPluginManager::plugins;

RoutingPluginInterface *RoutingPluginManager::near_branch_trampoline = NULL;

void RoutingPluginManager::registerPlugin(const char *name, RoutingPluginInterface *plugin) {
  DLOG(0, "register %s plugin", name);

  if (RoutingPluginManager::plugins == NULL) {
    RoutingPluginManager::plugins = new LiteMutableArray(8);
  }

  RoutingPluginManager::plugins->pushObject(reinterpret_cast<LiteObject *>(plugin));
}
