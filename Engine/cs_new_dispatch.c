/*
** CSound-parallel-dispatch.c
** 
** Martin Brain
** mjb@cs.bath.ac.uk
** 04/08/12
**
** Realisation in code John ffitch Jan 2013
**
** Fast system for managing task dependencies and dispatching to threads.
** 
** Has a DAG of tasks and has to assign them to worker threads while respecting
** dependency order.
**
** OPT marks code relevant to particular optimisations (listed below the code).
** INV marks invariants
** NOTE marks notes
*/

#ifdef NEW_DAG

#include <stdio.h>
#include <stdlib.h>
#include "csoundCore.h"
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"

/* Used as an error value */
typedef int taskID;
#define INVALID -1

/* Each task has a status */
enum state { INACTIVE = 4,         /* No task */
             WAITING = 3,          /* Dependencies have not been finished */
	     AVAILABLE = 2,        /* Dependencies met, ready to be run */
	     INPROGRESS = 1,       /* Has been started */
	     DONE = 0 };           /* Has been completed */

/* Array of states of each task */
static enum state *task_status = NULL;          /* OPT : Structure lay out */
static taskID *task_watch = NULL; 

/* INV : Read by multiple threads, updated by only one */
/* Thus use atomic read and write */

/* Sets of prerequiste tasks for each task */
typedef struct _watchList {
  taskID id;
  struct _watchList *next;
} watchList;

static watchList ** task_dep;                        /* OPT : Structure lay out */

#define INIT_SIZE (100)
static int task_max_size;
static int dag_dispatched;

/* For now allocate a fixed maximum number of tasks; FIXME */
void create_dag(CSOUND *csound)
{
    /* Allocate the main task status and watchlists */
    task_status = mcalloc(csound, sizeof(enum state)*(task_max_size=INIT_SIZE));
    task_watch = mcalloc(csound, sizeof(taskID)*task_max_size);
    task_dep = (watchList **)mcalloc(csound, sizeof(watchList*)*task_max_size);
}

static void dag_free_watch(CSOUND *csound, watchList *x)
{
    while (x) {
      watchList *t=x;
      x = x->next;
      mfree(csound, t);
    }
}

static INSTR_SEMANTICS *dag_get_info(CSOUND* csound, int insno)
{
    INSTR_SEMANTICS *current_instr =
      csp_orc_sa_instr_get_by_num(csound, insno);
    if (current_instr == NULL) {
      current_instr =
        csp_orc_sa_instr_get_by_name(csound,
                                     csound->engineState.instrtxtp[insno]->insname);
      if (current_instr == NULL)
        csound->Die(csound,
                    Str("Failed to find semantic information"
                        " for instrument '%i'"),
                    insno);
    }
    return current_instr;
}

static int dag_intersect(CSOUND *csound, struct set_t *current, struct set_t *later)
{
    struct set_t *ans;
    int res = 0;
    struct set_element_t *ele;
    csp_set_intersection(csound, current, later, &ans);
    res = ans->count;
    ele = ans->head;
    while (ele != NULL) {
      struct set_element_t *next = ele->next;
      csound->Free(csound, ele);
      ele = next; res++;
    }
    csound->Free(csound, ans);
    return res;
}

