#include "cache.h"
#include <linux/string.h>
#include <linux/kernel.h>

abac_cache_entry entries[ABAC_CACHE_ENTRIES];
int count = 0;

int search_cache(unsigned int uid, char* path) {
	/* Search the cache for past decisions based on given UID and object path */
	int i;
	for (i = 0; i < count; i++) {
		if (entries[i].uid == uid && strcmp(entries[i].path, path) == 0) {
			printk("found in cache");
			return entries[i].decision;
		}
	}
	/* entry not found */
	printk("not found in cache");
	return -1;
}

void insert_cache(unsigned int uid, char *path, int decision) {
	/* insert new decisions based on given UID and object path into the cache */
	if (count == ABAC_CACHE_ENTRIES) {
		/* Cache is already full, replace the first entry */
		/* THIS IS NOT FINAL. WE WILL WORK ON IMPLEMENTING A LRU Cache */
		entries[0].uid = uid;
		strcpy(entries[0].path, path);
		entries[0].decision = decision;
	} else {
		/* There is space in cache */
		entries[count].uid = uid;
		strcpy(entries[count].path, path);
		entries[count].decision = decision;
		count++;
	}
	printk("inserted into cache");
}

void clear_cache() {
	/* Clear the cache entries. Used when ABAC data is changed from userspace */
	int i;
	for (i = 0; i < count; i++) {
		entries[i].uid = 0;
		entries[i].path[0] = '\0';
	}
	count = 0;
	printk("cache cleared");
}
