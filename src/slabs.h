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

#ifndef SLABS_H
#define SLABS_H

int  slabs_init(const fc_conf_t *conf);
int  slabs_reset();
item_t *slabs_alloc(size_t size);

void* slabs_get_reset_base();

#endif
