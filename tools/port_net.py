"""port_net.py PICOMITE_DIR CMM2_DIR - the PicoMite's network code copied into the CMM2 (docs/Web_Plan.md, step 2;
Peter: port PicoMite code intact).

  PicoMite net/*.c, net/WiFi.h             -> Src/net/picomite/
  PicoMite third_party_mod/mqtt.c, tftp.c  -> Src/net/picomite/  (lwIP's MQTT client and TFTP server as the PicoMite
                                                                   changed them)
  PicoMite net/lwipopts_examples_common.h,
           net/mbedtls_config.h            -> Inc/net/  (as they are)
  PicoMite net/lwipopts.h                  -> Inc/net/lwipopts.h, with the CMM2's section added at its end

Mechanical changes only, so that a later PicoMite's network code can be taken by running this again:
  - SaveOptions() -> SaveOptions(1), SoftReset(SOFT_RESET) -> SoftReset(): the CMM2's signatures (a macro cannot map
    them: it would meet the CMM2's own prototypes too)
  - routinechecks() -> routinechecks(0): the CMM2's keyboard and USB part only. Its full checks (1) run ProcessWeb, and
    the call is in an lwIP callback, inside ProcessWeb
  - MMtcpserver.c, WEB TRANSMIT FILE's end-of-file test: its LittleFS half (the PicoMite's flash drive) left out, as the
    CMM2's drives are all FatFS: "(flash file && at its end) || (not flash file && f_eof)" -> f_eof
  - lwipopts.h: its "extern char MMErrMsg[];" left out (the CMM2's is a char *; the line is not used there)
Everything else the PicoMite's own headers and core give these files comes from Inc/net/WebMite_cmm2.h, which
CMakeLists.txt makes their first include (-include). Then build, and check on the board.
"""
import os
import re
import sys

pico, cmm2 = sys.argv[1], sys.argv[2]


def rd(path):
    return open(path, newline='').read()


def wr(path, text):
    open(path, 'w', newline='').write(text)


dst = os.path.join(cmm2, 'Src', 'net', 'picomite')
os.makedirs(dst, exist_ok=True)
files = [os.path.join('net', f) for f in sorted(os.listdir(os.path.join(pico, 'net')))
         if f.endswith('.c') or f == 'WiFi.h']
files += [os.path.join('third_party_mod', 'mqtt.c'), os.path.join('third_party_mod', 'tftp.c')]
REWRITES = [('SaveOptions()', 'SaveOptions(1)'), ('SoftReset(SOFT_RESET)', 'SoftReset()'), ('routinechecks()', 'routinechecks(0)')]
LFS_EOF = re.compile(r'\(\(filesource\[fn\] == FLASHFILE\) && \(lfs_file_tell\(&lfs, FileTable\[fn\]\.lfsptr\) == '
                     r'lfs_file_size\(&lfs, FileTable\[fn\]\.lfsptr\)\)\) \|\|\s*'
                     r'\(\(filesource\[fn\] != FLASHFILE\) && (f_eof\(FileTable\[fn\]\.fptr\))\)')
for f in files:
    t = rd(os.path.join(pico, f))
    n = []
    t, k = LFS_EOF.subn(lambda m: m.group(1), t)
    if k:
        n.append('LittleFS end-of-file test x%d' % k)
    for a, b in REWRITES:
        if a in t:
            n.append('%s x%d' % (a, t.count(a)))
            t = t.replace(a, b)
    wr(os.path.join(dst, os.path.basename(f)), t)
    print(f, ('(' + ', '.join(n) + ')') if n else '')

inc = os.path.join(cmm2, 'Inc', 'net')
for f in ('lwipopts_examples_common.h', 'mbedtls_config.h'):
    wr(os.path.join(inc, f), rd(os.path.join(pico, 'net', f)))
    print('net/' + f)

CMM2_SECTION = """
/* ---- The Colour Maximite 2 (docs/Web_Plan.md, step 2): what follows is the CMM2's; the PicoMite's settings above are
   as they are (tools/port_net.py), less their "extern char MMErrMsg[]", which the CMM2 declares otherwise. ----
   lwIP's heap is lwIP's own (mem.c), laid over the network block (NetBlock.h: 128 KB, taken at start-up only with
   OPTION WIFI: the top of the display memory on a G1, the top of the heap on a G2 or G3), and its pools come from it
   (MEMP_MEM_MALLOC). On the PicoMite they are the C library's heap; on the CMM2 that heap (newlib's) grows up from the
   bottom of MMBasic's SDRAM heap, which a G1 lends to its loadable programs, so the network keeps out of it. mbedTLS
   still takes MMBasic's heap through CallocMemory, as on the PicoMite (mbedtls_config.h). */
#include "NetBlock.h"
#undef MEM_LIBC_MALLOC
#define MEM_LIBC_MALLOC 0
#define MEMP_MEM_MALLOC 1
#define LWIP_RAM_HEAP_POINTER NetBlock
#undef MEM_SIZE
#define MEM_SIZE (NET_BLOCK_SIZE - 64)
/* The receive window must hold a whole TLS record: altcp_tls_mbedtls.c opens the window again only for what mbedTLS
   has decrypted, and mbedTLS decrypts only whole records, up to 16,413 bytes on the wire. The PicoMite's 8 x TCP_MSS
   stalls a TLS download for good at the server's first full-size record (PicoMite issue 153). */
#undef TCP_WND
#define TCP_WND (16 * TCP_MSS)
"""
t = rd(os.path.join(pico, 'net', 'lwipopts.h'))
nl = '\r\n' if '\r\n' in t else '\n'
line = 'extern char MMErrMsg[];' + nl
assert t.count(line) == 1, 'lwipopts.h: its MMErrMsg line has changed'
t = t.replace(line, '')
i = t.rindex('#endif')
t = t[:i] + CMM2_SECTION.replace('\n', nl) + t[i:]
wr(os.path.join(inc, 'lwipopts.h'), t)
print('net/lwipopts.h (with the CMM2 section)')
