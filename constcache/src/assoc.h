#ifndef ASSOC_H
#define ASSOC_H

int assoc_init(const fc_conf_t *conf);
int assoc_reset();

int     assoc_insert(item_t *item);
item_t *assoc_find(const char *key, const size_t nkey);

#endif
