# A web browser for the Colour Maximite 2: a design study

*27 November 2026. Whether a web browser can run on the CMM2 as a loadable program, which open-source code could
serve, and what such a browser could not do. A study only: nothing is built.*

*Peter's terms (27 Nov): the browser may ask the firmware for new services, and it may be for the G2 and G3 only,
which gives it their large memory.*

## The answer in brief

- **Yes, on a G2 or G3.** **NetSurf**, ported as a loadable program (`browser.app`), would fetch over the firmware's
  Wi-Fi with libcurl and mbedTLS, and run on top of the sockets programs already have (services version 21). It shows:
  - HTML5 pages laid out by CSS 2.1 with flexbox and media queries;
  - JPEG, PNG, GIF, WebP and SVG images;
  - forms, cookies and HTTPS;
  - anti-aliased text through FreeType.
- **It would show the readable web:** Wikipedia, documentation, forums, blogs and news articles. It would not run
  the web's applications.
- **The great limit is JavaScript,** which no small browser runs in earnest:
  - A 2023 study found 43% of pages work without it, and 67% are usable for their main content.
  - Google Search, YouTube, webmail, social media, most large sites' logins and Cloudflare's checks are out of reach.
  - A rendering proxy on a PC (WRP) can show those pages to the CMM2 as images or simplified HTML.
- **Other limits** (The limitations, below):
  - no CSS grid, custom properties or web fonts;
  - no video or audio;
  - no Chinese, Japanese, Korean, emoji or right-to-left text;
  - one page at a time;
  - a few seconds a page;
  - no protection from a malicious page.
- **The firmware:** nothing new is strictly needed. Two new services would lend the browser BASIC's free heap and the
  display pages it does not show, nearly 28 MB in all. TLS stays in the app.
- **NetSurf is the only candidate that fits.**
  - Links 2 ignores CSS, so today's pages come out as one long column.
  - litehtml is only a layout engine, and the rest of the browser would have to be written.
  - Dillo needs FLTK.
  - The text browsers waste a G2.
  - No port of any browser of this class to a microcontroller was found, so this would be the first.
