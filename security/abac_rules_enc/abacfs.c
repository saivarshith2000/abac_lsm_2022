#include "abacfs.h"
#include <linux/init.h>
#include <linux/security.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

const size_t MAX_FILE_SIZE = 8388608; // 8MB

struct dentry *abacfs;
struct dentry *user_attr_file;
struct dentry *obj_rules_file;
struct dentry *env_attr_file;
struct dentry *policy_file;
struct dentry *action_file;
struct dentry *perf_file;

char *user_attr_buf = NULL;
char *obj_rules_buf = NULL;
char *env_attr_buf = NULL;
char *policy_buf = NULL;
char perf_buf[64];
int recording = 0;
u64 prev_access_time = 0;

avp *env_attr = NULL;

// method for opening policy file
static int abac_open(struct inode *i, struct file *f)
{
	// TODO: Add a check to let only the root access the policy file
	return 0;
}

// method for writing to user_attrs file
static ssize_t user_attr_write(struct file *filp, const char __user *buffer,
			       size_t len, loff_t *off)
{
	if (len >= MAX_FILE_SIZE) {
		printk(KERN_INFO
		       "Write failed. Buffer too large %zu. Maximum file size is %zu\n",
		       len, MAX_FILE_SIZE);
		return -EFAULT;
	}
	if (user_attr_buf) {
		clear_user_attrs();
		kfree(user_attr_buf);
	}
	user_attr_buf = kmalloc(len + 1, GFP_KERNEL);
	if (!user_attr_buf) {
		printk(KERN_INFO
		       "Write failed. Failed to allocate memory for user attributes buffer\n");
		return -EFAULT;
	}
	if (copy_from_user(user_attr_buf, buffer, len + 1)) {
		printk(KERN_INFO "Write to user_attrs failed\n");
		return -EFAULT;
	}
	user_attr_buf[len] = '\0';
	printk("User attributes written to buffer. Attempting to parse...");
	parse_user_attr(user_attr_buf);
	//print_user_attrs();
	printk("User attributes loaded");
	return len;
}

// method for writing to obj_rules file
static ssize_t obj_rules_write(struct file *filp, const char __user *buffer, size_t len, loff_t *off)
{
	if (len >= MAX_FILE_SIZE) {
		printk(KERN_INFO "Write failed. Buffer too large %zu. Maximum file size is %zu\n", len, MAX_FILE_SIZE);
		return -EFAULT;
	}
	if (obj_rules_buf) {
		clear_obj_rule_map();
		kfree(obj_rules_buf);
	}
	obj_rules_buf = kmalloc(len + 1, GFP_KERNEL);
	if (!obj_rules_buf) {
		printk(KERN_INFO "Write failed. Failed to allocate memory for object rules buffer\n");
		return -EFAULT;
	}
	if (copy_from_user(obj_rules_buf, buffer, len + 1)) {
		printk(KERN_INFO "Write to obj_rules failed\n");
		return -EFAULT;
	}
	obj_rules_buf[len] = '\0';
	printk("Object rules written to buffer. Attempting to parse...");
	parse_obj_rule_map(obj_rules_buf);
	//print_obj_rule_map();
	printk("Object rules loaded");
	return len;
}

// method for writing to env_attrs file
static ssize_t env_attr_write(struct file *filp, const char __user *buffer,
			      size_t len, loff_t *off)
{
	if (len >= MAX_FILE_SIZE) {
		printk(KERN_INFO
		       "Write failed. Buffer too large %zu. Maximum file size is %zu\n",
		       len, MAX_FILE_SIZE);
		return -EFAULT;
	}
	if (env_attr_buf) {
		clear_avp_list(env_attr);
		kfree(env_attr_buf);
	}
	env_attr_buf = kmalloc(len + 1, GFP_KERNEL);
	if (!env_attr_buf) {
		printk(KERN_INFO
		       "Write failed. Failed to allocate memory for environment "
		       "attributes buffer\n");
		return -EFAULT;
	}
	if (copy_from_user(env_attr_buf, buffer, len + 1)) {
		printk(KERN_INFO "Write to env_attrs failed\n");
		return -EFAULT;
	}
	env_attr_buf[len] = '\0';
	printk("Environment attributes written to buffer. Attempting to parse...");
	env_attr = parse_env_attr(env_attr_buf);
	//print_env_attrs(env_attr);
	printk("Environment attributes loaded");
	return len;
}

// method for writing to policy file
static ssize_t policy_write(struct file *filp, const char __user *buffer, size_t len, loff_t *off)
{
	if (len >= MAX_FILE_SIZE) {
		printk(KERN_INFO "Write failed. Buffer too large %zu. Maximum file size is %zu\n", len, MAX_FILE_SIZE);
		return -EFAULT;
	}
	if (policy_buf) {
		clear_policy();
		kfree(policy_buf);
	}
	policy_buf = kmalloc(len + 1, GFP_KERNEL);
	if (!policy_buf) {
		printk(KERN_INFO "Write failed. Failed to allocate memory for policy buffer\n");
		return -EFAULT;
	}
	if (copy_from_user(policy_buf, buffer, len + 1)) {
		printk(KERN_INFO "Write to policy failed\n");
		return -EFAULT;
	}
	policy_buf[len] = '\0';
	printk("Policy written to buffer. Attempting to parse...");
	parse_policy(policy_buf);
	//print_policy();
	printk("Policy loaded");
	return len;
}

