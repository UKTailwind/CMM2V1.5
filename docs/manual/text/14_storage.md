# Program and Data Storage

The Colour Maximite 2 has full support for programs, files and directories on the SD card and on a USB flash drive. This includes opening files for reading, writing or random access and editing and running programs.

## Drives

- **A:** is the SD card in the front panel connector. It can also be called **B:**, as the PicoMite calls its SD card, so that PicoMite programs that use B: work unchanged.
- **C:** is a USB flash drive, plugged into the USB connector (directly or through a hub - see the chapter Keyboard, Mouse and Gamepad).

The active drive (the default) is A: on startup. It can be changed with the DRIVE command (eg, DRIVE "C:") or by typing A:, B: or C: at the command prompt, and then the drive prefix is not needed.

MMBasic will work with SD cards up to 128GB in capacity. Cards larger than 32GB should be formatted as exFAT and cards 32GB or less formatted as FAT32. The recommended size is 8GB formatted as FAT32. A USB flash drive can be formatted as FAT16, FAT32 or exFAT.

In the following note that:

- The filename can be a string expression, variable or constant. If it is a constant the string must be quoted (eg, KILL "MYPROG.BAS"). The exception is the RUN command where only a constant is allowed.
- Long file/directory names are supported in addition to the old 8.3 format.
- The maximum file/path length is 127 characters.
- Upper/lowercase characters and spaces are allowed although the file system is not case sensitive.
- Directory paths are allowed in file/directory strings (ie, OPEN "/dir1/dir2/file.txt" FOR …). A path that includes the drive letter must be a full path from the root (eg, "C:/mypath/myfile.txt").
- Forward slashes or back slashes are valid in paths between directories. Eg /dir/file.txt or \dir\file.txt.
- The current MMBasic time is used for file create and last access times.
- Up to ten files can be simultaneously open, on any of the drives.
- Input and output commands and functions can also use file #0 which refers to the console.
- Except for INPUT, LINE INPUT and PRINT the # in #fnbr is optional and may be omitted.

## Program Management Commands

All programs reside on the SD card (or a USB flash drive) and so it must be present when running, editing and listing programs (see All Programs Are Run From the SD Card in the chapter Using MMBasic).

- RUN "prog" - Run a program. 'prog' must be a string constant (ie, not a variable).
- EDIT fname$ - Edit a program or text file.
- LIST fname$ - List on the console a program or text file.
- AUTOSAVE fname$ - Receive a file streamed by a computer connected to the serial console.
- XMODEM RECEIVE fname$ - Receive a file from a computer connected to the serial console using the XModem protocol.
- XMODEM SEND fname$ - Send a file to a computer connected to the serial console using the XModem protocol.
- ZMODEM RECEIVE [fname$] - Receive one or more files using the ZModem protocol, which is faster and more reliable than XModem.
- ZMODEM SEND [fname$] - Send a file using the ZModem protocol.

## File Access Within a Program

- OPEN fname$ FOR mode AS #fnbr - Opens a file for reading or writing. 'fname$' is the file name, 'mode' can be INPUT, OUTPUT, APPEND or RANDOM, 'fnbr' is the file number (1 to 10).
- PRINT #fnbr, expression [[,; ]expression] … etc - Outputs text to the file opened as #fnbr.
- INPUT #fnbr, list of variables - Read a list of comma separated data into the variables specified from the file previously opened as #fnbr.
- LINE INPUT #fnbr, variable$ - Read a complete line into the string variable specified from the file previously opened as #fnbr.
- SEEK #fnbr, pos - Will position the read/write pointer in a file that has been opened for RANDOM access to the 'pos' byte.
- FLUSH #fnbr - Forces any buffered writes to be written to the drive. It is recommended that this command be used regularly where data loss could occur in the event of power loss.
- CLOSE #fnbr [,#fnbr] … - Close the file(s) previously opened with the file number '#fnbr'.

Also there are a number of functions that support the above commands:

