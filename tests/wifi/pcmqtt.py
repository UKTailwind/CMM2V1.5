"""pcmqtt.py [N]: a minimal MQTT 3.1.1 broker on this PC's port 1883 for the CMM2's WEB MQTT (mqtttest.bas), with no
install: CONNECT, SUBSCRIBE, UNSUBSCRIBE, PUBLISH at QoS 0, 1 and 2, PINGREQ and DISCONNECT, retained messages, and
topics matched exactly or with + and #. Once a client has subscribed to cmm2/in, it publishes N messages there
(default 5), one a second, and prints every PUBLISH a client sends; it ends 3 s after the last, or after 60 s."""
import socket, struct, sys, threading, time

N = int(sys.argv[1]) if len(sys.argv) > 1 else 5
subs = {}                                       # client socket -> list of topic filters
retained = {}
lock = threading.Lock()
seen = []
pid_out = [1]


def log(*a):
    print(time.strftime('%H:%M:%S'), *a, flush=True)


def enc_len(n):
    out = bytearray()
    while True:
        b = n % 128
        n //= 128
        out.append(b | (0x80 if n else 0))
        if not n:
            return bytes(out)


def read_packet(c):
    h = c.recv(1)
    if not h:
        return None, None
    mult, n = 1, 0
    while True:
        b = c.recv(1)[0]
        n += (b & 127) * mult
        mult *= 128
        if not b & 128:
            break
    body = b''
    while len(body) < n:
        d = c.recv(n - len(body))
        if not d:
            return None, None
        body += d
    return h[0], body


def match(f, t):
    fp, tp = f.split('/'), t.split('/')
    for i, p in enumerate(fp):
        if p == '#':
            return True
        if i >= len(tp) or (p != '+' and p != tp[i]):
            return False
    return len(fp) == len(tp)


def publish(topic, msg, retain=False):
    if retain:
        retained[topic] = msg
    with lock:
        targets = [c for c, fs in subs.items() if any(match(f, topic) for f in fs)]
    t = topic.encode()
    pkt = struct.pack('>H', len(t)) + t + msg
    for c in targets:
        try:
            c.sendall(bytes([0x30]) + enc_len(len(pkt)) + pkt)     # (QoS 0 to subscribers)
        except OSError:
            pass


def client(c, a):
    log('client from %s' % a[0])
    try:
        while True:
            h, body = read_packet(c)
            if h is None:
                break
            kind = h >> 4
            if kind == 1:                       # CONNECT
                l = struct.unpack('>H', body[0:2])[0]
                cid_off = 2 + l + 4
                cl = struct.unpack('>H', body[cid_off:cid_off + 2])[0]
                log('CONNECT', body[2:2 + l].decode(), 'client id', body[cid_off + 2:cid_off + 2 + cl].decode())
                c.sendall(b'\x20\x02\x00\x00')
            elif kind == 8:                     # SUBSCRIBE
                pid = body[0:2]
                i, granted, fs = 2, b'', []
                while i < len(body):
                    l = struct.unpack('>H', body[i:i + 2])[0]
                    f = body[i + 2:i + 2 + l].decode()
                    q = body[i + 2 + l]
                    fs.append(f)
                    granted += bytes([min(q, 1)])
                    i += 3 + l
                with lock:
                    subs.setdefault(c, []).extend(fs)
                c.sendall(bytes([0x90]) + enc_len(2 + len(granted)) + pid + granted)
                log('SUBSCRIBE', fs)
                for t, m in retained.items():
                    if any(match(f, t) for f in fs):
                        publish(t, m)
            elif kind == 10:                    # UNSUBSCRIBE
                c.sendall(b'\xb0\x02' + body[0:2])
            elif kind == 3:                     # PUBLISH
                qos, retain = (h >> 1) & 3, h & 1
                l = struct.unpack('>H', body[0:2])[0]
                topic = body[2:2 + l].decode()
                i = 2 + l
                pid = b''
                if qos:
                    pid = body[i:i + 2]
                    i += 2
                msg = body[i:]
                seen.append((topic, msg))
                log('PUBLISH %s qos %d%s: %s' % (topic, qos, ' retained' if retain else '', msg.decode('latin-1')))
                if qos == 1:
                    c.sendall(b'\x40\x02' + pid)
                elif qos == 2:
                    c.sendall(b'\x50\x02' + pid)
                publish(topic, msg, retain)
            elif kind == 6:                     # PUBREL
                c.sendall(b'\x70\x02' + body[0:2])
            elif kind == 12:                    # PINGREQ
                c.sendall(b'\xd0\x00')
            elif kind == 14:                    # DISCONNECT
                log('DISCONNECT')
                break
    except OSError as e:
        log('client error', e)
    with lock:
        subs.pop(c, None)
    c.close()


srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(('', 1883))
srv.listen(4)
threading.Thread(target=lambda: [threading.Thread(target=client, args=srv.accept(), daemon=True).start()
                                 for _ in iter(int, 1)], daemon=True).start()
log('broker on port 1883')
t0 = time.time()
while time.time() - t0 < 60 and not any(any(match(f, 'cmm2/in') for f in fs) for fs in list(subs.values())):
    time.sleep(0.1)
for i in range(N):
    time.sleep(1)
    publish('cmm2/in', ('message %d from the PC' % (i + 1)).encode())
    log('sent to cmm2/in: message %d' % (i + 1))
time.sleep(3)
echoes = [m for t, m in seen if t == 'cmm2/out' and m.startswith(b'echo')]
log('%d sent to the CMM2, %d echoes back' % (N, len(echoes)))
