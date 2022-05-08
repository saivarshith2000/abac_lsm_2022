#include <linux/limits.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/lsm_hooks.h>
#include <linux/timekeeping.h>
#include <linux/dcache.h>
#include <linux/cred.h>
#include "abacfs.h"

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

static int check_op(enum operation req_op, enum operation rule_op) {
	/* Check if operation is matching
	 * @req_op = access request operation
	 * @rule_op = abac rule operation
	 * If rule says modify, we also allow reads
	 */
	if (req_op == rule_op) return 1;
	if (req_op == ABAC_READ && rule_op == ABAC_MODIFY) return 1;
	return 0;
}

static int check_avps(avp *a, avp *r) {
	/* Check if avps are matching
	 * Here avps can be user or env
	 * @c = access request avp (user or current env avps)
	 * @r = rule avp (user or current env avps)
	 * If every avp in the r is also in the a, then allow
	 */
	avp *cursor;
	int r_count, a_count;
	r_count = 0;
	a_count = 0;
	while (r != NULL) {
		//printk("RULE: %d=%d.", r->name, r->value);
		cursor = a;
		while (cursor != NULL){
			//printk("CURSOR: %d=%d.", cursor->name, cursor->value);
			if (cursor->name == r->name && cursor->value == r->value) {
				// found a matching avp, got to the next avp in rule
				//printk("Match found for RULE: %d=%d and CURSOR: %d=%d", r->name, r->value, cursor->name, cursor->value);
				a_count ++;
				break;
			}
			cursor = cursor->next;
		}
		r_count ++;
		r = r->next;
	}
	return r_count == a_count;
}

static int resolve(avp *user_attr, obj_rule *head, enum operation op){
	/* Resolve access request using 
	 * 1. User attributes (*user_attr)
	 * 2. Covering rules of the object (abac_rule *head)
	 * 3. Current environmental attributes (avp *env_attr -> from abacfs)
	 * 4. Access operation (READ or MODIFY)
	 */
	abac_rule *r;
	if (user_attr == NULL) {
		/* If the user doesn't have any attributes, access is DENIED */
		return 0;
	}
	if (head == NULL) {
		/* If the object doesn't have any covering rules, access is DENIED */
		return 0;
	}
	if (op == ABAC_IGNORE) {
		/* If not a relevant operation, allow it */
		return 1;
	}
	// Iterate over covering rules
	while (head != NULL) {
		// Get rule from policy hash table
		r = get_rule(head->id);
		// compare operation
		//printk("checking operation");
		if (check_op(op, r->op) == 0) {
			//printk("operation did not match");
			head = head->next;
			continue;
		}
		//printk("operation matched");

		// compare user attrs
		//printk("checking user_attrs");
		if (check_avps(user_attr, r->user) == 0) {
			//printk("user attrs did not match");
			head = head->next;
			continue;
		}
		//printk("user_attrs matched");

		// compare env attrs
		//printk("checking env_attrs");
		if (check_avps(env_attr, r->env) == 0){
			//printk("env attrs did not match");
			head = head->next;
			continue;
		}
		//printk("env_attrs matched");

		// If we reached here, the current rule is satisfied
		return 1;
	}
	return 0;
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
	obj_rule *r;
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
	
	// Print environmental attrs
	//printk("Environmental attributes");
	//print_avp(env_attr);
	//printk("-----------------------------------");

	// Print object rules
	//printk("Object rules");
	r = get_obj_rule_list(path);
	//printk("pointer: %u", r);
	//print_obj_rule_list(r);
	//printk("-----------------------------------");
	kfree(buff);

	decision = resolve(user_attr, r, op);
	//printk("decision: %s\n", decision == 1 ? "ALLOWED" : "DENIED");
	if (recording) {
		//end = ktime_get_real_ns();
		end = ktime_get_ns();
		diff = end - start;
		prev_access_time = diff;
		snprintf(perf_buf, 64, "%llu\n", prev_access_time);
	}
	return decision == 1 ? 0 : -EPERM;
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
