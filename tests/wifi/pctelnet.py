"""pctelnet.py IP [command ...]: the CMM2's telnet console (OPTION TELNET CONSOLE ON) from this PC: connects to port 23,
answers nothing to the server's option negotiation (IAC sequences are dropped), types each command and prints what
comes back. With no command, PRINT 1+1 and PRINT MM.INFO(IP ADDRESS)."""
import socket, sys, time

ip = sys.argv[1]
cmds = sys.argv[2:] or ['PRINT 1+1', 'PRINT MM.INFO(IP ADDRESS)']


def strip_iac(b):
    out, i = bytearray(), 0
    while i < len(b):
        if b[i] == 255 and i + 1 < len(b):
            if b[i + 1] in (251, 252, 253, 254):    # WILL WONT DO DONT + option
                i += 3
                continue
            if b[i + 1] == 255:
                out.append(255)
            i += 2
            continue
        out.append(b[i])
        i += 1
    return bytes(out)


def rd(s, t):
    data, end = b'', time.time() + t
    while time.time() < end:
        try:
            d = s.recv(4096)
            if not d:
                break
            data += d
            end = time.time() + 0.6
        except socket.timeout:
            pass
    return strip_iac(data).decode('latin-1')


s = socket.create_connection((ip, 23), timeout=0.2)
print('connected:', repr(rd(s, 3)))
for c in cmds:
    s.sendall(c.encode() + b'\r\n')
    print('>>', c)
    print(rd(s, 3))
s.close()
