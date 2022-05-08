#include <linux/string.h>
#include <linux/slab.h>
#include "policy.h"

/* Array of rules and its size*/
struct abac_rule **policy = NULL;
unsigned int count;

static struct abac_rule *parse_line(char *line) {
	/* Parse a single line in the file */
	struct abac_rule *r;
	char *id_str;
	char *section;

	r = kcalloc(1, sizeof(struct abac_rule), GFP_KERNEL);
	r->op = ABAC_IGNORE;
	id_str = strsep(&line, ":");
	kstrtoint(id_str, 10, &(r->id));
	// User attributes
	section = strsep(&line, "|");
	r->user = parse_avp(section);
	// Environmental attributes
	section = strsep(&line, "|");
	r->env = parse_avp(section);
	// Operation
	if (strcmp(line, "MODIFY") == 0) {
		r->op = ABAC_MODIFY;
	} else if (strcmp(line, "READ") == 0){
		r->op = ABAC_MODIFY;
	}
	return r;
}

void parse_policy(char *data) {
	/*
	 * Parses ABAC policy written to 'policy' file in securityfs
	 * Rules are parsed and stored in an array
	 * File Format:
	 * <rule_count>
	 * <rule_id>:u_attr1=u_val1,u_attr2=u_val2|e_attr=e_val|op=MODIFY
	 * <rule_id>:u_attr2=u_val2|e_attr=e_val|op=READ
	 * ...
	 */
	struct abac_rule *r;
	char *line, *count_str;

	count_str = strsep(&data, "\n");
	kstrtouint(count_str, 10, &count);
	//policy = kmalloc(sizeof(struct abac_rule *), GFP_KERNEL);
	policy = kmalloc(sizeof(struct abac_rule *) * count, GFP_KERNEL);
	printk("Policy has %d rules", count);

	while((line = strsep(&data, "\n")) != NULL) {
		/* Ignore empty lines */
		if (strlen(line) < 2) {
			break;
		}
		r = parse_line(line);
		policy[r->id] = r;
		printk("Added rule %u to array", r->id);
	}
}

abac_rule *get_rule(unsigned int id) {
	/* Get rule to a ID */
	return policy[id];
}

void clear_policy() {
	// Clear the rules in policy array
	int i;
	printk("clearing policy array...");
	for (i = 0; i < count; i++) {
		clear_avp_list(policy[i]->user);
		clear_avp_list(policy[i]->env);
	}
	count = 0;
	kfree(policy);
	policy = NULL;
}

void print_policy() {
	int i;
	printk("Printing policy array...");
	printk("Contains %d rules", count);
	for (i = 0; i < count; i++) {
		printk("ID = %u", policy[i]->id);
		printk("User attributes");
		print_avp(policy[i]->user);
		printk("Environmental attributes");
		print_avp(policy[i]->env);
		printk("Operation");
		if (policy[i]->op == ABAC_MODIFY) printk("MODIFY");
		else if (policy[i]->op == ABAC_READ) printk("READ");
		else printk("IGNORE");
	}
}
