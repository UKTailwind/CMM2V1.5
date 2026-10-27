"""port_usbtouch.py PICOMITE_DIR CMM2_DIR - the PicoMite's USB touch screen code copied intact for the CMM2, as
Src/USBTouch.c and Inc/USBTouch.h (Peter, 27 Oct 2026: "copy the code across intact and only make changes where the
CMM2 environment mandates it"; PicoMite fe788dd/5856b73).

Copied verbatim, by their text: from input/USBKeyboard.c the touch variables, the descriptor parser and report analyser
(touch_read_bits ... process_touch_report), the fragment reassembler, the GET/SET_FEATURE completion and the Input Mode
handshake, the mount, unmount and report branches; from graphics/Pointer.c the gesture state machine; from
Hardware_Includes.h the types and the variables' declarations; from graphics/Draw.h the gestures' declarations.

The CMM2 environment's changes, and only these:
  - the PicoMite's own HID callbacks hold the mount, unmount, report and hid_app_task parts: here they are functions
    that the CMM2's callbacks (Src/USBKeyboard.c) call: USBTouchMount, USBTouchUnmount, USBTouchReport, USBTouchTask;
    its tuh_hid_get_report_complete_cb is USBTouchGetReportComplete (the CMM2's own, in Src/USBGamepad.c, calls it)
  - a USB callback here does not print (it can reach routinechecks): "if (!CurrentLinePtr) MMPrintString(...)" becomes
    USBMessage, which also says nothing while a program runs; a break out of the unmount loop becomes a return
  - the CMM2 counts its devices in the caller (Current_USB_devices++; return; becomes return true) and has no USB
    connect chime (USBSoundFlag)
  - time_us_64() is Getuptime(), the uptime clock (GetuSec() is BASIC's TIMER, which TIMER = n moves); GUICONTROLS and
    TOUCH_GESTURES are always on here; TOUCH_DEBUG's dumps are compiled out
    as the PicoMite compiles them out
  - Peter, 27 Oct: a touch monitor is enumerated once, as its multi-touch screen: its absolute pointer interface is not
    taken when its device has the multi-touch one (the PicoMite enumerates only that on this monitor, 27C0:0859)
  - until the PicoMite fixes its issue #144 (skipped once its touch_publish has prev_id): the contacts kept in a stable
    order by their IDs, as the issue proposes. 27C0:0859 puts a newly landed finger in its first slot, so contact 0
    became the new finger (the on-screen keyboard's chords did nothing); a panel that keeps the order, or reports no
    IDs, is unchanged
"""
import os
import re
import sys

pico, cmm2 = sys.argv[1], sys.argv[2]


def section(text, start, end, what, include_end=False):
    a = text.find(start)
    if a < 0:
        sys.exit('%s: start not found' % what)
    b = text.find(end, a + len(start))
    if b < 0:
        sys.exit('%s: end not found' % what)
    return text[a:b + (len(end) if include_end else 0)]


def once(d, old, new, what):
    n = d.count(old)
    if n != 1:
        sys.exit('%s: %d matches' % (what, n))
    return d.replace(old, new)


def no_print(block):
    """if (!CurrentLinePtr) { MMPrintString(...); PInt(...); ... } -> the text built and given to USBMessage"""
    def build(m):
        body, ind = m.group(2), m.group(1)
        out = [ind + '{', ind + '\tchar m[100] = "";']
        for call in re.findall(r'(MMPrintString|PInt)\((.*?)\);', body):
            out.append(ind + '\t' + ('strcat(m, %s);' % call[1] if call[0] == 'MMPrintString'
                                     else 'IntToStr(m + strlen(m), %s, 10);' % call[1]))
        out.append(ind + '\tUSBMessage(m);')
        out.append(ind + '}')
        return '\n'.join(out)
    block = re.sub(r'([ \t]*)if \(!CurrentLinePtr\)\n[ \t]*\{\n((?:[ \t]*(?:MMPrintString|PInt)\(.*?\);\n)+)[ \t]*\}', build, block)
    block = re.sub(r'if \(!CurrentLinePtr\)\n([ \t]*)MMPrintString\((.*?)\);', r'USBMessage(\2);', block)
    return block


kb = open(os.path.join(pico, 'input', 'USBKeyboard.c'), encoding='utf-8').read()
pt = open(os.path.join(pico, 'graphics', 'Pointer.c'), encoding='utf-8').read()
hw = open(os.path.join(pico, 'Hardware_Includes.h'), encoding='utf-8').read()
dh = open(os.path.join(pico, 'graphics', 'Draw.h'), encoding='utf-8').read()

