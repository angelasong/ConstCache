/*
 * slabs.c
 */
/*+---------------------------------------------------------------------------+
  |Copyright (c) 2008-2011 Beijing Kunlun Tech Co., Ltd.  All rights reserved.|
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
