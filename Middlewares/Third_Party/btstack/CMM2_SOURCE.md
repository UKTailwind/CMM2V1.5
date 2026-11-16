# BTstack in the CMM2 firmware

BTstack (BlueKitchen; LICENSE: personal, non-commercial use, accepted by Peter, 16 Nov 2026) from
https://github.com/bluekitchen/btstack at eb0bb8b5ea6d234ccb940313b47f7a5c3b4e20ec, the commit the PicoMite's pico-sdk (2.3.1) has.
Unchanged. Copied by tools/port_btstack.py: the .c files the PicoMite PC3 build compiles for its Bluetooth host
(59), less SEGGER RTT, yxml, md5 and pico-sdk's stdin, and the headers of their folders (205). Its settings are the
PicoMite's bluetooth/btstack_config.h, with the CMM2's additions (Inc/bt/btstack_config.h).
