"""port_bt.py PICOMITE_DIR CMM2_DIR - the PicoMite's Bluetooth host copied into the CMM2 (docs/Bluetooth_Plan.md, step 3;
Peter: port PicoMite code intact).

  PicoMite bluetooth/BTKeyboard.c .h, BTClassicKeyboard.c .h, BTSerial.c .h, BTAudio.c .h   -> Src/net/bt/picomite/
  PicoMite bluetooth/btstack_config.h                                                        -> Src/net/bt/picomite/
                                                    (Inc/net/btstack_config.h includes it, then the CMM2's additions)

BTConsole.c (the PicoMiteBT build's BLE console, not the PC3's) is left out.

Mechanical changes only, so that a later PicoMite's Bluetooth code can be taken by running this again:
  - SaveOptions() -> SaveOptions(1), SoftReset(SOFT_RESET) -> SoftReset(): the CMM2's signatures, as port_net.py
  - routinechecks() -> routinechecks(0): the CMM2's keyboard and USB part only (its full checks run ProcessWeb and
    ProcessBT, and the call is in a btstack callback, inside the poll), as port_net.py
  - __wrap_pico_flash_bank_instance -> pico_flash_bank_instance: the PicoMite links with --wrap so that pico-sdk's
    btstack_cyw43.c takes the bond store from Option.bt_tlv; the CMM2's own btstack_cyw43 (Src/net/bt) calls it by
    its plain name
  - COM7 -> COM8 (the words in messages and comments): the PicoMite's Bluetooth serial device is its COM7, the CMM2's
    is COM8, its COM4 to COM7 being the USB serial devices (the variable com7 is mapped by BTMite_cmm2.h)
Everything else the PicoMite's own headers and core give these files comes from Inc/net/BTMite_cmm2.h, which
CMakeLists.txt makes their first include (-include). Then build, and check on the board.
"""
import os
import subprocess
import sys

pico, cmm2 = sys.argv[1], sys.argv[2]


def rd(path):
    return open(path, newline='').read()


def wr(path, text):
    open(path, 'w', newline='').write(text)


dst = os.path.join(cmm2, 'Src', 'net', 'bt', 'picomite')
os.makedirs(dst, exist_ok=True)
FILES = ['BTKeyboard.c', 'BTKeyboard.h', 'BTClassicKeyboard.c', 'BTClassicKeyboard.h', 'BTSerial.c', 'BTSerial.h',
         'BTAudio.c', 'BTAudio.h', 'btstack_config.h']
REWRITES = [('SaveOptions()', 'SaveOptions(1)'), ('SoftReset(SOFT_RESET)', 'SoftReset()'),
            ('routinechecks()', 'routinechecks(0)'),
            ('__wrap_pico_flash_bank_instance', 'pico_flash_bank_instance'), ('COM7', 'COM8')]
commit = subprocess.run(['git', '-C', pico, 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True).stdout.strip()
for f in FILES:
    t = rd(os.path.join(pico, 'bluetooth', f))
    n = []
    for a, b in REWRITES:
        if a in t:
            n.append('%s x%d' % (a, t.count(a)))
            t = t.replace(a, b)
    wr(os.path.join(dst, f), t)
    print(f, ('(' + ', '.join(n) + ')') if n else '')
print('from the PicoMite at', commit)
