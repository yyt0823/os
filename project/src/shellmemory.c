#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

#define true 1
#define false 0

// Global timestamp counter for LRU tracking
static int access_counter = 0;

// Function to get the current timestamp
int
get_current_time ()
{
  return ++access_counter;
}

//frame struct



// ---------------------
// Line memory for program lines; for part 2.
// ---------------------

// We know that program lines will be read, but not modified, until they are
// removed from the line memory. Therefore, we can provide the line directly
// to requests, rather than copying it. Freeing a program will free all of its
// lines.
// When a line is saved, it should be copied. This way, it can be saved from
// a stack-allocated buffer without fear of losing it when the buffer is freed.


struct program_line linememory[VAR_MEM_SIZE];
struct frame frame_store[NUM_FRAMES];



// We have two choices:
//  1. Offer an API that lets the client allocate one line at a time.
//  2. Offer an API that allocates whole programs at a time, and tracks their
//      locations for being freed later.
// (2) sounds harder, and it is, but that work needs to be done _somewhere_.
// The description suggests that rather than doing it here, we can do it in
// the PCB, so we'll do that. This will translate better to having page tables
// in PCBs in part 3 so it's probably the more sensible approach.

// Therefore, our API is to allocate one line at a time. Normally, we'd have
// to worry about fragmentation and contiguity, but, we're supposed to allocate
// everything before anything runs, and everything runs to completion before
// any more commands can execute, AND there's no recursive source/exec.
// All of that combined means we don't have to worry about fragmentation
// at all: between invocations of source/exec, the linememory will be EMPTY!
//
// Actually, there is one case of 'recursive' exec: the tests that look like
//   exec .... #
//   echo shell
//   exec ....
// The 'shell program' will recursively exec. The correct behavior in these
// cases is to enqueue new program(s), not run them to completion immediately.
// We're told we can assume that 1000 lines is enough to hold every program
// that is being tested simultaneously, so we won't worry about the condition
// that the linememory is empty once exec has been called with #.
// At that point, the assumption tells us that there is enough memory left.

size_t next_free_line = 0;
// So we simply record the next free index in a variable.
// We can't reset this whenever a PCB is freed, because we might be running
// with # and the shell might be about to allocate another program and that
// program might not fit in the first hole! So we provide a function to reset
// this, which should be called whenever `exec` is called and we're not in
// 'background mode.'
void
reset_linememory_allocator ()
{
  next_free_line = 0;
  assert_linememory_is_empty ();
}

// The comments above detail an important, nontrivial invariant, that the
// linememory is completely empty at certain times.
// For sanity checking, we provide an additional function to assert that the
// the linememory is empty.

void
assert_linememory_is_empty ()
{
  for (size_t i = 0; i < FRAME_STORE_SIZE; ++i)
    {
      assert (!linememory[i].allocated);
      assert (linememory[i].line == NULL);
    }
}

// note that init_linemem is not exposed from the header.
// We made init_mem call init_linemem, down below.
void
init_linemem ()
{
  for (size_t i = 0; i < FRAME_STORE_SIZE; ++i)
    {
      linememory[i].allocated = false;
      linememory[i].line = NULL;
    }
}

size_t
allocate_line (const char *line)
{
  if (next_free_line >= FRAME_STORE_SIZE)
    {
      // out of memory!
      return (size_t) (-1);
    }
  size_t index = next_free_line++;
  assert (!linememory[index].allocated);

  linememory[index].allocated = true;
  // As described above, the allocator is only borrowing the string passed to it,
  // but linememory must own all strings it contains, so we need to copy the
  // string. (If you don't know what that means, see [Note: OBS].)
  linememory[index].line = strdup (line);
  return index;
}

// To free a line, we must deallocate it and mark it unused.
// We don't mess with next_free_line for the reasons described above.
void
free_line (size_t index)
{
  size_t frame = index / FRAME_SIZE;
  size_t offset = index % FRAME_SIZE;

  if (frame >= NUM_FRAMES)
    {
      fprintf (stderr, "ERROR: Trying to free invalid frame %zu\n", frame);
      return;
    }

  if (frame_store[frame].lines[offset].allocated)
    {
      free (frame_store[frame].lines[offset].line);
      frame_store[frame].lines[offset].allocated = 0;
      frame_store[frame].lines[offset].line = NULL;
    }
}

