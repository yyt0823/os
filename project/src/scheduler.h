#ifndef SCHEDULER_H
#   define SCHEDULER_H

typedef struct PCB {
    int pid;
    int start_line;
    int line_count;
    int pc;
    int job_score;              // For SJF with Aging
    struct PCB *next;
    char scriptName[64];        // <-- store the script’s name or unique ID



} PCB;

extern PCB *readyQueueHead;
extern PCB *readyQueueTail;
extern int nextPID;

int exec_command (char *args[], int arg_count);

/* Basic queue operations */
void enqueuePCB (PCB * pcb);    // Insert at tail (for FCFS, RR, etc.)
PCB *dequeuePCB (void);         // Remove from head
void sortedEnqueuePCB (PCB * pcb);      // Insert in ascending job_score order

/* Schedulers */
void runScheduler_FCFS (void);
void runScheduler_SJF (void);
void runScheduler_SJF_Aging (void);
void runScheduler_RR (void);
void runScheduler_RR30 (void);
void runScheduler (void);
int loadBatchScript (int *start, int *line_count);


#endif