- **The work:** the largest app yet (about 4 to 5 MB of code, against ScummVM's 3.2 MB), in seven steps. The first
  two settle whether it is worth going on: a `curl` for the CMM2, then NetSurf laying out pages with no screen, timed.

## What a program has on the CMM2 (V8)

Everything here is what a loadable program has today (services version 22, `Inc/cmm2img.h`), with the Wi-Fi module
and OPTION WIFI set.

| | G1 | G2 | G3 |
|---|---|---|---|
| Processor | STM32H743, 400 MHz | STM32H743, 480 MHz | As the G2 |
| Caches | 16 KB instructions, 16 KB data | The same | The same |
| Memory for a program with the network | The largest free part of BASIC's heap, about 4 MB (3 MB with OPTION COMPILE ON), code and data together | 8 MB at 0xD1800000 for code, data, heap and stack, and up to 64 blocks of BASIC's 15 MB heap through `alloc` | As the G2 |
| Fast memory | None free | 512 KB of AXI RAM and 24 KB of ITCM for hot code, when the display page is in the SDRAM (`--axi`, `--itcm`) | As the G2 |
| Display | Modes 1-15 and 17-19, depth 8 or 16 (15: 8 only) | As the G1, and MODE 16 (1920x1080, depth 8 only) | As the G2, on VGA and HDMI |
| Keyboard and mouse | USB (or PS/2) keyboard; the mouse with its wheel (services 4) | The same | The same |
| Files | The SD card (A:) and the USB drive (C:), eight files open at once | The same | The same |
| Network | Eight TCP/UDP sockets and DNS (services 21); no TLS service | The same | The same; a G3 V1.2 can have the radio fitted on the board |

Points that shape a browser:

- **The network is there, TLS is not.** `Src/net/LoaderNet.c` gives a program the firmware's lwIP as eight BSD-style
  handles, with DNS. The program pumps the network itself (`net_poll`, every few milliseconds). TLS is the program's
  own: MicroPython carries mbedTLS for its `tls` module, and a browser would do the same (mbedTLS 3.6.6, 166 KB of
  code with the firmware's settings).
- **What the network was measured at** (docs/Web_Plan.md, step 1, on the G3):
  - TCP: 2.1 MB/s from the CMM2 and 2.36 MB/s to it.
  - HTTPS: the handshake in 1.1 to 1.6 s (ECDSA certificate, TLS 1.2), and 1.13 MB/s of data decrypted on the CMM2.
  - An RSA chain with verification is still to be measured.
- **lwIP's limits for a program:**
  - The network block is 128 KB on every board. Data a program has not yet taken waits in it.
  - With a receive window of 23 KB (16 x 1,460), about four connections can be receiving at full speed at once.
  - DNS keeps one name (DNS_TABLE_SIZE 1), so a browser keeps its own table of names.
  - IPv4 only: lwIP is built without IPv6.
- **Why not the G1:** on a G1 the network block is the top 128 KB of the display memory, and a `--vram` program is
  lent that memory, so the network stops while it runs (WebLend, WebGiveBack). A G1 browser would have its borrowed
  heap only, about 4 MB for code and data together. The browser is for the G2 and G3 (built `--g2`).
- **A G2's 32 MB, and what a browser could have of it:**

  | Address | What | Size | For the browser |
  |---|---|---|---|
  | D0000000 | Display memory | 7 MB | The shown page (800x600 at 16 bits: 0.9 MB; 1280x720: 1.8 MB); the rest could be lent |
  | D0700000 | The compiler's stream | 1 MB | |
  | D0800000 | BASIC's heap (less the network's 128 KB) | 15 MB | `alloc` today: up to 64 blocks |
  | D1700000 | Executable window and program area | 1 MB | |
  | D1800000 | Loadable programs | 8 MB | Code, data, heap and stack |

  - **Today:** 8 MB, plus up to 64 blocks of the heap by `alloc`. That is enough for the browser's image cache and its
    pages, kept apart from its malloc.
  - **With two new services:** the free heap lent as one block (`heap2`, as a G1's `--vram` program is given its
    second block), and the display pages that are not shown: about 8 + 14.8 + 5 MB, **nearly 28 MB**.
- **Code runs from the SDRAM** through 16 KB caches (the guide's chapter Performance). A large program that jumps about,
  which a browser's layout and style code does, runs much of the time at the SDRAM's speed rather than the
  processor's. On a G2 or G3 the hottest code can go into the AXI RAM and the ITCM.
- **The display memory is not cached:** a browser draws into its own buffer and copies changed rectangles to the
  page (memcpy or `dma2d_copy`).
- **A fault restarts the CMM2,** as a fault in the firmware does. A program has the whole machine: no memory
  protection, no process to kill.
- **The C library:** newlib-nano, with no UTF-8 locale. Browsers do their own UTF-8, so this costs nothing.
- **C++ works:** ScummVM (3.2 MB) is the largest program so far.
- **The clock:** a certificate's dates are checked against the CMM2's clock, which WEB NTP or the DS3231 sets.
- **Already in the tree:**
  - mbedTLS, lwIP and the Wi-Fi driver (in the firmware; a program cannot call the firmware's mbedTLS).
  - picojpeg, upng, gifdec and puff (inflate) in the firmware, for LOAD JPG, PNG and GIF; not given to programs.
  - PDCursesMod with UTF-8 and the Spleen fonts (apps/pdcurses, for WordGrinder and sc-im).
  - libvterm (apps/vt100).

## What today's web asks for

From the HTTP Archive's Web Almanac (2025 Page Weight and Fonts; 2024 Media, Markup and HTTP; 2022 CSS, the last
CSS chapter), Qualys SSL Pulse (June 2025, 134,380 sites), and checks made from the PC on 27 Nov against 30 popular
sites:

| What | Figure | What it means here |
|---|---|---|
| A home page, median | 2.6 MB on mobile, 2.9 MB on desktop; 72 to 77 requests | 2 to 3 s at HTTPS's 1.1 MB/s |
| Of which JavaScript | 632 KB (mobile) to 697 KB, compressed; several times that unpacked; 98% of pages load some | Not run (see The limitations) |
| Images | 911 KB; the median image is 12 KB and about 240x240 | Within reach |
| Image formats, by request | JPEG 32%, PNG 28%, GIF 17%, WebP 12%, SVG 6%, ICO 1%, AVIF 1% | All but AVIF have small decoders |
| Elements in a page, median | 594 (mobile) | Small for NetSurf's DOM |
| CSS | Flexbox on 74% of pages, grid on 12%, custom properties (`var()`) on 43% | NetSurf has flexbox, not the other two |
| Web fonts | On 88% of pages | NetSurf ignores them: the page is drawn in the browser's fonts |
| HTTP/1.1 | 21% of home pages still use it; 29 of 30 sites answered an HTTP/1.1-only client | No HTTP/2 needed |
| Compression | Offered gzip only, no site sent Brotli or zstd | zlib is enough |
| TLS | 1.2: 99.96% of sites; 1.3: 75%; **1.3 only: none**. ECDHE 99.6%, P-256 98%, X25519 82%; certificates 78% RSA, 22% ECDSA | TLS 1.2 with ECDHE reaches every site today |
| Pages that need JavaScript | 43% of pages work without it; 67% are usable for their main content (the "Breaking Bad" study, ACM TOIT 2023, 6,384 pages) | A third of the web is out of reach, and much of what people use most |
| Bot walls | Cloudflare fronts 30% of sites (W3Techs, Nov 2026), and its challenges need JavaScript; 7 of 30 popular sites (Amazon, Reddit, LinkedIn, the New York Times, IMDb, Booking.com, raspberrypi.com) refused a plain client or sent a challenge page | Those sites stay shut |

## The open-source candidates

Researched 27 Nov 2026, from each project's own pages and source (the Links 2.30 tarball read in full; NetSurf's
docs and fetchers read on GitHub). Sizes are as installed on Linux, which includes resources.

| Candidate | Licence | Language | CSS | JavaScript | Size | Memory | Fit for the G2 |
|---|---|---|---|---|---|---|---|
| **NetSurf 3.11** (framebuffer front end) | GPL-2.0 only; its libraries MIT | C | Most of CSS 2.1, Media Queries 4, flexbox; no grid | Duktape 2.7, off by default, "primitive and incomplete" | About 4 MB | Atari: 32 MB minimum, 48 MB for demanding sites | **Best.** See below |
| **Links 2.30** (graphics mode) | GPL-2 | C | None ("HTML 4.0 without CSS") | None (removed in 2007) | 5.6 MB, 2.6 MB of it the built-in fonts as PNGs | Default caches 4 + 1 + 2 MB | Possible, but pages look as in 1999 |
| **litehtml 0.10** with gumbo | BSD-3; gumbo Apache-2.0 | C++17 | CSS 2.1, flexbox, `calc()`; grid in master since 11 Nov 2026, not released | None | Not published | Not published | An engine only: the fetcher, cache, forms, history and UI would all be ours |
| **Dillo 3.3** | GPL-3 | C and C++ | Part of CSS 2.1; no flexbox or grid | None | 2 MB | Low | No: it needs FLTK, which has no framebuffer driver, and runs `file:`, downloads and bookmarks as separate processes |
| **Lynx 2.9.3** | GPL-2 | C | None | None | 2 MB | Small | Text only; builds with PDCurses, which apps/pdcurses has |
| **ELinks 0.20** | GPL-2 | C | Its own, or NetSurf's libcss | SpiderMonkey, QuickJS-ng or MuJS | 1.8 MB without JS | Not published | Text only, needs a terminal; a DOS build exists |
| **w3m** | MIT-style | C | None | None | 3 MB | Not published | Text only; needs the Boehm garbage collector |
| **Tilefinch 0.1** | MIT | C11 | Flexbox and grid | QuickJS | About 10 MB | A 32 MB budget per page | Too big; on the PSP, with an operating system |
| **libwebsockets' HTML renderer** | MIT | C | A subset; CSS inline in the page only; no floats | None | Small | About 200 KB | Runs on an ESP32, but "pre-alpha" and too limited for real sites |
| **MicroWeb 2.1** | GPL-2 | C++, 16-bit | None | None | Small | 640 KB | HTTP only, GIF only: a DOS browser for an 8088 |
| **Arachne, Contiki's browser** | GPL-2, BSD-3 | C | CSS 1 subset, none | None | Small | Under 2 MB | Too old (16-bit DOS) or too small (text on one 80x20 page) |

No port of NetSurf, Links or litehtml to any microcontroller was found. The browsers that do run on
microcontrollers (on the ESP32: libwebsockets' renderer, Babe32) draw a text-like subset of a page.

### NetSurf

- **What it is:** a browser written for small machines: RISC OS first, then AmigaOS, Atari, Haiku, and a framebuffer
  front end that the Plan 9, KolibriOS and Nintendo 3DS ports start from. Single-threaded, plain C, no `fork`, no
  threads: work is done in callbacks that a scheduler runs, which is how a CMM2 program runs too.
- **Its own libraries** (MIT): hubbub (an HTML5 parser), libcss, libdom, libnsgif, libnsbmp, libparserutils,
  libwapcaplet, libnsfb (the framebuffer layer), libsvgtiny.
- **Optional:** libpng, libjpeg, libwebp, libjxl, FreeType, Duktape, utf8proc.
- **The front end** must give five tables of operations (the scheduler, windows, fetch support, bitmaps, and text
  measuring on UTF-8) and a table of plotters. The framebuffer front end gives all of them already, with its own
  small toolkit (fbtk): the toolbar, the URL bar, scroll bars, the status line, local history, find in page, a
  clipboard and an on-screen keyboard. Its defaults are 800x600 at 16 bits.
- **libnsfb's surfaces** are ram, sdl, x, vnc and wld. A new one supplies nine hooks (initialise, finalise, geometry,
  parameters, input, claim, update, cursor, defaults); the ram surface is 137 lines. A CMM2 surface is this size.
- **Fonts:** a built-in 8x16 bitmap font with Unicode sections, in regular, bold, italic and bold italic, scalable by
  two; or FreeType, with DejaVu by default, anti-aliased.
- **The fetcher is the one gap.** The only HTTP fetcher in NetSurf is libcurl's, through `curl_multi`. libcurl can be
  built with mbedTLS, and it wants BSD sockets and `select()`. The firmware's lwIP has no socket layer (it is built
  without an operating system), but services version 21 give handles that a small socket layer can sit on, as
  MicroPython's `socket` module does (modsocket_cmm2.c).
