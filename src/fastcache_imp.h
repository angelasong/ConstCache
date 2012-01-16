/* 
 * fastcache_imp.h
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

#ifndef FASTCACHE_IMP_H
#define FASTCACHE_IMP_H

//#define CTT_DEBUG 1

#include "fastcache.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SZALIGN(a,n) ((0==((a)%n))?(a):((a)+(n)-(a)%(n)))

#ifdef CTT_DEBUG
#ifndef DEBUG
#define DEBUG	1
#endif
#include <stdio.h>
#define TRACEMSG(type,fmt,...) fprintf(stderr, "[%s %s:%d]" fmt "\n", \
	type, __FILE__,__LINE__,##__VA_ARGS__)
#else
#include <syslog.h>
#define TRACEMSG(type,fmt,...) syslog(LOG_DAEMON|LOG_INFO, "[%s %s:%d]"fmt, \
	type, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#include <assert.h> /* used with DEBUG macro */

#define TRACE(fmt,...) TRACEMSG("TRACE",fmt,##__VA_ARGS__)
#define RDEBUG(fmt,...) TRACEMSG("DEBUG",fmt,##__VA_ARGS__)
#define RINFO(fmt,...) TRACEMSG("INFO",fmt,##__VA_ARGS__)
#define RWARN(fmt,...) TRACEMSG("WARN",fmt,##__VA_ARGS__)
#define RERROR(fmt,...) TRACEMSG("ERROR",fmt,##__VA_ARGS__)
#define RFATAL(fmt,...) TRACEMSG("FATAL",fmt,##__VA_ARGS__)

typedef struct _stritem {
	struct _stritem *next;
	struct _stritem *prev;
	struct _stritem *h_next; /* hash chain next */
	uint32_t nbytes;	/* size of value */
	uint8_t  nkey;		/* size of key */
	uint32_t flags;
	void * end[];
} item_t;

#define ITEM_key(item) ((char*)&((item)->end[0]))
#define ITEM_data(item) ((char*)&((item)->end[0])+(item)->nkey)
#define ITEM_ntotal(item) (sizeof(struct _stritem)+(item)->nkey +(item)->nbytes)

typedef struct _cache {
	item_t *item_head;	/* lru list  head */
	uint32_t total_items;	/* items allocated */
	uint32_t fail_items;	/* items failed allocation */
} cache_t;

typedef struct _fastcache {
	char    magic[sizeof(FCMAGIC)-1];
	size_t  memsize;
	uint8_t hashpower;
	uint8_t reserve[7];

	item_t** hashtab;
	cache_t* cachetab;

	void*  base; /* base allocation address */
	size_t size;  /* size of memory available */

	/* percache stat variables */
	uint32_t add_cmds;
	uint32_t add_hit;
	uint32_t get_cmds;
	uint32_t get_hit;
} fastcache_t;

extern fastcache_t *fc; /* vm addr of fast cache */
extern item_t** fch; 	/* vm addr of item hash table */
extern cache_t* fcc;	/* vm addr of cache array */

#define GPTR(a) ((typeof (a))((char*)fc+(size_t)(a)))
#define PPTR(a) ((typeof (a))((char*)(a)-(size_t)fc))

#define STATINC(field) fc->field++
#define FCC_INC(field) fcc->field++

#include "assoc.h"
#include "slabs.h"
#include "items.h"

#endif
