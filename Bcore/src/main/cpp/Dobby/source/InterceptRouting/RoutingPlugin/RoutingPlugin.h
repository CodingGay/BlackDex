#ifndef EXTRA_INTERNAL_PLUGIN_H
#define EXTRA_INTERNAL_PLUGIN_H

#include "dobby_internal.h"

#include "InterceptRouting/InterceptRouting.h"

class RoutingPluginInterface {
public:
  // @Return: if false will continue to iter next plugin
  virtual bool Prepare(InterceptRouting *routing) = 0;

  // @Return: if false will continue to iter next plugin
  virtual bool Active(InterceptRouting *routing) = 0;

  // @Return: if false will continue to iter next plugin
  virtual bool GenerateTrampolineBuffer(InterceptRouting *routing, void *src, void *dst) = 0;

private:
  char name_[256];
};

class RoutingPluginManager {
public:
  static void registerPlugin(const char *name, RoutingPluginInterface *plugin);

public:
  // global plugin array
  static LiteMutableArray *plugins;

  static RoutingPluginInterface *near_branch_trampoline;
};



#endif