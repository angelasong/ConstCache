/*
 * fastcache-imp.c
 */
/*
 * Copyright (c) 2008-2012 Beijing Kunlun Tech Co., Ltd.  All rights reserved.
 * Use is subject to license terms.
 */
/*
  +---------------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,           |
  | that is bundled with this package in the file LICENSE, and is             |
  | available through the world-wide-web at the following url:                |
  | http://www.php.net/license/3_01.txt                                       |
  | If you did not receive a copy of the PHP license and are unable to        |
  | obtain it through the world-wide-web, please send a note to               |
  | license@php.net so we can mail you a copy immediately.                    |
  +---------------------------------------------------------------------------+
 */

#include "fastcache_imp.h"
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#define SYSVSEM_SEM     0
#define SYSVSEM_INWRITE 1 // In writing
#define SYSVSEM_USAGE   2
#define SYSVSEM_SETVAL  3

#if !HAVE_SEMUN
union semun {
	int val;                    /* value for SETVAL */
	struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
	unsigned short int *array;  /* array for GETALL, SETALL */
	struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#undef HAVE_SEMUN
#define HAVE_SEMUN 1
#endif

fastcache_t *fc = NULL;
item_t **fch = NULL;
cache_t *fcc = NULL;
static int semid = -1;
static int shmid = -1;

static void _debug_hashtab(FILE *fp, int detail);
static void _debug_cachetab(FILE *fp, int detail);
static void _debug_fastcache(FILE *fp, int detail);
static void _debug_item(FILE *fp, item_t *item);
static void _dump_item(FILE *fp, item_t *it);
static void _dump_item_text(FILE *fp, item_t *it);
static int  _reset_memory();
static int  _initialize_memory(const fc_conf_t *conf);

static inline int _safe_read_lock();
static inline int _safe_read_unlock();

static item_t *item_get(const char *key, const size_t nkey);
static item_t *item_add(char *key, size_t nkey, char *val, int nbytes, uint32_t flags);

#define CHECK_KEYLEN(x) if ((x)<1 || (x)>MAXKEYLEN) { \
	errno=EINVAL; \
	return -1; \
}
#define CHECK_VALLEN(x) if ((x)<1 || (x)>MAXVALLEN) { \
	errno=EINVAL; \
	return -1; \
}

#ifdef CTT_DEBUG
static void _my_trap();
static void _check_integrity();
#define CHECK_INTEGRITY() _check_integrity()
#else
#define CHECK_INTEGRITY()
#endif

void fc_conf_init(fc_conf_t *conf)
{
	memset(conf, 0, sizeof(*conf));
	conf->hashpower = HASHPOWER;
}

