# The Amiga 500 Emulator

This is an Amiga 500 with 1 MB of memory (512 KB of chip RAM and 512 KB of slow RAM) and its original chipset. It is pico-amiga, drewpo28's emulator for the RP2350, with Karl Stenerud's Musashi 68000, made to run on the Colour Maximite 2. It runs a PAL Amiga at the Amiga's own 50 frames a second, in 720 x 576, with its sound, and the heaviest demos tried keep up.

It runs on the Generation 2 only, and needs firmware 7.00.00RC5 or later: older firmware refuses it with "Needs a newer firmware".

## What You Need

The emulator comes without the Amiga's own software. You need two things of your own:

- **A Kickstart ROM**, the Amiga's operating system in ROM. Kickstart is still copyright, so it cannot be given away with the emulator. Kickstart 1.3 is the one the Amiga 500 had and the one to use.
- **Disk images** (.adf files) of the Workbench disk and of the games and demos you want to run.

If you have an Amiga, you can use its Kickstart and disks, saved as files with the usual Amiga tools. If you do not, we recommend **Amiga Forever** from Cloanto (www.amigaforever.com). It is the legal way to get Kickstart and Workbench today, and costs little. Its Value Edition, the least costly, has everything this emulator needs: Kickstart 1.3, the Workbench 1.3 disk, and over 50 games and demos.

### Copying the Files from Amiga Forever

Install Amiga Forever on a Windows PC. Its files are then in **C:\\Users\\Public\\Documents\\Amiga Files**. Copy these to the folder **A:/amiga** on the SD card:

{widths: 26,14,14}
| Amiga Forever's file | What it is | Name on the card |
| Shared\\rom\\amiga-os-130.rom | Kickstart 1.3 | amiga-os-130.rom (keep its name) |
| Shared\\adf\\amiga-os-134-workbench.adf | The Workbench 1.3 disk | workbench.adf (or any name) |

The emulator looks for **amiga-os-130.rom** in its folder by itself, so keep that file's name exactly as it is.

The other ROMs in Shared\\rom work too, with -kick (below): amiga-os-120.rom is Kickstart 1.2, and amiga-os-110-pal.rom and amiga-os-110-ntsc.rom are Kickstart 1.1. Leave out amiga-os-310-a1200.rom: it is for the Amiga 1200, a later machine with a different processor and chipset, and will not start here.

An older edition of Amiga Forever may have its ROMs locked to it, in files 11 bytes longer than 256 KB. The emulator refuses those with a message giving their size. The current editions' ROMs are plain.

### Games and Demos from Amiga Forever

The games and demos are in **Titles\\Games** and **Titles\\Demoscene**, one .rp9 file each. An .rp9 file is a zip file with the title's disk images in it:

1. Copy the .rp9 file somewhere and rename it to end .zip, or open it with 7-Zip.

2. Take out the .adf files. A game on two or more disks has one for each, numbered.

3. Copy them to A:/amiga on the SD card, or to a folder inside it, such as A:/amiga/games.

Only the titles made for the Amiga 500 run here. A title whose zip has a .hdf file (a hard disk) rather than .adf files, or which is marked for the Amiga 1200 or 4000, will not.

## Starting It

```
CHDIR "A:/amiga"
RUN "amiga.app"
```

The disk menu comes up, with the disks in A:/amiga. Choose one with the arrow keys and Enter, then **Boot it**, and Kickstart starts it in drive DF0.

To start a disk straight away, name it after the comma, without quotes, with the folder if it is in one:

```
RUN "amiga.app", games/arkanoid.adf
```

The screen stays blank for a few seconds while the disk is read and the display set up.

What the Amiga writes to a disk goes into its .adf file on the card, as it would onto a real floppy disk, so saved games, high scores and your own work last (see Saving Your Work, below).

Disks load about four times as fast as on a real Amiga.

### The Disk Menu

Press **F11** at any time, in the middle of a game too, for the disk menu. The Amiga stops while it is up.

The menu lists the disk images (.adf) and the folders in a folder of the card. At first that is the folder of the disk in DF0, or the folder the program started in; after that, the folder you last left it in. The disk in DF0 is marked DF0.

{widths: 18,34}
| Keys | What they do |
| Up, Down, Page Up, Page Down, Home, End | Move through the list |
| A letter or digit | The next name that starts with it |
| Enter | Open the folder, or choose the disk |
| Backspace, Left | The folder above |
| Esc, F11 | Back to the Amiga as it was |
| F12 | Back to MMBasic |

A disk chosen offers four things:

- **Boot it**: the Amiga restarted, as if it had been switched off and on, with the disk in DF0. Use this to change to another game.
- **Put it in DF0 (a disk change)**: the disk changed, as by hand. Use this when a game asks for its next disk. The drive stays empty for two seconds first, so that the Amiga sees the change.
- **Put it in DF1, the second drive**: the Amiga has an external drive, DF1, as many Amiga 500s had. Put a work disk there, beside the Workbench disk in DF0.
- **Cancel**.

The menu also lists **[ A new blank disk, made here ]**. Choose it to make an empty disk in the folder shown, ready for the Amiga to use: work1.adf, named Work1 on the Amiga, then work2.adf and so on. The menu then offers to put it in a drive.

A gamepad works the menu too: up and down move, and a face button chooses.

### Choices

Add any of these after the disks:

