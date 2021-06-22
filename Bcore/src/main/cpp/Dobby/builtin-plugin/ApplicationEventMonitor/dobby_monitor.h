#ifndef DOBBY_MONITOR_H
#define DOBBY_MONITOR_H

#include <stdlib.h> /* getenv */
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <fstream>

#include <set>
#include <unordered_map>

#include "dobby.h"

#define LOG printf

#ifdef __cplusplus
extern "C" {
#endif

int DobbyHook(void *function_address, void *replace_call, void **origin_call);

#ifdef __cplusplus
}
#endif

#endif // !1DOBBY_MONITOR
