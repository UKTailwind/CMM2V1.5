"""pctftp.py IP [SIZE]: the CMM2's TFTP server (the PicoMite's, on unless OPTION TFTP OFF) from this PC, with a minimal
RFC 1350 client (octet mode, 512-byte blocks): a file of SIZE random bytes (default 20000) put as tftptest.bin, got
back, and compared; the time each way."""
import os, socket, struct, sys, time

ip = sys.argv[1]
size = int(sys.argv[2]) if len(sys.argv) > 2 else 20000
NAME = 'tftptest.bin'


def put(data):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(3)
    s.sendto(struct.pack('>H', 2) + NAME.encode() + b'\0octet\0', (ip, 69))
    d, peer = s.recvfrom(600)
    op, blk = struct.unpack('>HH', d[:4])
    if op != 4 or blk != 0:
        raise RuntimeError('WRQ refused: %r' % d)
    blk = 1
    for i in range(0, len(data) + 1, 512):
        chunk = data[i:i + 512]
        for attempt in range(5):
            s.sendto(struct.pack('>HH', 3, blk & 0xFFFF) + chunk, peer)
            try:
                d, _ = s.recvfrom(600)
                if struct.unpack('>HH', d[:4]) == (4, blk & 0xFFFF):
                    break
                if struct.unpack('>H', d[:2])[0] == 5:
                    raise RuntimeError('error from the CMM2: %r' % d[4:])
            except socket.timeout:
                pass
        else:
            raise RuntimeError('no ACK for block %d' % blk)
        blk += 1
        if len(chunk) < 512:
            break
    s.close()


def get():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(3)
    s.sendto(struct.pack('>H', 1) + NAME.encode() + b'\0octet\0', (ip, 69))
    out, want = b'', 1
    while True:
        d, peer = s.recvfrom(600)
        op = struct.unpack('>H', d[:2])[0]
        if op == 5:
            raise RuntimeError('error from the CMM2: %r' % d[4:])
        blk = struct.unpack('>H', d[2:4])[0]
        s.sendto(struct.pack('>HH', 4, blk), peer)
        if blk == want:
            out += d[4:]
            want += 1
            if len(d) - 4 < 512:
                break
    s.close()
    return out


data = os.urandom(size)
t0 = time.time()
put(data)
t1 = time.time()
back = get()
t2 = time.time()
print('put %d bytes in %.2f s (%.1f KB/s), got %d back in %.2f s (%.1f KB/s): %s' %
      (len(data), t1 - t0, len(data) / (t1 - t0) / 1024, len(back), t2 - t1, len(back) / (t2 - t1) / 1024,
       'identical' if back == data else 'DIFFERENT'))
