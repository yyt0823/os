#define MEM_SIZE 1000
void mem_init ();
char *mem_get_value (char *var);
void mem_set_value (char *var, char *value);

/* ===== Program Memory for Code Loading ===== */
#define MAX_PROGRAM_LINES 1000
#define MAX_LINE_LENGTH 256

/* A structure to store all program (script) lines. */
typedef struct {
    char *lines[MAX_PROGRAM_LINES];     // Array of pointers to lines.
    int count;                  // Number of lines currently stored.
} ProgramMemory;

/* Global instance of program memory. */
extern ProgramMemory progMem;

//loadProgram
int loadProgram (const char *filename, int *start, int *line_count);

/* clearProgramMemory:
 *   Frees all allocated program lines and resets progMem.
 */
void clearProgramMemory (void);

/* getProgramLine:
 *   Returns the program line at the given index, or NULL if out of bounds.
 */
char *getProgramLine (int index);
