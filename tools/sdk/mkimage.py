"""mkimage.py ELF OUT.app [--name NAME] [--stack BYTES] [--heap BYTES]

Makes a loadable program for the CMM2 G2 (Inc/cmm2img.h) from an ELF that app.ld linked with -Wl,--emit-relocs:
the 64-byte header, the image (every allocated section with contents, from the link address, gaps zeroed), then the
relocation words. Kept are the relocations that hold an absolute address of the image: R_ARM_ABS32 and R_ARM_TARGET1
words (CMM2IMG_REL_ABS32) and R_ARM_THM_MOVT_ABS instructions (CMM2IMG_REL_MOVT). A MOVW needs nothing, as the loader
moves an image by a multiple of 64 KB; PC-relative relocations need nothing, as the image moves whole; one against an
absolute or undefined symbol is left alone. Any other kind stops the build here, so it never fails at load time.
Standard library only.
"""
import argparse, struct, sys, zlib

SHF_ALLOC, SHT_NOBITS, SHT_SYMTAB, SHT_REL, SHT_RELA = 2, 8, 2, 9, 4
SHN_UNDEF, SHN_ABS = 0, 0xFFF1
R_ABS32, R_TARGET1, R_MOVW, R_MOVT = 2, 38, 47, 48
PC_RELATIVE = {
    0: 'NONE', 1: 'PC24', 3: 'REL32', 10: 'THM_CALL', 11: 'THM_PC8', 28: 'CALL', 29: 'JUMP24', 30: 'THM_JUMP24',
    40: 'V4BX', 42: 'PREL31', 49: 'THM_MOVW_PREL_NC', 50: 'THM_MOVT_PREL', 51: 'THM_JUMP19', 52: 'THM_JUMP6',
    53: 'THM_ALU_PREL_11_0', 54: 'THM_PC12', 102: 'THM_JUMP11', 103: 'THM_JUMP8',
}
HEADER = struct.Struct('<8sHHBBHIIIIIIII16s')
assert HEADER.size == 64


def sections(d):
    shoff, = struct.unpack_from('<I', d, 32)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 46)
    secs = []
    for i in range(shnum):
        f = struct.unpack_from('<10I', d, shoff + i * shentsize)
        secs.append(dict(name=f[0], type=f[1], flags=f[2], addr=f[3], offset=f[4], size=f[5], link=f[6], info=f[7]))
    strtab = secs[shstrndx]
    for s in secs:
        end = d.index(b'\0', strtab['offset'] + s['name'])
        s['name'] = d[strtab['offset'] + s['name']:end].decode()
    return secs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('elf')
    ap.add_argument('out')
    ap.add_argument('--name', default='')
    ap.add_argument('--stack', type=int, default=0)
    ap.add_argument('--heap', type=int, default=0)
    a = ap.parse_args()
    d = open(a.elf, 'rb').read()
    if d[:4] != b'\x7fELF' or d[4] != 1 or d[5] != 1:
        sys.exit('%s: not a 32-bit little-endian ELF' % a.elf)
    entry, = struct.unpack_from('<I', d, 24)
    secs = sections(d)
    alloc = [s for s in secs if s['flags'] & SHF_ALLOC and s['size']]
    data = [s for s in alloc if s['type'] != SHT_NOBITS]
    bss = [s for s in alloc if s['type'] == SHT_NOBITS]
    base = min(s['addr'] for s in data)
    end = max(s['addr'] + s['size'] for s in data)
    if base & 0xFFFF:
        sys.exit('the image starts at 0x%08X: it must be linked at a multiple of 64 KB' % base)
    image = bytearray(end - base)
    for s in data:
        image[s['addr'] - base:s['addr'] - base + s['size']] = d[s['offset']:s['offset'] + s['size']]
    for s in bss:
        if s['addr'] < end:
            sys.exit('%s (0x%08X) is not above the image' % (s['name'], s['addr']))
    bss_end = max([end] + [s['addr'] + s['size'] for s in bss])

    relocs, skipped, failed = set(), 0, []
    for s in secs:
        if s['type'] == SHT_RELA:
            sys.exit('%s: RELA relocations are not expected on ARM' % s['name'])
        if s['type'] != SHT_REL:
            continue
        target = secs[s['info']]
        if not target['flags'] & SHF_ALLOC or target['type'] == SHT_NOBITS:
            continue                                    # (debug information)
        symtab = secs[s['link']]
        for i in range(s['size'] // 8):
            off, info = struct.unpack_from('<II', d, s['offset'] + i * 8)
            kind, symi = info & 0xFF, info >> 8
            shndx, = struct.unpack_from('<H', d, symtab['offset'] + symi * 16 + 14)
            if kind in PC_RELATIVE:
                continue
            if kind in (R_ABS32, R_TARGET1, R_MOVW, R_MOVT) and shndx in (SHN_UNDEF, SHN_ABS):
                skipped += 1                            # an absolute address (a register, say), or an undefined weak: 0
                continue
            if not base <= off < end:
                failed.append('%s: offset 0x%08X outside the image' % (s['name'], off))
            elif kind in (R_ABS32, R_TARGET1):
                word, = struct.unpack_from('<I', image, off - base)
                if not base <= word <= bss_end:
                    print('warning: %s+0x%X holds 0x%08X, outside the image' % (target['name'], off - target['addr'], word))
                relocs.add(off - base)
            elif kind == R_MOVT:
                relocs.add((off - base) | (1 << 28))
            elif kind == R_MOVW:
                pass                                    # (a 64 KB step never changes its low half)
            else:
                failed.append('%s: relocation type %d at 0x%08X' % (s['name'], kind, off))
    if failed:
        sys.exit('cannot make a loadable image:\n  ' + '\n  '.join(failed[:20]))
    rel = b''.join(struct.pack('<I', r) for r in sorted(relocs, key=lambda r: r & 0x0FFFFFFF))
    crc = zlib.crc32(bytes(image) + rel) & 0xFFFFFFFF
    name = (a.name or a.out.replace('\\', '/').split('/')[-1].rsplit('.', 1)[0])[:15].encode()
    hdr = HEADER.pack(b'CMM2IMG\0', 1, 1, 1, 0x01, 0, base, len(image), bss_end - end, entry - base, a.stack, a.heap,
                      len(relocs), crc, name)
    open(a.out, 'wb').write(hdr + bytes(image) + rel)
    print('%s: %d bytes of image at 0x%08X, %d of .bss, entry +0x%X, %d relocations (%d absolute left alone), '
          'CRC %08X' % (a.out, len(image), base, bss_end - end, entry - base, len(relocs), skipped, crc))


if __name__ == '__main__':
    main()
