/*
 * items.c
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

#define ITEM_SIZE(nk,nv) (sizeof(item_t)+(nk)+(nv))

item_t *do_item_alloc(const char *key, const size_t nkey, const size_t nbytes) 
{
	item_t *it = NULL;
	it = slabs_alloc(ITEM_SIZE(nkey, nbytes));
	if(!it) return NULL;

	it->nbytes = nbytes;
	it->nkey = nkey;
	memcpy(ITEM_key(it), key, nkey);
	return it;
}
