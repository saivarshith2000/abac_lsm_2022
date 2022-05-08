#ifndef _ABAC_OBJ_H
#define _ABAC_OBJ_H

#include "avp.h"

typedef struct obj_rule obj_rule;
struct obj_rule {
	unsigned int id;
	obj_rule *next;
};

void parse_obj_rule_map(char *);
obj_rule *get_obj_rule_list(char *);
void clear_obj_rule_map(void);
void print_obj_rule_list(obj_rule *);
void print_obj_rule_map(void);

#endif /* _ABAC_OBJ_H */
