#ifndef __is_test_unit
#include <libc/stdlib.h>
#include <libc/unistd.h>
//#define MALLOC_PRINT
#define MALLOC_CHECK
#endif

#include <libc/string.h>
#include <util/test.h>
#include <util/paging.h>
#include <util/misc.h>

#ifdef MALLOC_PRINT
#include <libc/stdio.h>
#endif

#ifdef MALLOC_CHECK
#include <libc/stdio.h>
#define MALLOC_CHECK_SZ 1000
size_t malloc_check_nb;
uint_ptr malloc_check[MALLOC_CHECK_SZ];

static void check_malloc(uint_ptr ptr) {
	if (malloc_check_nb >= MALLOC_CHECK_SZ)
		fprintf(stderr, "malloc_check size\n");
	else
		malloc_check[malloc_check_nb++] = ptr;
}
static void check_free(uint_ptr ptr) {
	if (!ptr) return;
	for (size_t i = 0; i < malloc_check_nb; ++i)
		if (malloc_check[i] == ptr) {
			malloc_check[i] = malloc_check[--malloc_check_nb];
			return;
		}
	fprintf(stderr, "malloc_check free\n");
}
#endif

#define MBLOC_FREE      0x1
#define MBLOC_PREV_FREE 0x2
#define MBLOC_SIZE_MASK (~(uint32_t)0x7)

//MALLOC_MIN_DSZ + MALLOC_HD_SZ = 0 [MALLOC_ALIGN]
#define MALLOC_HD_SZ     4
#define MALLOC_MIN_DSZ   20
#define MALLOC_ALIGN     8

// blocs alignés à 8 octets, précédés d'un entête de 4 octets indiquant la
// taille de la zone et les flags
// taille minimale d'un bloc (sans entête): 4 + 2*8 = 20 octets
//
// bloc alloué:
//    | size.flags | data ...                           |
//     <--- 4B ---> <-------    dsize >= 20B     ------->
// bloc libre:
//    | size.flags |  prev  |  next  | ... | size.flags |
//                  <- 8B -> <- 8B ->
//     <-------------   size = 0 mod 8  --------------->

typedef struct free_bloc free_bloc_t;
struct free_bloc {
    // jamais nuls (liste circulaire)
    free_bloc_t *prev, *next;
};

// fin du dernier bloc, qui est nécessairement alloué
static uint_ptr    up_lim;
// valeur de brk
static uint_ptr    sbrk_lim;
static free_bloc_t root;

static void* malloc_sbrk_add(size_t inc) {
	void* rt = (void*)up_lim;
	if (up_lim + inc > sbrk_lim) {
		// alloc par page
		uint_ptr new_brk = (up_lim + inc + 0x1000 - 1) & ~(uint_ptr)0xfff;
		sbrk((intptr_t)new_brk - sbrk_lim);
		sbrk_lim = new_brk;
	}
	up_lim += inc;
	return rt;
}
static void malloc_sbrk_sub(size_t inc) {
	up_lim -= inc;
	uint_ptr new_brk = (up_lim + 0x1000 - 1) & ~(uint_ptr)0xfff;
	if (new_brk < sbrk_lim) {
		sbrk(((intptr_t)new_brk) - (intptr_t)sbrk_lim);
		sbrk_lim = new_brk;
	}
}

void _malloc_init() {
#ifdef MALLOC_CHECK
	malloc_check_nb = 0;
#endif
    up_lim = sbrk_lim = (uint_ptr) sbrk(0);
    if (up_lim % MALLOC_ALIGN != MALLOC_ALIGN - MALLOC_HD_SZ) {
        uint_ptr  add = (2 * MALLOC_ALIGN - MALLOC_HD_SZ
							- (up_lim % MALLOC_ALIGN)) % MALLOC_ALIGN;
        up_lim += add;
		malloc_sbrk_add((intptr_t)add);
    }
    root.next = root.prev = &root;
}

static inline uint32_t* bloc_head(void* bloc) {
    return ((uint32_t*)bloc) - 1;
}

static inline void llist_replace(free_bloc_t* old, free_bloc_t* new) {
    new->next       = old->next;
    old->next->prev = new;
    new->prev       = old->prev;
    old->prev->next = new;
}
static inline void llist_rem(struct free_bloc* b) {
    b->next->prev = b->prev;
    b->prev->next = b->next;
}
static inline void llist_insert_after(struct free_bloc* ref,
        struct free_bloc* b) {
    ref->next->prev = b;
    b->next   = ref->next;
    ref->next = b;
    b->prev   = ref;
}
static inline void llist_insert_before(struct free_bloc* ref,
        struct free_bloc* b) {
    ref->prev->next = b;
    b->prev   = ref->prev;
    ref->prev = b;
    b->next   = ref;
}

