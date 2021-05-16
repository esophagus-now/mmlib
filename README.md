## Marco's accumulated C libraries

These are C files I noticed myself copying into a lot of different projects. 
Mostly this is inspired by the [stb libraries][1], and I more or less stole the 
idea to use a single "implement.c" files from that project.

[1]: https://github.com/nothings/stb

## How to use

All of these files are `.h` files. Just include them in the usual way, but in
ONE C file, do something like:

```c
#define MM_IMPLEMENT
#include "vector.h"
#include "mm_err.h"
//etc.
```

When the `MM_IMPLEMENT` macro is defined, the many `#ifdef` statements in the 
headers will actually define functions rather than just the prototypes.

## What currently works?

The `mm_err.h`, `list.h`, and `vector.h` headers have stabilized. The `mm_err.h`
header has a big detailed comment at the top explaining how to use it. The `list.h`
header uses the same API as the Linux kernel linked lists. I still need to write
some detailed documentation for using `vector.h`, probably in the form of examples.

## What I'm working on

The `http_parse.h` and `websock.h` headers are part of an ongoing project to write
webapps in C. I assume most people reading this are immediately thinking I'm an 
idiot for wanting to do web dev in C, but hey, it's my life and my code, so quit
judging me already! Currently I want to add a little bit more functionality and 
I desperately want to remove the dependency on OpenSSL (I just need to get around
to providing a SHA-1 implementation, which I may end up needing to do myself).

Also, I'm currently prototyping a hash table implementation [here][2]. It's still
a little too rough for me to put in this repo. The HTTP and WebSockets code is 
actually functional and the API is what I want it to be, but I still have a lot of
design decisions left for the hash table. The biggest issue is that I want to 
preserve type safety and readability of user code, but it's not really going well.

[2]: https://repl.it/@Mahkoe/map

I have several projects that use tokenizing, and I've been trying to come with an 
idea for a simple tokenizer API. Sadly, I've never written any tokenizing code that 
I was ultimately happy with.

