/* queue.c */
#include "queue.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


struct queue *
make_queue ()
{
  struct queue *q = malloc (sizeof (struct queue));
  if (!q)
    {
      fprintf (stderr, "Failed to allocate queue.\n");
      return NULL;
    }

  // Initialize pointers
  q->head = NULL;
  q->tail = NULL;

  // Initialize the mutex
  if (pthread_mutex_init (&q->lock, NULL) != 0)
    {
      fprintf (stderr, "Failed to initialize mutex.\n");
      free (q);
      return NULL;
    }

  // Initialize the semaphore 
  if (sem_init (&q->items, 0, 0) != 0)
    {
      fprintf (stderr, "Failed to initialize semaphore.\n");
      //destroy lock just to be safe since we already init lock
      pthread_mutex_destroy (&q->lock);
      free (q);
      return NULL;
    }

  return q;
}

void
enqueue (struct queue *q, void *item)
{
  if (!q)
    return;
  // Make a new node
  queue_node_t *new_node = malloc (sizeof (queue_node_t));
  if (!new_node)
    {
      fprintf (stderr, "Failed to init node.\n");
      return;
    }
  new_node->item = item;
  new_node->next = NULL;
  new_node->prev = NULL;

  // critical session
  pthread_mutex_lock (&q->lock);

  if (q->tail == NULL)
    {
      // handle empty queue
      q->head = new_node;
      q->tail = new_node;
    }
  else
    {
      // append
      q->tail->next = new_node;
      new_node->prev = q->tail;
      q->tail = new_node;
    }

  // Unlock the queue after enqueue
  pthread_mutex_unlock (&q->lock);

  // Signal that a new item is available
  sem_post (&q->items);
}

void *
dequeue (struct queue *q)
{
  if (!q)
    return NULL;

  // Wait for an item to become available
  sem_wait (&q->items);

  // Lock the queue before removing the node at the head
  pthread_mutex_lock (&q->lock);

  queue_node_t *front = q->head;
  if (!front)
    {
      pthread_mutex_unlock (&q->lock);
      return NULL;
    }

  // Remove the front node
  void *item = front->item;

  // Advance the head pointer
  q->head = front->next;
  if (q->head == NULL)
    {
      // If that was the last node, tail becomes NULL as well
      q->tail = NULL;
    }
  else
    {
      // Otherwise, fix up the new head's prev pointer
      q->head->prev = NULL;
    }

  free (front);

  // Unlock the queue
  pthread_mutex_unlock (&q->lock);

  return item;
}

void
destroy_queue (struct queue *q)
{
  if (!q)
    return;

  // Destroy the mutex and semaphore
  pthread_mutex_destroy (&q->lock);
  sem_destroy (&q->items);

  // Free any nodes still in the queue
  while (q->head != NULL)
    {
      queue_node_t *tmp = q->head;
      q->head = q->head->next;
      free (tmp);
    }

  free (q);
}
