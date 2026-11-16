"""fwpack.py: Inc/net/cyw43_fw_z.h, the CYW43439's firmware blobs of Middlewares/Third_Party/cyw43-driver/firmware
compressed (raw deflate, which zlib's puff inflates: Src/net/cyw43_fw_cmm2.c), so that the chip's firmware takes about
140 KB of the flash rather than 225 KB (16 Nov 2026: room for Bluetooth, docs/Bluetooth_Plan.md). The blob files stay
as the driver has them; this script is run again when they change.

    python tools/fwpack.py

The WiFi firmware (with its CLM) is w43439A0_7_95_49_00_combined.h, or, with Bluetooth, wb43439A0_7_95_49_00_combined.h,
the combined one; each is packed when it is in the folder, under CYW43_ENABLE_BLUETOOTH's choice."""
import os, re, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FW = os.path.join(ROOT, 'Middlewares', 'Third_Party', 'cyw43-driver', 'firmware')
OUT = os.path.join(ROOT, 'Inc', 'net', 'cyw43_fw_z.h')
BLOBS = [('w43439A0_7_95_49_00_combined.h', '!CYW43_ENABLE_BLUETOOTH'),
         ('wb43439A0_7_95_49_00_combined.h', 'CYW43_ENABLE_BLUETOOTH')]


def read_blob(path):
    t = open(path, encoding='utf-8').read()
    arr = re.search(r'\[\]\s*CYW43_RESOURCE_ATTRIBUTE\s*=\s*\{(.*?)\};', t, re.S)
    data = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})\b', arr.group(1)))
    fw = int(re.search(r'#define CYW43_WIFI_FW_LEN \((\d+)\)', t).group(1))
    clm = int(re.search(r'#define CYW43_CLM_LEN \((\d+)\)', t).group(1))
    return data, fw, clm


out = ['/* cyw43_fw_z.h: made by tools/fwpack.py (do not edit) from Middlewares/Third_Party/cyw43-driver/firmware. The',
       '   CYW43439\'s firmware with its CLM, as raw deflate, for Src/net/cyw43_fw_cmm2.c to inflate before the driver',
       '   downloads it; CYW43_FW_RAW_LEN and CYW43_FW_RAW_CRC (CRC-32) are the blob as the driver has it. */', '']
done = 0
for name, cond in BLOBS:
    path = os.path.join(FW, name)
    if not os.path.exists(path):
        continue
    data, fw, clm = read_blob(path)
    assert len(data) >= fw + clm
    c = zlib.compressobj(9, zlib.DEFLATED, -15, 9)
    z = c.compress(data) + c.flush()
    assert zlib.decompress(z, -15) == data
    out.append(('#if %s' if done == 0 else '#elif %s') % cond)
    out.append('#define CYW43_FW_Z_FROM "%s"' % name)
    out.append('#define CYW43_WIFI_FW_LEN (%d)' % fw)
    out.append('#define CYW43_CLM_LEN (%d)' % clm)
    out.append('#define CYW43_FW_RAW_LEN (%d)' % len(data))
    out.append('#define CYW43_FW_RAW_CRC (0x%08XU)' % (zlib.crc32(data) & 0xFFFFFFFF))
    out.append('#define CYW43_FW_Z_LEN (%d)' % len(z))
    out.append('#ifdef CYW43_FW_Z_DATA                       /* (the array in cyw43_fw_cmm2.c only) */')
    out.append('static const unsigned char cyw43_fw_z[CYW43_FW_Z_LEN] = {')
    for i in range(0, len(z), 24):
        out.append('    ' + ','.join('0x%02x' % b for b in z[i:i + 24]) + ',')
    out.append('};')
    out.append('#endif')
    print('%s: %d bytes -> %d (%.0f%%)' % (name, len(data), len(z), 100.0 * len(z) / len(data)))
    done += 1
assert done, 'no firmware blob found'
out.append('#else')
out.append('#error "cyw43_fw_z.h: no packed firmware for this setting of CYW43_ENABLE_BLUETOOTH (run tools/fwpack.py)"')
out.append('#endif')
open(OUT, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
print('wrote', OUT)
