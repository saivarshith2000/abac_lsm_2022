#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include "env.h"

#define MAX_STR 64    // Max. size of an attribute's name and value

/*
 * Methods for parsing the environmental attribute securityfs file
 * The format of the file is as given below. The file is parsed to 
 * build a hash-table where the each is an environmental attribute
 * and values are the *current* values of the attributes.
 *
 * File Format:
 * ------------
 * day=weekday
 * users=600
 * location=office
 * time=afterhours
 */

avp *parse_env_attr(char *data)
{
	avp *head, *temp;
	char *pair, *name;
	head = NULL;
	while((pair = strsep(&data, "\n")) != NULL) {
		if (strlen(pair) < 2) {
			break;
		}
		name = strsep(&pair, "=");
		temp = kcalloc(1, sizeof(avp), GFP_KERNEL);
		temp->next = NULL;
		strcpy(temp->name, name);
		strcpy(temp->value, pair);
		if (head) {
			temp->next = head;
		}
		head = temp;
	}
	return head;
}

void print_env_attrs(avp *head)
{
	if (head == NULL) {
		return;
	}
	avp *cursor;
	cursor = head;
	printk("Environment Attributes");
	while (cursor != NULL) {
		printk("%s=%s", cursor->name, cursor->value);
		cursor = cursor->next;
	}
}
