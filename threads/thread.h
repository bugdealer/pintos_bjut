#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
/* Nice value boundary */
#define NICE_MIN -20
#define NICE_DEFAULT 0
#define NICE_MAX 20
/* recent_cpu in the begining */
#define RECENT_CPU_BEGIN 0

/******************* PROJECT 2 *******************/
#include "threads/synch.h"  
/******************* PROJECT 2 *******************/
/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

#ifdef USERPROG
/******************* PROJECT 2 *******************/
# define RET_STATUS_DEFAULT 0xcdcdcdcd
/******************* PROJECT 2 *******************/
#endif
/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. 有效优先级 */
    
     /******************* PROJECT 1 *******************/
    int base_priority; /* old priority */
    int64_t wakeup_tick;  /* how long the thread sleep */
    int32_t nice;        
    int64_t recent_cpu;  
    struct list locks;      /* locks holden by thread */
    bool donated;             /* whether the thread's priority is donated */
    struct lock *blocked;   /* the lock causes thread to be blocked */
     /******************* PROJECT 1 *******************/
     
	/******************* PROJECT 2 *******************/
    
    struct semaphore wait;              /* semaphore for process_wait */
    int return_status;                     /* return status */
    struct list files;                  /* all opened files */
    struct file *self;                  /* the image file on the disk */
    struct thread *parent;              /* parent process */
    
	
	/******************* PROJECT 2 *******************/
	
    struct list_elem allelem;           /* List element for all threads list. */
    

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */
#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */

#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);



/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);
int thread_get_load_avg (void);
int thread_get_recent_cpu (void);

/******************* PROJECT 1 *******************/
void thread_yield_equ (void);  /*与void thread_yield (void) 稍有区别，就是等号*/
void thread_set_priority_other (struct thread *curr, int new_priority); /*改*/
void check_wakeup(void);  /* 在timer.c中，用于检测有没有线程该被唤醒 */
void thread_sleep(int64_t ticks); /* 在timer.c中，用于克服忙等待，让线程睡眠 */
bool wakeup_tick_less (const struct list_elem *a,const struct list_elem *b,void *aux UNUSED); /* 在timer.c中比较线程睡眠的时间 */
bool priority_higher_equ (const struct list_elem *a,const struct list_elem *b,void *aux UNUSED); /* 比较优先级--稍有不同，就是等号*/
bool priority_more (const struct list_elem *a, const struct list_elem *b,void *aux UNUSED);

void thread_calculate_advanced_priority (void);
void calculate_advanced_priority_for_all (void);
void calculate_advanced_priority (struct thread *, void *aux);
void thread_calculate_recent_cpu (void);
void calculate_recent_cpu_for_all (void);
void calculate_recent_cpu (struct thread *, void *aux);
void calculate_load_avg (void);
/******************* PROJECT 1 *******************/

/******************* PROJECT 2 *******************/
struct thread *get_thread_by_tid (tid_t);
/******************* PROJECT 2 *******************/


#endif /* threads/thread.h */
