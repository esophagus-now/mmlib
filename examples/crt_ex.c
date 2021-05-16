#include <stdio.h>

#define MM_IMPLEMENT
#include <mm_coroutine.h>

__attribute__((noinline))
void silly_yeild(crt_ctx *ctx, int i) {
    printf("Returning %d!\n", i);
    crt_early_exit(ctx);
}

//Generates the sequence 1..10
void genseq(crt_ctx *ctx, void *arg) {
    int *ret = (int *)arg;
    int i;
    for (i = 1; i <= 10; i++) {
        *ret = i;
        silly_yeild(ctx, i);
    }
}

int main(void) {
    printf("Hello World\n");
    int val;
    crt_ctx *ctx = crt_new(genseq, &val);

    while(crt_run(ctx) == CRT_IN_PROGRESS) {
        printf("genseq returned %d\n", val);
    }
    
    crt_del(ctx);
    
    return 0;
}
