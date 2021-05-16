//FIXME: need implementations for other architectures. I should probably
//move all the arch-dependent stuff to a separate function. The only thing
//I really need as a way to set the stack pointer to a given variable. 

//FIXME: This code depends on frame pointers. It cannot be compiled with 
//-fomit-frame-pointer. It's definitely possible to fix this, but it's 
//tricky because of the way gcc emits code that accesses local variables 
//(to make a long story short, most of the time gcc accesses a local 
//variable it gets it by indexing rbp, and it's difficult to tell gcc "look
//just put this variable in a register"; I tried using the register keyword
//but it didn't work).

#ifdef MM_IMPLEMENT
	#ifndef MM_COROUTINE_H_IMPLEMENTED
		#define SHOULD_INCLUDE 1
		#define MM_COROUTINE_H_IMPLEMENTED 1
	#else
		#define SHOULD_INCLUDE 0
	#endif
#else
	#ifndef MM_COROUTINE_H
		#define SHOULD_INCLUDE 1
		#define MM_COROUTINE_H 1
	#else 
		#define SHOULD_INCLUDE 0
	#endif
#endif


#if SHOULD_INCLUDE
#undef SHOULD_INCLUDE


#ifdef MM_IMPLEMENT
#undef MM_IMPLEMENT
#include "mm_coroutine.h"
#define MM_IMPLEMENT 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "fast_fail.h"

////////////////////////////////
//Structs, typedefs, and enums//
////////////////////////////////

//For better or for worse, I've decided to make crt_ctx an opaque struct
#ifndef MM_IMPLEMENT

struct _crt_ctx;
typedef struct _crt_ctx crt_ctx;

typedef void crt_fn(crt_ctx *ctx, void *arg);

typedef enum {
  CRT_DONE = 1, //This helps us avoid returning zero using longjmp
  CRT_IN_PROGRESS,
  CRT_ERROR,
  CRT_INVALID
} crt_status;

#else

typedef enum {
  CRT_STATE_CREATED,
  CRT_STATE_READY,
  CRT_STATE_BLOCKED, //FIXME: currently unused
  CRT_STATE_RUNNING,
  CRT_STATE_DONE,
  CRT_STATE_ERROR
} crt_state;

//TODO: I know there is a way to use mmap to make "stack memory" that 
//automatically expands downards. One day I'll figure out how to use it
#define CRT_STACK_SZ 40960
struct _crt_ctx {
    jmp_buf exit_buf;
    jmp_buf resume_buf;
    char stack[CRT_STACK_SZ];
    crt_state state;
    //TODO: something similar to pthread's condition variables?
    crt_fn *fn;
    void *arg;
    crt_status ret;
};
#endif

//////////////////////////////////////////
//Functions for managing crt_ctx structs//
//////////////////////////////////////////

crt_ctx *crt_new(crt_fn *fn, void *arg) 
#ifndef MM_IMPLEMENT
;
#else
{
    crt_ctx *ret = malloc(sizeof(crt_ctx));
    if (!ret) FAST_FAIL("out of memory");
    
    ret->state = CRT_STATE_CREATED;
    ret->fn = fn;
    ret->arg = arg;
}
#endif

//Gracefully ignores NULL inputs
void crt_del(crt_ctx *ctx) 
#ifndef MM_IMPLEMENT
;
#else
{
    free(ctx);
}
#endif
    

//////////////////////////////////////////////////////
//Functions for calling/yielding/aborting coroutines//
//////////////////////////////////////////////////////

void crt_early_exit(crt_ctx *ctx) 
#ifndef MM_IMPLEMENT
;
#else
{
    int rc = setjmp(ctx->resume_buf);
    if (rc != 0) {
        ctx->state = CRT_STATE_RUNNING;
        return;
    } else {
        ctx->state = CRT_STATE_READY;
        ctx->ret = CRT_IN_PROGRESS;
        longjmp(ctx->exit_buf, 1); //GCC's __builtin_longjmp forces you to use 1 here...
    }
}
#endif

void crt_error(crt_ctx *ctx)
#ifndef MM_IMPLEMENT
;
#else
{
    while(1) {
        setjmp(ctx->resume_buf);
        ctx->state = CRT_STATE_ERROR;
        ctx->ret = CRT_ERROR;
        longjmp(ctx->exit_buf, 1);
    }
}
#endif

crt_status crt_run(crt_ctx *ctx) 
#ifndef MM_IMPLEMENT
;
#else
{
  if (ctx->state == CRT_STATE_DONE) return CRT_INVALID;
  int rc = setjmp(ctx->exit_buf);
  if (rc != 0) return ctx->ret;

  if (ctx->state == CRT_STATE_READY) {
    longjmp(ctx->resume_buf, 1);
  } else if (ctx->state == CRT_STATE_CREATED) {
    //The only place we need to do anything dirty... forcibly
    //set the stack pointer to the new stack
    //https://brennan.io/2020/05/24/userspace-cooperative-multitasking/
    void *newsp = (void*) ctx->stack + CRT_STACK_SZ;
#if defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
    //The calling convention on windows is super weird... a caller has to
    //make space on its own stack for arguments to any called functions,
    //even if it's only using registers to pass arguments. The reason is 
    //because the callee will often immediately save the arguments (passed
    //as registers) into this pre-allocated stack space. This means that
    //when we change to the new stack, we need to artifically make space.
    //What a way to run a railroad...
    newsp -= 32; //32 is extra-safe... technically we only need enough room
                 //for the two arguments to ctx->fn
#endif
    __asm volatile (
#ifdef __x86_64__
      "mov %[newsp], %%rsp \n"
#else
      "mov %[newsp], %%esp \n"
#endif
      : 
      : [newsp] "r" (newsp)
      : 
    );

    //Note: so long as you don't compile with -fomit-frame-pointer,
    //gcc will emit instructions that use rbp to find ctx, which is 
    //why this code is still safe
    ctx->state = CRT_STATE_RUNNING;
    ctx->fn(ctx, ctx->arg);
    
    ctx->ret = CRT_DONE;
    longjmp(ctx->exit_buf, 1);
  } else {
    fprintf(stderr, "Error: tried to run something invalid\n");
    return CRT_INVALID;
  }
}
#endif

#endif
