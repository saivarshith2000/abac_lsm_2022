#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR 32

struct user_attr {
	int uid;
	struct avp *head;
	struct user_attr *next;
};

struct avp {
	char name[MAX_STR];
	char value[MAX_STR];
	struct avp *next;
};

struct avp *parse_avp(char *avp_str) {
	/* Parse a collection of name=value pairs separated by commas */
	struct avp *head, *temp;
	char *pair, *name;
	head = NULL;
	while((pair = strsep(&avp_str, ",")) != NULL) {
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

struct user_attr *parse_line(char *line) {
	struct user_attr *attr;
	char *uid_str;

	attr = calloc(1, sizeof(struct user_attr));
	uid_str = strsep(&line, ":");
	attr->uid = atoi(uid_str);
	attr->head = parse_avp(line);
	attr->next = NULL;
	return attr;
}

struct user_attr *parse_file(char *data) {
	/* Iterate over the entire file and build a linked list of avps for each user */
	struct user_attr *head, *temp;
	char *line;
	head = NULL;
	while((line = strsep(&data, "\n")) != NULL) {
		if (strlen(line) < 2) {
			break;
		}
		temp = parse_line(line);
		if (head) {
			temp->next = head;
		}
		head = temp;
	}
	return head;
}

void print_avp(struct avp *head) {
	struct avp *cursor;
	cursor = head;
	while (cursor != NULL) {
		printf("%s=%s", cursor->name, cursor->value);
		cursor = cursor->next;
		if (cursor) {
			printf(",");
		}
	}
}

void print_user_attr(struct user_attr *head) {
	struct user_attr *cursor;
	cursor = head;
	while (cursor != NULL) {
		printf("%u: ", cursor->uid);
		print_avp(cursor->head);
		printf("\n");
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
		input = "user_attr";
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
	struct user_attr *head = parse_file(data);
	printf("-----------------------\n");
	print_user_attr(head);
	return 0;
}
