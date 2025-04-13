#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include "shellmemory.h"
#include "shell.h"
#include "scheduler.h"

int MAX_ARGS_SIZE = 6;

int badcommand () {
    printf ("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist () {
    printf ("Bad command: File not found\n");
    return 3;
}

//1.2.1 echo command
int echo (char *arg);
//1.2.3 my_ls
int my_ls ();
int my_mkdir (char *arg);
int my_touch (char *arg);
int my_cd (char *dirname);
int run (char *command_args[], int args_size);
int help ();
int quit ();
int set (char *var, char *value);
int print (char *var);
int source (char *script);
int badcommandFileDoesNotExist ();
int exec_command (char *args[], int arg_count);


// Interpret commands and their arguments
int interpreter (char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand ();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn (command_args[i], "\r\n")] = 0;
    }

    if (strcmp (command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand ();
        return help ();

    } else if (strcmp (command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand ();
        return quit ();

    } else if (strcmp (command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand ();
        return set (command_args[1], command_args[2]);

    } else if (strcmp (command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand ();
        return print (command_args[1]);

    } else if (strcmp (command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand ();
        return source (command_args[1]);

        // ADDED: echo command
    } else if (strcmp (command_args[0], "echo") == 0) {
        if (args_size != 2)
            return badcommand ();
        return echo (command_args[1]);


        // ADDED: my_ls command
    } else if (strcmp (command_args[0], "my_ls") == 0) {

        if (args_size != 1)
            return badcommand ();
        return my_ls ();
        // ADDED: my_mkdir command
    } else if (strcmp (command_args[0], "my_mkdir") == 0) {

        if (args_size != 2)
            return badcommand ();
        return my_mkdir (command_args[1]);
        // ADDED: my_touch command
    } else if (strcmp (command_args[0], "my_touch") == 0) {

        if (args_size != 2)
            return badcommand ();
        return my_touch (command_args[1]);

        // ADDED: my_cd command
    } else if (strcmp (command_args[0], "my_cd") == 0) {

        if (args_size != 2)
            return badcommand ();
        return my_cd (command_args[1]);

        // ADDED: run command branch (last part)
    } else if (strcmp (command_args[0], "run") == 0) {
        // At least one argument after "run" is required.
        if (args_size < 2)
            return badcommand ();
        return run (command_args, args_size);

        //exec
    } else if (strcmp (command_args[0], "exec") == 0) {
        // Valid exec calls: "exec prog1 POLICY" or "exec prog1 prog2 POLICY" or "exec prog1 prog2 prog3 POLICY"
        if (args_size < 3 || args_size > 6)
            return badcommand ();
        return exec_command (command_args, args_size);
    } else
        return badcommand ();
}

int help () {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with "Bye!"\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n \
echo STRING     return input string or looks for varname for input follow by '$', return varname value if found return empty new line if not";
    printf ("%s\n", help_string);
    return 0;
}

int quit () {
    printf ("Bye!\n");
    exit (0);
}

int set (char *var, char *value) {
    char *link = " ";

    // Hint: If "value" contains multiple tokens, you'll need to loop through them, 
    // concatenate each token to the buffer, and handle spacing appropriately. 
    // Investigate how `strcat` works and how you can use it effectively here.

    mem_set_value (var, value);
    return 0;
}


int print (char *var) {
    printf ("%s\n", mem_get_value (var));
    return 0;
}

int source (char *script) {
    int start, line_count;
    // Load the entire script into program memory.
    if (loadProgram (script, &start, &line_count) < 0) {
        // You can replace the following with your own error handling.
        return badcommandFileDoesNotExist ();
    }
    // Create a PCB for the script.
    PCB *pcb = (PCB *) malloc (sizeof (PCB));
    if (pcb == NULL) {
        printf ("Error: Memory allocation for PCB failed.\n");
        clearProgramMemory ();
        return 1;
    }
    pcb->pid = nextPID++;
    pcb->start_line = start;
    pcb->line_count = line_count;
    pcb->pc = start;
    pcb->next = NULL;

    // Enqueue the PCB into the ready queue.
    enqueuePCB (pcb);

    // Run the scheduler. For source, the ready queue will have only this one PCB.
    runScheduler ();

    // runScheduler() calls clearProgramMemory() after finishing.
    return 0;
}


int echo (char *arg) {
    //case when input varname
    if (arg[0] == '$') {
        //skip the first char by adding 1 on the pointer
        char *varname = arg + 1;
        //get the value using function mem_get_value
        char *value = mem_get_value (varname);
        // if the variable does not exist, echo print a blank line.
        if (strcmp (value, "Variable does not exist") == 0) {
            printf ("\n");
        } else {
            // Otherwise, print the value and free the allocated memory.
            printf ("%s\n", value);
            free (value);
        }
    } else {
        // If the argument does not start with '$', just print it.
        printf ("%s\n", arg);
    }
    return 0;
}


//helper function to sort
int compare (const void *a, const void *b) {
    char *s1 = *(char **) a;
    char *s2 = *(char **) b;
    return strcmp (s1, s2);
}

int my_ls () {

    struct dirent *entry;
    //error handling
    DIR *d = opendir (".");
    if (d == NULL) {
        printf ("Could not open current directory\n");
        return 1;
    }
    //some args
    char *files[1024];
    int count = 0;

    //read all dir name to buffer
    while ((entry = readdir (d)) != NULL) {
        files[count] = strdup (entry->d_name);
        count++;
    }
    closedir (d);

    //sort
    qsort (files, count, sizeof (char *), compare);

    //print
    for (int i = 0; i < count; i++) {
        printf ("%s\n", files[i]);
        free (files[i]);
    }

    return 0;
}

//helper funciton for single alphanumeric
int is_alphanumeric (const char *str) {
    if (!str || strlen (str) == 0) {
        return 0;
    }
    // Check if each character is alphanumeric
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalnum (str[i])) {
            return 0;
        }
    }
    return 1;
}



int my_mkdir (char *arg) {
    //buffer for name
    char actualName[256];

    //case 2 arg is a var
    if (arg[0] == '$') {
        char *value = mem_get_value (arg + 1);
        if (!value || strcmp (value, "Variable does not exist") == 0) {
            printf ("Bad command: my_mkdir\n");
            return 1;
        }
        // Validate if the literal string is alphanumeric
        if (!is_alphanumeric (value)) {
            printf ("Bad command: my_mkdir\n");
            return 1;
        }
        strncpy (actualName, value, sizeof (actualName));
        actualName[sizeof (actualName) - 1] = '\0';
    } else {
        // case 1   arg is a normal alphanumeric name
        strncpy (actualName, arg, sizeof (actualName));
        actualName[sizeof (actualName) - 1] = '\0';
    }

    //now we have the name and we create the dir with mode 0777 full permission or others
    if (mkdir (actualName, 0777) == -1) {
        printf ("Bad command: my_mkdir\n");
        return 1;
    }
    return 0;

}


//open file and save it
int my_touch (char *arg) {
    //open a file
    FILE *fp = fopen (arg, "w");
    if (fp == NULL) {
        // Could not create or open the file
        return 1;
    }
    fclose (fp);

    return 0;
}

//my_cd 
int my_cd (char *dirname) {
    DIR *dir = opendir (".");
    if (dir == NULL) {
        perror ("opendir");
        return 1;
    }

    int found = 0;
    struct dirent *entry;
    while ((entry = readdir (dir)) != NULL) {
        // Check if the entry name matches the given dirname.
        if (strcmp (entry->d_name, dirname) == 0) {
            found = 1;
            break;
        }
    }
    closedir (dir);

    if (!found) {
        printf ("Bad command: my_cd\n");
        return 1;
    }
    // Change directory to dirname.
    if (chdir (dirname) == -1) {
        printf ("Bad command: my_cd\n");
        return 1;
    }

    return 0;
}

// ADDED: runCommand function implementation
int run (char *command_args[], int args_size) {
    //added flush as discord Q and A
    fflush (stdout);
    //fork, create a new process
    pid_t pid = fork ();
    //fork fail 
    if (pid < 0) {
        perror ("fork failed");
        return 1;
        //fork success now work in child process
    } else if (pid == 0) {
        // Prepare a new argv array for execvp() by removing the "run" token.
        int newArgc = args_size - 1;
        char *newArgs[newArgc + 1];     // Last element must be NULL.
        for (int i = 0; i < newArgc; i++) {
            newArgs[i] = command_args[i + 1];
        }
        newArgs[newArgc] = NULL;
        // Execute the command using execvp().
        if (execvp (newArgs[0], newArgs) == -1) {
            perror ("execvp failed");
            exit (1);           // Terminate child if execvp fails.
        }
    } else {
        // In parent process: wait for the child to finish.
        int status;
        waitpid (pid, &status, 0);
    }
    return 0;
}
