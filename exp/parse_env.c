#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR 32

struct avp {
	char name[MAX_STR];
	char value[MAX_STR];
	struct avp *next;
};

struct avp *parse_file(char *data) {
	/* Parse a collection of name=value pairs separated by newlines */
	struct avp *head, *temp;
	char *pair, *name;
	head = NULL;
	while((pair = strsep(&data, "\n")) != NULL) {
		if (strlen(pair) < 2) {
			break;
		}
		name = strsep(&pair, "=");
		temp = calloc(1, sizeof(struct avp));
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

void print_env_attr(struct avp *head) {
	struct avp *cursor;
	cursor = head;
	while (cursor != NULL) {
		printf("%s=%s\n", cursor->name, cursor->value);
		cursor = cursor->next;
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
		input = "env_attr";
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
	struct avp *head = parse_file(data);
	printf("-----------------------\n");
	print_env_attr(head);
	return 0;
}
