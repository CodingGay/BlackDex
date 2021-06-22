#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/dyld_images.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <mach/vm_map.h>
#include <mach/mach.h>

#include <vector>

#include "SymbolResolver/dobby_symbol_resolver.h"
#include "SymbolResolver/macho/shared_cache_internal.h"

#include "common_header.h"

#include "logging/logging.h"

#include "PlatformUtil/ProcessRuntimeUtility.h"

#undef LOG_TAG
#define LOG_TAG "DobbySymbolResolver"

typedef struct macho_ctx {
  mach_header_t *header;

  uintptr_t slide;
  uintptr_t linkedit_base;

  segment_command_t *segments[16];
  int segments_count;

  segment_command_t *text_seg;
  segment_command_t *data_seg;
  segment_command_t *data_const_seg;
  segment_command_t *linkedit_seg;

  struct symtab_command *symtab_cmd;
  struct dysymtab_command *dysymtab_cmd;
  struct dyld_info_command *dyld_info_cmd;

  nlist_t *symtab;
  char *strtab;
  uint32_t *indirect_symtab;

} macho_ctx_t;

uintptr_t read_uleb128(const uint8_t **pp, const uint8_t *end) {
  uint8_t *p = (uint8_t *)*pp;
  uint64_t result = 0;
  int bit = 0;
  do {
    if (p == end)
      ASSERT(p == end);

    uint64_t slice = *p & 0x7f;

    if (bit > 63)
      ASSERT(bit > 63);
    else {
      result |= (slice << bit);
      bit += 7;
    }
  } while (*p++ & 0x80);

  *pp = p;

  return (uintptr_t)result;
}

intptr_t read_sleb128(const uint8_t **pp, const uint8_t *end) {
  uint8_t *p = (uint8_t *)*pp;

  int64_t result = 0;
  int bit = 0;
  uint8_t byte;
  do {
    if (p == end)
      ASSERT(p == end);
    byte = *p++;
    result |= (((int64_t)(byte & 0x7f)) << bit);
    bit += 7;
  } while (byte & 0x80);
  // sign extend negative numbers
  if ((byte & 0x40) != 0)
    result |= (~0ULL) << bit;

  *pp = p;

  return (intptr_t)result;
}

// dyld
// bool MachOLoaded::findExportedSymbol
uint8_t *walk_exported_trie(const uint8_t *start, const uint8_t *end, const char *symbol) {
  uint32_t visitedNodeOffsets[128];
  int visitedNodeOffsetCount = 0;
  visitedNodeOffsets[visitedNodeOffsetCount++] = 0;
  const uint8_t *p = start;
  while (p < end) {
    uint64_t terminalSize = *p++;
    if (terminalSize > 127) {
      // except for re-export-with-rename, all terminal sizes fit in one byte
      --p;
      terminalSize = read_uleb128(&p, end);
    }
    if ((*symbol == '\0') && (terminalSize != 0)) {
      return (uint8_t *)p;
      // skip flag == EXPORT_SYMBOL_FLAGS_REEXPORT
      read_uleb128(&p, end);
      return (uint8_t *)read_uleb128(&p, end);
    }
    const uint8_t *children = p + terminalSize;
    if (children > end) {
      // diag.error("malformed trie node, terminalSize=0x%llX extends past end of trie\n", terminalSize);
      return NULL;
    }
    uint8_t childrenRemaining = *children++;
    p = children;
    uint64_t nodeOffset = 0;
    for (; childrenRemaining > 0; --childrenRemaining) {
      const char *ss = symbol;
      bool wrongEdge = false;
      // scan whole edge to get to next edge
      // if edge is longer than target symbol name, don't read past end of symbol name
      char c = *p;
      while (c != '\0') {
        if (!wrongEdge) {
          if (c != *ss)
            wrongEdge = true;
          ++ss;
        }
        ++p;
        c = *p;
      }
      if (wrongEdge) {
        // advance to next child
        ++p; // skip over zero terminator
        // skip over uleb128 until last byte is found
        while ((*p & 0x80) != 0)
          ++p;
        ++p; // skip over last byte of uleb128
        if (p > end) {
          // diag.error("malformed trie node, child node extends past end of trie\n");
          return NULL;
        }
      } else {
        // the symbol so far matches this edge (child)
        // so advance to the child's node
        ++p;
        nodeOffset = read_uleb128(&p, end);
        if ((nodeOffset == 0) || (&start[nodeOffset] > end)) {
          // diag.error("malformed trie child, nodeOffset=0x%llX out of range\n", nodeOffset);
          return NULL;
        }
        symbol = ss;
        break;
      }
    }
    if (nodeOffset != 0) {
      if (nodeOffset > (uint64_t)(end - start)) {
        // diag.error("malformed trie child, nodeOffset=0x%llX out of range\n", nodeOffset);
        return NULL;
      }
      for (int i = 0; i < visitedNodeOffsetCount; ++i) {
        if (visitedNodeOffsets[i] == nodeOffset) {
          // diag.error("malformed trie child, cycle to nodeOffset=0x%llX\n", nodeOffset);
          return NULL;
        }
      }
      visitedNodeOffsets[visitedNodeOffsetCount++] = (uint32_t)nodeOffset;
      if (visitedNodeOffsetCount >= 128) {
        // diag.error("malformed trie too deep\n");
        return NULL;
      }
      p = &start[nodeOffset];
    } else
      p = end;
  }
  return NULL;
}

