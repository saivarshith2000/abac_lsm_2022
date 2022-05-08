#ifndef _ABAC_CACHE_H
#define _ABAC_CACHE_H

#include <linux/limits.h>

#define ABAC_CACHE_ENTRIES 256

struct abac_cache_entry {
	unsigned int uid;
	char path[PATH_MAX];
	int decision;
};
typedef struct abac_cache_entry abac_cache_entry;

int search_cache(unsigned int uid, char* path);
void insert_cache(unsigned int uid, char *path, int decision);
void clear_cache(void);

#endif /* _ABAC_CACHE_H */