void dag_build(CSOUND *csound, INSDS *chain)
{
    INSDS *save = chain;
    int i;
    if (task_status == NULL) create_dag(csound); /* Should move elsewhere */
    else { 
      memset(task_watch, '\0', sizeof(enum state)*(task_max_size=INIT_SIZE));
      for (i=0; i<task_max_size; i++) {
        dag_free_watch(csound, task_dep[i]);
        task_dep[i] = NULL;
      }
    }
    csound->dag_num_active = 0;
    while (chain != NULL) {
      INSTR_SEMANTICS *current_instr = dag_get_info(csound, chain->insno);
      csound->dag_num_active++;
      printf("insno %d: %p/%p/%p %d/%d/%d\n",
             chain->insno, current_instr->read, current_instr->write,
             current_instr->read_write, current_instr->read->count,
             current_instr->write->count, current_instr->read_write->count);
      //csp_dag_add(csound, dag, current_instr, chain);
      //dag->weight += current_instr->weight;
      chain = chain->nxtact;
    }
    if (csound->dag_num_active>task_max_size) {
      printf("**************need to extend task vector\n");
      exit(1);
    }
    csound->dag_changed = 0;
    printf("dag_num_active = %d\n", csound->dag_num_active);
    i = 0; chain = save;
    while (chain != NULL) {     /* for each instance check against later */
      int j = i+1;              /* count of instance */
      printf("\nWho depends on %d (instr %d)?\n", i, chain->insno);
      INSDS *next = chain->nxtact;
      INSTR_SEMANTICS *current_instr = dag_get_info(csound, chain->insno); 
      //csp_set_print(csound, current_instr->read);
      //csp_set_print(csound, current_instr->write);
      while (next) {
        INSTR_SEMANTICS *later_instr = dag_get_info(csound, next->insno);
        printf("%d ", j);
        //csp_set_print(csound, later_instr->read);
        //csp_set_print(csound, later_instr->write);
        //csp_set_print(csound, later_instr->read_write);
        if (dag_intersect(csound, current_instr->write, later_instr->read) ||
            dag_intersect(csound, current_instr->read_write, later_instr->read) ||
            dag_intersect(csound, current_instr->read, later_instr->write) ||
            dag_intersect(csound, current_instr->write, later_instr->write) ||
            dag_intersect(csound, current_instr->read_write, later_instr->write) ||
            dag_intersect(csound, current_instr->read, later_instr->read_write) ||
            dag_intersect(csound, current_instr->write, later_instr->read_write) ||
            dag_intersect(csound, 
                          current_instr->read_write, later_instr->read_write)) {
          watchList *n = (watchList*)mmalloc(csound, sizeof(watchList));
          n->id = i;
          n->next = task_dep[j];
          task_dep[j] = n;
          printf("yes ");
        }
        j++; next = next->nxtact;
      }
      i++; chain = chain->nxtact;
    }
}

void dag_reinit(CSOUND *csound)
{
    int i;
    dag_dispatched = 0;
    for (i=0; i<csound->dag_num_active; i++) {
      if (task_dep[i]==NULL) {
        task_status[i] = AVAILABLE;
        task_watch[i] = -1;     /* Probably unnecessary */
        //dispatch.add(id);
        //printf("Task %d available\n", i);
      }
      else {
        task_status[i] = WAITING;
        task_watch[i] = task_dep[i]->id; //pick_a_watch(i); /* Could optimise here */
        //printf("Task %d waiting for %d\n", i, task_watch[i]);
      }
    }
    for (i=csound->dag_num_active; i<task_max_size; i++)
      task_status[i] = DONE;
}

#if 0
/* INV : Acyclic */
/* INV : Each entry is read by a single thread,
 *       no writes (but see OPT : Watch ordering) */
/* Thus no protection needed */

/* Used to mark lists that should not be added to, see NOTE : Race condition */
watchList nullList;
watchList *doNotAdd = &nullList;
watchList endwatch = { NULL, NULL };

/* Lists of tasks that depend on the given task */
watchList ** watch;         /* OPT : Structure lay out */

/* INV : Watches for different tasks are disjoint */
/* INV : Multiple threads can add to a watch list but only one will remove
 *       These are the only interactions */
/* Thus the use of CAS / atomic operations */

/* Static memory management for watch list cells */
typedef struct watchListMemoryManagement {
  enum bool used;
  watchList s;
} watchListMemoryManagement;

watchListMemoryManagement *wlmm; /* OPT : Structure lay out */

/* INV : wlmm[X].s.head == X; */  /* OPT : Data structure redundancy */
/* INV : status[X] == WAITING => wlmm[X].used */
/* INV : wlmm[X].s is in watch[Y] => wlmm[X].used */


/* Watch list helper functions */

void initialiseWatch (watchList **w, taskID id) {
  wlmm[id].used = TRUE;
  wlmm[id].s.head = id;
  wlmm[id].s.tail = *w;
  *w = &(wlmm[id].s);
}

watchList * getWatches(taskID id) {

    return __sync_lock_test_and_set (&(watch[id]), doNotAdd);
}

int moveWatch (watchList **w, watchList *t) {
  watchList *local;

  t->tail = NULL;

  do {
    local = atomicRead(*w);

    if (local == doNotAdd) {
      return 0;
    } else {
      t->tail = local;
    }
  } while (!atomicCAS(*w,local,t));   /* OPT : Delay loop */

  return 1;
}

void appendToWL (taskID id, watchList *l) {
  watchList *w;

  do {
    w = watch[id];
    l->tail = w;
    w = __sync_val_compare_and_swap(&(watch[id]),w,l);
  } while (!(w == l));

}

void deleteWatch (watchList *t) {
  wlmm[t->head].used = FALSE;
}




typedef struct monitor {
  pthread_mutex_t l = PTHREAD_MUTEX_INITIALIZER;
  unsigned int threadsWaiting = 0;    /* Shadows the length of workAvailable wait queue */
  queue<taskID> q;                    /* OPT : Dispatch order */
  pthread_cond_t workAvailable = PTHREAD_COND_INITIALIZER;
  pthread_cond_t done = PTHREAD_COND_INITIALIZER;
} monitor;                                    /* OPT : Lock-free */

