#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

#include "../lib/kernel/float_point.h"

#include "devices/timer.h"

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

// multi level queue 
static struct list ready_multi[PRI_MAX + 1];

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

static struct list block_list;

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

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
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
void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  init_ready_lists();
  list_init (&all_list);
  list_init(&block_list);

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void) 
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
    
    if(thread_mlfqs){
      int64_t abc = timer_ticks();
      enum intr_level old_level = intr_disable();
      add_cpu();
      if(timer_ticks() % TIMER_FREQ == 0){
        avg_cal();
        thread_foreach(cpu_calc, NULL);
        // colocar o update priorities aqui ele nao trava, mas também nao passa
      }
      if(timer_ticks() % 4 == 0){ // tempo de 50 ele erra // block passando e os nice nao pegando
        update_priorities();
      }
      //printf("Chamado por %s Demorou %d ---- %d\n", t->name, timer_ticks(), abc);
      intr_set_level(old_level);
    }
  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;

#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;
   
  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE) {
    
    intr_yield_on_return ();
  }
  
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
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

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

  /* Add to run queue. */
  thread_unblock (t);

  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void) 
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
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
        //printf("status thread -> %d\n", t->status);
  ASSERT (t->status == THREAD_BLOCKED);
  add_ready (&t->elem);
  t->status = THREAD_READY;
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
  ASSERT (t->status == THREAD_RUNNING); // retirei esse assert pois ele da erro por algum motivo

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
     when it calls thread_schedule_tail(). */
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
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    add_ready(&cur->elem);
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


void
thread_foreach_n_list (struct list *n_list, thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (n_list); e != list_end (n_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, elem);
      func (t, aux);
    }
}

void
thread_set_priority (int new_priority) 
{
  if (!thread_mlfqs) {
    thread_current ()->priority = new_priority;
  }
}

/* Returns the current thread's priority. */
int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED) 
{
  struct thread *t = thread_current();
  enum intr_level old_level;
  
  t->nice = nice;
  update_priority(t);
    if(thread_mlfqs && t != idle_thread){
        // old_level = intr_disable();
        if(t->priority < max_current_pri()){
                thread_yield();    
        }
        // intr_set_level(old_level);
    }
  // recalcula a prioridade
  // int8_t actual_priority = t->nice;
  // update_priorities();

  // if (t->nice != actual_priority) {
  //   old_level = intr_disable ();
  //   if (t != idle_thread) 
  //       add_ready(&t->elem);
  //   t->status = THREAD_READY;
  //   schedule ();
  //   intr_set_level (old_level);
  // }

}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  return thread_current()->nice;
}

float_type a = FLOAT_DIV(INT_FLOAT(59), INT_FLOAT(60));
float_type b = FLOAT_DIV(INT_FLOAT(1), INT_FLOAT(60));

void avg_cal(){  
  // int c = (ready_list_size() + (thread_current() != idle_thread);
  int c = list_size(&all_list) - list_size(&block_list) - 1;
  avg = FLOAT_MUL(a, avg) + FLOAT_MUL(b, INT_FLOAT(c));
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{ 
  return  FLOAT_INT_ZERO(avg * 100);
}

// vai ser colocado no for para ele recalcular, tem que ta com a interrupção desabilitada
void cpu_calc(struct thread *t, void *aux){
  // recent_cpu = ((2*avg)/(2*avg + 1)) * recent_cpu  + nice
  if(t != idle_thread){
        float_type m1 = FLOAT_INT_ADD(2 * avg, 1);
        float_type m2 = FLOAT_DIV(2 * avg, m1);
    //t->recent_cpu_time = FLOAT_MUL(t->recent_cpu_time, FLOAT_DIV(2*avg, FLOAT_INT_ADD(2*avg, 1))) + INT_FLOAT(t->nice);
    t->recent_cpu_time = FLOAT_MUL(t->recent_cpu_time, m2) + t->nice;

  }
}

void add_cpu(){// a cada tick ele aumenta o cpu time
  struct thread *t = thread_current();
  if(t != idle_thread){
          t->recent_cpu_time = FLOAT_INT_ADD(t->recent_cpu_time, 1);
  }
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
  // enum intr_level old_level = intr_disable();
  int c = FLOAT_INT_ROUND(thread_current()->recent_cpu_time * 100);
  // intr_set_level(old_level);
  
  return c;
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
    enum intr_level old_level;

    ASSERT (t != NULL);
    ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
    ASSERT (name != NULL);
    // struct thread* y = thread_current();
    // struct thread* y = thread_current();
    memset (t, 0, sizeof *t);
    t->status = THREAD_BLOCKED;
    strlcpy (t->name, name, sizeof t->name);
    t->stack = (uint8_t *) t + PGSIZE;
    // if (!thread_mlfqs) {
    //   t->priority = priority;
    // }
    // else {
    //   t->priority = PRI_MAX;
    // }
    t->sleep_time = 0;
    t->magic = THREAD_MAGIC;
    // t->recent_cpu_time = 0; // se nao iniciar com zero nao passa no nice2
    // t->nice = 0;
    if(t != initial_thread){
        struct thread* y = thread_current();
        t->recent_cpu_time = y->recent_cpu_time;
        t->nice = y->nice;
        update_priority(t);
    } else {
        t->recent_cpu_time = 0;
        t->nice = 0;
        t->priority = priority;
    }
  

  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);
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
  if(!list_empty(&block_list)){
    wake(timer_ticks());
  }

  if (ready_empty())
    return idle_thread;
  else
    return list_entry (pop_next_ready(), struct thread, elem);
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
thread_schedule_tail (struct thread *prev)
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

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  /* TODO:
   * Ver de usar o thread_block, mas para o schedule 
   * tem de verificar se uma thread esta bloqueada, alem de implementar 
   * o unblock com o tempo
   * */
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
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

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);