- **Not supported by NetSurf on any machine:** CSS grid (`display: grid` is laid out as a block), web fonts (libcss
  parses `@font-face`, but NetSurf never uses it), video and audio, and most of the JavaScript DOM
  (docs/UnimplementedJavascript.md lists hundreds of missing bindings).
- **Releases:** 3.11 (28 Dec 2023) is the latest; master is active (commits on 18 Nov 2026).

### Why not the others

- **Links 2:** the nearest to a no-OS port (its DOS build emulates `select()` and runs its "threads" in line, and a
  new graphics driver is about 30 hooks, 577 lines in the DOS one), but it ignores CSS altogether. Today's pages are
  laid out by CSS, so most would come out as one long column.
- **litehtml:** a good layout engine with flexbox, and smaller than NetSurf, but only a layout engine. Everything
  NetSurf's framebuffer front end and its cache already do would have to be written. It is the fallback if NetSurf
  turns out too slow or too large.
- **The text browsers:** useful, and Lynx would build on apps/pdcurses, but a G2 can do much better.

### A rendering proxy, alongside

A browser on the CMM2 cannot run the JavaScript most sites now need. A proxy on a PC can:

- **WRP, the Web Rendering Proxy** (Apache-2.0, Go; 4.14.3): runs Chrome headless on a PC. It sends a page as a GIF,
  PNG or JPEG with a clickable image map, or as simplified HTML. NetSurf would use it as an ordinary HTTP proxy, with no
  code on the CMM2.
