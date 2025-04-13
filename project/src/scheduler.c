#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "shellmemory.h"        // For getProgramLine() and clearProgramMemory()
#include "shell.h"              // For parseInput()

// For "badcommand()"
extern int badcommand (void);

/* Global ready queue pointers and PID counter */
PCB *readyQueueHead = NULL;
PCB *readyQueueTail = NULL;
int nextPID = 1;

/* ====== READY QUEUE OPERATIONS ====== */

void enqueuePCB (PCB * pcb) {
    pcb->next = NULL;
    if (!readyQueueTail) {
        readyQueueHead = pcb;
        readyQueueTail = pcb;
    } else {
        readyQueueTail->next = pcb;
        readyQueueTail = pcb;
    }
}

PCB *dequeuePCB (void) {
    if (!readyQueueHead)
        return NULL;
    PCB *pcb = readyQueueHead;
    readyQueueHead = readyQueueHead->next;
    if (!readyQueueHead)
        readyQueueTail = NULL;
    pcb->next = NULL;
    return pcb;
}

/* Insert PCB in ascending order by job_score; tie-break by scriptName. */
void sortedEnqueuePCB (PCB * pcb) {
    pcb->next = NULL;
    // If queue is empty or pcb should go at the front
    if (!readyQueueHead ||
        (pcb->job_score < readyQueueHead->job_score) ||
        (pcb->job_score == readyQueueHead->job_score &&
         strcmp (pcb->scriptName, readyQueueHead->scriptName) <= 0)) {
        pcb->next = readyQueueHead;
        readyQueueHead = pcb;
        if (!readyQueueTail)
            readyQueueTail = pcb;
        return;
    }
    // Otherwise, insert in correct position
    PCB *curr = readyQueueHead;
    while (curr->next &&
           ((curr->next->job_score < pcb->job_score) ||
            (curr->next->job_score == pcb->job_score &&
             strcmp (curr->next->scriptName, pcb->scriptName) <= 0))) {
        curr = curr->next;
    }
    pcb->next = curr->next;
    curr->next = pcb;
    if (!pcb->next)
        readyQueueTail = pcb;
}

/* ====== AGING SORT HELPER (SJF-AGING) ====== */
PCB *agingSort (PCB * head) {
    if (!head || !head->next)
        return head;
    int swapped;
    PCB *lptr = NULL;
    do {
        swapped = 0;
        PCB *ptr = head;
        PCB *prev = NULL;
        while (ptr->next != lptr) {
            int needSwap = 0;
            if (ptr->job_score > ptr->next->job_score)
                needSwap = 1;
            else if (ptr->job_score == ptr->next->job_score &&
                     strcmp (ptr->scriptName, ptr->next->scriptName) > 0)
                needSwap = 1;

            if (needSwap) {
                // Swap adjacent
                PCB *temp = ptr->next;
                ptr->next = temp->next;
                temp->next = ptr;
                if (!prev)
                    head = temp;
                else
                    prev->next = temp;
                swapped = 1;
                prev = temp;
            } else {
                prev = ptr;
                ptr = ptr->next;
            }
        }
        lptr = ptr;
    } while (swapped);
    return head;
}

/* ====== BASIC SCHEDULERS ====== */

/* FCFS and SJF can share the same "run all lines" loop once the queue is sorted. */
void runScheduler_FCFS (void) {
    while (readyQueueHead) {
        PCB *current = dequeuePCB ();
        int scriptEnd = current->start_line + current->line_count;
        while (current->pc < scriptEnd) {
            char *line = getProgramLine (current->pc);
            if (line)
                parseInput (line);      // interpret/execute 1 command
            current->pc++;
        }
        free (current);
    }
    clearProgramMemory ();
}

void sortReadyQueue_SJF (void) {
    // Bubble sort by line_count ascending
    if (!readyQueueHead || !readyQueueHead->next)
        return;
    int swapped;
    PCB *lptr = NULL;
    do {
        swapped = 0;
        PCB *ptr = readyQueueHead;
        PCB *prev = NULL;
        while (ptr->next != lptr) {
            if (ptr->line_count > ptr->next->line_count) {
                PCB *temp = ptr->next;
                ptr->next = temp->next;
                temp->next = ptr;
                if (!prev)
                    readyQueueHead = temp;
                else
                    prev->next = temp;
                swapped = 1;
                prev = temp;
            } else {
                prev = ptr;
                ptr = ptr->next;
            }
        }
        lptr = ptr;
    } while (swapped);

    // Update tail
    PCB *tail = readyQueueHead;
    while (tail && tail->next)
        tail = tail->next;
    readyQueueTail = tail;
}

