# Game Playing Features

The Colour Maximite 2 has many features designed to help programmers in writing computer games. One of the most important features of this computer is its speed (approx 10 times faster than the original Colour Maximite) and its large program memory. These alone make it possible to write large and complex programs without the issues of optimising the program for speed or space.

Other useful features include managing the video output and drawing moveable images, a selection of text drawing methods and fonts, displaying images, playing audio and getting input from a keyboard or games controller.

## VGA Resolution, Colour Depth and Pages

The video output to the VGA monitor is controlled by the MODE command. With this you can select various resolutions from 1920x1080 (on the Generation 2) and 800x600 pixels (the normal default on startup) down to 240x216, and colour depths of 8, 12, 16 and (on the Generation 2) 32 bits. For the 8-bit colour mode each individual colour can be specified from a 16-bit palette using the MAP command. A colour depth of 4 bits gives the PicoMite's 16 colour display, and 1 bit its MODE 1 with coloured tiles, so that games written for the PicoMite can draw as they do there.

Particularly useful is the 12-bit colour mode which supports 4096 colours with an additional 4-bit alpha channel allowing 16 levels of transparency. This mode has provision for two image layers plus a background colour. Images on the top layer will cover the lower layer except where the alpha channel is set to allow transparency thereby allowing various degrees of the lower layer and/or background to show through.

In addition there are many extra video pages that are available to the programmer for building images (see the chapter Video Modes and Pages for how many there are in each mode). These pages can then be copied at high speed during the video blanking period to the main display page providing an instantaneous display update without any tearing artefacts. This is managed by the PAGE WRITE command which specifies the video page to be used for the output of subsequent graphics commands and the PAGE COPY command which will copy one page to another.

When drawing to the main page being displayed on the monitor the programmer can use the GETSCANLINE function to report on the line that is currently being drawn on the VGA monitor. Using this to time updates to the screen can avoid screen glitches caused by updates while the screen is being updated. A similar outcome can be achieved using an optional feature of the MODE command which will call a user defined subroutine at the start of the VGA frame blanking.

This and other features of the graphics subsystem are explained in detail in a tutorial presented on the Back Shed forum: https://www.thebackshed.com/forum/ViewTopic.php?FID=16&TID=12125.

## Scrolling, Sprites and Tile Maps

The PAGE SCROLL command will scroll a page horizontally or vertically by a specified number of pixels allowing the games programmer to create a smoothly scrolling background for platform games and the like.

The Colour Maximite 2 has extensive support for sprites which is an image that can be moved over the background without disturbing the background. This feature is far more sophisticated than that available on the original Colour Maximite. The sprite can be a PNG image or an image defined in a text file and can be of any size up to the video horizontal and vertical resolution. The transparency colour can be either black or defined in the image depending on the colour depth specified by the MODE command.

Multiple sprites can be loaded and they can be moved around the screen, hidden or displayed as a group or individually. MMBasic also includes a versatile mechanism for detecting when two sprites collide allowing (for example) the programmer to create a realistic bounce effect.

The TILEMAP command draws a background made of tiles (small images from one tile sheet) laid out as a map, and scrolls a view of it. It has sprites of its own, which move over the map and can test which tiles they touch. TILEMAP is the PicoMite's, so games written with it there run here too.

## A First-Person View

The RAY commands draw a first-person view of a map of walls, as in the game Wolfenstein 3D, with patterned walls, floor and ceiling, sliding doors, and sprites standing in the world. They also check movement against the walls and draw a map from above. RAY is the PicoMite's raycaster: it works in every colour depth, with RGB colours too. See Appendix P.

## Displaying Images

The Colour Maximite 2 can load and display images stored on the SD card in a variety of formats including BMP, GIF, PNG and JPG. These are read by the LOAD command and can be of any size and positioned anywhere on the screen. In addition on-screen images can be scaled and rotated under control of the program.

Images can also be loaded into memory with FLASH LOAD IMAGE, into the image slots 1 to 8 that the PicoMite's games use, and drawn from there with BLIT FLASH.

## Fonts

Built in to MMBasic is support for seven fonts ranging from small to large. In addition user supported fonts can be defined by the BASIC program or dynamically loaded from the SD card. All fonts can be displayed in various colours, scaled and rotated.

## Playing Audio

The Colour Maximite 2 can play audio on the stereo audio output in a variety of formats using the PLAY command. These include sound effects or music encoded as WAV, FLAC or MP3 files. Also supported is the playing of MOD files which is a standard for computer generated music. The PLAY EFFECT command will allow the simultaneous playing of a WAV file over a MOD file so that sound effects can be superimposed on the computer generated music playing in the background.

Other audio formats available to the programmer include pure sine wave tones or sound effects consisting of a mixture of sine wave, square wave, triangle wave, sawtooth wave or random noise (the PLAY TONE and PLAY SOUND commands), the BBC Micro's sound and envelopes (PLAY BBC SOUND and PLAY BBC ENVELOPE), and samples played from memory (PLAY SAMPLE and PLAY ARRAY). The programmer can also change the volume, pause any audio output, resume the playing or step forward or back to the next audio file.

To round off the list of audio features the PLAY TTS command will generate a robotic speech output based on a string which defines the words and sounds to be generated.

## Keyboard Keys

In games that use the keyboard for input the user will often hold down a number of keys simultaneously. This condition can be detected using the KEYDOWN() function. This will return the number of keys held down and their values in the order that they were depressed. MMBasic will track up to six simultaneous key depressions.

## Wii Nunchuk and Classic Controllers

The Wii Nunchuk (also spelt Nunchuck) and the Wii Classic are inexpensive games controllers which include a joystick and various buttons. MMBasic on the Colour Maximite 2 provides full support for up to three of these controllers including querying the state of all user inputs and their calibration constants.

The Nunchuk controller is opened with the CONTROLLER NUNCHUK OPEN command and its status is queried using the NUNCHUK() function. The Classic controller is opened with the CONTROLLER CLASSIC OPEN command and its status is queried using the CLASSIC() function.

## USB Gamepads

Up to two USB gamepads can be plugged into the USB socket, through a hub (see the chapter Keyboard, Mouse and Gamepad). They include the PlayStation 3 and 4 controllers, Xbox 360, Xbox One and Xbox Series controllers, the Nintendo Switch Pro controller and many generic gamepads. A gamepad is read with the DEVICE(GAMEPAD n, ...) function and set up with the GAMEPAD command, as on the PicoMite.
