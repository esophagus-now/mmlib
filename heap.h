#ifdef MM_IMPLEMENT
	#ifndef HEAP_H_IMPLEMENTED
		#define HEAP_H_IMPLEMENTED 1
		#define SHOULD_INCLUDE 1
	#else
		#define SHOULD_INCLUDE 0
	#endif
#else
	#ifndef HEAP_H
		#define HEAP_H 1
		#define SHOULD_INCLUDE 1
	#else
		#define SHOULD_INCLUDE 0
	#endif
#endif

#if SHOULD_INCLUDE
#undef SHOULD_INCLUDE

#ifdef MM_IMPLEMENT
#undef MM_IMPLEMENT
#include "heap.h"
#define MM_IMPLEMENT 1
#endif

#include <string.h>
#include "fast_fail.h"

#ifndef MM_IMPLEMENT
#ifndef HAVE_COMPAR_TYPEDEF
#define HAVE_COMPAR_TYPEDEF 1
typedef int compar_fn(void const *a, void const *b);
#endif
#endif

#ifdef MM_IMPLEMENT
//Implements a min-heap (specifically, the item at the root of the heap will
//compare as being smaller or equal to all the others acording to the given 
//comparison function [following the usual convention that a negative return 
//means the first argument goes before the second]).
//Assumes base[root] is empty, and fills it with the smallest of the following
//three elements: {base[2*root + 1], base[2*root + 2], elem}. If a child element
//was written, the function keeps bublling upwards into the space left behind
//after the child element was moved. Of course, this function uses parameter n 
//(the total size of the heap) to avoid going out of bounds. 
static void bubble_upwards(
	void *base, void const *elem, unsigned elem_sz, 
	unsigned root, unsigned n, compar_fn *cmp) 
{
	if (n == 0) {
		FAST_FAIL("Trying to add an element to an empty heap");
	}

	//While the current node has at least two children...
	//while (2*root + 2 < n)
	while (root < (n-1)/2) {
		unsigned left = 2*root + 1;
		unsigned rite = 2*root + 2;
		
		unsigned smaller;
		if (cmp(base + left*elem_sz, base + rite*elem_sz) <= 0) {
			smaller = left;
		} else {
			smaller = rite;
		}
		
		//If current element is less than or equal to smaller child,
		//just write it and quit early.
		//TODO: Should I do some benchmarking and change it to be 
		//strictly less-than?
		if (cmp(elem, base + smaller*elem_sz) <= 0) {
			//This is where the given element goes. Write it and
			//quit early
			memcpy(base + root*elem_sz, elem, elem_sz);
			return;
		}
		
		//Move the child onto the current node
		memcpy(base + root*elem_sz, base + smaller*elem_sz, elem_sz);
		root = smaller;
	}
	
	//Corner case: last node has only a left child
	if (root*2 + 1 < n) {
		unsigned left = root*2 + 1;
		//Notice that the condition is the opposite of the one in the
		//while loop just above us.
		//If the current element is actually greater than the left child,
		//copy hte left child onto the root and have the root point to 
		//the space left behind after moving the left child.
		if (cmp(elem, base + left*elem_sz) > 0) {
			memcpy(base + root*elem_sz, base + left*elem_sz, elem_sz);
			root = left;
		}
	}
	
	//root points to where the given element should be inserted
	memcpy(base + root*elem_sz, elem, elem_sz);
	return;
}

//Same semantics as bubble_upwards, but downwards
static void bubble_downwards(
	void *base, void const *elem, unsigned elem_sz,
	unsigned root, unsigned n, compar_fn *cmp
) {
	while (root > 0) {
		unsigned parent = (root - 1)/2;
		
		//If parent is less-or-equal to given element, break early
		if (cmp(base + parent*elem_sz, elem) <= 0) {
			break;
		}
		
		memcpy(base + root*elem_sz, base + parent*elem_sz, elem_sz);
		root = parent;
	}
	
	//root points to where we should put the given element
	//TODO: should I add a check that makes sure base+root*elem_sz != elem?
	memcpy(base + root*elem_sz, elem, elem_sz);
	return;
}
#endif

//n should be the size of the heap INCLUDING the element we are about to add
void __heap_insert(void *base, void const *elem, unsigned elem_sz, unsigned n, compar_fn *cmp)
#ifdef MM_IMPLEMENT
{
	bubble_downwards(base, elem, elem_sz, n-1, n, cmp);
}
#else
;
#define heap_insert(h, e, n, cmp) \
	__heap_insert(h, e, sizeof(*(h)), n, cmp)
	
#define vector_heap_insert(v, e, cmp)                \
do {                                                 \
	vector_extend_if_full(v);                        \
	v##_len++;                                       \
	__heap_insert(v, e, sizeof(*(v)), v##_len, cmp); \
} while(0)
#endif

//n is the current size of the heap. The calling code must make sure to decrement
//its own length variable
void __heap_pop(void *base, void *elem_dest, unsigned elem_sz, unsigned n, compar_fn *cmp)
#ifdef MM_IMPLEMENT
{
	if (n == 0) {
		FAST_FAIL("Error, popping from empty heap");
	}
	//Copy the root element to the given destination
	memcpy(elem_dest, base, elem_sz);

	if (n == 1) return; //No bubbling to be done
	
	//Bubble the last element of the heap downwards
	//TODO: there must be a more efficient way to do this...
	bubble_upwards(base, base + (n-1)*elem_sz, elem_sz, 0, n-1, cmp);
}
#else
;
#define heap_pop(h, e, n, cmp) \
	__heap_pop(h, e, sizeof(*(h)), n, cmp)

#define vector_heap_pop(v, e, cmp)                    \
do {                                                  \
	__heap_pop(v, e, sizeof(*(v)), v##_len, cmp); \
	vector_pop(v);                                \
} while(0)
#endif

//TODO: heapify

#endif
