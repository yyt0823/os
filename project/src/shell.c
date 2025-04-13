#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "shell.h"
#include "interpreter.h"
#include "scheduler.h"
#include "shellmemory.h"

int parseInput (char ui[]);

// Start of everything
int main (int argc, char *argv[]) {
    printf ("Shell version 1.4 created December 2024\n\n");

    char prompt = '$';          // Shell prompt
    char userInput[MAX_USER_INPUT];     // user's input stored here
    int errorCode = 0;          // zero means no error, default

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }

    //init shell memory
    mem_init ();

    // isatty function for check if interacte with terminal
    int interactive = isatty (fileno (stdin));

    while (1) {
        //1.2.2 no $ for batch mode
        if (interactive) {
            printf ("%c ", prompt);
        }
        // here you should check the unistd library 
        // so that you can find a way to not display $ in the batch mode

        //1.2.2
        //switch interative and batch mode
        if (interactive) {
            fgets (userInput, MAX_USER_INPUT - 1, stdin);
        } else
            //batch mode terminate when file ends
        if (fgets (userInput, MAX_USER_INPUT - 1, stdin) == NULL) {
            break;
        }


        errorCode = parseInput (userInput);
        if (errorCode == -1)
            exit (99);          // ignore all other errors
        memset (userInput, 0, sizeof (userInput));
    }

    return 0;
}

int wordEnding (char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ' || c == ';';
}

int parseInput (char inp[]) {
    int errorCode = 0;

    if (strchr (inp, ';') == NULL) {
        char tmp[200], *words[100];
        int ix = 0, w = 0;
        int wordlen;
        for (ix = 0; inp[ix] == ' ' && ix < 1000; ix++);        // skip white spaces
        while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
            // extract a word
            for (wordlen = 0; !wordEnding (inp[ix]) && ix < 1000;
                 ix++, wordlen++) {
                tmp[wordlen] = inp[ix];
            }
            tmp[wordlen] = '\0';
            words[w] = strdup (tmp);
            w++;
            if (inp[ix] == '\0')
                break;
            ix++;
        }
        errorCode = interpreter (words, w);
        return errorCode;
    } else {
        // Input contains semicolon(s) -- split into separate commands.
        char *p = inp;
        while (*p != '\0') {
            // Skip any leading white spaces.
            while (*p == ' ') {
                p++;
            }
            // If we reach newline or end-of-string, we're done.
            if (*p == '\n' || *p == '\0')
                break;
            // Copy the current command segment into a temporary buffer.
            char segment[1000];
            int segIx = 0;
            while (*p != ';' && *p != '\n' && *p != '\0' && segIx < 999) {
                segment[segIx++] = *p;
                p++;
            }
            segment[segIx] = '\0';
            // If a semicolon was encountered, skip it.
            if (*p == ';')
                p++;
            // Now parse this segment using your original parsing logic.
            {
                char tmp[200], *words[100];
                int ix = 0, w = 0;
                int wordlen;
                // Skip leading spaces in the segment.
                for (ix = 0; segment[ix] == ' ' && ix < 1000; ix++);
                while (segment[ix] != '\n' && segment[ix] != '\0' && ix < 1000) {
                    for (wordlen = 0; !wordEnding (segment[ix]) && ix < 1000;
                         ix++, wordlen++) {
                        tmp[wordlen] = segment[ix];
                    }
                    tmp[wordlen] = '\0';
                    words[w] = strdup (tmp);
                    w++;
                    if (segment[ix] == '\0')
                        break;
                    ix++;       // skip the delimiter
                }
                errorCode = interpreter (words, w);
                // Free allocated memory for this command.
                for (int i = 0; i < w; i++) {
                    free (words[i]);
                }
            }
        }
        return errorCode;
    }
}
