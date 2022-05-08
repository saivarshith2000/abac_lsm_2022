#include "avp.h"

enum operation {ABAC_MODIFY, ABAC_READ, ABAC_IGNORE};

// predefinition of node to make its pointer available for definition
//struct node;

// struct representing a branch in a node
struct branch {
	int value;
	//char value[MAX_STR];
	struct node *child;
	struct branch *next;
};
typedef struct branch branch;

// struct representing a node in the tree
// Branches are stored as linked lists
struct node {
	int attr;
	//char attr[MAX_STR];
	enum operation op;
	struct branch *head;
};

// struct representing the contents of a parsed node
struct node_cont {
	int nid;
	int pid;
	int attr;
	int value;
	enum operation op;
	//char attr[MAX_STR];
	//char value[MAX_STR];
};
typedef struct node_cont node_cont;

void parse_obj_attr(char *);
struct node *get_obj_tree(char *);
void clear_obj_attrs(void);
void print_obj_attrs(void);
void print_attr_tree(struct node *);
