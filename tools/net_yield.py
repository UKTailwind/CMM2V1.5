"""net_yield.py [--bt] DIR - the network's and Bluetooth's commands' waits let a program's other threads run
(docs/Threads_Plan.md, Stage 4b).

The PicoMite's network commands wait for an answer in loops of the form

    Timer4 = timeout;
    while (!state->complete && Timer4)
        if (startupcomplete)
            cyw43_arch_poll();

(or ProcessWeb(0); as the body), and its Bluetooth commands in loops that call CheckAbort() until a state changes or
their own time runs out. In the functions named below, each such poll becomes

    { cyw43_arch_poll(); NetYield(); }

and NetYield (Src/net/WebMite_cmm2.c) lets the other threads run, when there are any. Only these functions: each is a
BASIC command's, run in a thread at a statement, never from inside lwIP, btstack or ProcessWeb. ProcessWeb itself,
which every thread's CheckAbort runs (WebPoll), and the TCP server's send, which lwIP's callbacks reach, are left
alone: a switch inside them would let another thread into ProcessWeb while it is part way through. The Bluetooth
commands hold btstack's lock only around each call into it, never across these waits.

A poll is rewritten only where it is a statement on a line of its own, so comments that name it are untouched, and a
file already rewritten is left as it is. port_net.py and port_bt.py apply this to the files they copy; run alone, it
rewrites DIR's files in place (--bt: the Bluetooth rules). It prints each poll it rewrote, with its function.
"""
import os
import re
import sys

NET = (("cmd_mqtt", "mqtt_example_init", "cmd_tcpclient", "cmd_ntp", "cmd_ping", "cmd_udp", "cmd_web", "WebConnect"),
       re.compile(r"^(?P<ind>[ \t]*)(?P<call>cyw43_arch_poll\(\)|ProcessWeb\(0\));[ \t]*(?P<eol>\r?)$"))
BT = (("bta_wait", "ck_wait_not_connecting", "sp_wait_while", "bt_serial_disconnect"),
      re.compile(r"^(?P<ind>[ \t]*)(?P<call>CheckAbort\(\));[ \t]*(?P<eol>\r?)$"))
# a function's definition: a type, its name (or __not_in_flash_func(name)) and a bracket, no ; or = on the line
HEAD = re.compile(r"^[ \t]*(?:static[ \t]+)?(?:const[ \t]+)?[A-Za-z_]\w*[ \t\*]+(?:__not_in_flash_func\()?(?P<name>\w+)\)?[ \t]*\([^;=]*$")
NOTHEAD = ("if", "while", "for", "switch", "return", "else", "do", "case", "sizeof")


def add_yields(text, label="", rules=NET):
    """text with the polls in the rules' functions rewritten, and the list of what was"""
    funcs, poll = rules
    lines, func, done = text.split("\n"), None, []
    for i, line in enumerate(lines):
        m = HEAD.match(line)
        if m and m.group("name") not in NOTHEAD and line.split("(")[0].split()[0] not in NOTHEAD:
            func = m.group("name")
        m = poll.match(line)
        if m and func in funcs:
            lines[i] = "%s{ %s; NetYield(); }%s" % (m.group("ind"), m.group("call"), m.group("eol"))
            done.append("%s:%d %s %s" % (label, i + 1, func, m.group("call")))
    return "\n".join(lines), done


if __name__ == "__main__":
    args = sys.argv[1:]
    rules = BT if "--bt" in args else NET
    d = [a for a in args if a != "--bt"][0]
    for f in sorted(os.listdir(d)):
        if not f.endswith(".c"):
            continue
        p = os.path.join(d, f)
        t = open(p, newline="", encoding="latin-1").read()
        t2, done = add_yields(t, f, rules)
        if done:
            open(p, "w", newline="", encoding="latin-1").write(t2)
            print("\n".join(done))
