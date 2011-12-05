/*
 * slabs.c
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

/*
* Slabs memory allocation, based on powers-of-N. Slabs are up to 1MB in size
* and are divided into chunks. The chunk sizes start off at the size of the
* "item" structure plus space for a small key and value. They increase by
* a multiplier factor from there, up to half the maximum slab size. The last
* slab size is always 1MB, since that's the maximum item size allowed by the
* memcached protocol.
*/
#include "fastcache_imp.h"

static void _item_link_qhead(item_t* item) 
{
	item->prev = NULL;
	item->next = fcc->item_head;
	if (item->next) 
		GPTR(item->next)->prev = PPTR(item);
	fcc->item_head = PPTR(item);
}

int slabs_init(const fc_conf_t *conf) 
{
	size_t size = SZALIGN(sizeof(cache_t), 8);

	if (fc->size < size) {
		RERROR("no mem to init slabs:%u < %u",
			fc->size, size);
		errno = ENOMEM;
		return -1;
	}
	fc->cachetab = fc->base;
	fcc = GPTR(fc->cachetab);
	fc->base += size;
	fc->size = fc->memsize - (size_t)fc->base;

	memset(fcc, 0, size);
	return 0;
}

void* slabs_get_reset_base()
{
	size_t size = SZALIGN(sizeof(cache_t), 8);
	return fc->cachetab + size;
}

int slabs_reset()
{
	memset(fcc, 0, sizeof(cache_t));
	return 0;
}

item_t *slabs_alloc(size_t size) 
{
	item_t* item;
	if (fc->size < size) {
		FCC_INC(fail_items);
		return NULL;
	}
	item = GPTR(fc->base);
	fc->base += size;
	fc->size -= size;
	memset(item, 0, size);

	FCC_INC(total_items);
	_item_link_qhead(item);
	return item;
}
