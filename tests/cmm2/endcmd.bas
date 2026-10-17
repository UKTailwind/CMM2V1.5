' endcmd.bas - END cmd$ (plan step 8, the PicoMite's): with no SUB MM.END, cmd$ runs as though typed at the prompt
' once the program has ended. (endhook.bas has MM.END.)
Print "running"
End "Print " + Chr$(34) + "typed after the end" + Chr$(34) + ": Print 6 * 7"