- **Archaic** (Apache-2.0, C): WRP's own small native client, with its own image formats and keys sent as typed.
  It has been ported to SunOS, Mac System 6 and 7, FreeMiNT and Idris. A CMM2 port would be a small app of its own.
- **FrogFind** and **68k.news:** public servers that turn search results and news into plain HTML, over plain HTTP.

## The design: NetSurf as `browser.app`, G2 and G3

### The parts

```
 NetSurf core (HTML, CSS, layout, cache, cookies, history)   GPL-2.0, unchanged
   |-- framebuffer front end (fbtk: toolbar, URL bar, scroll bars)   unchanged
   |     `-- libnsfb + a new "cmm2" surface ----- video_mode, dma2d_copy, keys, mouse
   |-- fonts: FreeType + DejaVu (the built-in 8x16 font as fallback)
   |-- images: libnsgif, libnsbmp, libpng, libjpeg-turbo, libwebp, libsvgtiny
   `-- fetchers: about, data, file, resource (NetSurf's own)
         `-- curl: libcurl (HTTP/1.1, gzip, redirects, proxies)
               |-- mbedTLS 3.6 (TLS 1.2; 1.3 if it is cheap), CA bundle on the card
               `-- a socket layer: socket, connect, send, recv, select, getaddrinfo
                     `-- services 21: sock_*, net_dns, net_poll ----- the firmware's lwIP and Wi-Fi
