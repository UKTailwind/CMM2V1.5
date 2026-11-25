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

extern int ThreadCount;                             // the threads started and not yet ended: 0, and nothing here is active
void cmd_thread(void);
void ThreadSchedule(void);                          // the statement tail's call: another thread's turn (check_interrupt)
int ThreadsEnd(void);                               // the program has stopped or is being replaced: 1 if it had threads
void ThreadsStop(void);                             // END: no other thread runs from here on
int ThreadNumber(void);                             // the running thread's number: 0 the main program
void ThreadMainOnly(void);                          // an error in any thread but the main program
void ThreadsNone(void);                             // an error while the program has a thread
char *ThreadTokenBuffer(void);                      // EXECUTE's line: tknbuf for the main program, a thread's own
void ThreadStackLimits(void);                       // the stack check's limits as the running thread's (after PIXEL FILL)

#endif