// method for writing to action file
static ssize_t action_write(struct file *filp, const char __user *buffer,
			      size_t len, loff_t *off)
{
	char *action_buf;
	if (len >= MAX_FILE_SIZE) {
		printk(KERN_INFO
		       "Write failed. Buffer too large %zu. Maximum file size is %zu\n",
		       len, MAX_FILE_SIZE);
		return -EFAULT;
	}
	action_buf = kmalloc(len + 1, GFP_KERNEL);
	if (!action_buf) {
		printk(KERN_INFO
		       "Write failed. Failed to allocate memory for action buffer\n");
		return -EFAULT;
	}
	if (copy_from_user(action_buf, buffer, len + 1)) {
		printk(KERN_INFO "Write to action failed\n");
		return -EFAULT;
	}
	action_buf[len] = '\0';
	if (strcmp(action_buf, "RECORD") == 0) {
		//printk("Recording started...");
		prev_access_time = 0;
		recording = 1;
	} else if (strcmp(action_buf, "STOP") == 0) {
		//printk("Recording stopped...");
		recording = 0;
		//snprintf(perf_buf, 64, "%llu\n", prev_access_time);
		//printk("Time taken written to /sys/kernel/security/abac/perf");
		prev_access_time = 0;
	} else {
		printk("Invalid action...");
	}
	kfree(action_buf);
	return len;
}

static ssize_t perf_read(struct file *file, char __user *buf, size_t count, loff_t *off)
{
    loff_t pos = *off;
    loff_t len = strlen(perf_buf);

    if (pos >= len || !count)
        return 0;

    len -= pos;
    if (count < len)
        len = count;

    if (copy_to_user(buf, perf_buf, len))
        return -EFAULT;
    *off += len;
    return len;
}

static const struct file_operations user_attr_fops = {
	.open = abac_open,
	.write = user_attr_write,
};

static const struct file_operations obj_rules_fops = {
	.open = abac_open,
	.write = obj_rules_write,
};

static const struct file_operations env_attr_fops = {
	.open = abac_open,
	.write = env_attr_write,
};

static const struct file_operations policy_fops = {
	.open = abac_open,
	.write = policy_write,
};

static const struct file_operations action_fops = {
	.open = abac_open,
	.write = action_write,
};

static const struct file_operations perf_fops = {
	.open = abac_open,
	.read = perf_read,
};


static void destroy_abac_fs(void)
{
	if (user_attr_file) {
		securityfs_remove(user_attr_file);
	}
	if (obj_rules_file) {
		securityfs_remove(obj_rules_file);
	}
	if (env_attr_file) {
		securityfs_remove(env_attr_file);
	}
	if (policy_file) {
		securityfs_remove(policy_file);
	}
	if (action_file) {
		securityfs_remove(action_file);
	}
	if (perf_file) {
		securityfs_remove(perf_file);
	}
	if (abacfs) {
		securityfs_remove(abacfs);
	}
}

static struct dentry *create_file(const char *filename, const struct file_operations *fops) {
	struct dentry *f;
	f = securityfs_create_file(filename, 0666, abacfs, NULL, fops);
	if (!f) {
		printk(KERN_ERR "ABAC LSM: Failed to create file /sys/kernel/security/abac/%s", filename);
		destroy_abac_fs();
		return NULL;
	}
	printk(KERN_INFO "ABAC LSM: Created file /sys/kernel/security/abac/%s", filename);
	return f;
}

/* create the abac filesystem */
static void abac_create_fs(void)
{
	// create the root 'abac' directory
	abacfs = securityfs_create_dir("abac", NULL);
	if (!abacfs) {
		printk(KERN_ERR "ABAC LSM: Failed to create abac securityfs at /sys/kernel/security/abac/");
		destroy_abac_fs();
	}

	user_attr_file = create_file("user_attr", &user_attr_fops);
	if (!user_attr_file) {
		destroy_abac_fs();
		return ;
	}
	obj_rules_file = create_file("obj_rules", &obj_rules_fops);
	if (!obj_rules_file) {
		destroy_abac_fs();
		return ;
	}
	env_attr_file = create_file("env_attr", &env_attr_fops);
	if (!env_attr_file) {
		destroy_abac_fs();
		return ;
	}
	policy_file = create_file("policy", &policy_fops);
	if (!policy_file) {
		destroy_abac_fs();
		return ;
	}

	// Performance evaluation files
	action_file = create_file("action", &action_fops);
	if (!action_file) {
		destroy_abac_fs();
		return ;
	}
	perf_file = create_file("perf", &perf_fops);
	if (!perf_file) {
		destroy_abac_fs();
		return ;
	}
}

fs_initcall(abac_create_fs);
