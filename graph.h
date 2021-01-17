#ifdef MM_IMPLEMENT
	#ifndef GRAPH_H_IMPLEMENTED
		#define GRAPH_H_IMPLEMENTED 1
		#define SHOULD_INCLUDE 1
	#else
		#define SHOULD_INCLUDE 0
	#endif
#else
	#ifndef GRAPH_H
		#define GRAPH_H 1
		#define SHOULD_INCLUDE 1
	#else
		#define SHOULD_INCLUDE 0
	#endif
#endif

#if SHOULD_INCLUDE
#undef SHOULD_INCLUDE

#ifdef MM_IMPLEMENT
#undef MM_IMPLEMENT
#include "graph.h"
#define MM_IMPLEMENT 1
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "heap.h"
#include "map.h"
#include "tatham_coroutine.h"

//Only works with square matrices.
void adj_mat_mul(char *dest, char const *srcA, char const *srcB, int n) 
#ifdef MM_IMPLEMENT
{
	//Dest and srcs may not alias, but srcs may alias
	assert(((dest + n*n) <= srcA) || ((srcA + n*n) <= dest));
	assert(((dest + n*n) <= srcB) || ((srcB + n*n) <= dest));
	
	int i, j, k;
	for (i = 0; i < n; i++) {
		char *dest_row = dest + i*n;
		for (j = 0; j < n; j++) {
			char res = 0;
			for (k = 0; k < n; k++) {
				if (srcA[i*n + k] && srcB[k*n + j]) {
					res = 1;
					break;
				}
			}
			dest_row[j] = res;
		}
	}
}
#else
;
#endif

void print_adj_mat(char const *mat, int n) 
#ifdef MM_IMPLEMENT
{
	int i, j;
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			printf("%c", mat[i*n + j] ? '1' : '0');
		}
		puts("");
	}
}
#else 
;
#endif

// Technique: transitive closure of an adjacency A of size n by n is equal to:
//   A^n + A^(n-1) + ... + A
// where multiplication (between elements of A) is AND and and addition (between
// elements of A) is OR. In the above expression, A^n means repeated matrix products.
//
// It turns out this is equal to:
//  (A+I)^(n-1) * A
//
// This works because OR is idempotent. If you do the binomial expansion of 
// (A+I)^(n-1), the (scalar) multiplication in front of terms is actually 
// repeated OR'ing.
//
// One more thing. A graph-theoretical argument (that I won't prove here, mostly
// because I don't know how to prove it) shows that actually we can do
//
//  (A+I)^M * A, where M >= (n-1)
//
// Intuititively, this is because this product still find all the paths of length
// up to N, which is all that is needed to compute transitive closure. The reason
// to raise the matrix to a higher power is because we can just use repeated squaring
// and pick M to be the largest power of 2 that is greater than or equal to (n-1).
//
void transitive_closure(char *dest, char const *src, int n) 
#ifdef MM_IMPLEMENT
{
	//Matrices may not alias
	//TODO: it should be possible to get around this without sacrificing 
	//performance, because I had to copy src anyway to add the identity
	assert(((dest + n*n) <= src) || ((src + n*n) <= dest));
	
	//Corner cases
	assert(n > 0);
	if (n == 1) {
		*dest = *src;
		return;
	}
	
	char *tmp = malloc(n*n);
	if (!tmp) FAST_FAIL("out of memory");
	
	char *tmp2 = malloc(n*n);
	if (!tmp2) FAST_FAIL("out of memory");
	
	//Save (A + I) in tmp
	memcpy(tmp, src, n*n);
	int i;
	for (i = 0; i < n; i++) tmp[i*n + i] = 1;
	
	//Sanity check. We want this loop to run for clog2(n-1) times. Note that
	//n is assumed to be 2 or greater at this point.
	//Suppose n = 2: the loop doesn't run (correct)
	//If n = 3, the loop runs once (correct)
	//If n = 4, the loop runs twice (correct)
	//If n = 5, the loop runs twice (correct)
	//If n = 6, the loop runs three times (correct)
	//Okay, I'm satisfied
	int logerand = n - 2;
	while (logerand > 0) {
		adj_mat_mul(tmp2, tmp, tmp, n);
		char *swap_tmp = tmp;
		tmp = tmp2;
		tmp2 = swap_tmp;
		logerand >>= 1;
	}
	
	adj_mat_mul(dest, src, tmp, n);
	
	free(tmp2);
	free(tmp);
}
#else
;
#endif

//Graph functions (BFS, DFS, Dijkstra, A*, topo sort) all need 
//a generic way to get the neighbours of a node. I see several
//different ways this could be done:
//
// - Caller supplies function that fills a vector of void pointers
// - Caller supplies function that fills a byte array of fixed-size records
// - Caller supplies a coroutine that yields void pointers
// 
// The coroutine is by far my favourite approach, but it really
// makes it harder for others to use the library...
// You know what? The whole reason I'm making these libraries is 
// because I enjoy finding elegant solutions, and because I want 
// libraries that I can use and understand. I don't think using
// coroutines is difficult, and I wouldn't even be worried about 
// the graph library being harder to use if the C language 
// had coroutines built-in and everyone was used to them. They're 
// my libraries, so I decide! 
#ifndef MM_IMPLEMENT
//Return pointer to next neighbour of cur_elem. This is a re-entrant
//coroutine (see tatham_coroutine.h) to let you yield mutiple 
//neighbours for the same element. 
typedef void *neighbour_fn(ccrContext *ccrParam, void const *cur_elem);

//Return nonzero if you want graph search to stop. arg is whatever was 
//given to the graph search function.
typedef int visit_fn(void *elem, void *arg);
#endif

//dijkstra: takes
//     - Initial element 
//     - neighbour_fn
//     - visit_fn
//     - Function to free nodes when finished with them, or NULL to do nothing
//and returns a list of void pointers and a length. The user can free the 
//void pointers in the list if necessary, and should always free the list
//itself.

#endif
