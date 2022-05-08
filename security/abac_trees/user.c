#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include "user.h"

struct user_hnode {
	unsigned int uid;
	struct avp *attrs;
	struct hlist_node node;
};

struct abac_user {
	unsigned int uid;
	struct avp *attrs;
};

#define USER_BUCKETS 8 // (2 ^ 8 = 256 buckets)

DECLARE_HASHTABLE(user_attr_map, USER_BUCKETS);

static struct abac_user *parse_line(char *line) {
	/* Parse a single line in the file */
	struct abac_user *usr;
	char *uid_str;

	usr = kcalloc(1, sizeof(struct abac_user), GFP_KERNEL);
	uid_str = strsep(&line, ":");
	kstrtoint(uid_str, 10, &(usr->uid));
	usr->attrs = parse_avp(line);
	return usr;
}

void parse_user_attr(char *data) {
	/*
	 * Parse user attributes file
	 * Buffer format 
	 * <user-id1>:<attr-name1>=<attr-value1>,<attr-name2>=<attr-value2> 
	 * <user-id2>:<attr-name3>=<attr-value3>,<attr-name4>=<attr-value4> 
	 */

	struct abac_user *temp;
	struct user_hnode *u;
	char *line;

	hash_init(user_attr_map);

	while((line = strsep(&data, "\n")) != NULL) {
		/* Ignore empty lines */
		if (strlen(line) < 2) {
			break;
		}
		temp = parse_line(line);
		/* add new user to hash table */
		u = kcalloc(1, sizeof(struct user_hnode), GFP_KERNEL);
		u->uid = temp->uid;
		u->attrs = temp->attrs;
		hash_add(user_attr_map, &(u->node), u->uid);
		printk("Added %u to hashtable", u->uid);
	}
}

avp *get_user_attrs(unsigned int uid) {
	/* Get user attributes mapped to a UID */
	struct user_hnode *cur;
	avp *attrs = NULL;
	hash_for_each_possible(user_attr_map, cur, node, uid) {
		attrs = cur->attrs;
		break;
	}
	return attrs;
}

void clear_user_attrs() {
	// Clear the user attributes in hash table
	struct user_hnode *cur;
	unsigned bkt;
	printk("clearing user hashtable...");
    hash_for_each(user_attr_map, bkt, cur, node) {
		clear_avp_list(cur->attrs);
		hash_del(&(cur->node));
    }
}

void print_user_attrs() {
	struct user_hnode *cur;
	unsigned bkt;
	printk("Printing user hashtable...");
    hash_for_each(user_attr_map, bkt, cur, node) {
		printk("UID = %u", cur->uid);
		print_avp(cur->attrs);
    }
}
