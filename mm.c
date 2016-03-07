#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned char uchar;

uchar mem[65536];

struct block {
	struct block* pv;
	struct block* nx;
	int free;
	size_t sz;
};

typedef struct block block_t;

block_t* bhd = NULL;

void init1() {
	bhd = &mem[0];
	bhd->sz = 0;
	bhd->pv = NULL;
	bhd->free = 0;
	bhd->nx = &mem[16];
	bhd->nx->sz = 65536-48;
	bhd->nx->free = 1;
	bhd->nx->pv = bhd;
	bhd->nx->nx = &mem[65536-16];
	bhd->nx->nx->sz = 0;
	bhd->nx->nx->free = 0;
	bhd->nx->nx->pv = bhd->nx;
	bhd->nx->nx->nx = NULL;
}

void* malloc1(size_t sz) {
	//printf("malloc begin size=%d\n", sz);
	block_t *b = bhd, *c = NULL;
	//for (b = bhd ; b != NULL ; b = b->nx) 
	//	printf("addr=%d size=%d free=%d prev=%d next=%d\n", 
	//		(int)(b), b->sz, b->free, (int)(b->pv), (int)(b->nx));
	for (b = bhd ; b != NULL ; b = b->nx) {
		if (b->free && b->sz >= sz+16) {
			c = (block_t*)((uchar*)(b)+16+sz);
			b->nx->pv = c;
			c->nx = b->nx;
			c->pv = b;
			c->sz = b->sz-sz-16;
			c->free = 1;
			b->nx = c;
			b->sz = sz;
			b->free = 0;
			return (void*)((uchar*)(b)+16);
		}
	}
	return NULL;
};

void* malloc2(size_t sz) {
	//printf("malloc begin size=%d\n", sz);
	block_t *b = bhd, *opt = NULL, *c = NULL;
	for (b = bhd ; b != NULL ; b = b->nx) {
		//printf("addr=%d size=%d free=%d prev=%d next=%d\n", 
		//	(int)(b), b->sz, b->free, (int)(b->pv), (int)(b->nx));
		if (b->free && b->sz >= sz+16) 
			if (opt == NULL || b->sz < opt->sz) opt = b;
	} 
	if (opt == NULL) return NULL;
	c = (block_t*)((uchar*)(opt)+16+sz);
	c->nx = opt->nx;
	c->pv = opt;
	c->sz = opt->sz-sz-16;
	c->free = 1;
	opt->nx->pv = c;
	opt->nx = c;
	opt->sz = sz;
	opt->free = 0;
	return (void*)((uchar*)(opt)+16);
}

void* malloc3(size_t sz) {
	//printf("malloc begin size=%d\n", sz);
	block_t* b = bhd, *opt = NULL, *c = NULL;
	for (b = bhd ; b != NULL ; b = b->nx) {
		//printf("addr=%d size=%d free=%d prev=%d next=%d\n", 
		//	(int)(b), b->sz, b->free, (int)(b->pv), (int)(b->nx));
		if (b->free && b->sz >= sz+16) 
			if (opt == NULL || b->sz > opt->sz)	opt = b;
	} 
	if (opt == NULL) return NULL;
	c = (block_t*)((uchar*)(opt)+16+sz);
	c->nx = opt->nx;
	c->pv = opt;
	c->sz = opt->sz-sz-16;
	c->free = 1;
	opt->nx->pv = c;
	opt->nx = c;
	opt->sz = sz;
	opt->free = 0;
	return (void*)((uchar*)(opt)+16);
}

void free1(void* s) {
	block_t* b = (block_t*)(s-16);
	b->free = 1;
	//printf("free addr=%d size=%d prev=%d next=%d\n", 
	//	(int)(b), b->sz, (int)(b->pv), (int)(b->nx));
	while (b->pv->free != 0) {
		//printf("prev merge sz1=%d sz2=%d free1=%d free2=%d\n", b->sz, b->pv->sz, b->free, b->pv->free);
		b->pv->sz += b->sz+16;
		b->pv->nx = b->nx;
		b->nx->pv = b->pv;
		b = b->pv;
	}
	while (b->nx->free != 0) { 
		//printf("next merge sz1=%d sz2=%d free1=%d free2=%d\n", b->sz, b->nx->sz, b->free, b->nx->free);
		b->sz += b->nx->sz+16;
		b->nx->nx->pv = b;
		b->nx = b->nx->nx;
	}
	//printf("free end\n");
}

uchar bts[260];

void binit() {
	int lv = 7, i = 0;
	for ( ; lv >= 0 ; lv--) 
		for (i = 1<<(7-lv) ; i < 1<<(8-lv) ; i++)
			bts[i] = (1<<(lv+1))-1;
}