uintptr_t iterate_exported_symbol(mach_header_t *header, const char *symbol_name, uint64_t *out_flags) {
  segment_command_t *curr_seg_cmd;
  struct dyld_info_command *dyld_info_cmd = NULL;
  struct linkedit_data_command *exports_trie_cmd = NULL;
  segment_command_t *text_segment, *data_segment, *linkedit_segment;

  curr_seg_cmd = (segment_command_t *)((uintptr_t)header + sizeof(mach_header_t));
  for (int i = 0; i < header->ncmds; i++) {
    switch (curr_seg_cmd->cmd) {
    case LC_SEGMENT_ARCH_DEPENDENT: {
      if (strcmp(curr_seg_cmd->segname, "__LINKEDIT") == 0) {
        linkedit_segment = curr_seg_cmd;
      } else if (strcmp(curr_seg_cmd->segname, "__TEXT") == 0) {
        text_segment = curr_seg_cmd;
      }
    } break;
    case LC_DYLD_EXPORTS_TRIE: {
      exports_trie_cmd = (struct linkedit_data_command *)curr_seg_cmd;
    } break;
    case LC_DYLD_INFO:
    case LC_DYLD_INFO_ONLY: {
      dyld_info_cmd = (struct dyld_info_command *)curr_seg_cmd;
    } break;
    default:
      break;
    };
    curr_seg_cmd = (segment_command_t *)((uintptr_t)curr_seg_cmd + curr_seg_cmd->cmdsize);
  }

  if (text_segment == NULL || linkedit_segment == NULL) {
    return 0;
  }

  if (exports_trie_cmd == NULL && dyld_info_cmd == NULL)
    return 0;

  uint32_t trieFileOffset = dyld_info_cmd ? dyld_info_cmd->export_off : exports_trie_cmd->dataoff;
  uint32_t trieFileSize = dyld_info_cmd ? dyld_info_cmd->export_size : exports_trie_cmd->datasize;

  uintptr_t slide = (uintptr_t)header - (uintptr_t)text_segment->vmaddr;
  uintptr_t linkedit_base = (uintptr_t)slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;

  void *exports = (void *)(linkedit_base + trieFileOffset);
  if (exports == NULL)
    return 0;

  uint8_t *exports_start = (uint8_t *)exports;
  uint8_t *exports_end = exports_start + trieFileSize;
  uint8_t *node = (uint8_t *)walk_exported_trie(exports_start, exports_end, symbol_name);
  if (node == NULL)
    return 0;
  const uint8_t *p = node;
  const uintptr_t flags = read_uleb128(&p, exports_end);
  if (flags & EXPORT_SYMBOL_FLAGS_REEXPORT) {
    return 0;
  }
  if (out_flags)
    *out_flags = flags;
  uint64_t trieValue = read_uleb128(&p, exports_end);
  return trieValue;
#if 0
  if (off == (void *)0) {
    if (symbol_name[0] != '_' && strlen(&symbol_name[1]) >= 1) {
      char _symbol_name[1024] = {0};
      _symbol_name[0] = '_';
      strcpy(&_symbol_name[1], symbol_name);
      off = (void *)walk_exported_trie((const uint8_t *)exports, (const uint8_t *)exports + trieFileSize, _symbol_name);
    }
  }
#endif
}

