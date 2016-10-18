/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>

int question6_executer(char *line)
{
	/* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	printf("Not implemented yet: can not execute %s\n", line);

	/* Remove this line when using parsecmd as it will free it */
	free(line);

	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}

// Global variables
struct jobs_list {
	int pid;
	char *cmd;
	struct jobs_list *next;
};
struct jobs_list *list = NULL;

int fd[2];

struct jobs_list *get_next() {
	if (list == NULL) {
		list = malloc(sizeof(struct jobs_list));
		list->next = NULL;
		return list;
	}

	struct jobs_list *current = list;

	while(current->next != NULL) {
		current = current->next;
	}

	current->next = malloc(sizeof(struct jobs_list));
	current->next->next = NULL;

	return current->next;
}

void execute(char **cmd, int bg, bool in_pipe, bool out_pipe) {
	/* The fork() function returns an integer which can be either '-1' or '0'
     * for the a child process 
     */
	int pid = fork();

	// [CHILD PROCESS] pid < 0
	if (pid < 0) {
		printf("[ERROR] Process failed !\n");
        return;
	}
	// [CHILD PROCESS] pid == 0
	else if (pid == 0) {
        if(in_pipe) {
            dup2(fd[0], STDIN_FILENO);
        } 
        if (out_pipe) {
            dup2(fd[1], STDOUT_FILENO);
        }

		execvp(cmd[0], cmd);
        return;
	}

	// [PARENT PROCESS] pid = child pid
    close(fd[1]);

    if (bg == 0) {
	    int status;
        while (wait(&status) != pid);
    }
    else {
        struct jobs_list *end = get_next();
        end->pid = pid;
        end->cmd = malloc(strlen(cmd[0]) * sizeof(char));
        strcpy(end->cmd, cmd[0]);
    }
}

void jobs() {
	struct jobs_list *current = list;

	while(current != NULL) {
        int status;
        /* If the process is running, waitpid returns 0, else the pid */
        bool finished = (bool) waitpid(current->pid, &status, WNOHANG);

		printf("%d : %s [%s]\n", current->pid, current->cmd, finished ? "FINISHED" : "RUNNING");
		current = current->next;
	}
}

int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1) {
		struct cmdline *l;
		char *line=0;
		int i, j;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);

		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}

#if USE_GNU_READLINE == 1
		add_history(line);
#endif


#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif

		/* parsecmd free line and set it up to 0 */
		l = parsecmd( & line);

		/* If input stream closed, normal termination */
		if (!l) {
			terminate(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) printf("background (&)\n");

        pipe(fd);
		
        /* Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
                for (j=0; cmd[j]!=0; j++) {
                        printf("'%s' ", cmd[j]);
                }
			printf("\n");

			if (strcmp(cmd[0], "jobs") == 0) {
				jobs();
			} else {
				execute(
                    cmd,
                    l->bg,
                    i > 0,
                    l->seq[i+1] != 0
                );
			}
		}
	}
}
