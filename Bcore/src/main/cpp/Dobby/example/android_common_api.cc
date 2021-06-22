
#include "dobby.h"

#include "logging/logging.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <map>

std::map<void *, const char *> *func_map;

void common_handler(RegisterContext *ctx, const HookEntryInfo *info) {
  auto iter = func_map->find(info->function_address);
  if (iter != func_map->end()) {
    LOG(1, "func %s:%p invoke", iter->second, iter->first);
  }
}

// clang-format off
const char *func_array[] = {
   "__loader_dlopen",
   "dlsym",
   "dlclose",

   "open",
   "write",
   "read",
   "close",

   "socket",
   "connect",
   "bind",
   "listen",
   "accept",
   "send",
   "recv",

   // art::gc::Heap::PreZygoteFork
   "_ZN3art2gc4Heap13PreZygoteForkEv",

   // art::ClassLinker::DefineClass
   "_ZN3art11ClassLinker11DefineClassEPNS_6ThreadEPKcmNS_6HandleINS_6mirror11ClassLoaderEEERKNS_7DexFileERKNS_3dex8ClassDefE",

    // art::ClassLinker::ShouldUseInterpreterEntrypoint
   "_ZN3art11ClassLinker30ShouldUseInterpreterEntrypointEPNS_9ArtMethodEPKv",

   "_ZN3art11ClassLinkerC2EPNS_11InternTableE",
   "_ZN3art11ClassLinkerC2EPNS_11InternTableEb",
   "_ZN3art9hiddenapi6detail28ShouldDenyAccessToMemberImplINS_9ArtMethodEEEbPT_NS0_7ApiListENS0_12AccessMethodE",
   "_ZN3art9hiddenapi6detail28ShouldDenyAccessToMemberImplINS_8ArtFieldEEEbPT_NS0_7ApiListENS0_12AccessMethodE",
   "_ZN3art14OatFileManager24SetOnlyUseSystemOatFilesEbb",
   "_ZN3art7Runtime4InitEONS_18RuntimeArgumentMapE",
   "_ZN3art2gc4Heap13PreZygoteForkEv",
   "_ZN3art6mirror5Class15IsInSamePackageENS_6ObjPtrIS1_EE",
   "_ZNK23FileDescriptorWhitelist9IsAllowedERKNSt3__112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEE",
};
// clang-format on

namespace art {
namespace gc {
namespace Heap {
namespace _11 {
uint8_t PreZygoteFork[] = {
    0x2D, 0xE9, 0xF0, 0x4F, 0xAD, 0xF2, 0x04, 0x4D, 0x07, 0x46,
};
}
} // namespace Heap
} // namespace gc
} // namespace art

#if 1
__attribute__((constructor)) static void ctor() {
  void *func = NULL;
  log_set_level(0);

  func_map = new std::map<void *, const char *>();

  for (int i = 0; i < sizeof(func_array) / sizeof(char *); ++i) {
    func = DobbySymbolResolver(NULL, func_array[i]);
    if (func == NULL) {
      LOG(1, "func %s not resolve", func_array[i]);
      continue;
    }
    func_map->insert(std::pair<void *, const char *>(func, func_array[i]));
    DobbyInstrument(func, common_handler);
  }
  return;

  DobbyInstrument((void *)((addr_t)art::gc::Heap::_11::PreZygoteFork + 1), common_handler);

  pthread_t socket_server;
  uint64_t socket_demo_server(void *ctx);
  pthread_create(&socket_server, NULL, (void *(*)(void *))socket_demo_server, NULL);

  usleep(1000);
  pthread_t socket_client;
  uint64_t socket_demo_client(void *ctx);
  pthread_create(&socket_client, NULL, (void *(*)(void *))socket_demo_client, NULL);
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 8080

uint64_t socket_demo_server(void *ctx) {
  int server_fd, new_socket, valread;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[1024] = {0};
  char *hello = "Hello from server";

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }
  valread = recv(new_socket, buffer, 1024, 0);
  printf("%s\n", buffer);
  send(new_socket, hello, strlen(hello), 0);
  printf("Hello message sent\n");
  return 0;
}

uint64_t socket_demo_client(void *ctx) {
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char *hello = "Hello from client";
  char buffer[1024] = {0};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }
  send(sock, hello, strlen(hello), 0);
  printf("Hello message sent\n");
  valread = recv(sock, buffer, 1024, 0);
  printf("%s\n", buffer);
  return 0;
}
#endif