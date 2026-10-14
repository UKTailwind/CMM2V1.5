# zmodem_pc.py: a small ZMODEM sender and receiver for testing the CMM2's ZMODEM command from a PC.
#
#   zmodem_pc.py send PORT BAUD FILE [FILE ...]     the board runs  ZMODEM RECEIVE [name$ | "dir/*"]
#   zmodem_pc.py recv PORT BAUD OUTDIR              the board runs  ZMODEM SEND [name$]
#
# It speaks the subset the firmware does: hex headers for ZRQINIT/ZRINIT/ZFIN/ZRPOS/ZACK, CRC-32 binary
# headers (ZBIN32) for ZFILE/ZDATA/ZEOF, 1024-byte data subpackets, ZDLE escaping as lrzsz does it.
# The sender pauses for a ZACK (ZCRCW) every --window bytes (default 32 KB, the firmware's buffer) to exercise the receiver's
# write path, and goes back to the receiver's position on a ZRPOS.  Exit status 0 on success.
import sys, time, zlib, os, serial

ZPAD, ZDLE, ZBIN, ZHEX, ZBIN32, XON = 0x2A, 0x18, ord('A'), ord('B'), ord('C'), 0x11
ZRQINIT, ZRINIT, ZSINIT, ZACK, ZFILE, ZSKIP, ZNAK, ZABORT, ZFIN, ZRPOS, ZDATA, ZEOF, ZFERR = range(13)
ZCRCE, ZCRCG, ZCRCQ, ZCRCW, ZRUB0, ZRUB1 = ord('h'), ord('i'), ord('j'), ord('k'), ord('l'), ord('m')
CANFDX, CANFC32 = 0x01, 0x20
NAMES = {0:'ZRQINIT',1:'ZRINIT',2:'ZSINIT',3:'ZACK',4:'ZFILE',5:'ZSKIP',6:'ZNAK',7:'ZABORT',8:'ZFIN',9:'ZRPOS',10:'ZDATA',11:'ZEOF',12:'ZFERR'}
verbose = '-v' in sys.argv
def log(*a):
    if verbose: print(*a, file=sys.stderr)

def crc16(data, crc=0):
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def crc32raw(data, crc=0xFFFFFFFF):
    # the running register littlefs/the firmware use: ~ of this is zlib.crc32
    return zlib.crc32(data, crc ^ 0xFFFFFFFF) ^ 0xFFFFFFFF

