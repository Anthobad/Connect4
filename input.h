#ifndef INPUT_H
#define INPUT_H

int read_line(char *buf, int n);
int parse_action(const char *s, int *out_col0);

#endif