- INPUT$(nbr, #fnbr) - Will return a string composed of 'nbr' characters read from a file previously opened for INPUT or RANDOM. If less than 'nbr' characters are available the function will return with what it has.
- EOF(#fnbr) - Will return true if the file previously opened for INPUT with the file number '#fnbr' is positioned at the end of the file.
- LOC(#fnbr) - For an open file this will return the current position of the read/write pointer in the file.
- LOF(#fnbr) - Will return the current length of the file in bytes.

## File and Directory Management

- FILES [wildcard] [,sortorder] (or LIST FILES) - Search the current directory and list the files/directories found. These can only be used at the command prompt, not within a program. The file manager (FM) is a full screen alternative - see the chapter File Manager.
- KILL fname$ - Delete a file. A wildcard (eg, KILL "*.BAK") deletes all the matching files - see the KILL command for the details.
- COPY fromname$ TO toname$ - Copy a file, on the same drive or between drives. At the command prompt a wildcard copies all the matching files to a directory.
- RENAME fromname$ AS toname$ - Rename a file.
- MKDIR dname$ - Make a sub directory.
- CHDIR dname$ - Change into the directory dname$. It can also be ".." (dot dot) for up one directory or "/" for the root directory.
- RMDIR dir$ - Remove, or delete, the directory dir$.
- DRIVE drive$ - Sets the active drive to drive$ ("A:", "B:" or "C:").

And there are functions that are handy for searching and managing files/directories:

- DIR$(fspec, type) - Will search for files and return the names of the entries found.
- CWD$ - Will return the current working directory.
- MM.INFO(DRIVE) - The active drive.
- MM.INFO(FREE SPACE) and MM.INFO(DISK SIZE) - The free space on and the size of the active drive.
- MM.INFO(EXISTS FILE fname$) and MM.INFO(EXISTS DIR dirname$) - True if the file or directory exists.
- MM.INFO(FILESIZE fname$) and MM.INFO(MODIFIED fname$) - The size of a file and the date and time it was last modified.
- MM.INFO(PATH) - The path of the program being run. This allows a program to find the resource files it needs relative to itself.

## Play Audio Files

- PLAY WAV | FLAC | MP3 file$ [, interrupt] - Play a WAV, FLAC or MP3 audio file on the stereo audio output.
- PLAY MODFILE file$ - Play a MOD file on the stereo audio output.
- PLAY EFFECT filename$ [,interrupt] - Play a WAV file at the same time as a MOD file is playing.

See the chapter Sound Output for the details.

## Load and Save Images

An image can be loaded from a drive and displayed on the VGA monitor. This can be used to draw a logo or add a background on the display.

```
LOAD BMP filename$ [, StartX, StartY]
LOAD GIF filename$ [, StartX, StartY]
LOAD JPG filename$ [, StartX] [, StartY] [, dither] [, xOffset] [, yOffset] [, scale]
LOAD PNG filename$ [, StartX] [, StartY] [, transparent] [, alphacut] [, xOffset] [, yOffset]
```

Where 'filename$' is the image to load and 'StartX'/'StartY' are the coordinates of the top left corner of the image (these are optional and will default to the top left corner of the display if not specified). LOAD IMAGE is another name for LOAD BMP (the PicoMite's name for it).

LOAD JPG accepts four additional optional parameters. 'dither' selects a dithering mode (0 to 7) which can improve the appearance of the image in modes with a limited colour depth. 'xOffset' and 'yOffset' specify a pixel offset into the source image so that only the lower right portion of a large image is displayed. 'scale' reduces the displayed size of the image by pixel averaging: use 2 to display at 50% size, 4 at 25% and 8 at 12.5%. The default is 1 which displays the image at full size.

LOAD PNG accepts 'xOffset' and 'yOffset' in the same way, after its 'transparent' and 'alphacut' parameters (see LOAD PNG in the commands listing). With these formats an image larger than the display is cropped to fit.

The image must be in the appropriate format and MMBasic will add the extension to the file name if it is not specified.

The current image on the screen can be saved to a file using the following command:

```
SAVE IMAGE filename$ [, StartX, StartY, width, height]
```

This will save the image, or part of the image, as a 24-bit true colour BMP file (the extension .BMP will be added if an extension is not supplied). The Print Screen key does this at the command prompt, in the editor and in the file manager, saving the screen as A:/SCREENnnn.BMP.

## Sequential File Access

Sequential input/output is the standard method of reading or writing to a file and the easiest to understand. When a file is opened it is read from the beginning character by character or line by line. Similarly, when a file is opened for writing the output is sequentially added to the end of the file. This method is often used for recording data or saving temporary information.

In the example below a file is created and two lines are written to the file (using the PRINT command). The file is then closed.

```
OPEN "fox.txt" FOR OUTPUT AS #1
PRINT #1, "The quick brown fox"
PRINT #1, "jumps over the lazy dog"
CLOSE #1
```

You can read the contents of the file using the LINE INPUT command. For example:

```
OPEN "fox.txt" FOR INPUT AS #1
LINE INPUT #1,a$
LINE INPUT #1,b$
CLOSE #1
```

LINE INPUT reads one line at a time so the variable a$ will contain the text "The quick brown fox" and b$ will contain "jumps over the lazy dog".

Another way of reading from a file is to use the INPUT$() function. This will read a specified number of characters. For example:

```
OPEN "fox.txt" FOR INPUT AS #1
ta$ = INPUT$(12, #1)
tb$ = INPUT$(3, #1)
CLOSE #1
```

The first INPUT$() will read 12 characters and the second 3 characters. So the variable ta$ will contain "The quick br" and the variable tb$ will contain "own".

Files normally contain just text and the print command will convert numbers to text. So in the following example the first line will contain the line "123" and the second "56789".

```
nbr1 = 123 : nbr2 = 56789
OPEN "numbers.txt" FOR OUTPUT AS #1
PRINT #1, nbr1
PRINT #1, nbr2
CLOSE #1
```

Again you can read the contents of the file using the LINE INPUT command but then you would need to convert the text to a number using VAL(). For example:

```
OPEN "numbers.txt" FOR INPUT AS #1
LINE INPUT #1, a$
LINE INPUT #1, b$
CLOSE #1
x = VAL(a$) : y = VAL(b$)
```

Following this the variable x would have the value 123 and y the value 56789.

## Random File Access

Random access allows the program to jump around within a file so that sections in the middle (ie, not at the end) can be read or written. This method is often used for database type applications where the file consists of many records which have the same fixed length.

For random access the file should be opened with the keyword RANDOM. For example:

```
OPEN "filename" FOR RANDOM AS #1
```

To seek to a record within the file you would use the SEEK command which will position the read/write pointer to a specific byte. The first byte in a file is numbered one so, for example, the fifth record in a file that uses 64 byte records would start at byte 257. In that case you would use the following to point to it:

```
SEEK #1, 257
```

When reading from a random access file the INPUT$() function should be used as this will read a fixed number of bytes (ie, a complete record) from the file. For example, to read a record of 64 bytes you would use:

```
dat$ = INPUT$(64, #1)
```

When writing to the file a fixed record size should be used and this can be easily accomplished by adding sufficient padding characters (normally spaces) to the data to be written. For example:

```
PRINT #1, dat$ + SPACE$(64 - LEN(dat$));
```

The SPACE$() function is used to add enough spaces to ensure that the data written is an exact length (64 bytes in this example). The semicolon at the end of the print command suppresses the addition of the carriage return and line feed characters which would make the record longer than intended.

Two other functions can help when using random file access. The LOC() function will return the current byte position of the read/write pointer and the LOF() function will return the total length of the file in bytes.

The following program demonstrates random file access. Using it you can append to the file (to add some data in the first place) then read/write records using random record numbers. The first record in the file is record number 1, the second is 2, etc.

```
RecLen = 64
OPEN "test.dat" FOR RANDOM AS #1
DO
  abort: PRINT
  PRINT "Number of records in the file =" LOF(#1)/RecLen
  INPUT "Command (r = read,w = write, a = append, q = quit): ", cmd$
  IF cmd$ = "q" THEN CLOSE #1 : END
  IF cmd$ = "a" THEN
    SEEK #1, LOF(#1) + 1
  ELSE
    INPUT "Record Number: ", nbr
    IF nbr < 1 or nbr > LOF(#1)/RecLen THEN PRINT "Invalid record" : GOTO abort
    SEEK #1, RecLen * (nbr - 1) + 1
  ENDIF
  IF cmd$ = "r" THEN
    PRINT "The record = " INPUT$(RecLen, #1)
  ELSE
    LINE INPUT "Enter the data to be written: ", dat$
    PRINT #1,dat$ + SPACE$(RecLen - LEN(dat$));
  ENDIF
LOOP
```

Random access can also be used on a normal text file. For example, this will print out a file backwards:

```
OPEN "file.txt" FOR RANDOM AS #1
FOR i = LOF(#1) TO 1 STEP -1
  SEEK #1, i
  PRINT INPUT$(1, #1);
NEXT i
CLOSE #1
```
