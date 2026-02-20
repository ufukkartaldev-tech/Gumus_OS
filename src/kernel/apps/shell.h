#ifndef SHELL_H
#define SHELL_H

#define MAX_COMMAND_LEN 128

void shell_init();
void shell_input(char c);
void shell_parse_command(char* cmd);

#endif
