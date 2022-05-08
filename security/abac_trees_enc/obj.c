#include "obj.h"
#include <linux/limits.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hashtable.h>

struct obj_hnode {
	char path[PATH_MAX];
	struct node *root;
	struct hlist_node node;
};

struct abac_obj {
	char path[PATH_MAX];
	struct node *root;
};

#define OBJ_BUCKETS 10 // (2 ^ 10 = 1024 buckets)

DECLARE_HASHTABLE(obj_attr_map, OBJ_BUCKETS);

// Calculate hashes for file paths
static u32 simple_hash(const char *s) {
    u32 key = 0;
    char c;
    while ((c = *s++))
        key += c;
    return key;
}

static node_cont *parse_node(char *str, int is_root) {
	/* Parse the a single node and return its contents via the node_cont struct */
	char *token;
	node_cont *n = kmalloc(sizeof(struct node_cont), GFP_KERNEL);
	token = strsep(&str, " ");
	kstrtoint(token, 10, &(n->nid));
	if (is_root == 1) {
		// if the node is root, only the first and last fields have data
		n->pid = -1;
		n->value = -1;
		token = strsep(&str, " ");
		token = strsep(&str, " ");
	} else {
		// pid
		token = strsep(&str, " ");
		kstrtoint(token, 10, &(n->pid));
		// value
		token = strsep(&str, " ");
		kstrtoint(token, 10, &(n->value));
	}
	if (strcmp(str, "MODIFY") == 0) {
		n->attr = -1;
		n->op = ABAC_MODIFY;
	} else if (strcmp(str, "READ") == 0) {
		n->attr = -1;
		n->op = ABAC_READ;
	} else {
		kstrtoint(str, 10, &(n->attr));
	}
	// printk("%d.%d.%s.%s\n", n->nid, n->pid, n->value, n->attr);
	return n;
}

static struct abac_obj *parse_line(char *line) { 
	/* Parse a line in the input file */
	struct abac_obj *head;
	node_cont *nc;
	struct node *root, *child, **nodes;
	branch *b;
	char *path, *n_str, *node_str;
	int n;
	
	head = kmalloc(sizeof(struct abac_obj), GFP_KERNEL);
	// extract object path
	path = strsep(&line, ":");
	strcpy(head->path, path);
	
	// extract number of nodes and create nodes array
	n_str = strsep(&line, "|");
	kstrtoint(n_str, 10, &n);
	nodes = kcalloc(n, sizeof(struct node*), GFP_KERNEL);
	//printk("Line: %s\n", line);
	//printk("Path: %s - Nodes: %d\n", head->path, n);

	// extract the root node
	node_str = strsep(&line, "|");
	nc = parse_node(node_str, 1);
	root = kcalloc(1, sizeof(struct node), GFP_KERNEL);
	root->attr = nc->attr;
	nodes[0] = root;
	kfree(nc);
	
	// iterate over the remaining nodes and build the complete tree
	while((node_str = strsep(&line, "|")) != NULL) {
		nc = parse_node(node_str, 0);
		// create new child node
		child = kcalloc(1, sizeof(struct node), GFP_KERNEL);
		child->head = NULL;
		child->attr = nc->attr;
		child->op = nc->op;
		nodes[nc->nid] = child;
		// Add this node as a new branch to the parent node
		b = kcalloc(1, sizeof(branch), GFP_KERNEL);
		b->value = nc->value;
		b->child = child;
		b->next = NULL;
		if (nodes[nc->pid]->head) {
			b->next = nodes[nc->pid]->head;
		}
		nodes[nc->pid]->head = b;
		kfree(nc);
	}
	head->root = root;
	kfree(nodes);
	return head;
}

/* Used by abac securityfs for parsing the obj_attr file
 * Iterate over the entire file and build a linked list of trees for each object */
void parse_obj_attr(char *data) {

	struct abac_obj *temp;
	struct obj_hnode *o;
	char *line;

	hash_init(obj_attr_map);

	while((line = strsep(&data, "\n")) != NULL) {
		/* Ignore empty lines */
		if (strlen(line) < 2) {
			break;
		}
		temp = parse_line(line);
		/* add new user to hash table */
		o = kcalloc(1, sizeof(struct obj_hnode), GFP_KERNEL);
		strcpy(o->path, temp->path);
		o->root = temp->root;
		hash_add(obj_attr_map, &(o->node), simple_hash(o->path));
		printk("Added %s to hashtable", o->path);
	}
}

struct node *get_obj_tree(char *path) {
	/* Get object attributes tree mapped to a path */
	struct obj_hnode *cur;
	u32 key = simple_hash(path);
	struct node *root = NULL;
	hash_for_each_possible(obj_attr_map, cur, node, key) {
		/* Multiple paths can hash to the same bucket, so compare paths */
		if (strcmp(path, cur->path)) {
			continue;
		}
		root = cur->root;
		break;
	}
	return root;
}

static void clear_attr_tree(struct node *root) {
	branch *b, *to_free;
	if (root == NULL) {
		return ;
	}
	b = root->head;
	while (b != NULL) {
		clear_attr_tree(b->child);
		to_free = b;
		b = b->next;
		kfree(to_free);
	}
	kfree(root);
}

void clear_obj_attrs(void) {
	struct obj_hnode *cur;
	unsigned bkt;
	printk("clearing object hashtable...");
    hash_for_each(obj_attr_map, bkt, cur, node) {
		clear_attr_tree(cur->root);
		hash_del(&(cur->node));
    }
}


void print_attr_tree(struct node *root) {
	branch *cursor;

	if (root == NULL) {
		return ;
	}
	cursor = NULL;
	if (root->attr == -1) {
		/* If leaf node, print operation */
		if (root->op == ABAC_MODIFY) {
			printk("MODIFY\n");
		} else {
			printk("READ\n");
		}
	} else {
		printk("[%d]", root->attr);
	}
	cursor = root->head;
	while (cursor != NULL) {
		printk("%d", cursor->value);
		print_attr_tree(cursor->child);
		cursor = cursor->next;
	}
}

void print_obj_attrs() {
	struct obj_hnode *cur;
	unsigned bkt;
	printk("Printing object hashtable...");
    hash_for_each(obj_attr_map, bkt, cur, node) {
		printk("Path : %s", cur->path);
		print_attr_tree(cur->root);
    }
}