int fc_init(const fc_conf_t *fcconf)
{
	long perm = 0666, max_acquire = 1;
	struct sembuf sop[3];
	int count;
	int result = 0;

	semid = semget(fcconf->ksem, 4, perm|IPC_CREAT);
	if (semid < 0) {
		RERROR("semget failed:%d %s", errno, strerror(errno));
		return -1;
	}

	/* Find out how many processes are using this semaphore.  Note
	* that on Linux (at least) there is a race condition here because
	* semaphore undo on process exit is not atomic, so we could
	* acquire SYSVSEM_SETVAL before a crashed process has decremented
	* SYSVSEM_USAGE in which case count will be greater than it
	* should be and we won't set max_acquire.  Fortunately this
	* doesn't actually matter in practice.
	*/

	/* Wait for sem 1 to be zero . . . */
	sop[0].sem_num = SYSVSEM_SETVAL;
	sop[0].sem_op  = 0;
	sop[0].sem_flg = 0;

	/* . . . and increment it so it becomes non-zero . . . */
	sop[1].sem_num = SYSVSEM_SETVAL;
	sop[1].sem_op  = 1;
	sop[1].sem_flg = SEM_UNDO;

	/* . . . and increment the usage count. */
	sop[2].sem_num = SYSVSEM_USAGE;
	sop[2].sem_op  = 1;
	sop[2].sem_flg = SEM_UNDO;
	while (semop(semid, sop, 3) == -1) {
		if (errno != EINTR) {
			RERROR("semop failed:%d %s", errno, strerror(errno));
			return -1;
		}
	}

	/* Get the usage count. */
	count = semctl(semid, SYSVSEM_USAGE, GETVAL, NULL);
	if (count == -1) {
		RERROR("semctl failed:%d %s", errno, strerror(errno));
		result = -1;
	}

	/* If we are the only user, then take this opportunity to set the max. */
	if (count == 1) {
		/* This is correct for Linux which has union semun. */
		union semun semarg;
		semarg.val = max_acquire;
		if (semctl(semid, SYSVSEM_SEM, SETVAL, semarg) == -1) {
			RERROR("semctl setval error:%d %s", errno, strerror(errno));
			result = -1;

		} else if (_initialize_memory(fcconf) == -1) {
			RERROR("failed init mem layout, sz %u", fcconf->memsize);
			result = -1;
		}

		semarg.val = 0;
		if (semctl(semid, SYSVSEM_INWRITE, SETVAL, semarg) == -1) {
			RERROR("semctl setval error:%d %s", errno, strerror(errno));
			result = -1;
		}
	} else if (count > 1) { /* already initialized */
		shmid = shmget(fcconf->kmem, 0, 0);
		if (shmid < 0) {
			RERROR("get mem:%d %s", errno, strerror(errno));
			result = -1;
		} else {
			fc = shmat(shmid, NULL, 0);
			if ((void*)fc == (void*)-1) {
				RERROR("at mem:%d %s", errno, strerror(errno));
				result = -1;

			} else if (memcmp(fc->magic, FCMAGIC, sizeof(FCMAGIC)-1)) {
				RERROR("fc version not compatible");
				errno = EINVAL;
				result = -1;

			} else {
				fch = GPTR(fc->hashtab);
				fcc = GPTR(fc->cachetab);
			}
		}
	}

	/* Set semaphore 1 back to zero. */
	sop[0].sem_num = SYSVSEM_SETVAL;
	sop[0].sem_op  = -1;
	sop[0].sem_flg = SEM_UNDO;
	if (count == 1) {
		sop[1].sem_num = SYSVSEM_USAGE;
		sop[1].sem_op = 1;
		sop[1].sem_flg = 0; /* hold this sem */
		count++;

	} else {
		count = 1;
	}

	while (semop(semid, sop, count) == -1) {
		if (errno != EINTR) {
			RERROR("set sem back:%d %s", errno, strerror(errno));
			result = -1;
			break;
		}
	}
	return result;
}

int fc_destroy()
{
	if (shmid >= 0) {
		if (fc) {
			shmdt(fc);
			fc = NULL;
			fch = NULL;
			fcc = NULL;
		}
		shmctl(shmid, IPC_RMID, NULL);
		shmid = -1;
	}
	if (semid >= 0) {
		semctl(semid, 0, IPC_RMID, NULL);
		semid = -1;
	}
	return 0;
}

int fc_add(const fc_item_t *in)
{
	item_t *it;
	CHECK_KEYLEN(in->klen);
	CHECK_VALLEN(in->vlen);

	it = item_add(in->key, in->klen, in->value, in->vlen, in->flags);
	if (!it) {
		return -1;
	}
	return 0;
}


int fc_get(fc_item_t *inout)
{
	item_t *it;
	CHECK_KEYLEN(inout->klen);

	it = item_get(inout->key, inout->klen);
	if (!it) {
		errno = ESRCH;
		return -1;
	}
	if (it->nbytes > MAXVALLEN) { /* inconsistence happened */
		errno = EINVAL;
		return -1;
	}

	inout->value = ITEM_data(it);
	inout->vlen = it->nbytes;
	inout->flags = it->flags;
	return 0;
}