void macho_ctx_init(macho_ctx_t *ctx, mach_header_t *header) {
  ctx->header = header;
  segment_command_t *curr_seg_cmd;
  segment_command_t *text_segment, *data_segment, *data_const_segment, *linkedit_segment;
  struct symtab_command *symtab_cmd = NULL;
  struct dysymtab_command *dysymtab_cmd = NULL;
  struct dyld_info_command *dyld_info_cmd = NULL;

  curr_seg_cmd = (segment_command_t *)((uintptr_t)header + sizeof(mach_header_t));
  for (int i = 0; i < header->ncmds; i++) {
    if (curr_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
      //  BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB and REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB
      ctx->segments[ctx->segments_count++] = curr_seg_cmd;

      if (strcmp(curr_seg_cmd->segname, "__LINKEDIT") == 0) {
        linkedit_segment = curr_seg_cmd;
      } else if (strcmp(curr_seg_cmd->segname, "__DATA") == 0) {
        data_segment = curr_seg_cmd;
      } else if (strcmp(curr_seg_cmd->segname, "__DATA_CONST") == 0) {
        data_const_segment = curr_seg_cmd;
      } else if (strcmp(curr_seg_cmd->segname, "__TEXT") == 0) {
        text_segment = curr_seg_cmd;
      }
    } else if (curr_seg_cmd->cmd == LC_SYMTAB) {
      symtab_cmd = (struct symtab_command *)curr_seg_cmd;
    } else if (curr_seg_cmd->cmd == LC_DYSYMTAB) {
      dysymtab_cmd = (struct dysymtab_command *)curr_seg_cmd;
    } else if (curr_seg_cmd->cmd == LC_DYLD_INFO || curr_seg_cmd->cmd == LC_DYLD_INFO_ONLY) {
      dyld_info_cmd = (struct dyld_info_command *)curr_seg_cmd;
    }
    curr_seg_cmd = (segment_command_t *)((uintptr_t)curr_seg_cmd + curr_seg_cmd->cmdsize);
  }

  uintptr_t slide = (uintptr_t)header - (uintptr_t)text_segment->vmaddr;
  uintptr_t linkedit_base = (uintptr_t)slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;

  ctx->text_seg = text_segment;
  ctx->data_seg = data_segment;
  ctx->data_const_seg = data_const_segment;
  ctx->linkedit_seg = linkedit_segment;

  ctx->symtab_cmd = symtab_cmd;
  ctx->dysymtab_cmd = dysymtab_cmd;
  ctx->dyld_info_cmd = dyld_info_cmd;

  ctx->slide = slide;
  ctx->linkedit_base = linkedit_base;

  ctx->symtab = (nlist_t *)(ctx->linkedit_base + ctx->symtab_cmd->symoff);
  ctx->strtab = (char *)(ctx->linkedit_base + ctx->symtab_cmd->stroff);
  ctx->indirect_symtab = (uint32_t *)(ctx->linkedit_base + ctx->dysymtab_cmd->indirectsymoff);
}

uintptr_t iterate_symbol_table(char *name_pattern, nlist_t *symtab, uint32_t symtab_count, char *strtab) {
  for (uint32_t i = 0; i < symtab_count; i++) {
    if (symtab[i].n_value) {
      uint32_t strtab_offset = symtab[i].n_un.n_strx;
      char *symbol_name = strtab + strtab_offset;
#if 0
      LOG(1, "> %s", symbol_name);
#endif
      if (strcmp(name_pattern, symbol_name) == 0) {
        return symtab[i].n_value;
      }
      if (symbol_name[0] == '_') {
        if (strcmp(name_pattern, &symbol_name[1]) == 0) {
          return symtab[i].n_value;
        }
      }
    }
  }
  return 0;
}

static uintptr_t macho_kit_get_slide(mach_header_t *header) {
  uintptr_t slide = 0;

  segment_command_t *curr_seg_cmd;

  curr_seg_cmd = (segment_command_t *)((addr_t)header + sizeof(mach_header_t));
  for (int i = 0; i < header->ncmds; i++) {
    if (curr_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
      if (strcmp(curr_seg_cmd->segname, "__TEXT") == 0) {
        slide = (uintptr_t)header - curr_seg_cmd->vmaddr;
        return slide;
      }
    }
    curr_seg_cmd = (segment_command_t *)((addr_t)curr_seg_cmd + curr_seg_cmd->cmdsize);
  }
  return 0;
}

