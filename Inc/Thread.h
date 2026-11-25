/***************************************************************************
CMM2 MMBasic
Thread.h

Threads for a BASIC program (Src/Thread.c, docs/Threads_Plan.md).

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.
See MMBasic.h for the licence.
***************************************************************************/
#ifndef THREAD_H
#define THREAD_H

#define MAXTHREADS 16                               // the threads a program may start, numbered 1 to MAXTHREADS
#define THREAD_STACK (20 * 1024)                    // a thread's C stack
#define THREAD_GUARD (6 * 1024)                     // the part of it below the stack check's floor (Stage 0's figures)
#define THREAD_LOCALS 48                            // its local variable slots
#define MAXLOCKS 16                                 // THREAD LOCK's locks, numbered 1 to MAXLOCKS

extern int ThreadCount;                             // the threads started and not yet ended: 0, and nothing here is active
extern volatile int ThreadSliceLeft;                // milliseconds left of the running thread's slice (the tick, Timers.c)
extern int ThreadSliceLen;                          // THREAD SLICE
extern volatile unsigned int ThreadWakeMs;          // mSecTimer when the first sleeping thread is due (the tick); 0 none
extern char *ThreadIntPending;                      // an interrupt a thread found due: the main program's (check_interrupt)
/* No thread switch while BASIC runs from inside a CSUB or a module's call (its own data is not a thread's, and a
   module's call has one stack): ThreadHoldDepth is the ExecDepth such BASIC runs at, or more. The caller brackets its
   call with the two macros. An error that leaves the call by longjmp leaves the depth set: the scheduler clears it
   the first time it is asked at a shallower depth, which only happens once the call is over. */
extern int ThreadHoldDepth;
#define THREAD_HOLD_BEGIN() int threadhold_ = ThreadHoldDepth; if(ExecDepth + 1 < ThreadHoldDepth) ThreadHoldDepth = ExecDepth + 1
#define THREAD_HOLD_END() ThreadHoldDepth = threadhold_
void cmd_thread(void);
void ThreadSchedule(void);                          // the statement tail's call: another thread's turn (check_interrupt)
void ThreadSleepUntil(long long t);                 // PAUSE: the other threads run until the microsecond clock reads t
void ThreadToMain(void);                            // an interrupt waits for the main program: it runs now
int ThreadState(int n);                             // MM.INFO(THREAD n)
int ThreadLockOwner(int k);                         // MM.INFO(THREAD LOCK k): the thread that has lock k, -1 none
int ThreadArg(int n, long long int *i, MMFLOAT *f, char *s);    // MM.INFO(THREAD ARG n), for a thread's own call
void ThreadsStopped(void);                          // the prompt's landing: the list of threads after an error or Ctrl-C,
void ThreadsStoppedShow(void);                      // written down, and then printed after the error's message
int ThreadsEnd(void);                               // the program has stopped or is being replaced: 1 if it had threads
void ThreadsStop(void);                             // END: no other thread runs from here on
int ThreadNumber(void);                             // the running thread's number: 0 the main program
void ThreadMainOnly(void);                          // an error in any thread but the main program
void ThreadsNone(void);                             // an error while the program has a thread
char *ThreadTokenBuffer(void);                      // EXECUTE's line: tknbuf for the main program, a thread's own
void ThreadStackLimits(void);                       // the stack check's limits as the running thread's (after PIXEL FILL)

#endif