void* bmalloc(size_t sz) {
	sz += 4;		// store block size
	int i = 1, j = 0, ilv = 7, slv = 0;
	void* ret = NULL;
	void* bas = &mem[0];
	while (sz > (1<<(slv+9))) slv++;
	for (; ilv >= slv ; ilv--) {
		if (bts[i]&(1<<ilv)) {
			i <<= (ilv-slv);
			ret = bas; bts[i] = 0;
			while (++slv < 8) {
				if (slv <= ilv)
					bts[i^1] = (1<<slv)-1;
				i >>= 1;
				bts[i] = bts[(i<<1)+1] | bts[i<<1];
			}
			break;
		}
		if (!(bts[i]&(1<<slv)))
			break;
		if (bts[i<<1]&(1<<slv)) 
			i = (i<<1);
		else
			i = (i<<1)+1, bas += 1<<ilv;
	}
	if (ret == NULL) return NULL;
	*(size_t*)(ret) = sz;
	return ret+4;	
}

void bfree(void* s) {
	int sz = *(size_t*)(s-=4);
	int slv = 0;
	while (sz > (1<<(slv+9))) slv++;
	int i = 1, j = 0, ilv = 7;
	void* bas = &mem[0];
	for ( ; ilv >= slv ; ilv--) 
		if (s < bas+(1<<(ilv+8)))
			i = (i<<1);
		else
			i = (i<<1)+1, bas += 1<<ilv;
	bts[i] = (i<<(slv+1))-1;
	while (i >>= 1, ++ilv < 8) 
		bts[i] |= bts[i<<1] | bts[(i<<1)+1];
}
void* bs[256]; int bsN = 0, _bsN = 0;

int main(int argc, char** argv) {
	printf("sizeof(block_t)=%d\n", sizeof(block_t));
	srand(time(0));
	int t = 0, i = 0, j = 0;
	sscanf(argv[1], "%d", &t);
	init1();
	int malloc_ct = 0, miss_ct = 0;
	clock_t c1, c2;
	c2 = 0; bsN = 0;
	for (i = 0 ; i < t ; i++) {
		if (!(i>>4) || !bsN || rand()&1) {
			c1 = clock();
			bs[bsN] = malloc1(rand()%(1<<12));
			malloc_ct++;
			if (!bs[bsN]) miss_ct++;
					 else bsN++;
			c2 += clock()-c1;
		} else {
			while (bs[j=rand()%bsN] == NULL);
			c1 = clock();
			free1(bs[j]); 
			c2 += clock()-c1;
			for (bsN-- ; j < bsN ; j++)
				bs[j] = bs[j+1];
		}
	}
	printf("Strategy 1 : position first\n");
	printf("time         : %d ms\n", c2);
	printf("malloc count : %d\n", malloc_ct);
	printf("miss count   : %d\n", miss_ct); 
	init1(); malloc_ct = 0, miss_ct = 0;
	c1 = clock(); c2 = 0; bsN = 0;
	for (i = 0 ; i < t ; i++) {
		if (!(i>>4) || !bsN || rand()&1) {
			c1 = clock();
			bs[bsN] = malloc2(rand()%(1<<12));
			malloc_ct++;
			if (!bs[bsN]) miss_ct++;
					 else bsN++;
			c2 += clock()-c1;
		} else {
			while (bs[j=rand()%bsN] == NULL);
			c1 = clock();
			free1(bs[j]); 
			c2 += clock()-c1;
			for (bsN-- ; j < bsN ; j++)
				bs[j] = bs[j+1];
		}
	}
	printf("Strategy 2 : size suit first\n");
	printf("time         : %d ms\n", c2);
	printf("malloc count : %d\n", malloc_ct);
	printf("miss count   : %d\n", miss_ct); 
	init1(); malloc_ct = 0, miss_ct = 0;
	c1 = clock(); c2 = 0; bsN = 0;
	for (i = 0 ; i < t ; i++) {
		if (!(i>>4) || !bsN || rand()&1) {
			c1 = clock();
			bs[bsN] = malloc3(rand()%(1<<12));
			malloc_ct++;
			if (!bs[bsN]) miss_ct++;
					 else bsN++;
			c2 += clock()-c1;
		} else {
			while (bs[j=rand()%bsN] == NULL);
			c1 = clock();
			free1(bs[j]); 
			c2 += clock()-c1;
			for (bsN-- ; j < bsN ; j++)
				bs[j] = bs[j+1];
		}
	}
	printf("Strategy 3 : size large first\n");
	printf("time         : %d ms\n", c2);
	printf("malloc count : %d\n", malloc_ct);
	printf("miss count   : %d\n", miss_ct); 
	binit(); malloc_ct = 0, miss_ct = 0;
	c1 = clock(); c2 = 0; bsN = 0;
	for (i = 0 ; i < t ; i++) {
		if (!(i>>4) || !bsN || rand()&1) {
			c1 = clock();
			bs[bsN] = bmalloc(rand()%(1<<12));
			malloc_ct++;
			if (!bs[bsN]) miss_ct++;
					 else bsN++;
			c2 += clock()-c1;
		} else {
			while (bs[j=rand()%bsN] == NULL);
			c1 = clock();
			bfree(bs[j]); 
			c2 += clock()-c1;
			for (bsN-- ; j < bsN ; j++)
				bs[j] = bs[j+1];
		}
	}
	c2 = clock();
	printf("Strategy 4 : Buddy System\n");
	printf("time         : %d ms\n", c2);
	printf("malloc count : %d\n", malloc_ct);
	printf("miss count   : %d\n", miss_ct); 	
}