void* TEST_U(malloc)(size_t dsize) {
	if (!dsize) return NULL;

    size_t size = MALLOC_MIN_DSZ + MALLOC_HD_SZ;
    if (dsize > MALLOC_MIN_DSZ)
        size = align_to_size(dsize + MALLOC_HD_SZ, MALLOC_ALIGN);

    for (free_bloc_t* it = root.next; it != &root; it = it->next) {
        uint32_t* hd = bloc_head(it);
        if (*hd >= size) {
            uint32_t bsz = MBLOC_SIZE_MASK & *hd;

            if (bsz >= size + MALLOC_MIN_DSZ + MALLOC_HD_SZ) {
                free_bloc_t* rem = (free_bloc_t*) (size + (uint_ptr)it);
                uint32_t rsz     = bsz - size;

                *bloc_head(rem) 
					= *(uint32_t*)(bsz - MALLOC_HD_SZ + (uint_ptr)hd)
                    = rsz | MBLOC_FREE;

                llist_replace(it, rem);
                *hd = size | (MBLOC_PREV_FREE & *hd);
            } else {
                uint_ptr nx_hd = ((uint_ptr)it) + bsz - MALLOC_HD_SZ;
                if (nx_hd < up_lim)
                    *((uint32_t*)nx_hd) &= ~(uint32_t)MBLOC_PREV_FREE;

                llist_rem(it);

                *hd &= ~(uint32_t)MBLOC_FREE;
            }
#ifdef MALLOC_PRINT
			fprintf(stderr, "%d: malloc rt(0)=%p\n", getpid(), it);
#endif
#ifdef MALLOC_CHECK
			check_malloc((uint_ptr)it);
#endif
			return it;
        }
    }

    void *rt_hd       = malloc_sbrk_add(size);
    *(uint32_t*)rt_hd = size;
#ifdef MALLOC_PRINT
	fprintf(stderr, "%d: malloc rt(1)=%p\n", getpid(),
			(void*)(MALLOC_HD_SZ + (uint_ptr)rt_hd));
#endif
#ifdef MALLOC_CHECK
	check_malloc(MALLOC_HD_SZ + (uint_ptr)rt_hd);
#endif
    return (void*)(MALLOC_HD_SZ + (uint_ptr)rt_hd);
}

void TEST_U(free)(void* ptr) {
#ifdef MALLOC_PRINT
	fprintf(stderr, "%d: free %p\n", getpid(), ptr);
#endif
#ifdef MALLOC_CHECK
	check_free((uint_ptr)ptr);
#endif
    if (!ptr) return;

    free_bloc_t*      b = (free_bloc_t*) ptr;
    uint32_t        bsz = MBLOC_SIZE_MASK & *bloc_head(b);
    free_bloc_t* insert = &root;
    free_bloc_t* rem_b  = NULL;

    uint_ptr nx_hd = ((uint_ptr)b) + bsz - MALLOC_HD_SZ;
    if (nx_hd < up_lim && (MBLOC_FREE & *((uint32_t*)nx_hd))) {
        free_bloc_t* nb = (free_bloc_t*)(bsz + (uint_ptr)b);
        rem_b   = nb;
        insert  = nb->prev;
        llist_rem(nb);
        bsz    += MBLOC_SIZE_MASK & *(uint32_t*)nx_hd;
    }

    if (MBLOC_PREV_FREE & *bloc_head(b)) {
        uint32_t  pb_hd = *(bloc_head(b) - 1);
        free_bloc_t* pb = (free_bloc_t*)
            ( ((uint_ptr)b) - (MBLOC_SIZE_MASK & pb_hd) );
        if (pb->prev != rem_b)
            insert = pb->prev;
        llist_rem(pb);
        b    = pb;
        bsz += MBLOC_SIZE_MASK & pb_hd;
    }

    if ((bsz +  (uint_ptr)b) >= up_lim)
		malloc_sbrk_sub(bsz);
    else {
        uint32_t* nhd = (uint32_t*)(bsz + (uint_ptr)bloc_head(b));
        *bloc_head(b) = *(nhd - 1) = bsz | MBLOC_FREE;
        *nhd         |= MBLOC_PREV_FREE;

        if (b > insert)
            llist_insert_after(insert, b);
        else
            llist_insert_before(insert, b);
    }
}

void *calloc(size_t nmemb, size_t size) {
    void *ret = malloc(nmemb * size);
    if (ret) {
        memset(ret, 0, nmemb * size);
    }

    return ret;
}

void* realloc(void* ptr, size_t size) {
	if (!size) {
		free(ptr);
		return NULL;
	}
	void *rt = malloc(size);
	if (rt && ptr) {
		size_t sz = (*bloc_head(ptr)) & MBLOC_SIZE_MASK;
		mina_size_t(&sz, size);
		memcpy(rt, ptr, sz);
		free(ptr);
	}
	return rt;
}