class Z:
    def __init__(self, port, baud):
        # port may be an open serial.Serial (a test driver that also talks to the prompt), or a name
        self.s = port if isinstance(port, serial.Serial) else serial.Serial(port, baud, timeout=0.05)
        self.last = 0
        self.rx = b''
    # ---- output ----
    def put(self, b): self.s.write(bytes([b]))
    def escaped(self, data):
        # the ZDLE escaping of a run of bytes, as lrzsz does it, built in one go (one write per subpacket)
        out = bytearray(); last = self.last
        for b in data:
            if b in (ZDLE, 0x10, 0x11, 0x13, 0x90, 0x91, 0x93) or (b in (0x0D, 0x8D) and (last & 0x7F) == 0x40):
                out += bytes([ZDLE, b ^ 0x40])
            else:
                out.append(b)
            last = b
        self.last = last
        return bytes(out)
    def putbin(self, b): self.s.write(self.escaped(bytes([b])))
    def hexhdr(self, t, pos=0, flags=None):
        hdr = bytes([t]) + (bytes(flags) if flags is not None else pos.to_bytes(4, 'little'))
        line = b'**' + bytes([ZDLE, ZHEX]) + (hdr + crc16(hdr).to_bytes(2, 'big')).hex().encode() + b'\r\x8a'
        if t not in (ZFIN, ZACK): line += bytes([XON])
        self.s.write(line); log('>', NAMES.get(t, t), 'hex', pos)
    def binhdr(self, t, pos=0, flags=None):
        hdr = bytes([t]) + (bytes(flags) if flags is not None else pos.to_bytes(4, 'little'))
        self.s.write(bytes([ZPAD, ZDLE, ZBIN32]))
        for b in hdr: self.putbin(b)
        for b in zlib.crc32(hdr).to_bytes(4, 'little'): self.putbin(b)
        log('>', NAMES.get(t, t), 'bin32', pos)
    def data(self, buf, term):
        pkt = self.escaped(buf) + bytes([ZDLE, term])
        self.last = term
        pkt += self.escaped(zlib.crc32(buf + bytes([term])).to_bytes(4, 'little'))
        self.s.write(pkt)
    # ---- input ----
    def getc(self, timeout):
        end = time.time() + timeout
        while True:
            if self.rx:
                b, self.rx = self.rx[0], self.rx[1:]
                return b
            self.rx = self.s.read(256)
            if not self.rx and time.time() >= end:
                return -1
    def unescaped(self, timeout):
        # one data byte after ZDLE decoding; returns (byte, is_terminator)
        b = self.getc(timeout)
        if b < 0: return -1, False
        if b != ZDLE: return b, False
        b = self.getc(timeout)
        if b in (ZCRCE, ZCRCG, ZCRCQ, ZCRCW): return b, True
        if b == ZRUB0: return 0x7F, False
        if b == ZRUB1: return 0xFF, False
        return b ^ 0x40, False
    def hdr(self, timeout):
        """wait for a header: (type, pos/flags bytes) or (None, None) on timeout"""
        end = time.time() + timeout
        while time.time() < end:
            b = self.getc(end - time.time())
            if b != ZPAD: continue
            b = self.getc(1)
            while b == ZPAD: b = self.getc(1)
            if b != ZDLE: continue
            f = self.getc(1)
            if f == ZHEX:
                raw = bytes(self.getc(1) for _ in range(14))
                try: h = bytes.fromhex(raw.decode())
                except ValueError: continue
                while True:
                    c = self.getc(0.2)
                    if c in (-1, 0x0A, 0x8A): break
                if crc16(h) != 0: log('hex hdr crc bad'); continue
                log('<', NAMES.get(h[0], h[0]), 'hex', h[1:5].hex()); return h[0], h[1:5]
            if f in (ZBIN, ZBIN32):
                n = 9 if f == ZBIN32 else 7
                h = bytearray()
                while len(h) < n:
                    b, _ = self.unescaped(1)
                    if b < 0: break
                    h.append(b)
                if len(h) < n: continue
                ok = (crc32raw(bytes(h)) == 0xDEBB20E3) if f == ZBIN32 else (crc16(bytes(h)) == 0)
                if not ok: log('bin hdr crc bad'); continue
                log('<', NAMES.get(h[0], h[0]), 'bin', h[1:5].hex()); return h[0], bytes(h[1:5])
        return None, None
    def subpacket(self, timeout):
        """one data subpacket: (bytes, term) or (None, None) on a bad CRC or timeout"""
        buf = bytearray()
        while True:
            b, isterm = self.unescaped(timeout)
            if b < 0: return None, None
            if isterm:
                crc = bytearray()
                while len(crc) < 4:
                    c, _ = self.unescaped(timeout)
                    if c < 0: return None, None
                    crc.append(c)
                if crc32raw(bytes(buf) + bytes([b]) + bytes(crc)) != 0xDEBB20E3:
                    log('data crc bad', len(buf)); return None, None
                return bytes(buf), b
            buf.append(b)
            if len(buf) > 1024 + 8: return None, None

def send(port, baud, files, window):
    z = Z(port, baud)
    t0 = time.time(); total = 0
    for _ in range(20):
        z.hexhdr(ZRQINIT)
        t, h = z.hdr(3)
        if t == ZRINIT: break
    else:
        sys.exit('no ZRINIT from the board')
    rxbuf = h[0] | (h[1] << 8)
    if rxbuf and rxbuf < window: window = rxbuf
    for path in files:
        data = open(path, 'rb').read(); size = len(data); name = os.path.basename(path)
        for _ in range(10):
            z.binhdr(ZFILE, flags=[0, 0, 0, 1])
            z.data(name.encode() + b'\0' + f'{size} 0 100644'.encode() + b'\0', ZCRCW)
            while True:
                t, h = z.hdr(5)
                if t in (ZRPOS, ZSKIP, None): break   # anything else (a stale ZRINIT) is ignored
            if t in (ZRPOS, ZSKIP): break
        else:
            sys.exit('no ZRPOS for ' + name)
        if t == ZSKIP:
            print('skipped', name); continue
        pos = int.from_bytes(h, 'little'); tries = 0
        while True:
            start = pos
            z.binhdr(ZDATA, pos)
            rpos = None
            while True:
                n = min(1024, size - pos)
                chunk = data[pos:pos + n]; pos += n
                term = ZCRCE if pos >= size else (ZCRCW if pos - start >= window else ZCRCG)
                z.data(chunk, term)
                if term == ZCRCG:
                    if z.s.in_waiting:  # the receiver has something to say mid-stream: probably ZRPOS
                        t, h = z.hdr(0.5)
                        if t == ZRPOS: rpos = int.from_bytes(h, 'little'); break
                    continue
                if term == ZCRCE:
                    z.binhdr(ZEOF, pos)
                while True:
                    t, h = z.hdr(5)
                    if t == ZRPOS or t is None: break
                    if term == ZCRCW and t == ZACK: break
                    if term == ZCRCE and t == ZRINIT: break
                    if t == ZSKIP: break
                    log('ignoring', NAMES.get(t, t))   # a stale ZRINIT or ZACK
                if t == ZRPOS: rpos = int.from_bytes(h, 'little'); break
                if t == ZSKIP: print('skipped', name); rpos = None; pos = size; break
                if term == ZCRCW and t == ZACK: start = pos = int.from_bytes(h, 'little'); z.binhdr(ZDATA, pos); continue
                if term == ZCRCE and t == ZRINIT: break
                tries += 1
                if tries > 10: sys.exit('receiver stopped responding')
                rpos = start; break
            if rpos is None:
                break
            log('resend from', rpos); pos = rpos
        total += size
        print('sent %s, %d bytes' % (name, size))
    for _ in range(3):
        z.hexhdr(ZFIN)
        t, h = z.hdr(2)
        if t == ZFIN: break
    z.s.write(b'OO')
    dt = time.time() - t0
    print('%d files, %d bytes in %.2f s = %.1f KB/s' % (len(files), total, dt, total / dt / 1024 if dt else 0))