/* INV : q.size() + dispatched <= ID */
/* INV : foreach(id,q.contents()) { status[id] = AVAILABLE; } */
/* INV : threadsWaiting <= THREADS */

monitor dispatch;


void addWork(monitor *dispatch, taskID id) {
  pthread_mutex_lock(&dispatch->l);

  status[id] = AVAILABLE;
  dispatch->q.push(id);
  if (threadsWaiting >= 1) {
    pthread_cond_signal(&dispatch->c);
  }

  pthread_mutex_unlock(&dispatch->l);
  return;
}

taskID getWork(monitor *dispatch) {
  taskID returnValue;

  pthread_mutex_lock(&dispatch->l);

  while (q.empty()) {
    ++dispatch->threadsWaiting;

    if (dispatch->threadsWaiting == THREADS) {
      /* Will the last person out please turn off the lights! */
      pthread_cond_signal(&dispatch->done);
    }

    pthread_cond_wait(&dispatch->l,&dispatch->workAvailable);
    --dispatch->threadsWaiting;
    
    /* NOTE : A while loop is needed as waking from this requires
     * reacquiring the mutex and in the mean time someone
     * might have got it first and removed the work. */
  }

  returnValue = q.pop();

  pthread_mutex_unlock(&dispatch->l);
  return returnValue;

}

void waitForWorkToBeCompleted (monitor *dispatch) {
  /* Requires
   * INV : threadsWaiting == THREADS <=> \forall id \in ID . status[id] == DONE
   */

  pthread_mutex_lock(&dispatch->l);

  if (dispatch->threadsWaiting < THREADS) {
    pthread_cond_wait(&dispatch->l,&dispatch->done);
  }

  /* This assertion is more difficult to prove than it might first appear */
  assert(dispatch->threadsWaiting == THREADS);

  pthread_mutex_unlock(&dispatch->l);
  return;
}














void mainThread (State *s) {

  /* Set up the DAG */
  if (s->firstRun || s->updateNeeded) {
    dep = buildDAG(s);        /* OPT : Dispatch order */
    /* Other : Update anything that is indexed by task 
     * (i.e. all arrays given length ID) */
  }

  /* Reset the data structure */
  foreach (id in ID) {
    watch[id] = NULL;
  }

  /* Initialise the dispatch queue */
  foreach (id in ID) {       /* OPT : Dispatch order */
    if (dep[id] == EMPTYSET) {
      atomicWrite(status[id] = AVAILABLE);
      addWork(*dispatch,id);

    } else {
      atomicWrite(status[id] = WAITING);
      initialiseWatch(&watch[choose(dep[id])], id);  /* OPT : Watch ordering */

    }
  }

  /* INV : Data structure access invariants start here */
  /* INV : Status only decrease from now */
  /* INV : Watch list for id contains a subset of the things that depend on id */
  /* INV : Each id appears in at most one watch list */
  /* INV : doNotAdd only appears at the head of a watch list */
  /* INV : if (watch[id] == doNotAdd) then { status[id] == DONE; } */


  waitForWorkToBeCompleted(*dispatch);

  return;
}







void workerThread (State *s) {
  taskID work;
  watchList *tasksToNotify, next;
  bool canQueue;

  do {

    task = getWork(dispatch);

    /* Do stuff */
    atomicWrite(status[work] = INPROGRESS);
    doStuff(work);
    atomicWrite(status[work] = DONE);    /* NOTE : Race condition */
    
    
    tasksToNotify = getWatches(work);
    
    while (tasksToNotify != NULL) {
      next = tasksToNotify->tail;
      
      canQueue = TRUE;
      foreach (dep in dep[tasksToNotify->head]) {  /* OPT : Watch ordering */ 
	if (atomicRead(status[dep]) != DONE) {
	  /* NOTE : Race condition */
	  if (moveWatch(watch[dep],tasksToNotify)) {
	    canQueue = FALSE;
	    break;
	  } else {
	    /* Have hit the race condition, try the next option */
	    assert(atomicRead(status[dep]) == DONE);
	  }
	}
      }
      
      if (canQueue) {                    /* OPT : Save one work item */
	addWork(*dispatch,tasksToNotify->head);
	deleteWatch(tasksToNotify);
      }
      
      tasksToNotify = next;
    }
    
  } while (1);  /* NOTE : some kind of control for thread exit needed */

  return;
}