int fc_stat(fc_stat_t* stat)
{
	if (_safe_read_lock() < 0) {
		return -1;
	}

	long hashfilled = 0;
	int i;
	memcpy(stat->magic, fc->magic, 8);
	stat->magic[8] = 0;
	stat->total_size = fc->memsize;
	stat->free_size = fc->size;
	stat->add_cmds = fc->add_cmds;
	stat->add_hit = fc->add_hit;
	stat->get_cmds = fc->get_cmds;
	stat->get_hit = fc->get_hit;
	stat->total_items = fcc->total_items;
	stat->fail_items = fcc->fail_items;
	stat->hash_size = (unsigned long)1 << fc->hashpower;
	for (i = 0; i < stat->hash_size; i++) {
		if (fch[i]) {
			hashfilled++;
		}
	}
	stat->hash_filled = hashfilled;
	
	_safe_read_unlock();
	return 0;
}

int fc_debug(FILE *fp, int detail)
{
	if (_safe_read_lock() < 0) {
		return -1;
	}
	_debug_fastcache(fp, detail);
	_debug_cachetab(fp, detail);
	_debug_hashtab(fp, detail);
	fprintf(fp, "====================================================\n\n");

	_safe_read_unlock();
	return 0;
}

int fc_dump(FILE *fp)
{
	item_t *it;
	size_t i;

	if (_safe_read_lock() < 0) {
		return -1;
	}
	it = fcc->item_head;
	while (it) {
		it = GPTR(it);
		_dump_item_text(fp, it);
		fprintf(fp, "\n");
		it = it->next;
	}
	_safe_read_unlock();
	return 0;
}

#define DEBUGLIST(start,dirn) do { \
	typeof(start) tempelem = start; \
	while (tempelem) { \
	fprintf(fp, "0x%X ", tempelem); \
	tempelem = (dirn) ? GPTR(tempelem)->next:GPTR(tempelem)->prev; \
	} \
} while(0)

#define DEBUGLISTELEM(start,dirn,func) do { \
	typeof(start) tempelem = start; \
	while (tempelem) { \
	tempelem = GPTR(tempelem); \
	func(fp, tempelem); \
	tempelem = (dirn) ? tempelem->next:tempelem->prev; \
	} \
} while(0)

static void _dump_item(FILE *fp, item_t *it)
{
	static char *hextab = "0123456789ABCDEF";
	size_t j;
	unsigned char *ptr;

	ptr = ITEM_key(it);
	for (j = 0; j < it->nkey; j++)
		fprintf(fp, "%c", ptr[j]);
	ptr = ITEM_data(it);
	for (j = 0; j < it->nbytes; j++) {
		fprintf(fp, " %c%c", hextab[(ptr[j]>>4)&0x0F], 
			hextab[ptr[j]&0x0F]);
	}
}

static void _dump_item_text(FILE *fp, item_t *it)
{
	size_t j;
	unsigned char *ptr;

	ptr = ITEM_key(it);
	fprintf(fp, "key:");
	for (j = 0; j < it->nkey; j++)
		fprintf(fp, "%c", isprint(ptr[j])?ptr[j]:'.');
	fprintf(fp, " value:");
	ptr = ITEM_data(it);
	for (j = 0; j < it->nbytes; j++) {
		fprintf(fp, "%c", isprint(ptr[j])?ptr[j]:'.');
	}
}

static void _debug_item(FILE *fp, item_t *item)
{
#define IVP(x) #x,item->x
	unsigned char *ptr;
	size_t len;
	fprintf(fp, "item 0x%X = {", PPTR(item));
	fprintf(fp, "%s:0x%x, ", IVP(next));
	fprintf(fp, "%s:0x%x, ", IVP(prev));
	fprintf(fp, "%s:0x%x, ", IVP(h_next));
	fprintf(fp, "%s:%lu, ", IVP(nbytes));
	fprintf(fp, "%s:%lu, ", IVP(nkey));
	fprintf(fp, "data: ");
	_dump_item_text(fp, item);
	fprintf(fp, "}\n");
#undef IVP
}

