#include "Dyld2HideLibrary/dyld2_hide_library.h"

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/dyld_images.h>

#include <stdint.h>
#include <stdio.h>

#include <mach/mach.h>

#include <string.h>

#include <vector>

#include "dobby_internal.h"

typedef void ImageLoader;

typedef void ImageLoaderMachO;

static void *(*removeImageFromAllImages)(const struct mach_header *mh) = NULL;

static char *(*ImageLoader__getShortName)(ImageLoader *loader) = NULL;

static struct mach_header *(*ImageLoaderMachO__machHeader)(ImageLoaderMachO *loader) = NULL;

static std::vector<ImageLoader *> *sAllImages = NULL;

std::vector<char *> *g_prepare_remove_array;

static int dobby_hide_library_internal(const char *library_name) {
  if (removeImageFromAllImages == NULL) {
    removeImageFromAllImages =
        (typeof(removeImageFromAllImages))DobbySymbolResolver("dyld", "__Z24removeImageFromAllImagesPK11mach_header");
  }

  if (ImageLoader__getShortName == NULL) {
    ImageLoader__getShortName =
        (typeof(ImageLoader__getShortName))DobbySymbolResolver("dyld", "__ZNK11ImageLoader12getShortNameEv");
  }

  if (ImageLoaderMachO__machHeader == NULL) {
    ImageLoaderMachO__machHeader =
        (typeof(ImageLoaderMachO__machHeader))DobbySymbolResolver("dyld", "__ZNK16ImageLoaderMachO10machHeaderEv");
  }

  if (sAllImages == NULL)
    sAllImages = (typeof(sAllImages))DobbySymbolResolver("dyld", "__ZN4dyldL10sAllImagesE");

#if 0
  if (dyld3__AllImages__imageLoadAddressByIndex == NULL) {
    dyld3__AllImages__imageLoadAddressByIndex = (typeof(removeImageFromAllImages))DobbySymbolResolver(
        "libdyld.dylib", "__ZNK5dyld39AllImages23imageLoadAddressByIndexEj");
  }

  if (dyld3_sAllImages == NULL)
    dyld3_sAllImages = (typeof(dyld3_sAllImages))DobbySymbolResolver("libdyld.dylib", "__ZN5dyld310gAllImagesE");
#endif

#if 0
  typedef void AllImages;
  static void(*AllImages__decRefCount)(AllImages *_this, const struct mach_header *mh) = NULL;
  if(AllImages__decRefCount == NULL) {
    AllImages__decRefCount = (typeof(AllImages__decRefCount))DobbySymbolResolver("libdyld.dylib", "__ZN5dyld39AllImages11decRefCountEPK11mach_header");
  }
  
  static AllImages *gAllImages = NULL;
  if(gAllImages == NULL) {
    gAllImages = (typeof(gAllImages))DobbySymbolResolver("libdyld.dylib", "__ZN5dyld310gAllImagesE");
  }
#endif

  //  int count = _dyld_image_count();
  //  for (int i = 0; i < count; i++) {
  //    const char *name       = _dyld_get_image_name(i);
  //    const char *image_name = strrchr(name, '/');
  //    if (image_name == NULL) {
  //      continue;
  //    }
  //    image_name += 1;
  //    if (strcmp(image_name, library_name) == 0) {
  //      const struct mach_header *header = _dyld_get_image_header(i);
  //      AllImages__decRefCount(gAllImages, header);
  //      break;
  //    }
  //  }

  for (std::vector<ImageLoader *>::iterator it = sAllImages->begin(); it != sAllImages->end(); it++) {
    char *name = ImageLoader__getShortName(*it);
    // LOG(2, "load library : %s", name);
    if (strcmp(name, library_name) == 0) {
      LOG(2, "strip load library : %s", library_name);
      struct mach_header *header = ImageLoaderMachO__machHeader(*it);
      removeImageFromAllImages(header);
      sAllImages->erase(it);
      break;
    }
  }

  return 0;
}

PUBLIC int dyld2_hide_library(const char *library_name) {
#if 1
  dobby_hide_library_internal(library_name);
  return 0;
#endif

  if (g_prepare_remove_array == NULL)
    g_prepare_remove_array = new std::vector<char *>();
  g_prepare_remove_array->push_back((char *)library_name);
}

static void common_handler(RegisterContext *ctx, const HookEntryInfo *info) {
  if (g_prepare_remove_array == nullptr)
    return;
  for (auto name : *g_prepare_remove_array) {
    dobby_hide_library_internal(name);
  }
}

__attribute__((constructor)) static void ctor() {
#if 0
  void *dyld__notifyMonitoringDyldMain = DobbySymbolResolver("dyld", "__ZN4dyldL24notifyMonitoringDyldMainEv");
  DobbyInstrument(dyld__notifyMonitoringDyldMain, common_handler);
#endif

  log_switch_to_syslog();

#if defined(DOBBY_DEBUG) && 0
  dyld2_hide_library("Dobby");
  dyld2_hide_library("liblangid.dylib");
#endif
}
