#include <stdio.h>
#include <stdlib.h>
#include <string.h>		// memset
#include <limits.h>		// INT_MAX
#include "shell.h"		// MAX_USER_INPUT
#include "shellmemory.h"
#include "pcb.h"

static pid fresh_pid = 1;

int
pcb_has_next_instruction (struct PCB *pcb)
{
  // have next if pc < line_count.
  // Sanity check: count = 0  ==> never have next. Good!
  return pcb->pc < pcb->line_count;
}

// Helper function to find a free frame
int
find_free_frame ()
{
  for (int i = 0; i < NUM_FRAMES; i++)
    {
      if (!frame_store[i].lines[0].allocated)
	{
	  return i;
	}
    }
  return -1;			// No free frame found
}

// Helper function to pick a victim frame using LRU
int
pick_victim_lru ()
{
  int lru_frame = -1;
  int oldest_time = INT_MAX;

  // Find the frame with the oldest access time
  for (int i = 0; i < NUM_FRAMES; i++)
    {
      // Check if at least one line in this frame is allocated
      int has_allocated = 0;
      for (int j = 0; j < FRAME_SIZE; j++)
	{
	  if (frame_store[i].lines[j].allocated)
	    {
	      has_allocated = 1;
	      break;
	    }
	}

      if (has_allocated && frame_store[i].last_access_time < oldest_time)
	{
	  oldest_time = frame_store[i].last_access_time;
	  lru_frame = i;
	}
    }

  return lru_frame;
}

// Helper function to update victim's owner page table
void
update_victim_owner (int frame)
{
  // Free the lines in the victim frame
  for (int i = 0; i < FRAME_SIZE; i++)
    {
      if (frame_store[frame].lines[i].allocated)
	{
	  free (frame_store[frame].lines[i].line);
	  frame_store[frame].lines[i].allocated = 0;
	  frame_store[frame].lines[i].line = NULL;
	  frame_store[frame].lines[i].owner = NULL;
	}
    }

  // Reset the timestamp
  frame_store[frame].last_access_time = 0;
}

void
handle_page_fault (struct PCB *pcb, size_t page_index)
{
  // 1) Temporarily "interrupt" this process:
  //    In RR, that means: "put it at the back of the ready queue"
  //    You might need to do that in your scheduler logic, e.g.
  //    by returning something that says "I triggered a fault" or so.

  // 2) Find a free frame or evict a random victim:
  int frame = find_free_frame ();
  if (frame < 0)
    {
      // no free frame => pick a victim using LRU
      frame = pick_victim_lru ();
      printf ("Page fault! ");
      printf ("Victim page contents:\n\n");
      for (int i = 0; i < FRAME_SIZE; i++)
	{
	  if (frame_store[frame].lines[i].allocated)
	    {
	      printf ("%s\n", frame_store[frame].lines[i].line);
	    }
	}
      printf ("\nEnd of victim page contents.\n");

      // Free the victim frame's contents
      update_victim_owner (frame);
    }
  else
    {
      // Just a page fault with free frame available
      printf ("Page fault!\n");
    }

  // Check if frame is valid
  if (frame < 0 || frame >= NUM_FRAMES)
    {
      fprintf (stderr, "ERROR: Invalid frame number %d\n", frame);
      return;
    }

  // Update the timestamp for this frame
  frame_store[frame].last_access_time = get_current_time ();

  // 3) Load the page from disk. We'll open the file, skip pages that came before:
  FILE *f = fopen (pcb->name, "r");
  if (!f)
    {
      perror ("handle_page_fault: Could not re-open file");
      // handle error
      return;
    }

  // skip (page_index * FRAME_SIZE) lines
  char buffer[MAX_USER_INPUT];
  for (int i = 0; i < (int) page_index * FRAME_SIZE; i++)
    {
      if (!fgets (buffer, MAX_USER_INPUT, f))
	break;
    }

  // read up to FRAME_SIZE lines
  for (int j = 0; j < FRAME_SIZE; j++)
    {
      // free any existing old line in that frame cell (if it wasn't freed)
      if (frame_store[frame].lines[j].allocated)
	{
	  free (frame_store[frame].lines[j].line);
	  frame_store[frame].lines[j].allocated = 0;
	  frame_store[frame].lines[j].line = NULL;
	  frame_store[frame].lines[j].owner = NULL;
	}

      if (!fgets (buffer, MAX_USER_INPUT, f))
	{
	  // no more lines => empty slot
	  continue;
	}
      size_t len = strlen (buffer);
      if (len > 0 && buffer[len - 1] == '\n')
	{
	  buffer[len - 1] = '\0';
	}
      frame_store[frame].lines[j].allocated = 1;
      frame_store[frame].lines[j].line = strdup (buffer);
      frame_store[frame].lines[j].owner = pcb;
      pcb->duration++;
    }
  fclose (f);

  // 4) Update the page table for this page
  pcb->page_table[page_index] = frame;
}

