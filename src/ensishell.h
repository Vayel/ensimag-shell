#include <stdbool.h>

struct jobs_list {
	int pid;
	char *cmd;
	struct jobs_list *next;
};

struct jobs_list *get_next(void);
void execute(char **cmd, int bg, bool in_pipe, bool out_pipe,
			 char* input_file, char* output_file);
void jobs();