types = section(hw, '/* ============================================================================\n * Type definitions - USB multi-touch digitizer',
                '} touch_report_t;\n', 'types', True)
externs = section(hw, '#ifdef USBKEYBOARD\nextern volatile bool usb_touch_present;', '#endif\n', 'externs', True)
externs = externs.replace('#ifdef USBKEYBOARD\n', '').replace('#endif\n', '')
gestdecl = section(dh, '#ifdef TOUCH_GESTURES\nvoid touch_gesture_on_down', '#endif\n', 'gesture declarations', True)

variables = section(kb, '/* USB multi-touch -> generic "touch panel" integration.',
                    'volatile int16_t usb_touch_yn[MAX_TOUCH_CONTACTS] = {0};\n', 'variables', True)
parser = section(kb, '/* ============================================================================\n * USB multi-touch HID — descriptor parser',
                 '/**\n * Print touch_info for debugging at enumeration.', 'parser')
if 'prev_id' not in parser:
    parser = once(parser, '\tusb_touch_last_us = time_us_64();\n\tif (out->count > 0 && out->contacts[0].tip)\n',
                  '\tusb_touch_last_us = time_us_64();\n'
                  '\t/* (the CMM2, until PicoMite #144: the contacts in a stable order, followed by their IDs. A panel that\n'
                  '\t   puts a newly landed finger in its first slot (27C0:0859) made contact 0 the new finger; one that keeps\n'
                  '\t   the order, or reports no IDs, is unchanged) */\n'
                  '\t{\n'
                  '\t\tstatic uint8_t prev_id[2] = {0xFF, 0xFF};\n'
                  '\t\tfor (int p = 0; p < 2; p++)\n'
                  '\t\t\tfor (int k = p + 1; k < out->count; k++)\n'
                  '\t\t\t\tif (out->contacts[k].id == prev_id[p] && out->contacts[p].id != prev_id[p])\n'
                  '\t\t\t\t{\n'
                  '\t\t\t\t\ttouch_contact_t t = out->contacts[p];\n'
                  '\t\t\t\t\tout->contacts[p] = out->contacts[k];\n'
                  '\t\t\t\t\tout->contacts[k] = t;\n'
                  '\t\t\t\t}\n'
                  '\t\tprev_id[0] = out->count > 0 ? out->contacts[0].id : 0xFF;\n'
                  '\t\tprev_id[1] = out->count > 1 ? out->contacts[1].id : 0xFF;\n'
                  '\t}\n'
                  '\tif (out->count > 0 && out->contacts[0].tip)\n', 'stable contact order (PicoMite #144)')
dumps = section(kb, '#else /* !TOUCH_DEBUG — compile the dump calls out entirely */\n', '#endif /* TOUCH_DEBUG */', 'dumps')
dumps = dumps.split('\n', 1)[1]
reassembler = section(kb, '/**\n * Expected total length', "/* USB_TOUCH: a multi-touch screen's Device_type", 'reassembler')
feature = section(kb, '/* Scratch buffer for the touch digitizer-init GET_FEATURE reads.', '#define TOUCH_KEEPALIVE 0\n',
                  'feature', True)
feature = once(feature, 'void tuh_hid_get_report_complete_cb(', 'void USBTouchGetReportComplete(', 'get report callback')
task = section(kb, '\t\t/* Keep-alive: some panels (ILI2132) were seen', '\t\tif (HID[i].report_requested)\n\t\t\tcontinue;', 'task')
mount = section(kb, '\t\t/* Multi-touch USB digitizers enumerate as HID_ITF_PROTOCOL_NONE', '\t\t//\t\thid_info[instance].report_count',
                'mount')
unmount = section(kb, '\t\telse if (instance == HID[i].Device_instance && dev_addr == HID[i].Device_address && HID[i].Device_type == USB_TOUCH)',
                  '\t\telse if (instance == HID[i].Device_instance && dev_addr == HID[i].Device_address && HID[i].Device_type == UNKNOWN)',
                  'unmount')
report = section(kb, '\t\tif (HID[n].Device_type == USB_TOUCH || HID[n].Device_type == USB_TOUCHMOUSE)\n\t\t{\n\t\t\t/* Reassembles',
                 '\t\telse if (is_sony_ds4(dev_addr))', 'report')
gestures = section(pt, '#ifdef TOUCH_GESTURES\n/* ====', '#endif /* TOUCH_GESTURES — end of shared gesture state machine */\n',
                   'gestures', True)

