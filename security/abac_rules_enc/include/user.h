#ifndef _ABAC_USER_H
#define _ABAC_USER_H

#include "avp.h"

void parse_user_attr(char *);
avp *get_user_attrs(unsigned int);
void print_user_attrs(void);
void clear_user_attrs(void);

#endif /* _ABAC_USER_H */
