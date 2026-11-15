# WiFi and Internet Functions


With a WiFi module plugged into its rear I/O connector the Colour Maximite 2 can join a WiFi network and use the Internet. It can serve web pages, fetch data from web sites (with or without encryption), send and receive UDP messages, use an MQTT broker, get the time from a time server, and give its console to a remote terminal over Telnet. A PC can copy files to and from it with TFTP.

The networking is the PicoMite's: the WebMite firmware's code, unchanged, with the same WEB commands, options and functions. A program written for a WebMite runs as it does there, apart from the I/O pins it uses (see Differences from the WebMite at the end of this chapter).

This chapter describes how the Colour Maximite 2 works with WiFi and the Internet protocols and gives simple examples. It does not describe HTML, TCP and the many other protocols and conventions involved; a newcomer will need one of the many primers on the web.

The protocols are complex, and a program that runs unattended should be ready for the network to fail. A WATCHDOG will restart the Colour Maximite 2 if the program stops, and with OPTION AUTORUN the program then starts again. A forced restart from time to time (for example once a week, with CPU RESTART) also starts the network afresh.

## The WiFi Module

The module carries a Raspberry Pi RM2, the WiFi chip of the Raspberry Pi Pico W (an Infineon CYW43439), with its own 3.3V supply taken from the 5V pin. It plugs onto the rear I/O connector and passes the connector through to a second connector on its back, so that other things can still be wired to the pins it does not use. It uses four pins:

{widths: 15,25,60}
| Pin | Signal | Notes |
|---|---|---|
| 40 | Clock | SPI2's clock. The module holds it high with a pull-up, so it does not trigger the factory reset at power up (see the chapter Hardware Features). |
| 38 | Data | SPI2's data, in both directions, and the chip's interrupt. |
| 33 | Chip select | |
| 32 | Power on | Turns the chip on and off. |

The connector of a Generation 1 is the Generation 2's turned through 180 degrees (see External I/O Connector in the chapter Hardware Features), so the module plugs onto a Generation 1 the other way round: its pin 1 always goes to the connector's pin 1. Plugged in the wrong way round it would put 3.3V and 5V on I/O pins.

Without OPTION WIFI the module is switched off and the four pins are free for other uses. With OPTION WIFI set the four pins and SPI2 belong to the WiFi: SETPIN refuses them ("Pin 32 is reserved on startup"), the SPI2 command cannot open SPI2, and GP10 and GP11, the PicoMite names that refer to pins 40 and 38, go with them. If OPTION WIFI is set and no module answers at start-up, the Colour Maximite 2 prints "WiFi module not found", gives back the memory and the pins, and carries on without the network.

The link to the chip runs at 24MHz on a Generation 2 or 3, and at 20MHz on a Generation 1 running at 400MHz.

## Memory

With OPTION WIFI set the network keeps 128KB of memory for itself, from start-up until OPTION WIFI is turned off:

- On a Generation 2 or 3 it comes from the memory for BASIC's variables (MM.INFO(HEAP) is 128KB less).
- On a Generation 1 it is the top 128KB of the video memory, so that the memory for variables, which the loadable programs need, stays as it was. Some modes and colour depths then have one page fewer: MODE 9,16 has one page instead of two, 14,12 and 14,16 have two instead of three, 9,8 and 12,8 three instead of four, and 8,12, 8,16 and 18,8 four instead of five. LIST PAGES and MM.INFO(MAX PAGES) show the pages there are.

A secure (TLS) connection also takes some of the memory for variables while it is open, as do the buffers of the web server's connections while it uses them, and the certificates that WEB TLS CA loads take about 480KB of it until WEB TLS NOVERIFY.

The network is not serviced while a loadable program (a .app file) runs: requests wait until it ends. On a Generation 1, a loadable program built to use the video memory (for example Doom or the computer emulators) is lent the network's memory as well, so when it ends the network is started again from the beginning, as at power up, and joins the WiFi network again: about seven seconds. Interrupts that a BASIC program set with the WEB commands are kept.