# the mount: counted by the caller, no chime, no printing; a touch monitor taken once (its multi-touch interface)
for old in ('\t\t\t\tUSBSoundFlag = 1; /* deferred chime */\n',):
    if mount.count(old) != 2:
        sys.exit('mount: chime %d' % mount.count(old))
    mount = mount.replace(old, '')
if mount.count('\t\t\t\tCurrent_USB_devices++;\n\t\t\t\treturn;\n') != 2:
    sys.exit('mount: returns')
mount = mount.replace('\t\t\t\tCurrent_USB_devices++;\n\t\t\t\treturn;\n', '\t\t\t\treturn true;\n')
mount = once(mount, '\t\t\tif (desc_report && desc_len > 0 && tinfo.has_pointer_fallback)\n',
             '\t\t\t/* (the CMM2: a touch monitor is one touch screen, its multi-touch one: Peter, 27 Oct 2026) */\n'
             '\t\t\tfor (int i = 0; i < 4; i++)\n'
             '\t\t\t\tif (HID[i].active && HID[i].Device_address == dev_addr && HID[i].Device_type == USB_TOUCH)\n'
             '\t\t\t\t\treturn false;\n'
             '\t\t\tif (desc_report && desc_len > 0 && tinfo.has_pointer_fallback)\n', 'mount: once')
mount = no_print(mount)
if 'MMPrintString' in mount or 'CurrentLinePtr' in mount:
    sys.exit('mount: a print left')
unmount = unmount.replace('\t\telse if (instance', '\t\tif (instance', 1).replace('\t\t\tbreak;\n', '\t\t\treturn;\n')
unmount = no_print(unmount)
if 'MMPrintString' in unmount:
    sys.exit('unmount: a print left')
feature = no_print(feature)

h = '''/* USBTouch.h - the PicoMite's USB touch screen (its Hardware_Includes.h and graphics/Draw.h), copied by
   tools/port_usbtouch.py: do not edit, run it again. Included by keyboard.h (struct s_HID's touch fields). */
#ifndef USBTOUCH_H_
#define USBTOUCH_H_

#include <stdint.h>
#include <stdbool.h>

''' + types + '''
''' + externs + '''
#define TOUCH_GESTURES
''' + gestdecl + '''
/* the CMM2's callbacks call these (Src/USBKeyboard.c, Src/USBGamepad.c) */
bool USBTouchMount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len, int slot);
void USBTouchUnmount(uint8_t dev_addr, uint8_t instance, int i);
void USBTouchReport(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len, int n);
void USBTouchTask(int i);
void USBTouchGetReportComplete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len);
#endif
'''

c = '''/* USBTouch.c - the PicoMite's USB touch screen (input/USBKeyboard.c, graphics/Pointer.c's gestures), copied by
   tools/port_usbtouch.py: do not edit, run it again. What the CMM2 changes is listed there. */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "tusb.h"
#include "keyboard.h"
#include "USBGamepad.h"
#include <math.h>

#define time_us_64() ((uint64_t)Getuptime())
#define GUICONTROLS
extern void USBMessage(const char *m);

''' + variables + '''
''' + parser + dumps + '''
''' + reassembler + feature + '''
/* hid_app_task's part for a touch screen (the CMM2's hid_app_task calls this for each active slot) */
void USBTouchTask(int i)
{
	uint64_t timenow = time_us_64();
	(void)timenow;
''' + task + '''}

/* tuh_hid_mount_cb's part for an interface of HID_ITF_PROTOCOL_NONE: true if it is a touch screen, taken */
bool USBTouchMount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len, int slot)
{
	uint16_t vid, pid;
	tuh_vid_pid_get(dev_addr, &vid, &pid);
	HID[slot].vid = vid;
	HID[slot].pid = pid;
''' + mount + '''	return false;
}

/* tuh_hid_umount_cb's part for slot i */
void USBTouchUnmount(uint8_t dev_addr, uint8_t instance, int i)
{
''' + unmount + '''}

/* tuh_hid_report_received_cb's part for slot n */
void USBTouchReport(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len, int n)
{
''' + report + '''}

''' + gestures

open(os.path.join(cmm2, 'Inc', 'USBTouch.h'), 'w', encoding='utf-8', newline='\n').write(h)
open(os.path.join(cmm2, 'Src', 'USBTouch.c'), 'w', encoding='utf-8', newline='\n').write(c)
print('Src/USBTouch.c %d lines, Inc/USBTouch.h %d lines' % (c.count('\n'), h.count('\n')))