{widths: 12,40}
| Choice | What it does |
| -ntsc | An NTSC Amiga, as sold in the USA, at 60 frames a second in 640 x 480 |
| -pal60 | The PAL Amiga, shown at 60 frames a second in 640 x 480, for a monitor that does not take 50 Hz. It then runs 20% fast, music too |
| -nosound | No sound |
| -kick file | Another Kickstart, such as -kick amiga-os-120.rom |
| -df1 file | That disk in DF1, the second drive, at the start |
| -wp | Every disk write-protected: nothing is written to the card |

For example:

```
RUN "amiga.app", games/zoom.adf -nosound
```

## The Display

The PAL Amiga is shown in 720 x 576 at 50 Hz, the standard 576p picture, with each Amiga line drawn twice. Each Amiga frame is one frame of the monitor, so movement is smooth. Most monitors take this mode. If yours shows it off to one side, try its auto-adjust button, or use -pal60.

Most Amiga games and demos were made in Europe for the PAL Amiga. With -ntsc, such a title shows its picture too low, with its bottom cut off, and some stop altogether, as they did on an NTSC Amiga. Use -ntsc only for titles made for NTSC.

Interlaced screens, with 400 or 512 lines, show every other line for now.

## The Keys

The keys are taken by their place on the keyboard, as on an Amiga keyboard.

{widths: 18,34}
| Keys | What they do |
| F1 to F10 | The Amiga's F1 to F10 |
| F11 | The disk menu |
| F12, or Ctrl+Alt+End | Back to MMBasic |
| Ctrl+Alt+Del, or Ctrl and both Amiga keys | A reset, as Ctrl and both Amiga keys are on an Amiga: the disk in DF0 starts again |
| Left Windows key | The left Amiga key |
| Right Windows key, Menu | The right Amiga key |
| Alt | The Amiga's Alt |
| Insert | HELP |
| Delete | DEL |
| The arrows, keypad | The Amiga's |

Ctrl-C is the Amiga's Ctrl and C here, so it does not end the program.

Workbench 1.3 starts with the American key layout. On a British keyboard, open a Shell and type SetMap gb to have the symbols where your keys show them.

## Saving Your Work

What the Amiga writes to a disk goes into the disk's .adf file on the SD card as it is written, as it would onto a real floppy disk, and stays there. This is true of a game's saved positions and high scores, and of anything you save from Workbench or a Shell.

To work on your own programs or files:

1. Start with the Workbench disk in DF0.
2. Press F11, choose **[ A new blank disk, made here ]**, and put it in DF1.
3. Save your work to it. It is called Work1 on the Amiga, so a file saved as Work1:myprog is in work1.adf.

A disk whose .adf file is read-only (set on a PC) is a write-protected disk: the Amiga says so if it tries to write to it, and the file is never changed. Start the program with -wp to protect every disk, for example to keep a game's disk exactly as it came.

The RAM DISK is in the Amiga's memory, as on a real Amiga, so what is there is lost at a reset or when the program ends.

The Amiga has no BASIC in its ROM. AmigaBASIC, Microsoft's BASIC for the Amiga, came on the Extras disk of Workbench 1.3, which is not among Amiga Forever's Value Edition files. If you have that disk, put it in DF1 with Workbench in DF0, open its icon (Extras 1.3) and then AmigaBASIC's. Its folder BasicDemos has Microsoft's example programs.

If you do not have it, a copy of the Extras 1.3 disk with AmigaBASIC is on the Internet Archive, at archive.org/details/amiga_extras1.3_basic1.2: download and use it at your own risk. Copy its .adf file to A:/amiga.

## The Mouse and Joysticks

A USB mouse is the Amiga's mouse, in port 1.

The first USB gamepad is a joystick in port 2, where nearly all games look, and the second is the joystick in port 1. The d-pad or the left stick steers, and any of the four face buttons fires.

## Leaving It

Press **F12** or **Ctrl+Alt+End** to go back to MMBasic, in the disk menu too.

## What Has Been Tried

On a Generation 2 with firmware 7.00.00RC5, the PAL Amiga in 576p:

- **Demos:** Mental Hangover (Scoopex) and Arte (Sanity), the heaviest tried, at 50 frames a second all through with the sound on, and 242 (Virtual Dreams and Fairlight). D.O.S. (Andromeda) falls just short in a few places.
- **Workbench 1.3**, with the mouse and the keyboard.
- **Games from Amiga Forever**, each for a minute: Arkanoid, Aunt Arctic Adventure, Blasteroids, Datastorm, Exile, Gotcha, Lethal Xcess, Obsession Pinball, Pac-Mania, Rage, Statix, Toobin and Zoom! load and run at 50 frames a second. X-Balls and Logic load slowly through AmigaDOS, and had not finished in that minute. Tie Break shows only a black screen.
- **The keyboard, the mouse and a gamepad**, by hand, in Workbench and in games, and the disk menu.
- **AmigaBASIC**, from a Workbench 1.3 Extras disk in DF1, running its Demo program.
- **Saving:** a blank disk made from the menu and put in DF1; AmigaDOS wrote a file and a folder to it, and the .adf file, copied from the card to a PC, has them. With -wp the Amiga reported the disk write-protected, and the file was unchanged.

Titles made for later Amigas (the A1200 and A4000), and those on hard disk images, do not run.

## Licences

pico-amiga is under the GNU GPL version 3, and Musashi under the MIT licence. Kickstart, Workbench and the games are the property of their owners and do not come with the emulator.
