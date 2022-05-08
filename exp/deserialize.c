#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_STR 32

enum operation {MODIFY, READ};

// predefinition of node to make its pointer available for definition
//struct node;

// struct representing a branch in a node
struct branch {
	char value[MAX_STR];
	struct node *child;
	struct branch *next;
};

// struct representing a node in the tree
// Branches are stored as linked lists
struct node {
	char attr[MAX_STR];
	enum operation op;
	struct branch *head;
};

// struct representing an object and its attr tree
// next points to the next object  in the list and its tree
struct obj_attr {
	char path[PATH_MAX];
	struct node *root;
	struct obj_attr *next;
};

struct node_cont {
	int nid;
	int pid;
	char attr[MAX_STR];
	char value[MAX_STR];
};

struct node_cont *parse_node(char *str, int is_root) {
	/* Parse the a single node and return its contents via the node_cont struct */
	char *token;
	struct node_cont *n = calloc(1, sizeof(struct node_cont));
	token = strsep(&str, " ");
	n->nid = atoi(token);
	if (is_root == 1) {
		// if the node is root, only the first and last fields have data
		n->pid = -1;
		strcpy(n->value, "-");
		token = strsep(&str, " ");
		token = strsep(&str, " ");
	} else {
		// pid
		token = strsep(&str, " ");
		n->pid = atoi(token);
		// value
		token = strsep(&str, " ");
		strcpy(n->value, token);
	}
	strcpy(n->attr, str);
	//printf("%d.%d.%s.%s\n", n->nid, n->pid, n->value, n->attr);
	return n;
}

struct obj_attr *parse_line(char *line) { /* Parse a line in the input file */
	//printf("%s\n", line);
	struct obj_attr *head;
	struct node_cont *nc;
	struct node *root, *temp;
	struct branch *b;
	char *path, *n_str, *node_str;
	int n, nid, pid;
	
	head = calloc(1, sizeof(struct obj_attr));
	// extract object path
	path = strsep(&line, ":");
	strcpy(head->path, path);
	
	// extract number of nodes and create nodes array
	n_str = strsep(&line, "|");
	n = atoi(n_str);
	struct node **nodes = calloc(n, sizeof(struct node*));
	printf("Path: %s - Nodes: %d\n", head->path, n);

	// extract the root node
	node_str = strsep(&line, "|");
	nc = parse_node(node_str, 1);
	root = calloc(1, sizeof(struct node));
	strcpy(root->attr, nc->attr);
	nodes[0] = root;
	
	// iterate over the remaining nodes and build the complete tree
	while((node_str = strsep(&line, "|")) != NULL) {
		nc = parse_node(node_str, 0);
		temp = calloc(1, sizeof(struct node));
		strcpy(temp->attr, nc->attr);
		nodes[nc->nid] = temp;
		// Add this node as a branch to the parent node
		b = calloc(1, sizeof(struct branch));
		strcpy(b->value, nc->value);
		b->child = temp;
		if (nodes[nc->pid]->head == NULL) {
			nodes[nc->pid]->head = b;
		} else {
			b->next = nodes[nc->pid]->head;
			nodes[nc->pid]->head = b;
		}
		free(nc);
	}
	head->root = root;
	free(nodes);
	return head;
}

struct obj_attr *parse_file(char *data) {
	/* Iterate over the entire file and build a linked list of trees for each object */
	struct obj_attr *head, *cursor, *temp;
	char *line;
	head = NULL;
	while((line = strsep(&data, "\n")) != NULL) {
		temp = parse_line(line);
		if (head == NULL) {
			head = temp;
			cursor = head;
		} else {
			cursor->next = temp;
			cursor = cursor->next;
		}
	}
	return head;
}
void print_tree(struct node *root) {
	struct branch *cursor;
	printf("[%s] ", root->attr);
	cursor = root->head;
	while (cursor != NULL) {
		printf("%s ", cursor->value);
		print_tree(cursor->child);
		cursor = cursor->next;
	}
}

void print_obj_map(struct obj_attr *head) {
	struct obj_attr *cursor;
	cursor = head;
	while (cursor != NULL) {
		printf("Path: %s\n", cursor->path);
		print_tree(cursor->root);
		cursor = cursor->next;
		printf("\n");
	}
}

int main(char **argv, int argc){
	// check if input file name is given
	char *input, *data;
	FILE *f;
	long fsize;

	if (argc == 2) {
		input = argv[1];
	} else {
		input = "obj_attr";
	}
	// read file
	f = fopen(input, "rb");
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	rewind(f);

	data = malloc(fsize + 1);
	fread(data, fsize, 1, f);
	fclose(f);
	
	// iterate over each object and its tree representation
	struct obj_attr *head = parse_file(data);
	printf("-----------------------\n");
	print_obj_map(head);
	return 0;
}
