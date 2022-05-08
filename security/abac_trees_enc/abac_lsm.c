#include "abacfs.h"
#include <linux/limits.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/lsm_hooks.h>
#include <linux/timekeeping.h>
#include <linux/dcache.h>
#include <linux/cred.h>

static const char* secured_dir = "/home/secured/";
static const int secured_dir_len = 14;

// Check if path is secured
int is_secured(char *accessed_path)
{
	if (strncmp(secured_dir, accessed_path, secured_dir_len) == 0) {
		return 1;
	}
	return 0;
}

// get full filename
char *get_full_name(struct file *file, char *buf, int buflen)
{
	struct dentry *dentry = file->f_path.dentry;
	char *ret = dentry_path_raw(dentry, buf, buflen);
	return ret;
}

static struct node *get_child(avp *user_attrs, struct node *n) {
	/*
	 * Find the child node corresponding to the value of user or environmental attribute
	 */
	struct avp *u, *e;
	branch *b; 
	u = user_attrs;
	e = env_attr;
	while (u != NULL) {
		if (u->name == n->attr) {
			/* If the node's attribute is found in user attributes,
			 * look for corresponding branch in the node */
			b = n->head;
			while (b != NULL) {
				if (u->value == b->value) {
					//printk("Found child: %s for attr: %s", b->value, n->attr);
					return b->child;
				}
				b = b->next;
			}
		}
		u = u->next;
	}
	/* Check environmental attributes (similar to checking user attributes) */
	while (e != NULL) {
		if (e->name == n->attr) {
			b = n->head;
			while (b != NULL) {
				if (e->value == b->value) {
					//printk("Found child: %s for attr: %s", e->value, n->attr);
					return b->child;
				}
				b = b->next;
			}
		}
		e = e->next;
	}
	// branch not found
	return NULL;
}

static int resolve_r(avp *user_attr, struct node *n, enum operation op) {
	struct node *child;
	/* Recursive helper method for resolve() */
	if (n->attr == -1) {
		/* n is a leaf, so check only operation */
		if(n->op == op) {
			//printk("matched op");
			return 0;
		} else if (n->op == ABAC_MODIFY && op == ABAC_READ) {
			/* If the rule says MODIFY, then the user also has READ rights */
			//printk("subsumed op");
			return 0;
		}
		//printk("wrong op");
		return 1;
	}
	child = get_child(user_attr, n);
	if (!child) {
		/* Corresponding child not found in n */
		//printk("Child not found");
		return 1;
	}
	//printk("Child found");
	return resolve_r(user_attr, child, op);
}

static int resolve(avp *user_attr, struct node *obj_root, enum operation op){
	/* Resolve access request using 
	 * 1. User attributes (*user_attr)
	 * 2. Root of the object attribute tree (struct node *obj_root)
	 * 3. Current environmental attributes (avp *env_attr -> from abacfs)
	 * 4. Access operation (READ or MODIFY)
	 */
	if (user_attr == NULL) {
		/* If the user doesn't have any attributes, access is DENIED */
		return 1;
	}
	if (obj_root == NULL) {
		/* If the object doesn't have any attribute, access is DENIED */
		return 1;
	}
	if (op == ABAC_IGNORE) {
		/* If not a relevant operation, allow it */
		return 0;
	}
	return resolve_r(user_attr, obj_root, op);
}

static enum operation get_op(int mask) {
	/* Conver access bit mask into valid abac operation */
	//printk("Mask: %d", mask);
	switch (mask) {
		case MAY_WRITE:
		case MAY_APPEND:
			return ABAC_MODIFY;
		case MAY_READ:
		//case MAY_OPEN:
		//case MAY_ACCESS:
			return ABAC_READ;
	}
	return ABAC_IGNORE;
}

// File read/write hook
static int abac_file_permission(struct file *file, int mask)
{
	u64 start, end, diff;
	unsigned int uid;
	char *path, *buff;
	struct dentry *dentry;
	struct node *root;
	avp *user_attr;
	int decision;
	enum operation op;

	if (recording) {
		//start = ktime_get_real_ns();
		start = ktime_get_ns();
	}
	uid = current_uid().val;
	if (uid < 1000) {
		return 0;
	}
	path = NULL;
	dentry = file->f_path.dentry;
	buff = kmalloc(PATH_MAX, GFP_KERNEL);
	path = dentry_path_raw(dentry, buff, PATH_MAX);
	if (!is_secured(path)){
		kfree(buff);
		return 0;
	}
	op = get_op(mask);

	//printk("ABAC LSM: %d accessing %s\n", uid, path);
	// operation
	/*
	if (op == ABAC_READ) {
		printk("ABAC READ");
	} else if (op == ABAC_MODIFY) {
		printk("ABAC MODIFY");
	} else {
		printk("ABAC IGNORE");
	}
	*/
	
	// Print user attributes
	user_attr = get_user_attrs(uid);
	//printk("User attributes");
	//print_avp(user_attr);
	//printk("-----------------------------------");
	
	// Print environmental attributes
	//printk("Environmental attributes");
	//print_avp(env_attr);
	//printk("-----------------------------------");

	// Print object tree
	//printk("Object attribute tree");
	root = get_obj_tree(path);
	//print_attr_tree(root);
	//printk("-----------------------------------");
	kfree(buff);

	decision = resolve(user_attr, root, op);
	//printk("decision: %s\n", decision == 0 ? "ALLOWED" : "DENIED");
	if (recording) {
		//end = ktime_get_real_ns();
		end = ktime_get_ns();
		diff = end - start;
		prev_access_time = diff;
		snprintf(perf_buf, 64, "%llu\n", prev_access_time);
	}

	return decision == 0 ? 0 : -EPERM;
}

// The hooks we wish to be installed.
static struct security_hook_list abac_hooks[] __lsm_ro_after_init = {
	LSM_HOOK_INIT(file_permission, abac_file_permission),
};


// Initialize our module.
static int __init abac_init(void)
{
	security_add_hooks(abac_hooks, ARRAY_SIZE(abac_hooks), "abac");
	printk(KERN_INFO "ABAC LSM: Initialized.\n Files in %s are protected by ABAC policy\n", secured_dir);
	return 0;
}

DEFINE_LSM(abac) = {
	.init = abac_init,
	.name = "abac",
};
