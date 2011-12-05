/* 
 * assoc.c
 */
/*-
 * Copyright (C) 2011 Beijing Kunlun Tech Co. Ltd.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Beijing Kunlun Tech Co.Ltd. nor the names of
 *    its contributions may be used to endorse or promote products derived
 *    from this software without specific prior written permission.   
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINSS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CINTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
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
