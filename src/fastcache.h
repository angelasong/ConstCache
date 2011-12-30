#ifndef FASTCACHE_H
#define FASTCACHE_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FCMAGIC 		"CCACHE04"
#define FCVERSION		"0.4"

/* default settings */
#define HASHPOWER 		16
#define MAXKEYLEN		254
#define MAXVALLEN		(64*1024*1024)

typedef struct _fc_conf {
	key_t  ksem;
	key_t  kmem;
	size_t memsize;
	int    hashpower;
} fc_conf_t;

typedef struct _fc_item {
	char *key;
	unsigned char klen;
	char *value;
	size_t vlen;
	int flags;
} fc_item_t;

typedef struct _fc_stat {
	char magic[16];
	long total_size;
	long free_size;
	long add_cmds;
	long add_hit;
	long get_cmds;
	long get_hit;
	long total_items;
	long fail_items;
	long hash_size;
	long hash_filled;
} fc_stat_t;

void fc_conf_init(fc_conf_t *conf);
int  fc_init(const fc_conf_t *conf);
int  fc_destroy();
int  fc_add(const fc_item_t *in);
int  fc_get(fc_item_t *inout);
int  fc_debug(FILE *fp, int detail);
int  fc_dump(FILE *fp);
int  fc_reset();
int  fc_stat(fc_stat_t* stat);
		
#ifdef __cplusplus
}
#endif

#endif
