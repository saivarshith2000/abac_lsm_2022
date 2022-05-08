#ifndef _ABAC_POLICY_H
#define _ABAC_POLICY_H

#include "avp.h"

typedef struct abac_rule abac_rule;
struct abac_rule {
	unsigned int id;
	avp *user;
	avp *env;
	enum operation op;
};

void parse_policy(char *);
abac_rule *get_rule(unsigned int );
void print_policy(void);
void clear_policy(void);

#endif /* _ABAC_POLICY_H */