void runScheduler_SJF (void) {
    /*
     * 1) Look for a PCB whose scriptName == "BATCH_SCRIPT".
     *    If found, remove it from the queue and run it first (to completion).
     */
    PCB *prev = NULL;
    PCB *curr = readyQueueHead;
    PCB *batchPCB = NULL;

    // Search the queue for the batch script
    while (curr) {
        if (strcmp (curr->scriptName, "BATCH_SCRIPT") == 0) {
            batchPCB = curr;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    // If we found the batch script, remove it from the queue and run it immediately
    if (batchPCB) {
        // Remove from linked list
        if (batchPCB == readyQueueHead) {
            // It's at the head
            readyQueueHead = batchPCB->next;
            if (!readyQueueHead) {
                readyQueueTail = NULL;
            }
        } else {
            // It's in the middle or tail
            prev->next = batchPCB->next;
            if (batchPCB == readyQueueTail) {
                readyQueueTail = prev;
            }
        }
        batchPCB->next = NULL;

        // Run batch script to completion (SJF is non-preemptive anyway)
        while (batchPCB->pc < batchPCB->start_line + batchPCB->line_count) {
            char *line = getProgramLine (batchPCB->pc);
            if (line)
                parseInput (line);
            batchPCB->pc++;
        }
        // Done, free it
        free (batchPCB);
    }

    /*
     * 2) Now we do the normal SJF sorting on whatever remains in the queue.
     *    (Bubble sort by line_count ascending, as in your existing sortReadyQueue_SJF)
     */
    if (readyQueueHead && readyQueueHead->next) {
        int swapped;
        PCB *lptr = NULL;
        do {
            swapped = 0;
            PCB *ptr = readyQueueHead;
            PCB *prv = NULL;
            while (ptr->next != lptr) {
                if (ptr->line_count > ptr->next->line_count) {
                    // Swap adjacent PCBs
                    PCB *temp = ptr->next;
                    ptr->next = temp->next;
                    temp->next = ptr;
                    if (!prv) {
                        readyQueueHead = temp;
                    } else {
                        prv->next = temp;
                    }
                    swapped = 1;
                    prv = temp;
                } else {
                    prv = ptr;
                    ptr = ptr->next;
                }
            }
            lptr = ptr;
        } while (swapped);

        // Update readyQueueTail after sorting
        PCB *temp2 = readyQueueHead;
        while (temp2 && temp2->next) {
            temp2 = temp2->next;
        }
        readyQueueTail = temp2;
    }

    /*
     * 3) Finally, proceed with "FCFS run" on the sorted queue. (Non-preemptive SJF
     *    is effectively "sort first, then run FCFS".)
     */
    while (readyQueueHead) {
        PCB *current = dequeuePCB ();
        int scriptEnd = current->start_line + current->line_count;
        while (current->pc < scriptEnd) {
            char *line = getProgramLine (current->pc);
            if (line)
                parseInput (line);
            current->pc++;
        }
        free (current);
    }

    // Clear out program memory after finishing all scripts
    clearProgramMemory ();
}


/* Round Robin (time slice = 2) */
void runScheduler_RR (void) {
    const int timeSlice = 2;
    while (readyQueueHead) {
        PCB *current = dequeuePCB ();
        int scriptEnd = current->start_line + current->line_count;
        int instructionsRun = 0;
        while (instructionsRun < timeSlice && current->pc < scriptEnd) {
            char *line = getProgramLine (current->pc);
            if (line)
                parseInput (line);
            current->pc++;
            instructionsRun++;
        }
        // If not finished, requeue
        if (current->pc < scriptEnd) {
            enqueuePCB (current);
        } else {
            free (current);
        }
    }
    clearProgramMemory ();
}

/* RR30 (time slice = 30) */
void runScheduler_RR30 (void) {
    const int timeSlice = 30;
    while (readyQueueHead) {
        PCB *current = dequeuePCB ();
        int scriptEnd = current->start_line + current->line_count;
        int instructionsRun = 0;
        while (instructionsRun < timeSlice && current->pc < scriptEnd) {
            char *line = getProgramLine (current->pc);
            if (line)
                parseInput (line);
            current->pc++;
            instructionsRun++;
        }
        if (current->pc < scriptEnd)
            enqueuePCB (current);
        else
            free (current);
    }
    clearProgramMemory ();
}

/* ====== SJF with AGING (time slice = 1) ======
   1) Dequeue head (current), run 1 instruction.
   2) Age everyone in the queue by 1 (job_score = max(job_score-1,0)).
   3) Re-sort the queue by job_score.
   4) If current still has instructions, decide whether it goes to head or sorted:
      - If the head of the queue has a strictly lower job_score than current, current is enqueued sorted
      - Otherwise current goes back at the head.
*/
void runScheduler_SJF_Aging (void) {
    while (readyQueueHead) {
        PCB *current = dequeuePCB ();
        int scriptEnd = current->start_line + current->line_count;

        // Run exactly 1 instruction if any left
        if (current->pc < scriptEnd) {
            char *line = getProgramLine (current->pc);
            if (line)
                parseInput (line);
            current->pc++;
        }

        // If finished, free, else age others and reinsert current appropriately
        if (current->pc >= scriptEnd) {
            free (current);
        } else {
            // Age all waiting jobs
            PCB *temp = readyQueueHead;
            while (temp) {
                if (temp->job_score > 0)
                    temp->job_score--;
                temp = temp->next;
            }
            // Re-sort the queue
            readyQueueHead = agingSort (readyQueueHead);

            // Check if current stays in front
            if (!readyQueueHead
                || current->job_score <= readyQueueHead->job_score) {
                // put current back at head
                current->next = readyQueueHead;
                readyQueueHead = current;
                if (!readyQueueTail)
                    readyQueueTail = current;
            } else {
                // put it in sorted order
                sortedEnqueuePCB (current);
            }
        }
    }
    clearProgramMemory ();
}

/* Default fallback (if no policy matched) */
void runScheduler (void) {
    runScheduler_FCFS ();
}


/* ====== LOADING PROGRAMS / BATCH SCRIPTS ====== */

/* loadProgram() 
 *  - You must implement this in, e.g., shellmemory.c or here.
 *  - Reads an entire file (script) line-by-line into progMem.
 *  - Updates 'start' and 'line_count' with the memory offsets.
 *  Return 0 if success, <0 if failure.
 */
extern int loadProgram (const char *filename, int *start, int *line_count);

/* loadBatchScript()
 *  - Reads the remainder of standard input as a “batch script process.”
 *  - This is used when `exec` is called with the "#" parameter.
 */
int loadBatchScript (int *start, int *line_count) {
    char buffer[MAX_LINE_LENGTH];
    *start = progMem.count;
    *line_count = 0;
    while (fgets (buffer, sizeof (buffer), stdin) != NULL) {
        buffer[strcspn (buffer, "\r\n")] = '\0';
        progMem.lines[progMem.count] = strdup (buffer);
        if (!progMem.lines[progMem.count]) {
            fprintf (stderr,
                     "Error: Memory allocation failed while loading batch script.\n");
            return -1;
        }
        progMem.count++;
        (*line_count)++;
        if (progMem.count >= MAX_PROGRAM_LINES) {
            // reached memory limit
            break;
        }
    }
    return 0;
}

/* ====== EXEC COMMAND (MAIN ENTRY FOR PART 2) ======
   Usage: exec prog1 [prog2 prog3] POLICY [#]
   - If "#" is present, then the rest of the input is treated as a "batch script process".
   - That batch script process is placed in the front of the queue so it runs first.
   - Then all programs run under the chosen scheduling policy in a “simulated concurrency.”
*/
int exec_command (char *args[], int arg_count) {
    int background = 0;

    // Check if last arg is "#"
    if (strcasecmp (args[arg_count - 1], "#") == 0) {
        background = 1;
        arg_count--;            // effectively remove "#"
    }

    // Now we expect at least: exec prog1 POLICY  or exec prog1 prog2 POLICY
    if (arg_count < 3 || arg_count > 5) {
        printf
            ("Error: exec requires between 2 and 3 script filenames and a scheduling policy.\n");
        return badcommand ();
    }

    // The policy is the last argument
    char *policy = args[arg_count - 1];
    int policyType = -1;
    if (strcasecmp (policy, "FCFS") == 0)
        policyType = 0;
    else if (strcasecmp (policy, "SJF") == 0)
        policyType = 1;
    else if (strcasecmp (policy, "RR") == 0)
        policyType = 2;
    else if (strcasecmp (policy, "AGING") == 0)
        policyType = 3;
    else if (strcasecmp (policy, "RR30") == 0)
        policyType = 4;
    else {
        printf
            ("Error: Unsupported scheduling policy: %s. Supported: FCFS, SJF, RR, AGING, RR30.\n",
             policy);
        return badcommand ();
    }

    // The scripts are everything between "exec" and the policy
    int numScripts = arg_count - 2;     // e.g.: exec p1 p2 p3 POLICY => p1,p2,p3 are scripts
    if (numScripts < 1 || numScripts > 3) {
        printf ("Error: You must supply 1 to 3 scripts before the policy.\n");
        return badcommand ();
    }

    // Check for duplicate script names (assignment requirement)
    for (int i = 1; i <= numScripts; i++) {
        for (int j = i + 1; j <= numScripts; j++) {
            if (strcmp (args[i], args[j]) == 0) {
                printf ("Error: Duplicate script names are not allowed.\n");
                return badcommand ();
            }
        }
    }

    // Load each script into memory & create a PCB
    for (int i = 1; i <= numScripts; i++) {
        int start, line_count;
        if (loadProgram (args[i], &start, &line_count) < 0) {
            printf ("Error loading script: %s\n", args[i]);
            clearProgramMemory ();
            return badcommand ();
        }
        PCB *pcb = malloc (sizeof (PCB));
        if (!pcb) {
            printf ("Error: PCB allocation failed.\n");
            clearProgramMemory ();
            return badcommand ();
        }
        pcb->pid = nextPID++;
        pcb->start_line = start;
        pcb->line_count = line_count;
        pcb->pc = start;
        pcb->job_score = line_count;    // for SJF-AGING
        pcb->next = NULL;
        strncpy (pcb->scriptName, args[i], sizeof (pcb->scriptName));
        pcb->scriptName[sizeof (pcb->scriptName) - 1] = '\0';

        if (policyType == 3) {
            // SJF-AGING uses sorted queue
            sortedEnqueuePCB (pcb);
        } else {
            enqueuePCB (pcb);
        }
    }

    // If background mode => load the rest of stdin as the "batch script process" 
    // which runs *first* among all programs
    if (background) {
        int batchStart, batchCount;

        if (loadBatchScript (&batchStart, &batchCount) < 0) {
            printf ("Error loading batch script process from stdin.\n");
            clearProgramMemory ();
            return badcommand ();
        }

        PCB *batchPcb = malloc (sizeof (PCB));
        if (!batchPcb) {
            printf ("Error: Batch PCB allocation failed.\n");
            clearProgramMemory ();
            return badcommand ();
        }

        batchPcb->pid = nextPID++;
        batchPcb->start_line = batchStart;
        batchPcb->line_count = batchCount;
        batchPcb->pc = batchStart;
        batchPcb->job_score = batchCount;
        batchPcb->next = NULL;
        strncpy (batchPcb->scriptName, "BATCH_SCRIPT",
                 sizeof (batchPcb->scriptName));
        batchPcb->scriptName[sizeof (batchPcb->scriptName) - 1] = '\0';

        // Per requirement: the batch script must run first
        // so we insert it at the HEAD of the queue
        batchPcb->next = readyQueueHead;
        readyQueueHead = batchPcb;
        if (!readyQueueTail)
            readyQueueTail = batchPcb;
    }

    // Now run the chosen scheduler
    switch (policyType) {
        case 0:
            runScheduler_FCFS ();
            break;              // FCFS
        case 1:
            runScheduler_SJF ();
            break;              // SJF
        case 2:
            runScheduler_RR ();
            break;              // RR
        case 3:
            runScheduler_SJF_Aging ();
            break;              // AGING
        case 4:
            runScheduler_RR30 ();
            break;              // RR30
        default:
            // Fallback
            runScheduler_FCFS ();
            break;
    }

    return 0;                   // success
}
