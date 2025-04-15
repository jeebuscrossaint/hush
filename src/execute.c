#include "execute.h"

int hush_execute(char **args)
{
        int i;

        if (args[0] == NULL)
        {
                return 1;
        }

        for (i = 0; i < hush_num_builtins(); i++)
        {
                if (strcmp(args[0], builtin_str[i]) == 0)
                {
                        return (*builtin_func[i])(args);
                }
        }

        return hush_launch(args);
}