```

Everything above the services is the app's; the firmware gives only services. Each third-party library is kept as it
is, with any change as a patch in the app's folder, as apps/wordgrinder and apps/amiga do.

### The fetcher: libcurl over a socket layer

- **libcurl, not a fetcher of our own:** NetSurf's curl fetcher is its tested path for HTTP, HTTPS, redirects,
  chunked transfers, compression, keep-alive, proxies and POSTed forms. A fetcher of our own would have to do all of
  that again. libcurl builds with mbedTLS and zlib, without threads (no threaded resolver).
- **The socket layer:** a few hundred lines in the app, mapping `socket`, `connect`, `send`, `recv`, `close`,
  `getaddrinfo` and `select` onto the eight handles:
  - `select` loops on `net_poll` and `sock_state` until a handle is ready or the time is up.
  - `getaddrinfo` calls `net_dns` and `net_poll` until the answer comes, and keeps the names it has learnt, since the
    firmware's lwIP keeps only one.
  - Non-blocking connects map onto `sock_connect` and `sock_state`, which already work that way.
- **How many at once:** eight handles, and lwIP's 128 KB block holds about four connections receiving at full speed.
  NetSurf's `max_fetchers` (24 by default) goes down to 6, and `max_fetchers_per_host` (5) to 4. At 2 MB/s on the
  Wi-Fi link, more would not be faster.
- **Pumping:** the front end's main loop calls `net_poll` every pass, and the socket layer calls it while it waits,
  so the network is pumped every few milliseconds, as services 21 require.

### TLS

- **mbedTLS 3.6 in the app,** as MicroPython carries it, and as wifi.app measured it (handshake 1.1 to 1.6 s, 1.13
  MB/s), driven by libcurl's own mbedTLS back end.
- **TLS 1.2 is enough today:** SSL Pulse found no site among 134,380 that offers TLS 1.3 alone, and every one of the
  30 sites tried from the PC finished a TLS 1.2-only handshake. TLS 1.2 is the configuration wifi.app has already run.
  mbedTLS 3.6 has TLS 1.3 on by default, so step 1 tries it as well and keeps it if its cost is small.
- **mbedTLS is dual-licensed** (Apache-2.0 or GPL-2.0-or-later), so it can be linked with NetSurf under the GPL.
- **The CA bundle:** curl's bundle (cacert.pem) on the card, read once at start-up (about 300 KB parsed, step 1) and
  given to every connection through `CURLOPT_SSL_CTX_FUNCTION`, which curl's mbedTLS back end calls after it sets up a
  connection. Left to itself, that back end reads the whole bundle again for every connection: 0.6 to 0.8 s each,
  most of a handshake's time (step 1).
- **The clock:** certificates are checked against the date, so the CMM2's clock must be right: WEB NTP, or a DS3231.
  The browser says so when the year is before 2026, rather than refusing every site.
- **Memory for each connection:** mbedTLS's input buffer has to stay 16 KB. A smaller one needs the server to agree
  to the Max Fragment Length extension, and only 7 of 30 sites did. That makes about 40 KB a connection, and 240 KB for
  six.
- **A firmware TLS service instead** (TLS on the sockets, over the firmware's own altcp_tls: LoaderNet already opens
  its connections through altcp, so the firmware's side would be small) was weighed. It would save the app 166 KB of
  code, and the firmware's TLS 1.2 is enough. But:
  - libcurl drives TLS itself, through its back ends: the host name for SNI, certificate errors to show the user,
    sessions kept for the next connection. A TLS socket underneath would hide all of that from it.
  - The firmware's mbedTLS takes its memory from MMBasic's heap, which the browser wants to borrow.

  The app's own copy is simpler, and the firmware stays as it is.

### New firmware services (services version 23)

Peter's terms allow new services. The browser needs none of these to work, but the first two give it room:

1. **The free heap lent as one block** (an image flag, `--bigheap`, as `--vram` is one): on a G2 or G3, the loader
   gives the program BASIC's free heap as `heap2`, keeping back a reserve, as it lends a G1's. Without it the browser
   can still reach the heap through `alloc`, one block at a time (up to 64), with its allocator taking blocks of
   1 MB as it grows (dlmalloc's `mmap` hook): workable, but one block is simpler and wastes nothing.
2. **The display pages not shown** lent as well (the `--vram` flag, which a G2 ignores today): 7 MB of display
   memory less the page shown, about 5 MB more at 800x600 or 1280x720.
3. **Optional, after measuring:** DMA2D fills and alpha blends (`dma2d_fill`, `dma2d_blend`), if plotting
   rectangles and anti-aliased text in software shows up in the timings. The DMA2D can blend an 8-bit alpha mask in
   one colour onto RGB565, which is what drawing a glyph is.

Nothing else is missing: keys, the mouse and its wheel, files, the clock and the network are all there.

### Memory on a G2 or G3

| What | Size | Where |
|---|---|---|
| Code: NetSurf and its libraries, libcurl, mbedTLS, FreeType, the image libraries | About 4 to 5 MB (NetSurf's RISC OS binary is 4.0 MB; measured in step 1) | The 8 MB area |
| Stack | 256 KB | The 8 MB area |
| The rest of the 8 MB | About 3 MB | malloc's first arena |
| BASIC's free heap | About 14.8 MB | `--bigheap`, or `alloc` |
| The display pages not shown | About 5 MB | `--vram` on a G2 |
| **For pages, images and caches** | **About 22 MB** | |

- **NetSurf's memory cache,** 12 MB by default (`memory_cache_size`), set to 6 MB to start with.
- **No disc cache at first:** NetSurf's backing store would write the card on every page. It can come later, to a
  folder of its own.
- **Images are what fill memory:** NetSurf keeps a decoded image at 4 bytes a pixel, so a 4000 x 3000 photograph
  would be 48 MB. libjpeg decodes at 1/2, 1/4 or 1/8 of the size, and the app's JPEG handler (a patch) asks for the
  largest that fits 2048 pixels. PNG, GIF and WebP cannot be decoded smaller: past a limit they are shown as a box,
  as when an image fails.
- **Running out:** a fault restarts the CMM2, so the browser checks every allocation, as NetSurf does, and shows "Not
  enough memory" for the page rather than going on.

### The screen

- **The mode:** 800x600 (MODE 1), 1024x768 (MODE 9) or 1280x720 (MODE 11), all at 16 bits (RGB565, which libnsfb
  has). 1920x1080 is 8-bit only and stays out. The G3's HDMI has the same modes.
- **The cmm2 surface:** NetSurf draws into a buffer in the program's memory (as the ram surface does), and `update`
  copies the changed rectangle to the page with `dma2d_copy`, since the page is not cached.
- **The pointer:** libnsfb draws its own.

### Fonts

- **FreeType with DejaVu** (Sans, Serif, Sans Mono, each in four styles), anti-aliased, from A:/browser/fonts. About
  5 MB of files, read into memory once: FreeType's glyph cache then does the rest.
- **The built-in 8x16 font** if the fonts are missing, so the browser always starts.
- **The language a font covers is the limit:** DejaVu has Latin, Greek, Cyrillic, Armenian, Georgian and more, but no
  Chinese, Japanese or Korean, and no emoji. A CJK font (Noto Sans CJK, about 16 MB a weight) does not fit in memory.

### Keys and mouse

- The framebuffer front end's own keys: arrows and Page Up/Down to scroll, Tab through links and fields, Enter, the
  URL bar.
- The mouse: click, and the wheel to scroll (libnsfb's buttons 4 and 5, which the front end handles).
- Ctrl-C does not end the browser (`break_take`): a key in the toolbar does.

### Files

- **Downloads** to A:/downloads, through the SDK's `open` and `write` (the card's long names are kept).
- **Bookmarks, cookies and settings** in A:/browser, as NetSurf writes them on any machine.
- **file:** URLs open the card's own HTML files.

### Speed

- **The machine NetSurf asks for:** its Atari minimum is a 32 MHz 68030, with 60 MHz recommended. Even running from
  the SDRAM through its 16 KB caches, a 480 MHz M7 is many times that.
- **What will take the time,** to be measured in step 2 rather than guessed:
  - the network: about 0.25 s for each new TLS connection with the CA bundle read once (step 1), then up to 1.1
    MB/s;
  - laying out a long page, and restyling it on every resize or image arriving;
  - decoding large JPEGs and PNGs.
- **If the layout is slow:** the hottest code (the plotters, FreeType's rasteriser, inflate, the JPEG IDCT) goes into
  the AXI RAM and the ITCM (`--axi`, `--itcm`), as the Amiga emulator's does. The AXI RAM is free while the page is
  in the SDRAM, which it is at 16 bits.

## The limitations

In three groups: what no small browser can do, what NetSurf does only in part, and what the CMM2 itself sets.

### What it cannot do

**JavaScript.** This is the limit that matters most, and no amount of porting removes it.
- **What it shuts out:**
  - web applications: Gmail, Google Docs, YouTube, maps, and the social sites;
  - Google Search, which has needed JavaScript since January 2025;
  - most logins of the large sites, two-factor prompts, passkeys and "sign in with" buttons;
  - Cloudflare's checks and captchas (Cloudflare fronts 30% of sites);
  - menus that open by script, "load more" and endless scrolling, and images that a script loads (`data-src`);
  - cookie banners that only a script can close. Some cover the page and stay.
- **Why an engine is not the answer:** the engines are small enough (on the M7, Duktape is 153 KB and QuickJS about
  394 KB; Fabrice Bellard's MicroQuickJS is about 100 KB and runs in 10 KB). What a page's scripts need besides is
  the rest of the web platform: the whole DOM, events, timers, `fetch` and XMLHttpRequest, and layout read back by
  script. That is what Firefox, Chrome and Ladybird are made of. NetSurf has Duktape, off by default, with hundreds
  of DOM bindings missing.
- **And the cost:**
  - Lightpanda, a cut-down headless browser that runs scripts, peaks at 21 MB on a simple shop page.
  - The median mobile page holds a phone's processor for 1.8 s (Lighthouse's Total Blocking Time, 2025), on a
    processor many times the CMM2's.
  - The median page's 0.6 to 0.7 MB of compressed script would be several megabytes of source to compile and run
    from the SDRAM.
- **What is left:** 43% of pages work without JavaScript, and 67% are usable for their main content (the 2023
  study). That is the readable web:
  - Wikipedia, documentation, blogs, forums (The Back Shed among them), news articles and their text editions
    (lite.cnn.com, text.npr.org);
  - search through DuckDuckGo's HTML edition (html.duckduckgo.com) or FrogFind;
  - pages served by a CMM2 or a WebMite.
- **The way round it:** the WRP proxy on a PC (A rendering proxy, above). Chrome runs the scripts there, and the
  CMM2 shows the result as an image map or simplified HTML.

**Video and audio.** `<video>` and `<audio>` are not supported: H.264, VP9 and AV1 are far beyond the processor,
and the players are JavaScript anyway. Links to MP3 and MOD files can be downloaded and played from BASIC.

**The rest of the platform:** canvas, WebGL, WebAssembly, WebRTC and WebSockets (all reached through scripts),
showing PDFs in the browser (they are downloaded instead), extensions, printing, and saved passwords.

**IPv6:** lwIP is built for IPv4 only, so a site with only an IPv6 address cannot be reached. Such sites are rare.

### What NetSurf does only in part

| Feature | NetSurf | What the user sees |
|---|---|---|
| CSS layout | Most of CSS 2.1, Media Queries 4, flexbox (on 74% of pages) | Most pages laid out as meant. At 800x600 the media queries pick a site's tablet or phone layout, which suits the screen |
| CSS grid (12% of pages) | Laid out as blocks | Those parts come out as one column, all there but long |
| Custom properties, `var()` (43% of pages) | Not parsed by libcss | Colours, spacing and sizes set that way fall back to their defaults |
| Transforms, transitions, animations | Not supported | Still pages; animated GIFs do move |
| Web fonts (88% of pages) | Not loaded | Text in DejaVu, readable. **Icon fonts** (Font Awesome, Material Icons) show as letters, words or boxes on buttons and menus |
| Images | JPEG (progressive too), PNG, GIF, WebP, BMP, ICO; SVG Tiny through libsvgtiny | About 97% of images by request. Not AVIF (1%). Very large images shrunk (JPEG) or left out |
| Forms | GET and POST, text, passwords, check boxes, lists | Searches, comments and simple logins work. Checks that scripts make do not run; date and colour fields are plain text |
| Cookies | Kept and saved | Sessions survive between pages and runs |
| Writing systems | UTF-8 throughout, but no text shaping and no right-to-left layout | European scripts, Greek and Cyrillic are good. Arabic, Hebrew and the Indian scripts come out wrong; Chinese, Japanese, Korean and emoji show as boxes (no font fits in memory) |
| Windows | One window | No tabs: one page at a time, with Back and Forward |

### What the CMM2 sets

- **Speed** (the network measured in step 1; the rest estimates until step 2 measures it):
  - Each new site costs a TLS handshake of about 0.25 s (0.13 to 0.24 s measured). Pages from the same site reuse
    the connection.
  - Transfer is 0.4 to 1.1 MB/s over HTTPS, as the Wi-Fi link allows. A median page less its scripts and fonts,
    which the browser need not fetch, is about 2 MB, so 2 to 5 s.
  - Then the layout and the images: probably a few seconds for a long page, less for a short one.
  - So a median page might take 3 to 10 s, and a light one (Wikipedia, a forum thread) a second or two. Scrolling
    a page already laid out is quick: it is a copy of the picture.
- **Memory:** about 22 MB for pages and images. Enough for any one ordinary page; a page of many large photographs
  reaches the image limits, and some of its images are left out.
- **Connections:** six fetches at once (eight sockets, and lwIP's 128 KB). A page of a hundred small files loads in
  rounds.
- **The screen:** 1280x720 at most, at 16 bits. 1920x1080 is 8-bit only on the CMM2, too few colours for photographs
  and anti-aliased text.
- **Machines:** a G2 or G3 with V8, the Wi-Fi module (or a G3 V1.2 with the radio fitted) and OPTION WIFI. Not a
  G1.
- **Keyboard:** the layouts OPTION USBKEYBOARD gives. No input of other alphabets.
- **Bot checks by the TLS handshake:** some services (Cloudflare's bot management among them) recognise browsers by
  their TLS hello. mbedTLS's is not a common browser's, and is likely to be challenged more often. That is a guess,
  to be seen in use.
- **Security:**
  - The browser parses what strangers send, with the whole machine in its hands: no memory protection, and the card
    and the flash within reach.
  - A fault restarts the CMM2, and a page made to exploit a fault in a parser could run its own code there.
  - NetSurf is careful code, but it is not hardened as Chrome or Firefox are.
  - Certificates are only as current as the bundle on the card.
  - It is a browser for reading, not for banking.
- **The clock:** with the wrong date, every certificate fails. WEB NTP at start-up, or a DS3231, sees to it.

## Steps, if Peter goes ahead

Each step is a program that runs on a G2 or G3 and shows something, so the hard parts are met first.

1. **curl.app: libcurl, mbedTLS and the socket layer,** as a command-line `curl` (`RUN "curl.app", url`). It
   saves the page to the card and prints the times. **Done when:**
   - HTTPS pages from a dozen large sites come back whole, gzip included, with certificates verified;
   - the handshake is timed for ECDSA and RSA chains, TLS 1.2 and 1.3;
   - four downloads at once reach about 1 MB/s together;
   - the code's size is known.
2. **NetSurf with no screen:** the core and its libraries built for the CMM2 with the ram surface, laying out a
   page fetched by step 1's code and saving the picture as a PNG. **Done when:**
   - the size of the whole app is known, and fits the 8 MB with room;
   - pages from the step's list lay out, and the time and memory for each are measured.
   This is where NetSurf is kept or given up for litehtml.
3. **The cmm2 surface:** the page on the screen, the keyboard and the mouse; the toolbar and URL bar; scrolling.
4. **Fonts:** FreeType and DejaVu; the built-in font when they are missing.
5. **Memory:** services version 23 (`--bigheap`, `--vram` on a G2) in the firmware, and the browser's allocator
   over them; the image limits.
6. **The rest:** downloads, bookmarks, cookies, settings, the proxy option for WRP; the clock warning; tuning
   (`--axi`, `--itcm`) if step 2's times call for it.
7. **The App User Guide:** a chapter for the browser, with what it cannot do.

### Step 1 done (27 Nov 2026): curl.app

**What it is:** curl 8.23.0's own command-line tool with libcurl, mbedTLS 3.6.8 and zlib 1.3.1, all built as
released (apps/curl/make.py fetches them, SHA-256 checked). Built as fetch.app in apps/browser, it became curl.app
in apps/curl the same day, a program of its own for the app library (Peter), with its chapter in the App User Guide
(docs/apps/text/14_curl.md). The CMM2's side is all in apps/curl, which the browser builds on:
- `port/cmm2_net.c`: BSD sockets, `select` and `getaddrinfo` on services version 21;
- `port/cmm2_tls.c`: the RNG for mbedTLS's entropy;
- `port/curl_config.h` and `port/mbedtls_user_config.h`;
- `port/cmm2_stdio.c`: 16 KB file buffers;
- `curl_main.c`: cacert.pem beside the app; a word about the clock when a certificate is refused.

It is a full curl for HTTP, HTTPS and `file:`. Left out: FTP and the other protocols, HTTP/2, IPv6, Brotli,
international domain names. Run on the G3 (V8 8.000005, its Wi-Fi module, COM50), with curl's CA bundle beside it:

```
RUN "A:/curl/curl.app", -o page.html https://en.wikipedia.org/wiki/Main_Page
```

**The step's four tests:**

| Test | Result |
|---|---|
| HTTPS from a dozen large sites, gzip, certificates verified | **Met.** 17 sites, every certificate verified. 15 answered 200; raspberrypi.com (403) and DuckDuckGo's HTML search (202) were bot walls, as the study expects. Two files compared byte for byte with the PC's copies: one sent gzip-compressed (10,490 bytes on the wire, 29,640 decoded) and the 200 KB CA bundle |
| Handshakes, ECDSA and RSA, TLS 1.2 and 1.3 | **Met.** Every site chose TLS 1.3 by itself; TLS 1.2 forced works too. ECDSA P-256 and RSA 2048 and 4096 chains all verify. The handshake itself: 0.13 s (debian.org, RSA 4096) to 0.24 s (Google). Reading the 118-certificate bundle adds 0.6 to 0.8 s for every connection (see TLS, above) |
| Four downloads at once, about 1 MB/s | **Nearly met:** 8 MB in 9.3 s, 860 KB/s together. One download alone reached 1.14 MB/s at best, as wifi.app did on 15 Nov, and 0.4 MB/s at worst |
| The code's size | 828 KB: libcurl 346 KB, mbedTLS 228 KB (with TLS 1.3), curl's manual 87 KB, curl's tool 84 KB, the C library 51 KB, zlib 17 KB, the CMM2's code 5 KB |

**Also measured:**
- **Memory:** a verified HTTPS fetch peaks at 467 KB of heap, 166 KB without the bundle, so the parsed bundle is
  about 300 KB. Four connections at once: 1.3 MB, as each reads its own bundle. Everything is freed at the end.
- **Ctrl-C** ends a download in 0.12 s (exit code 130).
- **The Wi-Fi link** decided most of the speed on the day: 5 to 37 ms to the PC with up to 5% loss, against 1 to 6 ms
  on 15 Nov.
  - `RUN "curl.app", --cmm2-bench ip port /path` times a plain HTTP download on the services alone: 750 to 870 KB/s
    from the PC; through port/cmm2_net.c, 720 KB/s.
  - curl over plain HTTP got 465 KB/s at the same time: curl's own work for each read, from the SDRAM. That is
    tuning for later.
- **Fixed on the way, in the socket layer:**
  - The network was pumped only while waiting, so data in hand starved the radio. It is now pumped at least every
    250 us.
  - USB was serviced every 2 ms; it is now every 20 ms, ample for Ctrl-C.
- **Writing to the card** is the card's speed, not the network's: 123 KB/s for a download written to A:, against
  190 KB/s for BASIC writing 2 MB to the same card (nearly full: 6.8 MB free).
- **www.gnu.org** stalled once over TLS 1.2: 24 KB of about 31 KB, then curl's error 56 after 36 s. After that it
  refused connections from the PC (IPv4) as well as the G3, which share one public address. That is gnu.org's
  blocking, not the CMM2's; to try again another day.

**Optional afterwards:**
- JavaScript through NetSurf's Duktape, off by default, for the few pages its small DOM can serve.
- An Archaic client for WRP, as a small app of its own.
- Lynx on apps/pdcurses, for a G1.

## Decisions for Peter

- **Go or not:** decided 27 Nov, go. Step 1 is done, and curl.app is a program of its own.
- **TLS in the app** (recommended: libcurl drives it, and the firmware is unchanged) or a TLS service in the
  firmware.
- **Services version 23:** `--bigheap` and `--vram` on a G2, or the browser making do with `alloc`.
- **Fonts on the card:** DejaVu is free to give away (the Bitstream Vera licence); the app's zip could carry it, as
  the RC zips carry the shareware games' files.
- **The licence:** NetSurf is GPL-2.0 only, so the app as a whole is GPL-2.0, and its source is published with it,
  as the other apps' is.

## Risks

- **Speed from the SDRAM:** a browser's code is large and jumps about, the worst case for 16 KB caches. Step 2
  measures it before anything else is built on it.
- **Code size:** if NetSurf and its libraries come to much more than 5 MB, the 8 MB area is tight. The image
  libraries are the first to trim (WebP, SVG).
- **Memory faults:** NetSurf checks its allocations, but libraries under it may not; a fault restarts the CMM2.
- **mbedTLS with TLS 1.3** needs its PSA crypto layer: more code, and a configuration not yet tried on the CMM2. It
  is not on the critical path: TLS 1.2 reaches every site today.
- **libcurl with no operating system:** it expects `select()`, `getaddrinfo()` and non-blocking sockets. The socket
  layer gives them; anything else it expects (signals, `alarm`) is turned off in its build.
