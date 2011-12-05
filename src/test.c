/*
 * test.c
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
