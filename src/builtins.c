#include "builtins.h"
#include "environment.h"
#include "history.h"
#include "alias.h"
#include "dir_stack.h"
#include "jobs.h"
#include "variables.h"

// Define the arrays here - only once in the entire program
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "export",
    "history",
    "alias",
    "unalias",
    "pushd",
    "popd",
    "dirs",
    "jobs",
    "fg",
    "bg",
    "wait",
    "disown",
    "set",
    "unset",
    "shift"
};

int (*builtin_func[])(char **) = {
    &hush_cd,
    &hush_help,
    &hush_exit,
    &hush_export,
    &hush_history,
    &hush_alias,
    &hush_unalias,
    &hush_pushd,
    &hush_popd,
    &hush_dirs,
    &hush_jobs,
    &hush_fg,
    &hush_bg,
    &hush_wait,
    &hush_disown,
    &hush_set,
    &hush_unset,
    &hush_shift
};

int hush_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int hush_cd(char **args)
{
        if (args[1] == NULL)
        {
                fprintf(stderr, "hush: expected argument to \"cd\"\n");
        }
        else
        {
                if (chdir(args[1]) != 0)
                {
                        perror("hush");
                }
        }
        return 1;
}

int hush_help(char **args)
{
        int i;
        printf("Amarnath Patel's hush\n");
        printf("Type program names and arguments, and hit enter.\n");
        printf("The following are builtins:\n");

        for (i = 0; i < hush_num_builtins(); i++)
        {
                printf("  %s\n", builtin_str[i]);
        }

        printf("Use the man command for information on other programs.\n");
        return 1;
}

int hush_exit(char **args)
{
        return 0;
}