PUBLIC void *DobbySymbolResolver(const char *image_name, const char *symbol_name_pattern) {
  uintptr_t result = 0;

  std::vector<RuntimeModule> ProcessModuleMap = ProcessRuntimeUtility::GetProcessModuleMap();

  for (auto module : ProcessModuleMap) {
    if (image_name != NULL && strstr(module.path, image_name) == NULL)
      continue;

    mach_header_t *header = (mach_header_t *)module.load_address;
    size_t slide = 0;

    if (header) {
      if (header->magic == MH_MAGIC_64)
        slide = macho_kit_get_slide(header);
    }

#if 0
    LOG(1, "resolve image: %s", path);
#endif

    nlist_t *symtab = NULL;
    uint32_t symtab_count = 0;
    char *strtab = NULL;

#if defined(__arm__) || defined(__aarch64__)
    static int shared_cache_ctx_init_once = 0;
    static shared_cache_ctx_t shared_cache_ctx;
    if (shared_cache_ctx_init_once == 0) {
      shared_cache_ctx_init_once = 1;
      memset(&shared_cache_ctx, 0, sizeof(shared_cache_ctx_t));
      shared_cache_ctx_init(&shared_cache_ctx);
    }
    if (shared_cache_ctx.runtime_shared_cache) {
      // shared cache library
      if (shared_cache_is_contain(&shared_cache_ctx, (addr_t)header, 0)) {
        shared_cache_get_symbol_table(&shared_cache_ctx, header, &symtab, &symtab_count, &strtab);
      }
    }
#endif
    if (symtab && strtab) {
      result = iterate_symbol_table((char *)symbol_name_pattern, symtab, symtab_count, strtab);
    }
    if (result) {
      result = result + slide;
      break;
    }

    // binary symbol table
    macho_ctx_t macho_ctx;
    memset(&macho_ctx, 0, sizeof(macho_ctx_t));
    macho_ctx_init(&macho_ctx, header);
    result = iterate_symbol_table((char *)symbol_name_pattern, macho_ctx.symtab, macho_ctx.symtab_cmd->nsyms,
                                  macho_ctx.strtab);
    if (result) {
      result = result + slide;
      break;
    }

    // binary exported table(uleb128)
    uint64_t flags;
    result = iterate_exported_symbol((mach_header_t *)header, symbol_name_pattern, &flags);
    if (result) {
      switch (flags & EXPORT_SYMBOL_FLAGS_KIND_MASK) {
      case EXPORT_SYMBOL_FLAGS_KIND_REGULAR: {
        result += (uintptr_t)header;
      } break;
      case EXPORT_SYMBOL_FLAGS_KIND_THREAD_LOCAL: {
        result += (uintptr_t)header;
      } break;
      case EXPORT_SYMBOL_FLAGS_KIND_ABSOLUTE: {
      } break;
      default:
        break;
      }
      return (void *)result;
    }
  }

  mach_header_t *dyld_header = NULL;
  if (image_name != NULL && strcmp(image_name, "dyld") == 0) {
    // task info
    task_dyld_info_data_t task_dyld_info;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_DYLD_INFO, (task_info_t)&task_dyld_info, &count)) {
      return NULL;
    }

    // get dyld load address
    const struct dyld_all_image_infos *infos =
        (struct dyld_all_image_infos *)(uintptr_t)task_dyld_info.all_image_info_addr;
    dyld_header = (mach_header_t *)infos->dyldImageLoadAddress;

    macho_ctx_t dyld_ctx;
    memset(&dyld_ctx, 0, sizeof(macho_ctx_t));
    macho_ctx_init(&dyld_ctx, dyld_header);
    result =
        iterate_symbol_table((char *)symbol_name_pattern, dyld_ctx.symtab, dyld_ctx.symtab_cmd->nsyms, dyld_ctx.strtab);
    if (result) {
      result = result + (addr_t)dyld_header;
    }
  }

  return (void *)result;
}

#if defined(DOBBY_DEBUG) && 0
__attribute__((constructor)) static void ctor() {
  mach_header_t *header = NULL;
  header = (mach_header_t *)_dyld_get_image_header(0);

  void *addr = (void *)((addr_t)iterate_exported_symbol(header, "_mainxx") + (addr_t)header);
  LOG(1, "export %p", addr);
}
#endif
