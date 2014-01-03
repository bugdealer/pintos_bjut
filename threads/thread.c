#include "../threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "../devices/timer.h"
#include "../threads/flags.h"
#include "../threads/interrupt.h"
#include "../threads/intr-stubs.h"
#include "../threads/malloc.h"
#include "../threads/palloc.h"
#include "../threads/switch.h"
#include "../threads/synch.h"
#include "../threads/vaddr.h"
  /***************************/
#include "../threads/fixed-point.h"
  /***************************/
#include "../lib/kernel/list.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
Used to detect stack overflow.  See the big comment at the top
of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of processes in THREAD_READY state, that is, processes
that are ready to run but not actually running. */
static struct list ready_list;

/* List of all processes.  Processes are added to this list
when they are first scheduled and removed when they exit. */
static struct list all_list;

/**************************/
static struct list sleep_list;
/**************************/

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
{
	void *eip;                  /* Return address. */
	thread_func *function;      /* Function to call. */
	void *aux;                  /* Auxiliary data for function. */
};

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

/* If false (default), use round-robin scheduler.
If true, use multi-level feedback queue scheduler.
Controlled by kernel command-line option "-o mlfqs". */

bool thread_mlfqs;

  /***************************/
static int load_avg;
static struct list queue_list[PRI_MAX+1];
static int mlfqs_queue_size; //�༶���������н��̵ĸ���
  /***************************/

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

/* Initializes the threading system by transforming the code
that's currently running into a thread.  This can't work in
general and it is possible in this case only because loader.S
was careful to put the bottom of the stack at a page boundary.

Also initializes the run queue and the tid lock.

After calling this function, be sure to initialize the page
allocator before trying to create any threads with
thread_create().

It is not safe to call thread_current() until this function
finishes. */
void thread_init (void) 
{
	ASSERT (intr_get_level () == INTR_OFF);

	lock_init (&tid_lock);
	list_init (&ready_list);
	list_init (&all_list);
	list_init (&sleep_list);

	  /***************************/
	//�༶�������г�ʼ��
	if(thread_mlfqs)
	{
		int i;
		for(i = 0; i <= PRI_MAX; i++)
		{
			list_init(&queue_list[i]);      
		}
		mlfqs_queue_size = 0; 
	}
	  /***************************/

	/* Set up a thread structure for the running thread. */
	initial_thread = running_thread ();
	init_thread (initial_thread, "main", PRI_DEFAULT);
	initial_thread->status = THREAD_RUNNING;
	initial_thread->tid = allocate_tid ();
}

