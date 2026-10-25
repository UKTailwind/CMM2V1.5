/* PrintScreen.h - Print Screen and Ctrl-\ (Console.c), for the console producers too: the serial consoles'
   interrupts (stm32h7xx_it.c) and the USB keyboard (KeyboardMap.c) */
#ifndef PRINTSCREEN_H
#define PRINTSCREEN_H
/* Print Screen's key code, 0x9D, and Ctrl-\ (0x1C, which any keyboard or terminal can send): the display saved
   (ScreenShot, Console.c). PrtScrTake(c) is each console producer's test for OPTION PRINTSCREEN ON: such a key is
   taken, not queued, and the running program saves the screen (PrintScreenNow). */
#define KEY_IS_PRTSCR(c) ((c) == 0x9D || (c) == 0x1C)
extern void ScreenShot(void);
extern volatile int PrintScreenOn, PrintScreenPending;
extern int PrtScrTakeKey(int c);
extern void PrintScreenNow(void);
#define PrtScrTake(c) (PrintScreenOn && PrtScrTakeKey(c))
#endif
