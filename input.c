#include "input.h"
#include "gamelogic.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int read_line(char *buf, int n) {
	if (!fgets(buf, n, stdin))
		return 0;
	return 1;
}

int parse_action(const char *s, int *out_col0) {
	while (isspace((unsigned char)*s))
		s++;

	if (*s == 'q' || *s == 'Q')
		return -1;
	if (*s == 'u' || *s == 'U')
		return -2;
	if (*s == 'r' || *s == 'R')
		return -3;

	char *end = NULL;
	long v = strtol(s, &end, 10);
	if (end == s)
		return 0;
	if (v < 1 || v > COLS)
		return 0;

	*out_col0 = (int)(v - 1);
	return 1;
}