/* FileManager.h - FM, the file manager (Src/FileManager.c, the PicoMite's): what the rest of MMBasic sees of it.
   FM runs a program through the prompt (RUN, as END cmd$ does) and the editor directly; these say what it is doing
   so that the prompt, error(), END and the editor bring it back afterwards, as the PicoMite does (19 Oct 2026). */
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

extern int fm_active;                            // FM is open (its screen: the USB notices wait)
extern int fm_program_launched_from_fm;         // a program FM ran is running: at its end, the prompt runs FM again
extern int fm_relaunch_status_valid;             // fm_relaunch_status is the line FM shows when it comes back
extern char fm_relaunch_status[];
extern int fm_editing;                           // FM's EDIT is open: F2 runs the file and comes back to FM
extern char fm_last_launched_bas[];              // the program FM ran (or is editing), "A:/dir/name.bas"
extern int fm_pending_edit_seek_valid;           // the editor opens at this line and column (an error's)
extern int fm_pending_edit_seek_line, fm_pending_edit_seek_char;

extern void cmd_fm(void);
extern void FmNoteError(int line);              // error(): a program FM ran stopped at line (0: none, or not its file)

#endif
