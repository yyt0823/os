#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"
#include "scheduler.h"


//code loading 
/* ===== Program Memory Implementation ===== */

/* Global instance of program memory */
ProgramMemory progMem = {.count = 0 };

int loadProgram (const char *filename, int *start, int *line_count) {
    FILE *fp = fopen (filename, "r");
    if (fp == NULL) {
        fprintf (stderr, "Error: Cannot open file %s\n", filename);
        return -1;
    }

    /* Record the starting index for this program. */
    *start = progMem.count;

    char buffer[MAX_LINE_LENGTH];
    while (fgets (buffer, sizeof (buffer), fp)) {
        if (progMem.count >= MAX_PROGRAM_LINES) {
            fclose (fp);
            fprintf (stderr,
                     "Error: Program memory full; cannot load script\n");
            return -2;
        }

        /* Remove any trailing newline or carriage return characters. */
        buffer[strcspn (buffer, "\r\n")] = '\0';

        progMem.lines[progMem.count] = strdup (buffer);
        if (progMem.lines[progMem.count] == NULL) {
            fclose (fp);
            fprintf (stderr,
                     "Error: Memory allocation failed while loading script\n");
            return -3;
        }
        progMem.count++;
    }
    fclose (fp);

    /* Compute the number of lines loaded for this script. */
    *line_count = progMem.count - *start;
    return 0;
}

void clearProgramMemory (void) {
    for (int i = 0; i < progMem.count; i++) {
        free (progMem.lines[i]);
        progMem.lines[i] = NULL;
    }
    progMem.count = 0;
}

char *getProgramLine (int index) {
    if (index < 0 || index >= progMem.count) {
        return NULL;
    }
    return progMem.lines[index];
}



//////////////////////////////////////////////////////////////////////////////

struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

// Helper functions
int match (char *model, char *var) {
    int i, len = strlen (var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i])
            matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else
        return 0;
}

// Shell memory functions

void mem_init () {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value (char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp (shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup (value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp (shellmemory[i].var, "none") == 0) {
            shellmemory[i].var = strdup (var_in);
            shellmemory[i].value = strdup (value_in);
            return;
        }
    }

    return;
}

//get value based on input key
char *mem_get_value (char *var_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp (shellmemory[i].var, var_in) == 0) {
            return strdup (shellmemory[i].value);
        }
    }
    return "Variable does not exist";
}
