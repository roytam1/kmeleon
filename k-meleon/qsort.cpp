/* qsort -- qsort interface implemented by faster quicksort.
   J. L. Bentley and M. D. McIlroy, SPE 23 (1993) 1249-1265.
   Copyright 1993, John Wiley.
*/

typedef int cmp_t(const char *, const char *, unsigned int);

    /*assume sizeof(long) is a power of 2 */
#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;
#define swapcode(TYPE, parmi, parmj, n) {  \
    register TYPE *pi = (TYPE *) (parmi);  \
    register TYPE *pj = (TYPE *) (parmj);  \
    do {                                   \
        register TYPE t = *pi;             \
        *pi++ = *pj;                       \
        *pj++ = t;                         \
    } while ((n -= sizeof(TYPE)) > 0);     \
}
#include <stddef.h>
static void swapfunc(char *a, char *b, size_t n, int swaptype)
{   if (swaptype <= 1) swapcode(long, a, b, n)
    else swapcode(char, a, b, n)
}
#define swap(a, b)                         \
    if (swaptype == 0) {                   \
        t = *(long*)(a);                   \
        *(long*)(a) = *(long*)(b);         \
        *(long*)(b) = t;                   \
    } else                                 \
        swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n) if (n > 0) swapfunc(a, b, n, swaptype)

#ifndef min
#define min(x, y) ((x)<=(y) ? (x) : (y))
#endif

static char *med3(char *a, char *b, char *c, cmp_t *cmp, unsigned int flag)
{	return cmp(a, b, flag) < 0 ?
		  (cmp(b, c, flag) < 0 ? b : cmp(a, c, flag) < 0 ? c : a)
		: (cmp(b, c, flag) > 0 ? b : cmp(a, c, flag) > 0 ? c : a);
}

void quicksort(char *a, size_t n, size_t es, cmp_t *cmp, unsigned flag)
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int r, swaptype;
	long t;
	size_t s;

loop:	SWAPINIT(a, es);
	if (n < 7) {	 /* Insertion sort on smallest arrays */
		for (pm = a + es; pm < a + n*es; pm += es)
			for (pl = pm; pl > a && cmp(pl-es, pl, flag) > 0; pl -= es)
				swap(pl, pl-es);
		return;
	}
	pm = a + (n/2)*es;    /* Small arrays, middle element */
	if (n > 7) {
		pl = a;
		pn = a + (n-1)*es;
		if (n > 40) {    /* Big arrays, pseudomedian of 9 */
			s = (n/8)*es;
			pl = med3(pl, pl+s, pl+2*s, cmp, flag);
			pm = med3(pm-s, pm, pm+s, cmp, flag);
			pn = med3(pn-2*s, pn-s, pn, cmp, flag);
		}
		pm = med3(pl, pm, pn, cmp, flag); /* Mid-size, med of 3 */
	}
	swap(a, pm);
	pa = pb = a + es;
	pc = pd = a + (n-1)*es;
	for (;;) {
		while (pb <= pc && (r = cmp(pb, a, flag)) <= 0) {
			if (r == 0) { swap(pa, pb); pa += es; }
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a, flag)) >= 0) {
			if (r == 0) { swap(pc, pd); pd -= es; }
			pc -= es;
		}
		if (pb > pc) break;
		swap(pb, pc);
		pb += es;
		pc -= es;
	}
	pn = a + n*es;
	s = min(pa-a,  pb-pa   ); vecswap(a,  pb-s, s);
	s = min(pd-pc, pn-pd-(long)es); vecswap(pb, pn-s, s);
	if ((s = pb-pa) > es) quicksort(a,    s/es, es, cmp, flag);
	if ((s = pd-pc) > es) {
		/* Iterate rather than recurse to save stack space */
		a = pn - s;
		n = s / es;
		goto loop;
	}
	/* quicksort(pn-s, s/es, es, cmp, flag); */
}
