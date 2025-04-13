#define FRAME_SIZE 3		// Each frame holds exactly 3 lines


#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 30	// Total lines in frame store
#endif

#ifndef VAR_MEM_SIZE
#define VAR_MEM_SIZE 1000	// Total lines in variable store
#endif

#define NUM_FRAMES (FRAME_STORE_SIZE / FRAME_SIZE)	// Total frames = total lines / 3

void assert_linememory_is_empty (void);
size_t allocate_line (const char *line);
void free_line (size_t index);
const char *get_line (size_t index);
void reset_linememory_allocator (void);

void mem_init ();
char *mem_get_value (char *var);
void mem_set_value (char *var, char *value);

int allocate_frame ();
int get_current_time ();	// Function to get the current timestamp for LRU

struct program_line
{
  int allocated;		// for sanity-checking
  char *line;
  struct PCB *owner;		// Track which PCB owns this line
};
extern struct program_line linememory[VAR_MEM_SIZE];	// Variable store

struct frame
{
  struct program_line lines[FRAME_SIZE];
  int last_access_time;		// Timestamp for LRU tracking
};
extern struct frame frame_store[NUM_FRAMES];	// Frame store
