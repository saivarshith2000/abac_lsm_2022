#ifndef _ABAC_AVP_H
#define _ABAC_AVP_H

#define MAX_STR 32

enum operation {ABAC_MODIFY, ABAC_READ, ABAC_IGNORE};

typedef struct avp avp;
struct avp {
    int name;
    int value;
    avp *next;
};

avp *parse_avp(char *);
void print_avp(avp *);
void clear_avp_list(avp *);

#endif /* _ABAC_AVP_H */