def recv(port, baud, outdir):
    z = Z(port, baud)
    t0 = time.time(); files = []
    z.hexhdr(ZRINIT, flags=[ZRXBUFLEN & 0xFF, ZRXBUFLEN >> 8, 0, CANFDX | CANFC32])
    name = None; buf = bytearray(); size = 0; tries = 0
    while True:
        t, h = z.hdr(5)
        if t is None:
            tries += 1
            if tries > 20: sys.exit('sender stopped responding')
            if name is None: z.hexhdr(ZRINIT, flags=[ZRXBUFLEN & 0xFF, ZRXBUFLEN >> 8, 0, CANFDX | CANFC32])
            else: z.hexhdr(ZRPOS, len(buf))
            continue
        tries = 0
        if t == ZRQINIT:
            z.hexhdr(ZRINIT, flags=[ZRXBUFLEN & 0xFF, ZRXBUFLEN >> 8, 0, CANFDX | CANFC32])
        elif t == ZFILE:
            d, term = z.subpacket(3)
            if d is None: z.hexhdr(ZNAK); continue
            name = d.split(b'\0')[0].decode(); size = int(d.split(b'\0')[1].split()[0])
            buf = bytearray(); log('file', name, size)
            z.hexhdr(ZRPOS, 0)
        elif t == ZDATA:
            if name is None:  # between files: the sender has not seen our ZRINIT yet
                z.hexhdr(ZRINIT, flags=[ZRXBUFLEN & 0xFF, ZRXBUFLEN >> 8, 0, CANFDX | CANFC32]); continue
            pos = int.from_bytes(h, 'little')
            if pos != len(buf): z.hexhdr(ZRPOS, len(buf)); continue
            while True:
                d, term = z.subpacket(3)
                if d is None: z.hexhdr(ZRPOS, len(buf)); break
                buf += d
                if term in (ZCRCW, ZCRCQ): z.hexhdr(ZACK, len(buf))
                if term in (ZCRCW, ZCRCE): break
        elif t == ZEOF:
            if name is None:  # a repeat of an end we have already answered
                z.hexhdr(ZRINIT, flags=[ZRXBUFLEN & 0xFF, ZRXBUFLEN >> 8, 0, CANFDX | CANFC32]); continue
            if int.from_bytes(h, 'little') != len(buf): continue
            path = os.path.join(outdir, name); open(path, 'wb').write(buf); files.append((name, len(buf)))
            print('received %s, %d bytes' % (name, len(buf)))
            name = None
            z.hexhdr(ZRINIT, flags=[ZRXBUFLEN & 0xFF, ZRXBUFLEN >> 8, 0, CANFDX | CANFC32])
        elif t == ZFIN:
            z.hexhdr(ZFIN)
            z.getc(1); z.getc(1)  # OO
            break
        elif t in (ZABORT, ZFERR):
            sys.exit('cancelled by the sender')
    dt = time.time() - t0; total = sum(n for _, n in files)
    print('%d files, %d bytes in %.2f s = %.1f KB/s' % (len(files), total, dt, total / dt / 1024 if dt else 0))

ZRXBUFLEN = 32768
if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if a != '-v']
    window = 32768
    if '--window' in args:
        i = args.index('--window'); window = int(args[i + 1]); del args[i:i + 2]
    if args[0] == 'send': send(args[1], int(args[2]), args[3:], window)
    elif args[0] == 'recv': recv(args[1], int(args[2]), args[3])
    else: sys.exit(__doc__)