/* Compares the value of two list elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool ord (const struct list_elem *a, const struct list_elem *b, void *aux){
  struct thread *A = list_entry(a, struct thread, elem), *B = list_entry(b, struct thread, elem);

  return A->sleep_time < B->sleep_time;

}

void thread_yield_block(int sleep_time){
    struct thread *t = thread_current();
    enum intr_level old_level;

    ASSERT (!intr_context ());

    if(t != idle_thread){
      t->sleep_time = sleep_time;
      old_level  = intr_disable();
      
      list_insert_ordered(&block_list, &(t->elem), ord, NULL);
      thread_block();
      intr_set_level(old_level);
    }
}

void wake(int64_t ticks){
  
  enum intr_level old_level;
  struct list_elem *e = list_begin(&block_list);
  while(e != list_end(&block_list)){
    struct thread *t = list_entry(e, struct thread, elem);

    if(t->sleep_time <= ticks){
      old_level = intr_disable();
      list_pop_front(&block_list);
      //list_push_back(&ready_list, &(t->elem)); // usar o block e o unblock ele buga no assert do thread_current
      thread_unblock(t);
      intr_set_level(old_level);

      // unblock
      if(!list_empty(&block_list)){
        e = list_front(&block_list);
      } else {
        break;
      }
    } else {
      break;
    }
  }
}

void init_ready_lists(void){
  switch (thread_mlfqs)
  {
  case true:
    for(int i = 0; i < PRI_MAX + 1; i++){
        list_init (&ready_multi[i]);
    }
    break;
  case false:
    list_init (&ready_list);
    break;
  default:
    break;
  }
}

size_t ready_list_size() {
  // switch (thread_mlfqs)
  // {
  // case true:
  //   size_t total = 0;
  //   for (int i = 0; i <= PRI_MAX; i++) {
  //       total += list_size(&ready_multi[i]);
  //   }
  //   return total;
  // case false:
  //   return list_size(&ready_list);    
  // default:
  //   break;
  // }
  return list_size(&all_list) - list_size(&block_list) - 1;
}

// round_robin
void rr_add_ready(struct list_elem* elem) {
  list_push_back(&ready_list, elem);
}

bool rr_ready_empty(void) {
  return list_empty(&ready_list);
}

struct list_elem *rr_pop_next_ready(void) {
  return list_pop_front(&ready_list);
}

// multi level feedback queue
void ml_add_ready(struct list_elem* elem) {
  struct thread *t = list_entry(elem, struct thread, elem);
  list_push_back(&ready_multi[(int) t->priority], elem);
}

bool ml_ready_empty(void) {
  for(int i = 0; i <= PRI_MAX; i++){
    if(!list_empty(&ready_multi[i])){
        return false;
    }
  }

  return true;
}

struct list_elem *ml_pop_next_ready(void) {
  for(int i = PRI_MAX; i >= 0; i--){
    if(!list_empty(&(ready_multi[i]))){
        return list_pop_front(&(ready_multi[i]));
    }
  }

  return NULL;
}

void print_mlfqs(void) {
  printf("MLFQS----\n");
  
  for (int i = PRI_MAX; i >= 0; i--) {
    printf("Priority %d, size %d\n", i, list_size(&ready_multi[i]));
    struct list_elem *e;
    bool p_newline = false;
    for (e = list_begin(&ready_multi[i]); e != list_end(&ready_multi[i]); e = list_next(e)) {
      struct thread *t = list_entry(e, struct thread, elem);
      printf("Thread %s | ", t->name);
      p_newline = true;
    }
    if (p_newline)
      printf("\n");
  }
}

void update_priority(struct thread *t) {
  t->priority = (PRI_MAX) - (( FLOAT_INT_ROUND(t->recent_cpu_time)) / ( 4)) 
                                    - ((2) * (t->nice));
  
  if (t->priority < PRI_MIN)
    t->priority = PRI_MIN;
  
  if (t->priority > PRI_MAX)
    t->priority = PRI_MAX;

}

void up_for(struct thread* t, void* aux){
  if( t->status != THREAD_BLOCKED && t != idle_thread && t != thread_current()){
    int i = t->priority;
    update_priority(t);
    if(t->priority != i){
      list_remove(&t->elem);
      //add_ready(&t->elem);
      list_push_back(&ready_multi[t->priority], &t->elem);
    }
  }
}

void update_priorities(void) {
  enum intr_level old_level = intr_disable();

  // for current thread
  if (thread_current() != idle_thread) {
    update_priority(thread_current());
  }
  thread_foreach(up_for, NULL);
  intr_set_level(old_level);
}

// for handling multiple schedules
void add_ready(struct list_elem* elem) {
  switch (thread_mlfqs)
  {
  case true:
    return ml_add_ready(elem);
  case false:
    return rr_add_ready(elem);
  default:
    break;
  }
}

bool ready_empty(void) {
  switch (thread_mlfqs)
  {
  case true:
    return ml_ready_empty();
  case false:
    return rr_ready_empty();
  default:
    break;
  }
}

struct list_elem *pop_next_ready(void) {
  switch (thread_mlfqs)
  {
  case true:
    return ml_pop_next_ready();
  case false:
    return rr_pop_next_ready();
  default:
    return NULL;
  }
}

int max_current_pri(){
    for(int i = PRI_MAX; i >= 0; i--){
        if(!list_empty(&ready_multi[i])){
            return i;
        }
    }
    return 0;
}