size_t
pcb_next_instruction (struct PCB *pcb)
{
  size_t page = pcb->pc / FRAME_SIZE;
  size_t offset = pcb->pc % FRAME_SIZE;

  if (pcb->page_table[page] == -1)
    {
      // page fault!
      // we typically just return a special "PAGEFAULT" code or do
      // something that your scheduler can see. We'll do it inline:
      handle_page_fault (pcb, page);
      // Return -1 to indicate that the process needs to be rescheduled
      // This will signal the scheduler to move the process to the back of the ready queue
      return (size_t) -1;
    }

  // Get the frame number from the page table
  int frame = pcb->page_table[page];

  // Check if frame is valid
  if (frame < 0 || frame >= NUM_FRAMES)
    {
      fprintf (stderr, "ERROR: Invalid frame number %d\n", frame);
      return (size_t) -1;
    }

  // Update the timestamp for this frame
  frame_store[frame].last_access_time = get_current_time ();

  // Calculate actual line index
  size_t line_index = frame * FRAME_SIZE + offset;
  pcb->pc++;
  return line_index;
}

struct PCB *
clone_pcb (struct PCB *pcb)
{
  struct PCB *new_pcb = malloc (sizeof (struct PCB));
  new_pcb->pid = fresh_pid++;
  new_pcb->name = strdup (pcb->name);
  new_pcb->next = NULL;
  new_pcb->pc = 0;
  new_pcb->line_base = pcb->line_base;
  new_pcb->line_count = pcb->line_count;
  new_pcb->duration = pcb->duration;

  // Clone the page table
  new_pcb->page_count = pcb->page_count;
  new_pcb->page_table = malloc (sizeof (int) * pcb->page_count);
  if (pcb->page_table)
    {
      memcpy (new_pcb->page_table, pcb->page_table,
	      sizeof (int) * pcb->page_count);
    }

  new_pcb->next = NULL;
  return new_pcb;
}

struct PCB *
create_process (const char *filename)
{

  // We have 2 main tasks:
  // load all the code in the script file into shellmemory, and
  // allocate+fill a PCB.

  // We don't want to allocate a PCB until we know we actually need one,
  // so let's first make sure we can open the file.
  FILE *script = fopen (filename, "rt");
  if (!script)
    {
      perror ("failed to open file for create_process");
      return NULL;
    }
  struct PCB *pcb = create_process_from_FILE (script);
  // Update the pcb name according to the filename we received.
  pcb->name = strdup (filename);
  return pcb;
}