static void _debug_fastcache(FILE *fp, int detail)
{
#define IVP(x) #x,fc->x
#define ELEMDOUBLE(x) fprintf(fp,"%s:%lu, ", IVP(x ## _cmds)); \
	fprintf(fp, "%s:%lu, ", IVP(x ## _hit))

	fprintf(fp, "fc = {");
	fprintf(fp,"magic:'%c%c%c%c%c%c%c%c', ",fc->magic[0],fc->magic[1],
		fc->magic[2],fc->magic[3],fc->magic[4],fc->magic[5],fc->magic[6],fc->magic[7]);
	fprintf(fp, "total_size:%luk, ",fc->memsize/1024);
	fprintf(fp, "free_size:%luk, ",fc->size/1024);
	ELEMDOUBLE(add);
	ELEMDOUBLE(get);
	fprintf(fp, "%s:%lu, ",IVP(hashpower));
	fprintf(fp, "%s:0x%x, ",IVP(hashtab));
	fprintf(fp, "%s:0x%x, ",IVP(cachetab));
	fprintf(fp, "%s:0x%x, ",IVP(base));
	fprintf(fp, "}\n");
#undef IVP
#undef ELEMDOUBLE
}

static void _debug_cachetab(FILE *fp, int detail)
{
#define IVP(x) #x, fcc->x
#define DETAILEL(x,dir) do { \
	fprintf(fp,"%s:0x%x%s",IVP(x),detail?"[ ":", "); \
	if(detail) { \
	DEBUGLIST(fcc->x,dir); \
	fprintf(fp, "], "); \
	} \
	} while(0)
#define ELEM(x) fprintf(fp,"%s:%lu, ",IVP(x))

	fprintf(fp, "fcc = {");
	ELEM(total_items);
	ELEM(fail_items);
	DETAILEL(item_head,1);
	fprintf(fp,"}\n");
	if (detail) {
		DEBUGLISTELEM(fcc->item_head,1, _debug_item);
	}
#undef IVP
#undef ELEM
#undef DETAILEL
}

static void _debug_hashtab(FILE *fp, int detail)
{
	size_t hashsize = (unsigned long)1 << fc->hashpower;
	size_t hashfilled = 0;
	item_t *elem;
	size_t i;
	for (i = 0; i < hashsize; i++) {
		if (fch[i]) {
			hashfilled++;
			if (detail) {
				fprintf(fp, "fch[%lu] = [ ", i);
				elem = fch[i];
				while(elem) {
					fprintf(fp, "0x%X ", elem);
					elem = GPTR(elem)->h_next;
				}
				fprintf(fp, "]\n");
			}
		}
	}
	fprintf(fp, "fch = {hash_size:%lu, hash_filled:%lu, load_factor:%.2f%%}\n",
		hashsize, hashfilled, (double)hashfilled*100.0/(double)hashsize);
}

#ifdef CTT_DEBUG
static void _my_trap()
{
	exit(1);
}

static void _check_integrity()
{
#define CHECKADDR(x) if ((size_t)(x) >= fc->memsize) { \
	RERROR("Oops!!! Check addr fail:%s %p", #x, x); \
	_my_trap(1); \
}

	size_t hashsize = (unsigned long)1 << fc->hashpower;
	item_t *elem;

	size_t i;
	for (i = 0; i < hashsize; i++) {
		if (fch[i]) {
			elem = fch[i];
			while(elem) {
				CHECKADDR(elem);
				elem = GPTR(elem);
				CHECKADDR(elem->next);
				CHECKADDR(elem->prev);
				elem = elem->h_next;
			}
		}
	}

	elem = fcc->item_head;
	if (elem) {
		while (elem) {
			CHECKADDR(elem);
			elem = GPTR(elem);
			CHECKADDR(elem->prev);
			CHECKADDR(elem->h_next);
			elem = elem->next;
		}
	}
}
#endif


static int _reset_memory()
{
	fc->base = slabs_get_reset_base();
	fc->size = fc->memsize - (size_t)fc->base;
#define CLEARFC(x) fc->x = 0
	CLEARFC(add_cmds);
	CLEARFC(add_hit);
	CLEARFC(get_cmds);
	CLEARFC(get_hit);
#undef CLEARFC

	if (0 != assoc_reset() || 0 != slabs_reset()) {
		RERROR("%s", "error reseting assoc and slabs");
		return -1;
	}

	RINFO("%s","shared memory reseted");
	return 0;
}

static int _initialize_memory(const fc_conf_t *conf)
{
	long perm = 0666;
	size_t fcsize = SZALIGN(sizeof(fastcache_t), 8);
	if (conf->memsize < fcsize) { /* No enough mem to store ds */
		RERROR("no mem available:%u < %u", conf->memsize, fcsize);
		errno = ENOMEM;
		return -1;
	}

	shmid = shmget(conf->kmem, conf->memsize, perm|IPC_CREAT);
	if (shmid < 0) {
		RERROR("shmget fail:%d %s", errno, strerror(errno));
		return -1;
	}

	fc = (fastcache_t *)shmat(shmid, NULL, 0);
	if ((void *)fc == (void *)-1) {
		RERROR("shmat fail:%d %s", errno, strerror(errno));
		return -1;
	}

	memset(fc, 0, fcsize);
	memcpy(fc->magic, FCMAGIC, sizeof(FCMAGIC)-1);
	fc->memsize = conf->memsize;
	fc->hashpower = conf->hashpower;

	fc->base = (void *)fcsize;
	fc->size = conf->memsize - (size_t)fc->base;

	if (assoc_init(conf) == -1 || slabs_init(conf) == -1) {
		shmdt(fc);
		RERROR("%s", "error init assoc and slabs");
		return -1;
	}
	fch = GPTR(fc->hashtab);
	fcc = GPTR(fc->cachetab);
	RINFO("%s", "shared memory initialized");
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
static int _sem_lock(int op)
{
	struct sembuf sop[2];
	sop[0].sem_num = SYSVSEM_SEM;
	sop[0].sem_op = op;
	sop[0].sem_flg = SEM_UNDO;

	sop[1].sem_num = SYSVSEM_INWRITE;
	sop[1].sem_op = -op;
	sop[1].sem_flg = SEM_UNDO;

	while (semop(semid, sop, 2) == -1) {
		if (errno != EINTR) {
			RERROR("semop %d error:%d %s", op, errno, strerror(errno));
			return -1;
		}
	}
	return 0;
}

static int _sem_read_lock()
{
	struct sembuf sop[1];
	sop[0].sem_num = SYSVSEM_INWRITE;
	sop[0].sem_op = 0;
	sop[0].sem_flg = SEM_UNDO;

	while (semop(semid, sop, 1) == -1) {
		if (errno != EINTR) {
			RERROR("semop %d error:%d %s", 0, errno, strerror(errno));
			return -1;
		}
	}
	return 0;
}

static inline int _safe_write_lock()
{
	return _sem_lock(-1);
}

static inline int _safe_write_unlock()
{
	CHECK_INTEGRITY();
	_sem_lock(1);
	return 0;
}

static inline int _safe_read_lock()
{
	return _sem_read_lock();
}

static inline int _safe_read_unlock()
{
	return 0;
}

////////////////////////////////////////////////////////////////////
static item_t *item_add(char *key, size_t nkey, char *val, int nbytes, uint32_t flags)
{
	item_t *it;
	if (_safe_write_lock() < 0) {
		return NULL;
	}
	it = assoc_find(key, nkey);
	if(it) {
		errno = EEXIST;
		it = NULL;
	} else {
		it = do_item_alloc(key, nkey, nbytes);
		if(it) {
			assoc_insert(it);
			memcpy(ITEM_data(it), val, nbytes);
			it->flags = flags;
		} else {
			errno = ENOMEM;
		}
	}
	_safe_write_unlock();

	STATINC(add_cmds);
	if (it) STATINC(add_hit);
	return it;
}

static item_t *item_get(const char *key, const size_t nkey)
{
	item_t *it;
	if (_safe_read_lock() < 0) {
		return NULL;
	}
	it = assoc_find(key, nkey);
	_safe_read_unlock();

	STATINC(get_cmds);
	if (it) STATINC(get_hit);
	return it;
}

int fc_reset()
{
	int ret;
	if(_safe_write_lock() < 0) {
		return -1;
	}
	ret = _reset_memory();
	CHECK_INTEGRITY();
	_safe_write_unlock();
	return ret;
}
