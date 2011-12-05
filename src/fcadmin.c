/*
 * fcadmin.c
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
#include <unistd.h>

#define SHOWERR(fmt,...) fprintf(stderr, "ERROR [%s:%d]"fmt"\n", \
	__FILE__,__LINE__,##__VA_ARGS__)

void usage()
{
	fprintf(stderr, "USAGE: fcadmin -S {semkey} -M {memkey} [-N {memsize}] [-h {hashpower}]\n"
		"\t[-k {key}] [-v {value}] COMMAND\n"
		"COMMAND:\n"
		"\tinit     Initialize shared memory and semaphore\n"
		"\tadd      Add (key,value) to cache\n"
		"\tget      Get the value of key from cache\n"
		"\tdebug    Debug data structure of cache\n"
		"\tvdebug   Debug detail of data structure of cache\n"
		"\tdump     Dump data of cache\n"
		"\treset    Reset cache memory\n"
		"\tdestroy  Destroy cache\n");
}

static inline int nbase(const char *str)
{
	if (!str || !str[0] || !str[1]) {
		return 0;

	} else if (str[0]=='0') {
		if (str[1]=='x' || str[1]=='X')
			return 16;
		return 8;
	}
	return 10;
}

int main(int argc, char* const argv[])
{
	fc_conf_t conf;
	fc_item_t item;
	int o;
	char *cmd;

	if (argc == 1) {
		usage();
		return 0;
	}

	fc_conf_init(&conf);
	memset(&item, 0, sizeof(item));
	while ((o=getopt(argc,argv,"S:M:N:h:k:v:")) != -1) {
		switch(o) {
		case 'S':
			conf.ksem = strtoul(optarg,NULL,nbase(optarg));
			if (conf.ksem <= 0) {
				SHOWERR("invalid sem key %lu",conf.ksem);
				return 1;
			}
			break;
		case 'M':
			conf.kmem = strtoul(optarg,NULL,nbase(optarg));
			if (conf.kmem <= 0) {
				SHOWERR("invalid mem key %lu",conf.kmem);
				return 1;
			}
			break;
		case 'N':
			conf.memsize = strtoul(optarg,NULL,nbase(optarg));
			if (conf.memsize < 1024*1024) { /* min 1M */
				SHOWERR("invalid memsize %lu",conf.memsize);
				return 1;
			}
			break;
		case 'h':
			conf.hashpower = strtoul(optarg,NULL,nbase(optarg));
			if (conf.hashpower < 1) {
				SHOWERR("invalid hashpower %d",conf.hashpower);
				return 1;
			}
			break;
		case 'k':
			item.key = strdup(optarg);
			item.klen = strlen(item.key);
			break;
		case 'v':
			item.value = strdup(optarg);
			item.vlen = strlen(item.value);
			break;
		default:
			SHOWERR("invalid option '-%c'",o);
			return 1;
		}
	}

	if (optind+1 != argc) {
		usage();
		return 1;
	}

	cmd = argv[optind];
	if (!conf.ksem || !conf.kmem) {
		SHOWERR("%s","incorrect option '-S' and '-M'");
		return 1;
	}

	if (0 != fc_init(&conf)) {
		SHOWERR("failed init:%d %s", errno, strerror(errno));
		return 1;
	}

#define FCCALL(kstr,expr) \
	else if(!strcmp(cmd,kstr)) { \
		if (0 != (expr)) { \
			SHOWERR("failed call '%s':%d %s",#expr,errno, strerror(errno)); \
			return 1; \
		} \
	}

	if (!strcmp(cmd, "init"))
		return 0;
	FCCALL("debug",fc_debug(stdout,0))
	FCCALL("vdebug",fc_debug(stdout,1))
	FCCALL("dump",fc_dump(stdout))
	else if (!strcmp(cmd, "destroy")) {
		if (0 != fc_destroy()) {
			SHOWERR("failed call 'fc_destroy':%d %s",errno,strerror(errno));
			return 1;
		}
	}
	else if (!strcmp(cmd, "reset")) {
		if (0 != fc_reset()) {
			SHOWERR("failed call 'fc_reset':%d %s",errno,strerror(errno));
			return 1;
		}
	}

	FCCALL("add",fc_add(&item))
	else if (!strcmp(cmd, "get")) {
		if (0 != fc_get(&item)) {
			SHOWERR("failed call 'fc_get':%d %s",errno,strerror(errno));
			return 1;
		}
		printf("%s %s\n", item.key, item.value);
	}
	else {
		usage();
		return 1;
	}

	return 0;
}
