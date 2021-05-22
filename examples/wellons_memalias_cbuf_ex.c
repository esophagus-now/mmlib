/*
This is a little proof-of-concept for some future mmlib code. Uses Chris
Wellons's memory aliasing API to implement a circular buffer. Because it's 
just a POC, I only bothered to get it working on my MinGW installation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define MM_IMPLEMENT
#include "wellons_memalias.h"
//#include "os_common.h"

#define CBUF_SIZE_PAGES 1

typedef struct {
    char *buf_addrs[2];
    char *buf; //For convenience, will just always be equal to buf_addrs[0]
    int pos, len, cap;
} cbuf;


//Usual semantics
int cbuf_write(cbuf *c, char const* buf, int len) {
    if (len < 0) return -1;
    
    //Write as much into the cbuf as we can
    int write_len = len;
    int space_left = c->cap - c->len;
    if (write_len > space_left) write_len = space_left;
    
    char *dest = c->buf + (c->pos + c->len) % c->cap;
    
    //Notice this doesn't use memmove. Maybe it should?
    memcpy(dest, buf, write_len);
    
    c->len += write_len;
    
    return write_len;
}

int cbuf_read(cbuf *c, char *buf, int len) {
    if (len < 0) return -1;
    
    if (c->len == 0) return 0;
    
    //Can't read more than what's in the cbuf
    int read_len = len;
    if (c->len < read_len) read_len = c->len;
    
    char *src = c->buf + c->pos;
    memcpy(buf, src, read_len);
    
    //Update pos and len in cbuf
    c->len -= read_len;
    c->pos = (c->pos + read_len) % c->cap;
    
    return read_len;
}

int main() {
    int page_sz = getpagesize();
    printf("The page size is %d bytes\n", page_sz);
    //Use VirtualAlloc to find a free area in virtual address space
    
    char *base = VirtualAlloc(
      NULL,
      CBUF_SIZE_PAGES * 2 * page_sz,
      MEM_RESERVE,
      PAGE_READWRITE
    );
    
    if (base == NULL) {
        //printf("Could not allocate space: %s\n", sockstrerror(sockerrno));
        puts("Could not allocate space");
        return -1;
    } else {
        printf("Found space in virtual address space starting at %p\n", base);
        VirtualFree(base, 0, MEM_RELEASE);
    }
    
    cbuf c = {
        .buf_addrs = {base, base + CBUF_SIZE_PAGES * page_sz},
        .buf = base,
        .pos = 0, 
        .len = 0,
        .cap = CBUF_SIZE_PAGES * page_sz
    };
    
    int rc = memory_alias_map(CBUF_SIZE_PAGES * page_sz, 2, (void**) c.buf_addrs);
    if (rc != 0) {
        //printf("Could not allocate the aliased memory regions: %s\n", sockstrerror(sockerrno));
        puts("Could not allocate the aliased memory regions");
        return -1;
    } else {
        puts("Allocated memory succesfully!");
        
        int wr_gen = 0, rd_gen = 0;
        char *test_buf = malloc(CBUF_SIZE_PAGES * page_sz);
        
        int num_iters = 100;
        while (num_iters --> 0) {
            //Generate a random write length
            int wr_len = 30 + rand() % 4000;
            
            //Generate a buffer with predictable data
            int i;
            for (i = 0; i < wr_len; i++) {
                test_buf[i] = (char)((int)(wr_gen + i) % 251);
            }
            
            //Write this test buffer to the cbuf
            int bytes_written = cbuf_write(&c, test_buf, wr_len);
            wr_gen = (wr_gen + bytes_written) % 251;
            
            //Generate a random read length
            int rd_len = 30 + rand() % 4000;
            
            //Read out from the cbuf
            int bytes_read = cbuf_read(&c, test_buf, rd_len);
            
            //Check if the data is what we expect it to be
            for (i = 0; i < bytes_read; i++) {
                if ((test_buf[i] & 0xFF) != rd_gen) {
                    puts("Something bad happened");
                    goto done;
                }
                
                rd_gen = (rd_gen + 1) % 251;
            }
        }
        
        puts("cbuf seems to be working!");
        
        done:
        
        memory_alias_unmap(CBUF_SIZE_PAGES * page_sz, 2, (void**) c.buf_addrs);
    }
    
    
    return 0;
}
