/*
I did not write this code! It is taken from this excellent article by Chris
Wellons: https://nullprogram.com/blog/2016/04/10/

I did however convert it to the "single-header" method used by all the other 
files in this library. I also added a Windows implementation of getpagesize,
and I modified the Linux version to use the MAP_FIXED flag if the given 
address is non-NULL. It is now up to the user to first do an anonymous 
mapping that gets replaced by the given addresses (or they can just give a
NULL address fro automatic selection).

Tested using:
 - MinGW, 32-bit
 - cygwin, 64-bit

By the way, I'm often critcal of the OS API on Windows, but this is one 
case where it's actually slightly better than the Linux API. Credit where
credit is due!

*/

/*
Usage (copied from the article linked above):

Having some fun with this, I came up with a general API to allocate an 
aliased mapping at an arbitrary number of addresses.

int  memory_alias_map(size_t size, size_t naddr, void **addrs);
void memory_alias_unmap(size_t size, size_t naddr, void **addrs);

Values in the address array must either be page-aligned or NULL to allow 
the operating system to choose, in which case the map address is written to 
the array.

It returns 0 on success. It may fail if the size is too small (0), too 
large, too many file descriptors, etc.

Pass the same pointers back to memory_alias_unmap to free the mappings. 
When called correctly it cannot fail, so there’s no return value. 
*/

#ifdef MM_IMPLEMENT
	#ifndef WELLONS_MEMALIAS_H_IMPLEMENTED
		#define WELLONS_MEMALIAS_H_IMPLEMENTED 1
		#define SHOULD_INCLUDE 1
	#else
		#define SHOULD_INCLUDE 0
	#endif
#else
	#ifndef WELLONS_MEMALIAS_H
		#define WELLONS_MEMALIAS_H 1
		#define SHOULD_INCLUDE 1
	#else
		#define SHOULD_INCLUDE 0
	#endif
#endif

#if SHOULD_INCLUDE
#undef SHOULD_INCLUDE

#ifdef MM_IMPLEMENT
#undef MM_IMPLEMENT
#include "wellons_memalias.h"
#define MM_IMPLEMENT 1
#endif


/* POSIX: cc -std=c99 -Wall -Wextra -Os memalias.c -lrt
 * MinGW: cc -std=c99 -Wall -Wextra -Os memalias.c
 */


#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)

////////////////////////////
// Windows implementation //
////////////////////////////

#include <windows.h>

//https://superuser.com/a/747958/317094
int getpagesize() 
#ifndef MM_IMPLEMENT
;
#else
{
    //For once the Windows API is kind of nice here. The only reason to make
    //this wrapper is to have something portable
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    return si.dwAllocationGranularity;
}
#endif

int memory_alias_map(size_t size, size_t naddr, void **addrs)
#ifndef MM_IMPLEMENT
;
#else
{
    HANDLE m = CreateFileMapping(INVALID_HANDLE_VALUE,
                                 NULL,
                                 PAGE_READWRITE,
                                 0, size,
                                 NULL);
    if (m == NULL)
        return -1;
    DWORD access = FILE_MAP_ALL_ACCESS;
    size_t i;
    for (i = 0; i < naddr; i++) {
        addrs[i] = MapViewOfFileEx(m, access, 0, 0, size, addrs[i]);
        if (addrs[i] == NULL) {
            memory_alias_unmap(size, i, addrs);
            CloseHandle(m);
            return -1;
        }
    }
    CloseHandle(m);
    return 0;
}
#endif

void memory_alias_unmap(size_t size, size_t naddr, void **addrs)
#ifndef MM_IMPLEMENT
;
#else
{
    (void)size;
    for (size_t i = 0; i < naddr; i++)
        UnmapViewOfFile(addrs[i]);
}
#endif

//////////////////////////
// POSIX implementation //
//////////////////////////


#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

int memory_alias_map(size_t size, size_t naddr, void **addrs)
#ifndef MM_IMPLEMENT
;
#else
{
    char path[128];
    snprintf(path, sizeof(path), "/%s(%lu,%p)",
             __FUNCTION__, (long)getpid(), addrs);
    int fd = shm_open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd == -1)
        return -1;
    shm_unlink(path);
    ftruncate(fd, size);
    size_t i;
    for (i = 0; i < naddr; i++) {
        addrs[i] = mmap(addrs[i], size,
                        PROT_READ | PROT_WRITE, 
                        ((addrs[i])?MAP_FIXED:0) | MAP_SHARED,
                        fd, 0);
        if (addrs[i] == MAP_FAILED) {
            memory_alias_unmap(size, i, addrs);
            close(fd);
            return -1;
        }
    }
    close(fd);
    return 0;
}
#endif

void memory_alias_unmap(size_t size, size_t naddr, void **addrs)
#ifndef MM_IMPLEMENT
;
#else
{
    for (size_t i = 0; i < naddr; i++)
        munmap(addrs[i], size);
}
#endif

#endif

#endif
