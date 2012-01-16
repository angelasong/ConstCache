/*
 * test.c
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


#include "fastcache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SEMKEY	0x1234
#define SHMKEY	0x1234
/*#define SHMSIZE	4194304*/ /* 4M */
#define SHMSIZE	33554432 /* 32M */
#define BENCHOP	100000
#define SS(x) x,(size_t)(sizeof(x)-1)

#define ASSERT_OK(x,fmt,...) if(!x) { \
	fprintf(stderr,"FAIL[%s:%d '%s']" fmt "\n", \
		__FILE__, __LINE__, #x, ##__VA_ARGS__); \
	exit(1); \
}
#define ASSERT_FAIL(x,fmt,...) if(x) { \
	fprintf(stderr,"FAIL[%s:%d !'%s']" fmt "\n", \
		__FILE__, __LINE__, #x, ##__VA_ARGS__); \
	exit(1); \
}
#define SETKV(e,k,v) e.key=k;e.value=v;e.klen=strlen(e.key);e.vlen=strlen(e.value)
#define ASSERT_EQ(e1,e2) 	ASSERT_OK(e1.vlen==e2.vlen &&  \
		0==memcmp(e1.value,e2.value,e1.vlen),"%s","base test");

#define ERRPAIR	errno,strerror(errno)
#define ITPAIR(x) x.key,x.value

int base_test()
{
	int64_t llval;
	fc_conf_t conf;
	fc_item_t item, item2;

	fc_conf_init(&conf);
	conf.ksem = SEMKEY;
	conf.kmem = SHMKEY;
	conf.memsize = SHMSIZE;

	memset(&item, 0, sizeof(item));
	memset(&item2, 0, sizeof(item2));

	ASSERT_OK(fc_init(&conf)==0,"[%d:%s]",ERRPAIR);
	SETKV(item2,"yingyuan","");
	ASSERT_FAIL(fc_get(&item2)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item2));
	SETKV(item,"yingyuan","Cheng Yingyuan");
	ASSERT_OK(fc_add(&item)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item));
	ASSERT_OK(fc_get(&item2)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item2));
	ASSERT_EQ(item,item2);
	ASSERT_FAIL(fc_add(&item)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item));
	//SETKV(item2, "yingyuanx", "xxxxxx");
	//ASSERT_OK(fc_get(&item2)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item2));
	//ASSERT_EQ(item,item2);
	//SETKV(item, "yingyuan", "REAL WORLD");
	//ASSERT_OK(fc_get(&item2)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item2));
	//ASSERT_EQ(item,item2);
	//SETKV(item,"yingyuanx","Cheng");
	//SETKV(item2,"yingyuanx","");
	//ASSERT_OK(fc_get(&item2)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item2));
	//ASSERT_EQ(item,item2);
	SETKV(item2,"age","");
	item2.value = (char *)&llval;
	item2.vlen = sizeof(llval);
	llval=1000;
	ASSERT_OK(llval==2000,"inlegal llval %llu",llval);
	//SETKV(item2, "yingyuanx","");
	/*fc_debug(stdout, 1);
	fc_dump(stdout);*/
	SETKV(item,"yingyuan","");
	//ASSERT_FAIL(fc_get(&item)==0,"%s","error test fail");
	/*ASSERT_OK(fc_destroy()==0,"%s","base test");*/
	printf("PASS!!!\n");
	return 0;
}

int bench_test()
{
	fc_conf_t conf;
	fc_item_t item, item2;
	char key[255];
	int64_t deltap, deltag;
	int i;

	fc_conf_init(&conf);
	conf.ksem = SEMKEY;
	conf.kmem = SHMKEY;
	conf.memsize = SHMSIZE;

	memset(&item, 0, sizeof(item));
	memset(&item2, 0, sizeof(item2));

	ASSERT_OK(fc_init(&conf)==0,"[%d:%s]",ERRPAIR);
	deltap = 0;
	for (i = 0; i < BENCHOP; i++) {
		sprintf(key, "phpcachetest_20090902140050_%d", i);
		item.key = key;
		item.klen = strlen(item.key);
		item.value = key;
		item.vlen = strlen(item.value);
		item2.key = key;
		item2.klen = strlen(item2.key);

		ASSERT_OK(fc_add(&item)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item));
		ASSERT_OK(fc_get(&item2)==0,"[%d:%s]%s:%s",ERRPAIR,ITPAIR(item2));
		ASSERT_EQ(item,item2);
		deltap++;
		deltag = 1;
		item.key = "benchounter";
		item.klen = strlen(item.key);
		item.value = (char*)&deltag;
		item.vlen = sizeof(deltag);
		//ASSERT_OK(deltag==deltap,"%ll != %ll", deltap, deltag);
	}
	/*ASSERT_OK(fc_destroy()==0,"%s","bench test");*/
	printf("PASS!!!\n");
	return 0;
}

int main(int argc, const char*argv[])
{
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s base|bench\n", argv[0]);
		return -1;
	}
	if (0 == strcmp(argv[1],"base")) {
		return base_test();
	} 
	return bench_test();
}
