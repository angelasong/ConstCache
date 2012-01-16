/* 
 * assoc.c
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
#include "hash.h"

typedef unsigned int  ub4;   /* unsigned 4-byte quantities */
typedef unsigned char ub1;   /* unsigned 1-byte quantities */

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

int assoc_reset()
{
	size_t size = SZALIGN(hashsize(fc->hashpower)*sizeof(item_t *), 8);
	memset(fch, 0, size);
	return 0;
}

int assoc_init(const fc_conf_t *conf) 
{
	size_t size;

	size = SZALIGN(hashsize(conf->hashpower)*sizeof(item_t *), 8);
	if (fc->size < size) {
		TRACE("no mem to init assoc(%u < %u)",fc->size,size);
		errno = ENOMEM;
		return -1;
	}

	fch = GPTR(fc->base);
	memset(fch, 0, size);
	fc->hashtab = fc->base;
	fc->base += size;
	fc->size -= size;
	return 0;
}

item_t *assoc_find(const char *key, const size_t nkey)
{
	uint32_t hv = hash(key, nkey, 0);
	item_t *it = fch[hv & hashmask(fc->hashpower)];
	item_t *ret = NULL;

	while (it) {
		it = GPTR(it);
		if ((nkey==it->nkey) && (memcmp(key,ITEM_key(it),nkey)==0)) {
			ret = it;
			break;
		}
		it = it->h_next;
	}
	return ret;
}

/* Note: this isn't an assoc_update.  The key must not already exist to call this */
int assoc_insert(item_t *it) 
{
	uint32_t hv;

	/* shouldn't have duplicately named things defined */
	assert(assoc_find(ITEM_key(it),it->nkey)==0);

	hv = hash(ITEM_key(it), it->nkey, 0);
	it->h_next = fch[hv & hashmask(fc->hashpower)];
	fch[hv & hashmask(fc->hashpower)] = PPTR(it);
	return 1;
}
