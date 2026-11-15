' mqtttest.bas - WEB MQTT on the CMM2 (docs/Web_Plan.md, step 3): RUN "mqtttest.bas", broker-ip (the PC running
' pcmqtt.py). It connects, subscribes to cmm2/in, says hello on cmm2/out, and echoes each message it gets (its MQTT
' interrupt: MM.TOPIC$, MM.MESSAGE$) to cmm2/out.
Option Explicit
Option Default Integer
Dim n
Dim string broker$ = MM.CmdLine$
WEB MQTT CONNECT broker$, 1883, "", "", got
WEB MQTT SUBSCRIBE "cmm2/in"
WEB MQTT PUBLISH "cmm2/out", "hello from the CMM2", 1, 0
Print "MQTT with "; broker$; ": any key ends"
Do : Loop Until Inkey$ <> ""
WEB MQTT CLOSE
End

Sub got
  Inc n
  Print MM.TOPIC$; ": "; MM.MESSAGE$
  WEB MQTT PUBLISH "cmm2/out", "echo" + Str$(n) + ": " + MM.MESSAGE$, 1, 0
End Sub