// Return a const pointer to ensure the caller doesn't do something horrific,
// like try to free it.
const char *
get_line (size_t index)
{
  // Calculate which frame and offset this index corresponds to
  size_t frame = index / FRAME_SIZE;
  size_t offset = index % FRAME_SIZE;

  // Bounds check
  if (frame > NUM_FRAMES)
    {
      printf ("NUM_FRAMES: %d\n", NUM_FRAMES);

      fprintf (stderr, "ERROR: Trying to access frame %zu but max is %d\n",
	       frame, NUM_FRAMES - 1);
      return NULL;
    }

  // Check if the line is allocated
  if (!frame_store[frame].lines[offset].allocated)
    {
      fprintf (stderr,
	       "ERROR: Trying to access unallocated line at frame %zu, offset %zu\n",
	       frame, offset);
      return NULL;
    }

  return frame_store[frame].lines[offset].line;
}

// [Note: OBS]
// Thinking about memory in terms of "owners" and "borrowers" was
// popularized by Rust, and recently formalized by so-called
// Ownership and Borrowing Systems. They are a concrete way to ensure that our
// programs do not leak memory or access invalid memory, at the cost of
// potentially copying data sometimes that it is not necessary to.
// A small price to pay!
// We highly recommend looking into how this works and using it as
// a mental model when you program.

// --------------------- 
// Key-value memory for variables; from part 1.
// ---------------------

struct memory_struct
{
  char *var;
  char *value;
};

struct memory_struct shellmemory[FRAME_STORE_SIZE];

// Helper functions
int
match (char *model, char *var)
{
  int i, len = strlen (var), matchCount = 0;
  for (i = 0; i < len; i++)
    {
      if (model[i] == var[i])
	matchCount++;
    }
  if (matchCount == len)
    {
      return 1;
    }
  else
    return 0;
}

// Shell memory functions

void
mem_init ()
{
  int i;
  for (i = 0; i < FRAME_STORE_SIZE; i++)
    {
      // Include a character that's impossible to type so that
      // we don't mistake an empty entry for an entry named 'none'.
      shellmemory[i].var = "none\1";
      shellmemory[i].value = "none";
    }

  init_linemem ();
}

// Set key value pair
void
mem_set_value (char *var_in, char *value_in)
{
  int i;

  for (i = 0; i < FRAME_STORE_SIZE; i++)
    {
      if (strcmp (shellmemory[i].var, var_in) == 0)
	{
	  free (shellmemory[i].value);
	  shellmemory[i].value = strdup (value_in);
	  return;
	}
    }

  //Value does not exist, need to find a free spot.
  for (i = 0; i < FRAME_STORE_SIZE; i++)
    {
      if (strcmp (shellmemory[i].var, "none\1") == 0)
	{
	  shellmemory[i].var = strdup (var_in);
	  shellmemory[i].value = strdup (value_in);
	  return;
	}
    }

  return;
}

//get value based on input key
char *
mem_get_value (char *var_in)
{
  int i;

  for (i = 0; i < FRAME_STORE_SIZE; i++)
    {
      if (strcmp (shellmemory[i].var, var_in) == 0)
	{
	  return strdup (shellmemory[i].value);
	}
    }
  return NULL;
}









//frame struct
// In this example, we assume that an unused frame has its first line not allocated.
int
allocate_frame ()
{
  for (int i = 0; i < NUM_FRAMES; i++)
    {
      // Check if all lines in this frame are not allocated
      int all_free = 1;
      for (int j = 0; j < FRAME_SIZE; j++)
	{
	  if (frame_store[i].lines[j].allocated)
	    {
	      all_free = 0;
	      break;
	    }
	}

      if (all_free)
	{
	  // Mark all the lines in this frame as not allocated yet
	  for (int j = 0; j < FRAME_SIZE; j++)
	    {
	      frame_store[i].lines[j].allocated = 0;
	      frame_store[i].lines[j].line = NULL;
	      frame_store[i].lines[j].owner = NULL;
	    }
	  // Initialize the timestamp
	  frame_store[i].last_access_time = get_current_time ();
	  return i;
	}
    }
  return -1;			// No free frame available
}
