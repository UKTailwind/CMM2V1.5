# Interrupts and Threads

## An interrupt belongs to the thread that set it up

SETTICK, SETPIN with an interrupt, ON KEY, a COM port's interrupt, the GUI's, the network's and all the others name a routine to call when something happens. With threads, the routine runs in the thread that set it up:

- a SETTICK set up by the main program, or before any thread was started, runs in the main program, as it always has;
- a SETTICK set up by thread 2 runs in thread 2.

The routine is its own context, as it always was: a SUB with its own LOCAL variables, that sees the global variables. It runs with its thread's print position, colours, font and write page, and it runs to its end, IRETURN or END SUB, before any other thread runs. Interrupt routines should still be short.

If the interrupt comes while its thread is not the one running, that thread runs at once to take it, ahead of the others. When the routine ends, the thread it interrupted has the rest of its turn, so a fast SETTICK in one thread does not take the other threads' time.

## The pattern: set up, then wait

A thread can set up an interrupt and then sleep until the routine says something has happened:

```
Dim Integer reading

Sub Sampler
  SetTick 100, Sample, 1
  Do
    Thread Wait Flag 1            ' asleep until the routine sets it
    Flag(1) = 0
    Text 0, 20, "Reading: " + Str$(reading) + "   "
  Loop
End Sub

Sub Sample
  reading = Pin(7)
  Flag(1) = 1
End Sub

Thread Start 1, Sampler
Input "Command"; c$               ' the sampler goes on meanwhile
```

The thread uses no processor time between readings; the main program can do anything, even sit in INPUT.

## When the interrupt waits

An interrupt waits, and is not lost, while its thread cannot take it:

- **its thread is paused** (THREAD PAUSE): it runs when the thread is resumed;
- **its thread is in a wait that takes no interrupt**: INPUT, a frame wait, SYNC, TEMPR(), a network or Bluetooth command. These have never taken interrupts; the interrupt runs when the wait ends. The other threads' interrupts are not held up;
- **another thread is between THREAD HOLD and THREAD RELEASE**;
- **another interrupt routine is running**: routines do not interrupt each other.

Ticks are counted, keys are kept in the keyboard's buffer and pin changes are remembered, so nothing is lost while an interrupt waits. (A SETTICK that falls more than one period behind takes the missed ticks as one, as it always has.)

## When its thread ends

The interrupts a thread set up stay set up after it ends, and their routine then runs in the main program. A thread that should take its interrupts with it turns them off before it ends (SETTICK 0, 0, n and so on).

If two threads set up the same SUB as an interrupt routine, it runs in the one that set it up last.

## Inside an interrupt routine

No other thread runs while an interrupt routine runs, so a routine cannot wait for something another thread must do. These are errors in a routine: THREAD YIELD, THREAD EXIT, THREAD STOP of the routine's own thread ("Invalid in an interrupt routine"; to end its thread, a routine sets a flag that the thread's loop tests), THREAD LOCK of a lock another holds, THREAD WAIT for something not yet so, and a WEB or BLUETOOTH command while another thread is using the network ("is in use by thread n").
