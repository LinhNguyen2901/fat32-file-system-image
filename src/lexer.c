#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_input(void) {
	char *buffer = NULL;
	int bufsize = 0;
	char line[5];
	while (fgets(line, 5, stdin) != NULL)
	{
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;
		buffer = (char *)realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;
		if (newln != NULL)
			break;
	}
	buffer = (char *)realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;
	return buffer;
}

tokenlist *new_tokenlist(void) {
	tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **)malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item) {
	int i = tokens->size;

	tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *)malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

tokenlist *get_tokens(char *input) {
	tokenlist *tokens = new_tokenlist();
	if (input == NULL)
		return tokens;
	char *buf = (char *)malloc(strlen(input) + 1);
	const char *p = input;
	while (*p != '\0')
	{
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == '\0')
			break;
		int len = 0;
		bool in_quotes = false;

		while (*p != '\0') {
			if (*p == '\\' && *(p + 1) != '\0') {
				p++;
				buf[len++] = *p++;
			} else if (*p == '"') {
				in_quotes = !in_quotes;
				p++;
			} else if (!in_quotes && (*p == ' ' || *p == '\t')) {
				break;
			} else {
				buf[len++] = *p++;
			}
		}
		buf[len] = '\0';
		add_token(tokens, buf);
	}
	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens) {
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}
