# puff in the CMM2 firmware

Mark Adler's puff, version 2.3 (zlib licence, in puff.h and puff.c), from https://github.com/madler/zlib
contrib/puff at v1.3.1 (51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf). Unchanged.

It inflates the CYW43439's firmware, which the flash holds as raw deflate (tools/fwpack.py, Inc/net/cyw43_fw_z.h,
Src/net/cyw43_fw_cmm2.c): 141 KB in place of 225 KB, which made room for Bluetooth (docs/Bluetooth_Plan.md, 16 Nov
2026). Taken: puff.c, puff.h and README.