/* OPT : Structure lay out
 *
 * All data structures that are 1. modified by one or more thread and
 * 2. accessed by multiple threads, should be aligned to cache lines and
 * padded so that there is only one instance per cache line.  This will reduce
 * false memory contention between objects that just happen to share a cache
 * line.  Blocking to 64 bytes will probably be sufficient and if people really
 * care about performance that much they can tune to their particular
 * architecture.
 */

/* OPT : Watch ordering
 *
 * Moving a watch is relatively cheap (in the uncontended case) but 
 * it would be best to avoid moving watches where possible.  The ideal
 * situation would be for every task to watch the last pre-requisite.
 * There are two places in the code that affect the watch ordering;
 * the initial assignment and the movement when a watch is triggered.
 * Prefering WAITING tasks (in the later) and lower priority tasks 
 * (if combined with the dispatch order optimisation below) are probably
 * good choices.  One mechanism would be to reorder the set (or array) of
 * dependencies to store this information.  When looking for a (new) watch, 
 * tasks are sorted with increasing status first and then the first one picked.
 * Keeping the list sorted (or at least split between WAITING and others) with
 * each update should (if the dispatch order is fixed / slowly adapting) result
 * in the best things to watch moving to the front and thus adaptively give
 * the best possible tasks to watch.  The interaction with a disaptch order
 * heuristic would need to be considered.  Note that only one thread will
 * look at any given element of dep[] so they can be re-ordered without
 * needing locking.
 */

/* OPT : Structure lay out
 *
 * Some of the fields are not strictly needed and are just there to make
 * the algorithm cleaner and more intelligible.  The head fields of the watch
 * lists are not really needed as there is one per task and their position
 * within the watchListMemoryManager array allows the task to be infered.
 * Likewise the used flag in the memory manager is primarily for book-keeping
 * and checking / assertions and could be omitted.
 */

/* OPT : Delay loop
 * 
 * In theory it is probably polite to put a slowly increasing delay in
 * after a failed compare and swap to reduce pressure on the memory
 * subsystem in the highly contended case.  As multiple threads adding
 * to a task's watch list simultaneously is probably a rare event, the
 * delay loop is probably unnecessary.
 */

/* OPT : Dispatch order
 *
 * The order in which tasks are dispatched affects the amount of 
 * parallelisation possible.  Picking the exact scheduling order, even
 * if the duration of the tasks is known is probably NP-Hard (see 
 * bin-packing*) so heuristics are probably the way to go.  The proporition
 * of tasks which depend on a given task is probably a reasonable primary
 * score, with tie-breaks going to longer tasks.  This can either be 
 * applied to just the initial tasks (either in ordering the nodes in the DAG)
 * or in the order in which they are traversed.  Alternatively by
 * sorting the queue / using a heap / other priority queuing structure
 * it might be possible to do this dynamically.  The best solution would
 * probably be adaptive with a task having its priority incremented 
 * each time another worker thread blocks on a shortage of work, with these
 * increments also propagated 'upwards' in the DAG.
 *
 * *. Which means that a solver could be used to give the best possible
 *    schedule / the maximum parallelisation.  This could be useful for
 *    optimisation.
 */

/* OPT : Lock-free
 *
 * A lock free dispatch mechanism is probably possible as threads can 
 * scan the status array for things listed as AVAILABLE and then atomicCAS
 * to INPROGRESS to claim them.  But this starts to involve busy-waits or
 * direct access to futexes and is probably not worth it.
 */

/* OPT : Save one work item
 *
 * Rather than adding all watching tasks who have their dependencies met to
 * the dispatch queue, saving one (perhaps the best, see OPT : Dispatch order)
 * means the thread does not have to wait.  In the case of a purely linear DAG
 * this should be roughly as fast as the single threaded version.
 */


/* NOTE : Race condition
 *
 * There is a subtle race condition:
 *
 *   Thread 1                             Thread 2
 *   --------                             --------
 *                                        atomicRead(status[dep]) != DONE
 *   atomicWrite(status[work] = DONE);
 *   tasksToNotify = getWatches(work);
 *                                        moveWatch(watch[dep],tasksToNotify);
 * 
 * The key cause is that the status and the watch list cannot be updated
 * simultaneously.  However as getWatches removes all watches and moves or
 * removes them, it is sufficient to have a doNotAdd watchList node to detect
 * this race condition and resolve it by having moveWatch() fail.
 */

void newdag_alloc(CSOUND *csound, int numtasks)
{  
    doNotAdd = &endwatch;
??
    watch = (watchList **)mcalloc(csound, sizeof(watchList *)*numtasks);
    wlmm = (watchListMemoryManagement *)
      mcalloc(csound, sizeof(watchListMemoryManagement)*numtasks);

}

#endif

#endif