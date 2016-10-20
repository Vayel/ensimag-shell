#include <stdbool.h>
#include <signal.h>

struct jobs_list {
	int pid;
	char *cmd;
	int sec;
	struct jobs_list *next;
};

struct jobs_list *get_next(void);
int getSec(int pid);

void execute(char **cmd, int bg, bool in_pipe, bool out_pipe,
			 char* input_file, char* output_file);
void handler(int sig, siginfo_t *info, void *context);
void jobs();