## Connecting to a WiFi Network

The WiFi is turned on with this command at the command prompt:

```
OPTION WIFI ssid$, password$
```

'ssid$' is the name of the WiFi network and 'password$' is its password. Both are strings, so constants must be quoted:

```
OPTION WIFI "MyNetwork", "secret"
```

The command can also set the name of the Colour Maximite 2 on the network, a fixed IP address and the WiFi country code. The full forms are:

```
OPTION WIFI ssid$, password$ [, name$] [, country$]
OPTION WIFI ssid$, password$, name$, ipaddress$, mask$, gateway$ [, country$]
```

'name$' is the name the Colour Maximite 2 gives the network; without it a name is made from the processor's unique ID, such as PICO0131697F (the PicoMite's form). 'country$' is a two letter ISO country code (for example "GB", "DE" or "AU") that sets the WiFi's regulatory domain. The default, "XX" (worldwide), keeps the radio to the most cautious set of channels and transmit power; setting the local country is recommended, and is often needed to join a European router that uses channel 12 or 13. The second form gives a fixed IP address, mask and gateway instead of asking the router for an address (DHCP).

The Colour Maximite 2 saves the option and restarts. The console is not lost, as it is on a PicoMite, and shows:

```
PICO0131697F connecting to WiFi...
Connected 192.168.1.106
```

The address is the one the router gave, and will vary from network to network. If the Colour Maximite 2 cannot connect it says "failed to connect." Then check the router's configuration:

- The WiFi security must be WPA-PSK with TKIP or AES encryption (or both).
- The WiFi must have a password.
- DHCP must be on, unless a fixed address is given.

The option is remembered: the Colour Maximite 2 joins the network at every start-up. OPTION LIST shows it, with the password as stars. MM.INFO(IP ADDRESS) returns the address while it is connected, and "0.0.0.0" while it is not:

```
> PRINT MM.INFO(IP ADDRESS)
192.168.1.106
```

OPTION WIFI "", "" turns the WiFi off again (and restarts).

The joining takes a few seconds and some routers need a second attempt, so a program started by OPTION AUTORUN should wait for the address at its start:

```
TIMER = 0
DO WHILE MM.INFO(IP ADDRESS) = "0.0.0.0"
  IF TIMER > 10000 THEN CPU RESTART
LOOP
```

This waits ten seconds for a connection and restarts the Colour Maximite 2 if there is still none.

The password need not be typed where others can see it. OPTION WIFI takes expressions, so the name and password can be kept in a file on the SD card (the name on its first line, the password on its second) and the option set from it with this line at the command prompt:

```
OPEN "A:/wifi/wifi.cfg" FOR INPUT AS #1 : LINE INPUT #1, s$ : LINE INPUT #1, p$ : CLOSE #1 : OPTION WIFI s$, p$
```

## Remote Console Access

The console can also be used over WiFi with Telnet, which is handy when the Colour Maximite 2 is somewhere hard to reach. Everything that can be done at the serial console can be done over Telnet. Turn it on with:

```
OPTION TELNET CONSOLE ON
```

This is remembered and restarts the Colour Maximite 2. Connect a Telnet program to its address, port 23 (Tera Term, for example, has a good VT100 emulation). OPTION TELNET CONSOLE ONLY sends the console's output to Telnet and not to the serial port (the screen still shows it), and OPTION TELNET CONSOLE OFF turns Telnet off.

## File Transfer

Files can be copied to and from the Colour Maximite 2 over WiFi with TFTP. The TFTP server is on whenever the Colour Maximite 2 is connected (OPTION TFTP OFF turns it off). The PC needs a TFTP client; there are many for Windows, Mac OS and Linux. Tutorials about TFTP often describe setting up a TFTP server: that is not needed, only a client.

