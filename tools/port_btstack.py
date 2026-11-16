"""port_btstack.py BTSTACK_DIR CMM2_DIR: BTstack copied into Middlewares/Third_Party/btstack (docs/Bluetooth_Plan.md,
step 3), unchanged, from a checkout at the commit pico-sdk 2.3.1 has (eb0bb8b: apps/bt/make.py fetches it into
apps/bt/btstack). The .c files are the ones the PicoMite's PC3 build compiles (its BT host: BLE and Classic, HID
hosts, RFCOMM, SDP, A2DP and AVDTP with the SBC encoder), less SEGGER RTT, yxml, md5 and the stdin of pico-sdk; the
firmware's CMakeLists.txt compiles every .c under Middlewares, so only those are copied. Every header of the folders
they come from is copied, with the licence.

    python tools/port_btstack.py apps/bt/btstack .
"""
import glob, os, shutil, subprocess, sys

src, cmm2 = sys.argv[1], sys.argv[2]
dst = os.path.join(cmm2, 'Middlewares', 'Third_Party', 'btstack')
C = '''src/ad_parser.c src/btstack_crypto.c src/btstack_hid.c src/btstack_hid_parser.c src/btstack_linked_list.c
src/btstack_memory.c src/btstack_memory_pool.c src/btstack_ring_buffer.c src/btstack_run_loop.c src/btstack_run_loop_base.c
src/btstack_tlv.c src/btstack_tlv_none.c src/btstack_util.c src/hci.c src/hci_cmd.c src/hci_dump.c src/hci_event.c
src/hci_event_builder.c src/l2cap.c src/l2cap_signaling.c
src/ble/att_db.c src/ble/att_db_util.c src/ble/att_dispatch.c src/ble/att_server.c src/ble/gatt_client.c
src/ble/gatt_service_client.c src/ble/le_device_db_tlv.c src/ble/sm.c src/ble/gatt-service/hids_host.c
src/ble/gatt-service/battery_service_client.c src/ble/gatt-service/device_information_service_client.c
src/classic/a2dp.c src/classic/a2dp_source.c src/classic/avdtp.c src/classic/avdtp_acceptor.c src/classic/avdtp_initiator.c
src/classic/avdtp_source.c src/classic/avdtp_util.c src/classic/btstack_link_key_db_tlv.c src/classic/btstack_sbc_bluedroid.c
src/classic/btstack_sbc_encoder_bluedroid.c src/classic/hid_host.c src/classic/rfcomm.c src/classic/sdp_client.c
src/classic/sdp_client_rfcomm.c src/classic/sdp_server.c src/classic/sdp_util.c
platform/embedded/btstack_tlv_flash_bank.c platform/embedded/hci_dump_embedded_stdout.c
3rd-party/micro-ecc/uECC.c 3rd-party/rijndael/rijndael.c
3rd-party/bluedroid/encoder/srce/sbc_analysis.c 3rd-party/bluedroid/encoder/srce/sbc_dct.c
3rd-party/bluedroid/encoder/srce/sbc_dct_coeffs.c 3rd-party/bluedroid/encoder/srce/sbc_enc_bit_alloc_mono.c
3rd-party/bluedroid/encoder/srce/sbc_enc_bit_alloc_ste.c 3rd-party/bluedroid/encoder/srce/sbc_enc_coeffs.c
3rd-party/bluedroid/encoder/srce/sbc_encoder.c 3rd-party/bluedroid/encoder/srce/sbc_packing.c'''.split()
HDIRS = ['src', 'src/ble', 'src/ble/gatt-service', 'src/classic', 'src/le-audio', 'src/le-audio/gatt-service',
         'platform/embedded', '3rd-party/micro-ecc',
         '3rd-party/rijndael', '3rd-party/bluedroid/encoder/include', '3rd-party/bluedroid/encoder/srce',
         '3rd-party/bluedroid/decoder/include', '3rd-party/yxml', '3rd-party/md5']   # (yxml.h, md5.h: btstack.h's headers include them)

commit = subprocess.run(['git', '-C', src, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
# (copied over what is there: Dropbox can hold a folder open, so it is not removed first)
n = 0
for f in C:
    os.makedirs(os.path.dirname(os.path.join(dst, f)), exist_ok=True)
    shutil.copy2(os.path.join(src, f), os.path.join(dst, f))
    n += 1
h = 0
for d in HDIRS:
    for f in glob.glob(os.path.join(src, d, '*.h')) + glob.glob(os.path.join(src, d, 'asm_arm*.inc')):
        os.makedirs(os.path.join(dst, d), exist_ok=True)
        shutil.copy2(f, os.path.join(dst, d, os.path.basename(f)))
        h += 1
for f in ('LICENSE',):
    shutil.copy2(os.path.join(src, f), os.path.join(dst, f))
for f in glob.glob(os.path.join(src, '3rd-party', 'micro-ecc', 'LICENSE*')) + \
         glob.glob(os.path.join(src, '3rd-party', 'bluedroid', '*', 'NOTICE*')) + \
         glob.glob(os.path.join(src, '3rd-party', 'bluedroid', 'LICENSE*')):
    rel = os.path.relpath(f, src)
    os.makedirs(os.path.dirname(os.path.join(dst, rel)), exist_ok=True)
    shutil.copy2(f, os.path.join(dst, rel))
open(os.path.join(dst, 'CMM2_SOURCE.md'), 'w', newline='\n').write(
    '# BTstack in the CMM2 firmware\n\n'
    'BTstack (BlueKitchen; LICENSE: personal, non-commercial use, accepted by Peter, 16 Nov 2026) from\n'
    'https://github.com/bluekitchen/btstack at %s, the commit the PicoMite\'s pico-sdk (2.3.1) has.\n'
    'Unchanged. Copied by tools/port_btstack.py: the .c files the PicoMite PC3 build compiles for its Bluetooth host\n'
    '(%d), less SEGGER RTT, yxml, md5 and pico-sdk\'s stdin, and the headers of their folders (%d). Its settings are the\n'
    'PicoMite\'s bluetooth/btstack_config.h, with the CMM2\'s additions (Inc/bt/btstack_config.h).\n' % (commit, n, h))
print('%d .c and %d headers into %s (BTstack %s)' % (n, h, dst, commit))