/* Starts preemptive thread scheduling by enabling interrupts.
Also creates the idle thread. */
void thread_start (void) 
{
	/* Create the idle thread. */
	struct semaphore idle_started;
	sema_init (&idle_started, 0);
	thread_create ("idle", PRI_MIN, idle, &idle_started);

	/* Start preemptive thread scheduling. */
	intr_enable ();

	/* Wait for the idle thread to initialize idle_thread. */
	sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
Thus, this function runs in an external interrupt context. */
void
	thread_tick (void) 
{
	struct thread *t = thread_current ();

	/* Update statistics. */
	if (t == idle_thread)
		idle_ticks++;
#ifdef USERPROG
	else if (t->pagedir != NULL)
		user_ticks++;
#endif
	else
		kernel_ticks++;

    /***************************/
	if(thread_mlfqs)
	{
		
		thread_current ()->recent_cpu = fp_add_integer(thread_current()->recent_cpu, 1);
		//recent_cpu ����
		 
		if(timer_ticks() % 4== 0)
		{
			thread_compute_priorities();  //�������ȼ�
		}
	
		if(timer_ticks() % 100 == 0)
		{
			thread_compute_recent_cpu();    //����recent_cpu
			thread_compute_load_average();  //����load_average
		} 
	}
	  /***************************/

	/* Enforce preemption. */
	if (++thread_ticks >= TIME_SLICE)
		intr_yield_on_return ();
}

/* Prints thread statistics. */
void
	thread_print_stats (void) 
{
	printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
		idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
PRIORITY, which executes FUNCTION passing AUX as the argument,
and adds it to the ready queue.  Returns the thread identifier
for the new thread, or TID_ERROR if creation fails.

If thread_start() has been called, then the new thread may be
scheduled before thread_create() returns.  It could even exit
before thread_create() returns.  Contrariwise, the original
thread may run for any amount of time before the new thread is
scheduled.  Use a semaphore or some other form of
synchronization if you need to ensure ordering.

The code provided sets the new thread's `priority' member to
PRIORITY, but no actual priority scheduling is implemented.
Priority scheduling is the goal of Problem 1-3. */
tid_t thread_create (const char *name, int priority,
	thread_func *function, void *aux) 
{
	struct thread *t;
	struct kernel_thread_frame *kf;
	struct switch_entry_frame *ef;
	struct switch_threads_frame *sf;
	tid_t tid;
	enum intr_level old_level;

	ASSERT (function != NULL);

	/* Allocate thread. */
	t = palloc_get_page (PAL_ZERO);
	if (t == NULL)
		return TID_ERROR;

	/* Initialize thread. */
	init_thread (t, name, priority);
	tid = t->tid = allocate_tid ();
	list_init (&t->rec_donations);
	/* Prepare thread for first run by initializing its stack.
	Do this atomically so intermediate values for the 'stack' 
	member cannot be observed. */
	old_level = intr_disable ();

	/* Stack frame for kernel_thread(). */
	kf = alloc_frame (t, sizeof *kf);
	kf->eip = NULL;
	kf->function = function;
	kf->aux = aux;

	/* Stack frame for switch_entry(). */
	ef = alloc_frame (t, sizeof *ef);
	ef->eip = (void (*) (void)) kernel_thread;

	/* Stack frame for switch_threads(). */
	sf = alloc_frame (t, sizeof *sf);
	sf->eip = switch_entry;
	sf->ebp = 0;

	intr_set_level (old_level);

	thread_unblock (t);

	return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
again until awoken by thread_unblock().

This function must be called with interrupts turned off.  It
is usually a better idea to use one of the synchronization
primitives in synch.h. */
void thread_block (void) 
{
	ASSERT (!intr_context ());
	ASSERT (intr_get_level () == INTR_OFF);

	thread_current ()->status = THREAD_BLOCKED;
	schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
This is an error if T is not blocked.  (Use thread_yield() to
make the running thread ready.)

This function does not preempt the running thread.  This can
be important: if the caller had disabled interrupts itself,
it may expect that it can atomically unblock a thread and
update other data. */
void thread_unblock (struct thread *t) 
{
	enum intr_level old_level;

	ASSERT (is_thread (t));
	ASSERT (t->status == THREAD_BLOCKED);

	old_level = intr_disable ();

   /***************************/
	if(thread_mlfqs)
		thread_add_to_queue(t); //����༶��������
	else
		list_insert_ordered(&ready_list, &t->elem, thread_priority_func, NULL); //����read_list

	t->status = THREAD_READY;

	 //��ռ����
	if(thread_current() != idle_thread)
	{
		int cur_priority = thread_get_priority();
		int this_priority = thread_get_priority_for_thread(t);
		if(cur_priority < this_priority)
			thread_yield(); 
	}  
    /***************************/

	intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
	thread_name (void) 
{
	return thread_current ()->name;
}

/* Returns the running thread.
This is running_thread() plus a couple of sanity checks.
See the big comment at the top of thread.h for details. */
struct thread *
	thread_current (void) 
{
	struct thread *t = running_thread ();

	/* Make sure T is really a thread.
	If either of these assertions fire, then your thread may
	have overflowed its stack.  Each thread has less than 4 kB
	of stack, so a few big automatic arrays or moderate
	recursion can cause stack overflow. */
	ASSERT (is_thread (t));
	ASSERT (t->status == THREAD_RUNNING);

	return t;
}

/* Returns the running thread's tid. */
tid_t
	thread_tid (void) 
{
	return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
returns to the caller. */
void
	thread_exit (void) 
{
	ASSERT (!intr_context ());

#ifdef USERPROG
	process_exit ();
#endif

	/* Remove thread from all threads list, set our status to dying,
	and schedule another process.  That process will destroy us
	when it call schedule_tail(). */
	intr_disable ();
	list_remove (&thread_current()->allelem);
	thread_current ()->status = THREAD_DYING;
	schedule ();
	NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
may be scheduled again immediately at the scheduler's whim. */
void
	thread_yield (void) 
{
	struct thread *cur = thread_current ();
	ASSERT (!intr_context ());
	enum intr_level old_level;

	old_level = intr_disable ();

	    /***************************/
	if (cur != idle_thread) 
	{
		if(thread_mlfqs)
			thread_add_to_queue(cur);
		else 
			list_insert_ordered(&ready_list, &cur->elem, thread_priority_func, NULL);
	}
	    /***************************/
	cur->status = THREAD_READY;
	schedule ();
	intr_set_level (old_level);
}

/* Invoke function 'func' on all threads, passing along 'aux'.
This function must be called with interrupts off. */
void
	thread_foreach (thread_action_func *func, void *aux)
{
	struct list_elem *e;

	ASSERT (intr_get_level () == INTR_OFF);

	for (e = list_begin (&all_list); e != list_end (&all_list);
		e = list_next (e))
	{
		struct thread *t = list_entry (e, struct thread, allelem);
		func (t, aux);
	}
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
	thread_set_priority (int new_priority) 
{
	struct thread *curr = thread_current();
	curr->priority = new_priority;
	 
	  /***************************/ 
	if(!list_empty(&ready_list))  //�������ȼ��󣬶Ա��Ƿ�ready_list��һ���������ȼ����ڵ�ǰ���ȼ�
	{

		struct thread *t=list_entry (list_begin (&ready_list), struct thread, elem);
		if ( thread_get_priority_for_thread(t)> thread_get_priority_for_thread(curr))
			thread_yield();
	}
	    /***************************/
}

/* Returns the current thread's priority. */
int
	thread_get_priority (void) 
{
	   /***************************/
	return thread_get_priority_for_thread(thread_current());
	  /***************************/

}

/* Sets the current thread's nice value to NICE. */
void
	thread_set_nice (int nice UNUSED) 
{
	  /***************************/
	thread_current ()->nice = nice;
	thread_compute_priority_for_thread(thread_current(), NULL);
	  /***************************/
}

/* Returns the current thread's nice value. */
int
	thread_get_nice (void) 
{
	/* Not yet implemented. */
	  /***************************/
	return thread_current ()->nice;
	  /***************************/
}

/* Returns 100 times the system load average. */
int
	thread_get_load_avg (void) 
{
	/* Not yet implemented. */
	  /***************************/
	return fp_fixed_to_integer_zero(fp_multiply_integer(load_avg, 100));
	  /***************************/
}

/* Returns 100 times the current thread's recent_cpu value. */
int
	thread_get_recent_cpu (void) 
{
	/* Not yet implemented. */
	  /***************************/
	return fp_fixed_to_integer_zero(fp_multiply_integer(thread_current ()->recent_cpu, 100));
	  /***************************/
}

/* Idle thread.  Executes when no other thread is ready to run.

The idle thread is initially put on the ready list by
thread_start().  It will be scheduled once initially, at which
point it initializes idle_thread, "up"s the semaphore passed
to it to enable thread_start() to continue, and immediately
blocks.  After that, the idle thread never appears in the
ready list.  It is returned by next_thread_to_run() as a
special case when the ready list is empty. */
static void
	idle (void *idle_started_ UNUSED) 
{
	struct semaphore *idle_started = idle_started_;
	idle_thread = thread_current ();
	sema_up (idle_started);

	for (;;) 
	{
		/* Let someone else run. */
		intr_disable ();
		thread_block ();

		/* Re-enable interrupts and wait for the next one.

		The `sti' instruction disables interrupts until the
		completion of the next instruction, so these two
		instructions are executed atomically.  This atomicity is
		important; otherwise, an interrupt could be handled
		between re-enabling interrupts and waiting for the next
		one to occur, wasting as much as one clock tick worth of
		time.

		See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
		7.11.1 "HLT Instruction". */
		asm volatile ("sti; hlt" : : : "memory");
	}
}

/* Function used as the basis for a kernel thread. */
static void
	kernel_thread (thread_func *function, void *aux) 
{
	ASSERT (function != NULL);

	intr_enable ();       /* The scheduler runs with interrupts off. */
	function (aux);       /* Execute the thread function. */
	thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
	running_thread (void) 
{
	uint32_t *esp;

	/* Copy the CPU's stack pointer into `esp', and then round that
	down to the start of a page.  Because `struct thread' is
	always at the beginning of a page and the stack pointer is
	somewhere in the middle, this locates the curent thread. */
	asm ("mov %%esp, %0" : "=g" (esp));
	return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
	is_thread (struct thread *t)
{
	return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
NAME. */
static void
	init_thread (struct thread *t, const char *name, int priority)
{
	ASSERT (t != NULL);
	ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
	ASSERT (name != NULL);

	memset (t, 0, sizeof *t);
	t->status = THREAD_BLOCKED;
	strlcpy (t->name, name, sizeof t->name);
	t->stack = (uint8_t *) t + PGSIZE;
	t->priority = priority;
	t->magic = THREAD_MAGIC;
	list_push_back (&all_list, &t->allelem);


	list_init(&t->rec_donations);
	t->blocked_for_lock=NULL;

	t->nice = 0;
	t->recent_cpu = 0;
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
returns a pointer to the frame's base. */
static void *
	alloc_frame (struct thread *t, size_t size) 
{
	/* Stack data is always allocated in word-size units. */
	ASSERT (is_thread (t));
	ASSERT (size % sizeof (uint32_t) == 0);

	t->stack -= size;
	return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
return a thread from the run queue, unless the run queue is
empty.  (If the running thread can continue running, then it
will be in the run queue.)  If the run queue is empty, return
idle_thread. */
static struct thread *
	next_thread_to_run (void) 
{
	  /***************************/
	if(thread_mlfqs)
	{
		if(mlfqs_queue_size == 0)
		{
			return idle_thread;
		}else{
			return thread_pop_max_priority_list();  //�������Ⱥ��ó��༶�������������ȼ���ߵ�
		}
	}
	 
	else
	{
		if (list_empty (&ready_list))
			return idle_thread;
		else
			return list_entry (list_pop_front (&ready_list), struct thread, elem);  //���ȷ������ó���ͷ���ȼ���ߵ�
	}
	 /***************************/
}

/* Completes a thread switch by activating the new thread's page
tables, and, if the previous thread is dying, destroying it.

At this function's invocation, we just switched from thread
PREV, the new thread is already running, and interrupts are
still disabled.  This function is normally invoked by
thread_schedule() as its final action before returning, but
the first time a thread is scheduled it is called by
switch_entry() (see switch.S).

It's not safe to call printf() until the thread switch is
complete.  In practice that means that printf()s should be
added at the end of the function.

After this function and its caller returns, the thread switch
is complete. */
void
	schedule_tail (struct thread *prev) 
{
	struct thread *cur = running_thread ();

	ASSERT (intr_get_level () == INTR_OFF);

	/* Mark us as running. */
	cur->status = THREAD_RUNNING;

	/* Start new time slice. */
	thread_ticks = 0;

#ifdef USERPROG
	/* Activate the new address space. */
	process_activate ();
#endif

	/* If the thread we switched from is dying, destroy its struct
	thread.  This must happen late so that thread_exit() doesn't
	pull out the rug under itself.  (We don't free
	initial_thread because its memory was not obtained via
	palloc().) */
	if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
	{
		ASSERT (prev != cur);
		palloc_free_page (prev);
	}
}

/* Schedules a new process.  At entry, interrupts must be off and
the running process's state must have been changed from
running to some other state.  This function finds another
thread to run and switches to it.

It's not safe to call printf() until schedule_tail() has
completed. */
static void
	schedule (void) 
{
	struct thread *cur = running_thread ();
	struct thread *next = next_thread_to_run ();
	struct thread *prev = NULL;

	ASSERT (intr_get_level () == INTR_OFF);
	ASSERT (cur->status != THREAD_RUNNING);
	ASSERT (is_thread (next));

	if (cur != next)
		prev = switch_threads (cur, next);
	schedule_tail (prev); 
}

/* Returns a tid to use for a new thread. */
static tid_t
	allocate_tid (void) 
{
	static tid_t next_tid = 1;
	tid_t tid;

	lock_acquire (&tid_lock);
	tid = next_tid++;
	lock_release (&tid_lock);

	return tid;
}

 /***************************/

int thread_get_priority_for_thread(struct thread* t)
{
	int priority = t->priority;    //��ȡ�̵߳�ǰ���ȼ�
	if (!list_empty(&t->rec_donations)) { //�鿴���߳̾��������Ƿ�Ϊ�գ�������գ������ѡ��һ����ԭ���ȼ������Ϊ���أ���û�з���ԭ���ȼ�
		struct donation_elem *d_elem =list_entry(list_front(&t->rec_donations), struct donation_elem, elem);
		if (d_elem->priority > priority)
			priority = d_elem->priority; 
	}
	return priority;
}

bool thread_priority_func(const struct list_elem *a, const struct list_elem *b,
						  void *aux UNUSED)
{
	struct thread *t1 = list_entry(a, struct thread, elem);
	struct thread *t2 = list_entry(b, struct thread, elem);

	return thread_get_priority_for_thread(t1)>thread_get_priority_for_thread(t2);  
}

bool wakeup_tick_late (const struct list_elem *a,const struct list_elem *b,void *aux UNUSED)
{
	struct thread *at,*bt;
	at = list_entry (a, struct thread, elem);
	bt = list_entry (b, struct thread,elem);

	if(at->sleep_to_ticks!= bt->sleep_to_ticks)
		return(at->sleep_to_ticks < bt->sleep_to_ticks);
	else if(at->sleep_to_ticks==bt->sleep_to_ticks)
		return thread_get_priority_for_thread(at)>thread_get_priority_for_thread(bt);
}

void sleep_wakeup()    //���̴߳�˯���л���
{
	
	struct thread *t;
	struct list_elem *e;
	if(!list_empty(&sleep_list))
	{
		e = list_begin (&sleep_list);
		t=list_entry (e, struct thread, elem);
	
		while(timer_ticks()>=t->sleep_to_ticks )
		{   

			t=list_entry (list_pop_front (&sleep_list), struct thread, elem);
			ASSERT (t->status == THREAD_BLOCKED);
			if(thread_mlfqs)
				thread_add_to_queue(t); 
			else
				list_insert_ordered(&ready_list,&t->elem,thread_priority_func,NULL);

			t->status = THREAD_READY;

			if( !list_empty(&sleep_list))
				t=list_entry (list_begin (&sleep_list), struct thread, elem);
			else
				break;	
		}
	
	}
}


void thread_sleep(int64_t ticks)  //�߳�˯��
{
	struct thread * cur=thread_current();
	ASSERT(!intr_context());
	ASSERT(is_thread(cur));
	enum intr_level old_level;

	old_level = intr_disable ();
	cur->sleep_to_ticks=timer_ticks()+ticks; 
	list_insert_ordered(&sleep_list,&cur->elem,wakeup_tick_late,NULL);
	cur->status = THREAD_BLOCKED;
	schedule ();
	intr_set_level (old_level);
}


void
	thread_reinsert_list(struct thread *t, struct list *list)  //һ���̵߳����ȼ������仯�������Ǳ������󣩣����²���ready_list
{
	struct list_elem *elem = &t->elem;
	list_remove(elem);
	list_insert_ordered(list, elem, thread_priority_func, NULL);
}

bool                                                        //�ȽϾ������������ȼ��Ĵ�С
	thread_donation_priority_less_func (const struct list_elem *a, 
	const struct list_elem *b, void *aux UNUSED)
{
	struct donation_elem* t1 = list_entry(a, struct donation_elem, elem);
	struct donation_elem* t2 = list_entry(b, struct donation_elem, elem);

	return t1->priority > t2->priority;
}

void add_thread_donation(struct lock * t,struct thread* cur)  //��Ӿ���
{
	bool found=0;
	struct donation_elem *le;
	struct list_elem *a0;
	struct lock * lock=t;
	struct thread* th=cur;
	int priority,old_pri;

	while(lock!=NULL)
	{
		priority=thread_get_priority_for_thread(th);
		old_pri=thread_get_priority_for_thread(lock->holder);
		//�鿴һ���̱߳������б����Ƿ����������������Ԫ��
		for (a0 = list_begin (&lock->holder->rec_donations); a0 != list_end (&lock->holder->rec_donations);)
		{
			le=list_entry(a0,struct donation_elem,elem);
			if(le->lock==lock)
			{      
				found=1;
				break;
			}
			a0=list_next(a0);
		}

		if(found) //�ҵ���Ϊ��ǰ��������Ԫ�أ�����µ����ȼ��ظ���ԭ�е����ȼ����޸���ԭ�е����ȼ�
		{
			if(le->priority < priority)
			{   
				list_remove(&le->elem);
				struct donation_elem *cur_elem= (struct donation_elem*) malloc(
					sizeof(struct donation_elem));

				cur_elem->lock=lock;		
				cur_elem->priority=priority;
				list_insert_ordered (&lock->holder->rec_donations, &cur_elem->elem, thread_donation_priority_less_func, NULL);
			}
		}
		else
		{   //���û�ҵ�������µľ���Ԫ��
			struct donation_elem *cur_elem= (struct donation_elem*) malloc(sizeof(struct donation_elem));
			cur_elem->lock=lock;		
			cur_elem->priority=priority;
			list_insert_ordered (&lock->holder->rec_donations, &cur_elem->elem, thread_donation_priority_less_func, NULL);

		}
		barrier();
		th=lock->holder;
		lock=th->blocked_for_lock;
	}
	//���ȼ����ܻ�ı䣬���²���ready_list
	if (priority > old_pri && th->status == THREAD_READY) 
		thread_reinsert_list(th, &ready_list);

}



//�Ƴ�һ���߳���Ϊ���ľ���Ԫ��
void rm_thread_donation( struct lock* lock,struct thread* t)
{
	struct list_elem *e;
	struct thread* cur=t;

	if(!list_empty(&cur->rec_donations))
	{
		for(e = list_begin(&cur->rec_donations); e != list_end(&cur->rec_donations);)
		{
			struct donation_elem *cur_elem = list_entry(e, struct donation_elem,elem);
			if (cur_elem->lock == lock) {
				list_remove(e);
				break;
			}
			e = list_next(e);
		}

	}
	  list_sort(&sleep_list,wakeup_tick_late,NULL);
}

//����һ���̵߳�recent_cpu
void 
	thread_compute_recent_cpu_for_thread(struct thread* t, void *aux)
{
	int multiplier = *(int*)aux;
	int left = fp_multiply(multiplier, t->recent_cpu);
	int result = fp_add_integer(left, t->nice);
	t->recent_cpu = result;
}

//���������̵߳�recent_cpu
void thread_compute_recent_cpu(void)
{
	int top = fp_multiply_integer(load_avg, 2);
	int bot = fp_add_integer(fp_multiply_integer(load_avg, 2), 1);
	int multiplier = fp_divide(top, bot);
	thread_foreach(thread_compute_recent_cpu_for_thread, &multiplier);
}	

//���������̵߳����ȼ�
void 
	thread_compute_priorities(void)
{ 
	ASSERT (intr_get_level () == INTR_OFF);
	struct list_elem *a0,*b0;
	struct thread *le;
	int i;
	enum intr_level old_level;
	old_level = intr_disable ();

	thread_foreach(thread_compute_priority_for_thread, NULL);
	for(i=PRI_MIN;i<=PRI_MAX;i++)   
	{ 
		if(!list_empty(&queue_list[i])) 	
		{  
			for (a0=list_begin (&queue_list[i]); a0 != list_end (&queue_list[i]);a0=b0) 	
			{
				le=list_entry(a0, struct thread, elem); 
				if(le->priority!=i && le->status==THREAD_READY)
				{
					b0=list_next(a0); 
					list_remove(a0);
					thread_add_to_queue(le);
					mlfqs_queue_size--;
				}
				else b0=list_next(a0);                		  
			}  	  
		}  
	}
	intr_set_level (old_level);
}

//����һ���̵߳�load_average
void 
	thread_compute_load_average(void)
{
	int left = fp_multiply(LOAD_AVG_MULTIPLIER, load_avg);
	int running_thread = 1;
	if( thread_current () == idle_thread)
		running_thread = 0;
	int right = fp_multiply_integer(LOAD_AVG_READY_MULTIPLIER, 
		mlfqs_queue_size + running_thread);
	load_avg = fp_add(left, right);
}

//��һ���̼߳���༶��������
static void
	thread_add_to_queue(struct thread* t)
{
	struct list* cur_list = &queue_list[t->priority];
	list_push_back(cur_list, &t->elem);
	mlfqs_queue_size++; 
}

//һ�������ȼ��̼���
void
	thread_compute_priority_for_thread(struct thread* t, void *aux UNUSED)
{
	int cpu_part = fp_fixed_to_integer_zero(fp_divide_integer(t->recent_cpu, 4));
	t->priority = PRI_MAX - cpu_part - (t->nice * 2);

	if(t->priority < PRI_MIN)
	{
		t->priority = PRI_MIN; 
	}

	if(t->priority > PRI_MAX)
	{
		t->priority = PRI_MAX;
	}
}

//�����ȼ���߶����е�һ���߳��Ƴ�
static struct thread *
	thread_pop_max_priority_list(void)
{
	ASSERT(mlfqs_queue_size > 0);
	int cur_priority;
	for(cur_priority = PRI_MAX; cur_priority >= 0; cur_priority--)
	{
		struct list* cur_list = &queue_list[cur_priority];
		if(!list_empty(cur_list))
		{
			mlfqs_queue_size--;
			return list_entry(list_pop_front(cur_list), struct thread, elem);
		}
	}

}

 /***************************/


/* Offset of `stack' member within `struct thread'.
Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
