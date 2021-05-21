//
// Created by Milk on 2021/5/17.
//

#include <asm/fcntl.h>
#include <fcntl.h>
#include <unistd.h>
#include "PointerCheck.h"

bool PointerCheck::check(void *addr) {
    int nullfd = open("/dev/random", O_WRONLY);
    bool valid = true;
    if (write(nullfd, (void *) addr, sizeof(addr)) < 0) {
        valid = false;
    }
    close(nullfd);
    return valid;
}