Windows has a TFTP client, which must be turned on in the Control Panel: Programs and Features, Turn Windows features on or off, then tick TFTP Client. Then, in a Command or PowerShell window ('IP-Addr' is the Colour Maximite 2's address):

```
TFTP -i IP-Addr PUT filename
TFTP -i IP-Addr GET filename
```

A file goes into, or comes from, the current directory. A name can start with a drive (A:/, B:/ or C:/) to choose another.

## Getting the Time

The WEB NTP command sets the clock from a time server on the Internet, which also shows that the Internet can be reached:

```
WEB NTP [timeoffset [, NTPserver$ [, timeout]]]
```

'timeoffset' is the time zone, in hours from UTC (the clock is set to UTC without it). 'NTPserver$' is the name or address of the time server; without it a public pool of servers is used. For example, in Los Angeles:

```
> WEB NTP -8
ntp address 45.138.55.60
got ntp response: 15/11/2026 06:38:31
```

The time also goes into the real time clock. If the Internet cannot be reached WEB NTP gives an error, which a program can trap and act on:

```
ON ERROR SKIP 3
WEB NTP -8
IF MM.ERRNO THEN WEB NTP -8
IF MM.ERRNO THEN PRINT "Failure to connect to the Internet" : CPU RESTART
```

WEB NTP is tried twice because the first attempt can fail by bad luck in the timing; the second normally works.

## Implementing a Web Server

A common use is a web server that shows the data a program collects in a page that any browser can display. First the server is set up, with this command at the command prompt:

```
OPTION TCP SERVER PORT 80
```

80 is the usual port for web pages. The option is remembered and restarts the Colour Maximite 2, which then says:

```
Starting TCP server at 192.168.1.106 on port 80
```

The server answers requests only while a program runs; at the command prompt every request gets "404 not found". In the program, WEB TCP INTERRUPT names the subroutine to call when a request comes in. It reads the request with WEB TCP READ and answers with WEB TRANSMIT PAGE. This is a whole program:

```
1  DIM buff%(4096/8)
2  WEB TCP INTERRUPT WebInterrupt
3  DO
4    '<do some processing here>'
5  LOOP
6
7  SUB WebInterrupt
8    LOCAL a%, p%, t%
9    FOR a% = 1 TO MM.INFO(MAX CONNECTIONS)
10     WEB TCP READ a%, buff%()
11     p% = LINSTR(buff%(), "GET")
12     t% = LINSTR(buff%(), "HTTP")
13     IF (p% <> 0) AND (t% > p%) THEN
14       WEB TRANSMIT PAGE a%, "index.html"
15     ENDIF
16   NEXT a%
17 END SUB
```

Line by line:

>> Line 1	A 4KB buffer for the request, as a long string (see the chapter Long Strings). Its size limits how much of a request is read.
>> Line 2	Starts the server, with WebInterrupt as the subroutine for each request.
>> Line 4	The program's main loop, which would be collecting data, switching outputs and so on.
>> Line 7	The subroutine, called each time a browser makes a request.
>> Line 9	Goes through all the connections: there can be several at once (up to MM.INFO(MAX CONNECTIONS)).
>> Line 10	Reads a connection's request into the buffer (an empty string if it has none).
>> Lines 11-13	Checks that the request has the keywords GET and HTTP, in that order.
>> Line 14	Sends the page: the file index.html, in the current directory, in HTML (so it can hold tags such as <h1>A heading</h1>). See A Typical Web Page below.

The server can answer several browsers at once.

### Inserting Data in the Web Page

A program usually puts its data into the page. WEB TRANSMIT PAGE does this for it: the name of a BASIC variable between curly brackets ({ and }) in the page is replaced by its value. If the program has a variable CurrentTemp holding 24, the page

```
The temperature is {CurrentTemp}
```

is shown in the browser as "The temperature is 24". Between the brackets can be a float, an integer or a string, an array element, or any expression, including functions: Str$() can format a number with the right number of decimal places. An error in the expression is an error in the WEB TRANSMIT PAGE command. Two opening brackets ({{) send one; a closing bracket on its own is sent as it is.

### Sending Several Pages

A browser that asks for a server's default page sends GET / HTTP; with a link such as <a href="page2.html">Next page</a> in the page, a click on it sends GET /page2.html HTTP. The program can pick out the name between GET and HTTP and send the page asked for. In place of line 14 above:

```
s$ = LGETSTR$(buff%(), p% + 4, t% - p% - 5)
IF s$ = "/" THEN
  WEB TRANSMIT PAGE a%, "index.html"
ELSEIF s$ = "/page2.html" THEN
  WEB TRANSMIT PAGE a%, "page2.html"
ELSEIF s$ = "/pix.png" THEN
  WEB TRANSMIT FILE a%, "pix.png", "image/png"
ELSE
  WEB TRANSMIT CODE a%, 404
ENDIF
```

WEB TRANSMIT FILE sends a file as it is, such as an image that a page shows with <img src="pix.png"/>. "image/png" is its MIME type; others are image/jpeg, image/bmp and image/gif. WEB TRANSMIT CODE sends just a code: 404 means "page not found". The Colour Maximite 2 is not a fast server, so small images are best.

## Live Graphics in a Web Page

A page can show a graph or chart drawn by the program with the graphics commands, saved as a PNG file and sent as an image. The drawing can be done on another page, so the screen is not disturbed:

```
PAGE WRITE 1
CLS
CIRCLE 100, 100, 50, 1, 1, RGB(red), RGB(blue)
LINE 10, 10, 200, 200, 1, RGB(yellow)
SAVE PNG "graph.png", 0, 0, 320, 240
PAGE WRITE 0
```

SAVE PNG saves the area given (x, y, width and height) of the page being written to. The web page shows it with <img src="graph.png"/>, and the server sends it when asked:

```
ELSEIF s$ = "/graph.png" THEN
  WEB TRANSMIT FILE a%, "graph.png", "image/png"
```

The program only needs to draw and save the graph again when a browser asks for it.

## A Complete General Purpose Server

This server sends whatever file a browser asks for, choosing the right WEB TRANSMIT command from the file's extension. It can be added to any program that needs a web server:

```
WEB TCP INTERRUPT WebInterrupt
DO
  '<do some processing here>'
LOOP

' the subroutine for all the web server's requests
SUB WebInterrupt
  LOCAL a%, p1%, p2%, file$, buff%(4096/8)
  FOR a% = 1 TO MM.INFO(MAX CONNECTIONS)
    WEB TCP READ a%, buff%()
    p1% = LINSTR(buff%(), "GET")
    p2% = LINSTR(buff%(), "HTTP")
    IF (p1% <> 0) AND (p2% > p1%) THEN
      file$ = LCASE$(LGETSTR$(buff%(), p1% + 4, p2% - p1% - 5))
      IF file$ = "/" THEN file$ = "/index.html"
      IF NOT MM.INFO(EXISTS FILE file$) THEN WEB TRANSMIT CODE a%, 404 : CONTINUE FOR
      SELECT CASE RIGHT$(file$, 4)
        CASE "html", ".htm", ".txt"
          WEB TRANSMIT PAGE a%, file$
        CASE ".bmp", ".png", ".gif"
          WEB TRANSMIT FILE a%, file$, "image/" + RIGHT$(file$, 3)
        CASE ".jpg", "jpeg"
          WEB TRANSMIT FILE a%, file$, "image/jpeg"
        CASE ".ico"
          WEB TRANSMIT FILE a%, file$, "image/vnd.microsoft.icon"
        CASE ELSE
          WEB TRANSMIT CODE a%, 404
      END SELECT
    ENDIF
  NEXT a%
END SUB
```

The browser's names start with a /, which is the root of the current drive, so the files go in the root of the SD card (or of the drive that is current).

## A Typical Web Page

A page sent with WEB TRANSMIT PAGE is HTML. It can be one line of text with no formatting:

```
The temperature is {CurrentTemp}
```

or have some formatting:

```
<title>Colour Maximite 2</title>
<h2>Temperature Monitor</h2>
The temperature is {CurrentTemp}
```

A fuller page has head and body sections, with its text in paragraphs. This is the skeleton of one:

```
<html>
<head>
<title>Colour Maximite 2</title>
</head>
<body>
<h1>This is a heading</h1>
<br>
<p>The temperature is {CurrentTemp}</p>
</body>
</html>
```

There are many tutorials in HTML for beginners on the Internet (for example http://www.simplehtmlguide.com/), and many HTML editors.

## Input Fields and Control

A page can hold buttons, check boxes, radio buttons, text fields and so on, so that its user can switch things on and off or change settings from the browser. They go in an HTML form. In this example two radio buttons turn a device on and off:

```
<form method='get'>
  <input name='RB' type='radio' value='OFF' {offb$} onClick='this.form.submit()'> Off
  <input name='RB' type='radio' value='ON' {onb$} onClick='this.form.submit()'> On
</form>
```

offb$ and onb$ are BASIC variables, replaced by WEB TRANSMIT PAGE: an empty string shows the button unchecked, and "checked" shows it checked. The program should set them when it starts. A click on the first button makes the browser send GET /?RB=OFF HTTP, and on the second GET /?RB=ON HTTP, which the program acts on in its list of pages:

```
ELSEIF s$ = "/?RB=OFF" THEN
  ' <turn off the device>
  offb$ = "checked" : onb$ = ""
  WEB TRANSMIT PAGE a%, "index.html"
ELSEIF s$ = "/?RB=ON" THEN
  ' <turn on the device>
  offb$ = "" : onb$ = "checked"
  WEB TRANSMIT PAGE a%, "index.html"
```

It switches the device, sets onb$ and offb$ to match, and sends the page again. Forms can do much more: several inputs, text, lists, passwords and so on, sent with method='post' in the body of the request. Geoff Graham's Garden Watering Controller (https://geoffg.net/retic.html) is a full example, and runs on the Colour Maximite 2 with its pin numbers changed.

## Implementing a TCP Client

The Colour Maximite 2 can also ask other servers for data, as a TCP client. A connection is opened with:

```
WEB OPEN TCP CLIENT address$, port
```

'address$' is a name (such as "api.openweathermap.org") or an IP address, and 'port' is normally 80 for a web site. With the connection open, a request is sent and its answer read with:

```
WEB TCP CLIENT REQUEST request$, buff%() [, timeout]
```

The answer goes into the long string 'buff%()', whose size limits how much is kept; 'timeout' is in milliseconds. For a web site the request can be as simple as "GET / HTTP", which returns its default page; the program then picks out what it wants from the answer. The connection is closed with:

```
WEB CLOSE TCP CLIENT
```

Only one client connection can be open at a time. WEB TCP CLIENT READ reads what a server sends without sending a request first (such as a mail server's greeting), and WEB TCP CLIENT WRITE sends a whole long string at once, without waiting for an answer, which is much faster for a large amount of data.

This program gets the temperature in Paris from Open Weather Map. It needs a (free) account there (https://openweathermap.org), whose API key, 32 hex digits, goes in the first line:

```
CONST Key = "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
CONST Query = "GET /data/2.5/weather?q=Paris,fra&APPID=" + Key + CHR$(13) + CHR$(10)
DIM buff%(4096/8)
WEB OPEN TCP CLIENT "api.openweathermap.org", 80
WEB TCP CLIENT REQUEST Query, buff%()
WEB CLOSE TCP CLIENT
temp = VAL(JSON$(buff%(), "main.temp"))
PRINT "Current temperature in Paris is:" temp - 273
```

The function JSON$() picks the value out of the answer, which is in JSON (JavaScript Object Notation).

## Secure Connections (TLS)

The Colour Maximite 2 can connect to secure (https) servers with TLS (Transport Layer Security), with one of:

```
WEB OPEN TLS CLIENT address$, port [, timeout]
WEB OPEN TLS STREAM address$, port [, timeout]
```

They take the same parameters as WEB OPEN TCP CLIENT and WEB OPEN TCP STREAM, but the connection is encrypted; the port of a secure web site is normally 443. Once open the connection is used as a TCP connection, with WEB TCP CLIENT REQUEST and WEB CLOSE TCP CLIENT. The weather example above needs only one line changed:

```
WEB OPEN TLS CLIENT "api.openweathermap.org", 443
```

By default the connection is encrypted but the server's identity is not checked. To check it, load a bundle of trusted root certificates (in PEM format) from the SD card:

```
WEB TLS CA "A:/cacert.pem"
```

Every TLS connection after this needs a certificate signed by one of them, or it fails (with the error "TLS client error -15"). The check includes the certificate's dates, so the clock must be right first (WEB NTP). The usual bundle, cacert.pem, can be downloaded from https://curl.se/docs/caextract.html; loading it takes about a quarter of a second, and its certificates take about 480KB of the memory for variables. WEB TLS NOVERIFY turns the check off again and gives the memory back. The MQTT client uses TLS too when it connects to port 8883, the secure MQTT port.

## Using UDP

UDP (User Datagram Protocol) sends short messages between devices quickly. It does not promise that a message arrives, or that messages arrive in order, but it is often used where speed matters more. A UDP server is set up like the TCP server, with an option at the command prompt (which also restarts):

```
OPTION UDP SERVER PORT port_nbr
```

In the program, WEB UDP INTERRUPT names the subroutine to call when a message arrives. The sender's IP address is then in MM.ADDRESS$ and the message (up to 255 characters) in MM.MESSAGE$. A message is sent with:

```
WEB UDP SEND address$, port_nbr, message$
```

This example uses two Colour Maximite 2s (or one and a WebMite) on the same network, both with OPTION UDP SERVER PORT 77. On the first, note its IP address and run:

```
WEB UDP INTERRUPT myint
DO
  '<do some processing here>'
LOOP

SUB myint
  PRINT "Received " + MM.MESSAGE$ + " from " + MM.ADDRESS$
  PAUSE 100
  WEB UDP SEND MM.ADDRESS$, 77, "Maximite #1 message"
END SUB
```

On the second run this, with the first one's address:

```
WEB UDP INTERRUPT myint
WEB UDP SEND "192.168.1.127", 77, "Starting UDP echo"
DO
  '<do some processing here>'
LOOP

SUB myint
  PRINT "Received " + MM.MESSAGE$ + " from " + MM.ADDRESS$
  PAUSE 100
  WEB UDP SEND MM.ADDRESS$, 77, "Maximite #2 message"
END SUB
```

The two then play a fast game of ping-pong with their messages.

## Sending Emails

A remote Colour Maximite 2 can send an email to raise an alarm or report its state. It does so with the SMTP protocol, through a server that relays the email to its destination. This example uses SMTP2GO's free relay service (https://www.smtp2go.com/), which allows 1000 emails a month; it does not accept people with a generic free email address (such as a Gmail address). Create a login there, register a Verified Sender, and create a user name and password for it. The program sends them in Base64, which MATH(BASE64 ENCODE) makes:

```
CONST user$ = "MyUserName"
CONST paswd$ = "MyPassword"
CONST mailfrom$ = "from@server.com"
CONST mailto$ = "to@server.com"
CONST subject$ = "Test EMail"
CONST message$ = "Test of SMTP2GO"
CONST cr = CHR$(13) + CHR$(10)
DIM buff%(4096/8), body$, userB64$, paswdB64$, n
n = MATH(BASE64 ENCODE user$, userB64$)
n = MATH(BASE64 ENCODE paswd$, paswdB64$)
body$ = "From: " + mailfrom$ + cr + "To: " + mailto$ + cr
body$ = body$ + "Subject: " + subject$ + cr + cr
body$ = body$ + message$ + cr + "." + cr
WEB OPEN TCP CLIENT "mail.smtp2go.com", 2525
WEB TCP CLIENT READ buff%(), 30000
WEB TCP CLIENT REQUEST "EHLO maximite" + cr, buff%()
WEB TCP CLIENT REQUEST "AUTH LOGIN" + cr, buff%()
WEB TCP CLIENT REQUEST userB64$ + cr, buff%()
WEB TCP CLIENT REQUEST paswdB64$ + cr, buff%()
WEB TCP CLIENT REQUEST "MAIL FROM:<" + mailfrom$ + ">" + cr, buff%()
WEB TCP CLIENT REQUEST "RCPT TO:<" + mailto$ + ">" + cr, buff%()
WEB TCP CLIENT REQUEST "DATA" + cr, buff%()
WEB TCP CLIENT REQUEST body$, buff%()
WEB CLOSE TCP CLIENT
IF LINSTR(buff%(), "250") = 0 THEN
  PRINT "Email send failed"
ELSE
  PRINT "Email sent OK"
ENDIF
```

The program reads the server's greeting with WEB TCP CLIENT READ before it sends anything, as some servers refuse a client that speaks first. mailfrom$ must be the address registered as the Verified Sender, or SMTP2GO rejects the email. Other relay services can be used too, but each has its own variations on SMTP and the program must follow them. A file (such as a picture or a log) can be attached by sending the message in MIME multipart form, with the file in Base64, using WEB TCP CLIENT WRITE for the large part.

## MQTT Client

MQTT lets clients such as the Colour Maximite 2 post messages to a server (an MQTT broker) and receive the messages others post, rather like a bulletin board made for machines talking to machines. A Colour Maximite 2 might post a reading every few minutes for a program on a PC to collect and graph. There are many free brokers (search for "free MQTT broker"). These commands are used:

{widths: 35,65}
| Command | Use |
|---|---|
| WEB MQTT CONNECT | Connect to a broker, with a subroutine to call when a message arrives. |
| WEB MQTT PUBLISH | Post a message under a topic. |
| WEB MQTT SUBSCRIBE | Receive the messages posted under a topic. |
| WEB MQTT UNSUBSCRIBE | Stop receiving them. |
| WEB MQTT CLOSE | Close the connection. |

When a message arrives, its topic is in MM.TOPIC$ and its contents in MM.MESSAGE$. This program echoes every message posted under cmm2/in to cmm2/out:

```
WEB MQTT CONNECT "192.168.1.20", 1883, "", "", got
WEB MQTT SUBSCRIBE "cmm2/in"
DO : LOOP

SUB got
  WEB MQTT PUBLISH "cmm2/out", "echo: " + MM.MESSAGE$
END SUB
```

## Ping

The Colour Maximite 2 answers pings, so a PC can check that it is running and reachable; a service such as https://uptimerobot.com/ can raise the alarm if it stops. WEB PING pings another device and prints the results:

```
> WEB PING "192.168.1.1"
```

## Differences from the WebMite

A WebMite program runs on the Colour Maximite 2 as it is, apart from these:

- The I/O pins. A PicoMite's pin numbers are its own; GP names refer to the Colour Maximite 2's pins with the same functions (see the chapter Porting Programs), except GP10 and GP11, which are the WiFi module's, and GP22, pin 32 here. A program that uses those must use other pins.
- OPTION COUNT is accepted with OPTION PICOMITE only when it names the Colour Maximite 2's own counting pins, and WATCHDOG HW is the ordinary WATCHDOG with OPTION PICOMITE (see their entries).
- The network takes memory as described above, and is not serviced while a loadable program runs.
- There is no LED for the heartbeat.
- There is no PLAY STREAM (it needs a VS1053 chip), so WEB OPEN TCP STREAM and WEB TCP CLIENT STREAM fill their buffer, but nothing plays it.
- WEB TCP INTERRUPT 0, which the WebMite's manual gives for stopping the interrupt, is an error at present, on the WebMite too. WEB UDP INTERRUPT 0 does stop the UDP interrupt.
- A PicoMite's virtual displays (OPTION LCDPANEL VIRTUAL_C and VIRTUAL_M) are not needed: the Colour Maximite 2 draws on its own pages (see Live Graphics in a Web Page).