struct PCB *
create_process_from_FILE (FILE * script)
{

  // We can open the file, so we'll be making a process.
  struct PCB *pcb = malloc (sizeof (struct PCB));


  // The PID is the only weird part. They need to be distinct,
  // so let's use a static counter.

  pcb->pid = fresh_pid++;

  // name should be the empty string, according to doc comment.
  pcb->name = "";
  // next should be NULL, according to doc comment.
  pcb->next = NULL;

  // pc is always initially 0.
  pcb->pc = 0;
  pcb->page_count = 0;
  pcb->page_table = NULL;	// will be allocated after loading pages

  // create initial values for base and count, in case we fail to read
  // any lines from the file. That way we'll end up with an empty process
  // that terminates as soon as it is scheduled -- reasonable behavior for
  // an empty script.
  // If we don't do this, and the loop below never runs, base would be
  // unitialized, which would be catastrophic.
  pcb->line_count = 0;
  pcb->line_base = 0;
  pcb->duration = 0;		// Initialize duration to 0

  // We're told to assume lines of files are limited to 100 characters.
  // That's all well and good, but for implementing # we need to read
  // actual user input, and _that_ is limited to 1000 characters.
  // It's unclear if we should assume it's also limited to 100 for this
  // purpose. If you did assume that, that's OK! We didn't.
  char linebuf[MAX_USER_INPUT];

  // First pass: count total lines to determine total pages needed
  while (fgets (linebuf, MAX_USER_INPUT, script))
    {
      pcb->line_count++;
    }

  // Calculate total pages needed (ceiling of line_count/FRAME_SIZE)
  pcb->page_count = (pcb->line_count + FRAME_SIZE - 1) / FRAME_SIZE;

  // Allocate page table with -1 for unloaded pages
  pcb->page_table = malloc (sizeof (int) * pcb->page_count);
  if (!pcb->page_table)
    {
      perror ("malloc failed for page_table");
      free_pcb (pcb);
      fclose (script);
      return NULL;
    }

  // Initialize all pages as unloaded (-1)
  for (size_t i = 0; i < pcb->page_count; i++)
    {
      pcb->page_table[i] = -1;
    }

  // Reset file pointer to beginning
  fseek (script, 0, SEEK_SET);

  // Load only the first two pages (or one if program is smaller)
  size_t pages_to_load = (pcb->page_count < 2) ? pcb->page_count : 2;
  for (size_t page = 0; page < pages_to_load; page++)
    {
      // Allocate a frame in the first available hole
      int frame = allocate_frame ();
      if (frame == -1)
	{
	  fprintf (stderr, "Out of frame store memory\n");
	  free_pcb (pcb);
	  fclose (script);
	  return NULL;
	}
      pcb->page_table[page] = frame;

      // Load lines for this page
      for (int offset = 0; offset < FRAME_SIZE; offset++)
	{
	  if (!fgets (linebuf, MAX_USER_INPUT, script))
	    {
	      break;		// End of file
	    }

	  // Remove trailing newline if present
	  size_t len = strlen (linebuf);
	  if (len > 0 && linebuf[len - 1] == '\n')
	    {
	      linebuf[len - 1] = '\0';
	    }

	  frame_store[frame].lines[offset].allocated = 1;
	  frame_store[frame].lines[offset].line = strdup (linebuf);
	  frame_store[frame].lines[offset].owner = pcb;
	  pcb->duration++;
	}

      // Initialize the timestamp for this frame
      frame_store[frame].last_access_time = get_current_time ();
    }

  // We're done with the file, don't forget to close it!
  fclose (script);

  // For backward compatibility, define line_base as the global index
  // corresponding to the first frame's first line.
  if (pcb->page_count > 0)
    {
      pcb->line_base = pcb->page_table[0] * FRAME_SIZE;
    }

  return pcb;
}

void
free_pcb (struct PCB *pcb)
{


  // Free all frames used by this PCB
  for (size_t i = 0; i < pcb->page_count; i++)
    {
      int frame = pcb->page_table[i];
      if (frame == -1)
	continue;		// Skip unloaded pages

      // Free only the lines owned by this PCB
      for (int j = 0; j < FRAME_SIZE; j++)
	{
	  if (frame_store[frame].lines[j].allocated &&
	      frame_store[frame].lines[j].owner == pcb)
	    {
	      free (frame_store[frame].lines[j].line);
	      frame_store[frame].lines[j].allocated = 0;
	      frame_store[frame].lines[j].line = NULL;
	      frame_store[frame].lines[j].owner = NULL;
	    }
	}
    }

  // Free the page table
  if (pcb->page_table)
    {
      free (pcb->page_table);
    }

  // Free the process name, but only if it's not the empty string
  if (strcmp ("", pcb->name))
    {
      free (pcb->name);
    }

  free (pcb);
}
