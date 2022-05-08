#include <linux/limits.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include "obj.h"

struct obj_hnode {
	char path[PATH_MAX];
	obj_rule *head;
	struct hlist_node node;
};

/* struct to represent a single object's path and rule list */
struct abac_obj {
	char path[PATH_MAX];
	obj_rule *head;
};

#define OBJ_BUCKETS 10 // (2 ^ 10 = 1024 buckets)

DECLARE_HASHTABLE(obj_rule_map, OBJ_BUCKETS);

// Calculate hashes for file paths
static u32 simple_hash(const char *s) {
    u32 key = 0;
    char c;
    while ((c = *s++))
        key += c;
    return key;
}

static struct abac_obj *parse_line(char *line) {
	char *path, *id_str;
	struct abac_obj *obj;
	obj_rule *r;

	obj = kcalloc(1, sizeof(struct abac_obj), GFP_KERNEL);
	path = strsep(&line, ":");
	strcpy(obj->path, path);
	while ((id_str = strsep(&line, ",")) != NULL) {
		r = kcalloc(1, sizeof(obj_rule), GFP_KERNEL);
		kstrtouint(id_str, 10, &(r->id));
		if (obj->head != NULL) {
			r->next = obj->head;
		}
		obj->head = r;
	}
	return obj;
}

/* Used by abac securityfs for parsing the obj_rules file
 * Iterate over the entire file and build a linked list of trees for each object */
void parse_obj_rule_map(char *data) {
	struct abac_obj *temp;
	struct obj_hnode *o;
	char *line;

	hash_init(obj_rule_map);

	while((line = strsep(&data, "\n")) != NULL) {
		/* Ignore empty lines */
		if (strlen(line) < 2) {
			break;
		}
		temp = parse_line(line);
		/* add new user to hash table */
		o = kcalloc(1, sizeof(struct obj_hnode), GFP_KERNEL);
		strcpy(o->path, temp->path);
		o->head = temp->head;
		hash_add(obj_rule_map, &(o->node), simple_hash(o->path));
		printk("Added %s to hashtable", o->path);
	}
}

obj_rule *get_obj_rule_list(char *path) {
	/* Get rules mapped to object at a given path */
	struct obj_hnode *cur;
	obj_rule *head; 
	u32 key = simple_hash(path);
	head = NULL;
	hash_for_each_possible(obj_rule_map, cur, node, key) {
		/* Multiple paths can hash to the same bucket, so compare paths */
		if (strcmp(path, cur->path)) {
			continue;
		}
		head = cur->head;
		break;
	}
	return head;
}

static void clear_rule_list(obj_rule *head) {
	obj_rule *to_free;
	while (head != NULL) {
		to_free = head;
		head = head->next;
		kfree(to_free);
	}
}

void clear_obj_rule_map(void) {
	struct obj_hnode *cur;
	unsigned bkt;
	printk("clearing object hashtable...");
    hash_for_each(obj_rule_map, bkt, cur, node) {
		clear_rule_list(cur->head);
		hash_del(&(cur->node));
    }
}

void print_obj_rule_list(obj_rule *r) {
	while (r != NULL) {
		printk("%u-", r->id);
		r = r->next;
	}
}

void print_obj_rule_map() {
	struct obj_hnode *cur;
	unsigned bkt;
	printk("Printing object hashtable...");
    hash_for_each(obj_rule_map, bkt, cur, node) {
		printk("Path : %s", cur->path);
		print_obj_rule_list(cur->head);
    }
}
