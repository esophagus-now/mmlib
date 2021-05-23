/*
Turns out it's also pretty straightforward to get the cbuf allocation 
working in Linux. There's just a subtle trick involving MAP_FIXED. See here
for details:

https://stackoverflow.com/a/28576674/2737696

Text reproduced below for posterity:

MAP_FIXED is dup2 for memory mappings, and it's useful in exactly the same 
situations where dup2 is useful for file descriptors: when you want to 
perform a replace operation that atomically reassigns a resource identifier 
(memory range in the case of MAP_FIXED, or fd in the case of dup2) to refer 
to a new resource without the possibility of races where it might get 
reassigned to something else if you first released the old resource then 
attempted to regain it for the new resource.

As an example, take loading a shared library (by the dynamic loader). It 
consists of at least three types of mappings: read+exec-only mapping of the 
program code and read-only data from the executable file, read-write 
mapping of the initialized data (also from the executable file, but 
typically with a different relative offset), and read-write 
zero-initialized anonymous memory (for .bss). Creating these as separate 
mappings would not work because they must be at fixed relative addresses 
relative to one another. So instead you first make a dummy mapping of the 
total length needed (the type of this mapping doesn't matter) without 
MAP_FIXED just to reserve a sufficient range of contiguous addresses at a 
kernel-assigned location, then you use MAP_FIXED to map over top of parts 
of this range as needed with the three or more mappings you need to create.

Further, note that use of MAP_FIXED with a hard-coded address or a random 
address is always a bug. The only correct way to use MAP_FIXED is to 
replace an existing mapping whose address was assigned by a previous 
successful call to mmap without MAP_FIXED, or in some other way where you 
feel it's safe to replace whole pages. This aspect too is completely 
analogous to dup2; it's always a bug to use dup2 when the caller doesn't 
already have an open file on the target fd with the intent to replace it. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MM_IMPLEMENT
#include "mmlib/wellons_memalias.h"
#include "mmlib/os_common.h"

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

    //Find large enough contiguous memory space. On Linux, to 
    //guarantee contiguous memory regions over multiple mmaps,
    //you first need to set up a mapping for the whole size
    //(letting the OS choose the location in virtual address
    //space) and then use MAP_FIXED in the mmap arguments (this
    //is done inside memory_alias_map) to have the OS atomically
    //release portions from the large mmap'ed region and then 
    //give them to the new memory alias mapping.
    char *base = mmap(
        NULL,
        2 * CBUF_SIZE_PAGES * page_sz,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, //These can be anything, as long as they don't include MAP_FIXED
        -1,
        0
    ); 
    
    cbuf c = {
        .buf_addrs = {base, base + CBUF_SIZE_PAGES * page_sz},
        .buf = base,
        .pos = 0, 
        .len = 0,
        .cap = CBUF_SIZE_PAGES * page_sz
    };

    int rc = memory_alias_map(CBUF_SIZE_PAGES * page_sz, 2, (void**) c.buf_addrs);

    if (rc != 0) {
        printf("Could not allocate the aliased memory regions: %s\n", sockstrerror(sockerrno));
        //puts("Could not allocate the aliased memory regions");
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
