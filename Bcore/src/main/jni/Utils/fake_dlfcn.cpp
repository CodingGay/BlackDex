#include <sys/types.h>
#include <cstdio>
#include <sys/mman.h>
#include <elf.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include "fake_dlfcn.h"

#ifdef __arm__
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym  Elf32_Sym
#elif defined(__i386__)
    #define Elf_Ehdr Elf32_Ehdr
    #define Elf_Shdr Elf32_Shdr
    #define Elf_Sym  Elf32_Sym
#elif defined(__aarch64__)
    #define Elf_Ehdr Elf64_Ehdr
    #define Elf_Shdr Elf64_Shdr
    #define Elf_Sym  Elf64_Sym
#else
    #error "Arch unknown, please port me"
#endif

struct ctx {
    intptr_t load_addr;
    void *dynstr;
    void *dynsym;
    int nsyms;
    off_t bias;
};

int fake_dlclose(void *handle) {
    if (handle) {
        struct ctx *ctx = (struct ctx *) handle;
        if (ctx->dynsym) free(ctx->dynsym);    /* we're saving dynsym and dynstr */
        if (ctx->dynstr) free(ctx->dynstr);    /* from library file just in case */
        free(ctx);
    }
    return 0;
}

void *fake_dlopen(const char *libpath, int flags) {
    FILE *maps;
    char buff[256];
    struct ctx *ctx = 0;
    off_t load_addr, size;
    int k, fd = -1, found = 0;
    intptr_t shoff;
    Elf_Ehdr *elf = (Elf_Ehdr *) MAP_FAILED;


    maps = fopen("/proc/self/maps", "r");
    if (!maps) goto err_exit;

    while (!found && fgets(buff, sizeof(buff), maps))
        if (strstr(buff, "r-xp") && strstr(buff, libpath)) found = 1;

    fclose(maps);

    if (!found) goto err_exit;

    if (sscanf(buff, "%lx", &load_addr) != 1)
        goto err_exit;

    /* Now, mmap the same library once again */

    fd = open(libpath, O_RDONLY);
    if (fd < 0) goto err_exit;

    size = lseek(fd, 0, SEEK_END);
    if (size <= 0) goto err_exit;

    elf = (Elf_Ehdr *) mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    fd = -1;

    if (elf == MAP_FAILED) goto err_exit;

    ctx = (struct ctx *) calloc(1, sizeof(struct ctx));
    if (!ctx) goto err_exit;

    ctx->load_addr = (intptr_t) load_addr;
    shoff = (intptr_t) elf + elf->e_shoff;

    for (k = 0; k < elf->e_shnum; k++, shoff += elf->e_shentsize) {

        Elf_Shdr *sh = (Elf_Shdr *) shoff;
        switch (sh->sh_type) {

            case SHT_DYNSYM:
                /* .dynsym */
                ctx->dynsym = malloc(sh->sh_size);
                if (!ctx->dynsym) goto err_exit;
                memcpy(ctx->dynsym, (const void *) ((intptr_t) elf + sh->sh_offset), sh->sh_size);
                ctx->nsyms = (sh->sh_size / sizeof(Elf_Sym));
                break;

            case SHT_STRTAB:
                if (ctx->dynstr) break;    /* .dynstr is guaranteed to be the first STRTAB */
                ctx->dynstr = malloc(sh->sh_size);
                if (!ctx->dynstr) goto err_exit;
                memcpy(ctx->dynstr, (const void *) (((intptr_t) elf) + sh->sh_offset), sh->sh_size);
                break;

            case SHT_PROGBITS:
                if (!ctx->dynstr || !ctx->dynsym) break;
                /* won't even bother checking against the section name */
                ctx->bias = (off_t) sh->sh_addr - (off_t) sh->sh_offset;
                k = elf->e_shnum;  /* exit for */
                break;
        }
    }

    munmap(elf, (size_t) size);
    elf = 0;

    if (!ctx->dynstr || !ctx->dynsym) goto err_exit;

#undef fatal


    return ctx;

    err_exit:
    if (fd >= 0) close(fd);
    if (elf != MAP_FAILED) munmap(elf, size);
    fake_dlclose(ctx);
    return 0;
}

void *fake_dlsym(void *handle, const char *name) {
    int k;
    struct ctx *ctx = (struct ctx *) handle;
    Elf_Sym *sym = (Elf_Sym *) ctx->dynsym;
    char *strings = (char *) ctx->dynstr;

    for (k = 0; k < ctx->nsyms; k++, sym++)
        if (strcmp(strings + sym->st_name, name) == 0) {
            /*  NB: sym->st_value is an offset into the section for relocatables,
            but a VMA for shared libs or exe files, so we have to subtract the bias */
            void *ret = (void *) (ctx->load_addr + sym->st_value - ctx->bias);
            return ret;
        }
    return 0;

}