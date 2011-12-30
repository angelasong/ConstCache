#ifndef SLABS_H
#define SLABS_H

int  slabs_init(const fc_conf_t *conf);
int  slabs_reset();
item_t *slabs_alloc(size_t size);

void* slabs_get_reset_base();

#endif
