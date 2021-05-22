/*
I did not write this code! It is taken from this excellent article by Chris
Wellons: https://nullprogram.com/blog/2016/04/10/

I just modified it slightly to use the functions from the header.
*/

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define MM_IMPLEMENT
#include "wellons_memalias.h"

int main(void) {
    size_t size = sizeof(uint32_t);
    void *addr[3] = {0};
    size_t naddr = sizeof(addr) / sizeof(addr[0]);
    int r = memory_alias_map(size, naddr, addr);
    assert(r == 0);
    *(uint32_t *)addr[0] = 0xdeadbeef;
    for (size_t i = 0; i < naddr; i++)
        printf("*%p = 0x%x\n", addr[i], *(uint32_t *)addr[i]);
    *(uint32_t *)addr[0] = 0xcafebabe;
    for (size_t i = 0; i < naddr; i++)
        printf("*%p = 0x%x\n", addr[i], *(uint32_t *)addr[i]);
    memory_alias_unmap(size, naddr, addr);
    return 0;
}
