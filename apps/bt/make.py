"""make.py: builds apps/bt/out/bt.app, step 1 of docs/Bluetooth_Plan.md: the Bluetooth half of the CYW43439 on Peter's
RM2 module, brought up from a loadable program with the firmware as it is. BTstack (BlueKitchen; its licence allows
personal, non-commercial use, Peter's decision of 16 Nov 2026) is fetched the first time into apps/bt/btstack (git
ignores it) at the commit pico-sdk 2.3.1 has, as the PicoMite builds it. cyw43-driver is apps/wifi's (fetched by
apps/wifi/make.py if it is not there), with the combined WiFi and Bluetooth firmware and the Bluetooth patch from its
firmware folder.

port/ holds pico-sdk 2.3.1's Bluetooth glue (BSD-3-Clause): the HCI transport over the chip's shared bus
(btstack_hci_transport_cyw43.c, btstack_chipset_cyw43.c, cybt_shared_bus*.c), copied as it is, and the PicoMite's
bluetooth/btstack_config.h as it is (built as the BT host's, PICOMITEBTH). The bus, the pins and the poll are
apps/wifi/port's. BTstack runs on its own embedded run loop here, where pico-sdk used an async context.

    python apps/bt/make.py
then put out/bt.app on the card, and at the CMM2's prompt:
    RUN "bt.app", scan [seconds]
"""
import glob, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT = os.path.join(HERE, 'out')
WIFI = os.path.join(REPO, 'apps', 'wifi')
CYW43 = os.path.join(WIFI, 'cyw43-driver')
BTSTACK = os.path.join(HERE, 'btstack')
BTSTACK_URL = 'https://github.com/bluekitchen/btstack.git'
BTSTACK_COMMIT = 'eb0bb8b5ea6d234ccb940313b47f7a5c3b4e20ec'   # pico-sdk 2.3.1's lib/btstack, as the PicoMite builds it

sys.path.insert(0, WIFI)
import make as wifi_make                        # (its fetch(), and its cyw43-driver commit)


def sources():
    s = [os.path.join(HERE, 'bt.c')]
    s += [os.path.join(HERE, 'port', f) for f in ('btstack_hci_transport_cyw43.c', 'btstack_chipset_cyw43.c',
                                                  'cybt_shared_bus.c', 'cybt_shared_bus_driver.c',
                                                  'btstack_run_loop_cmm2.c')]
    s += [os.path.join(WIFI, 'port', f) for f in ('cyw43_bus_cmm2.c', 'cyw43_driver_cmm2.c', 'cyw43_arch_cmm2.c')]
    s += [os.path.join(CYW43, 'src', f) for f in ('cyw43_ll.c', 'cyw43_ctrl.c', 'cyw43_stats.c')]
    b = os.path.join(BTSTACK, 'src')
    # BTstack: pico-sdk's pico_btstack_base, _ble and the Classic parts the PicoMite's BT host uses (but A2DP: no
    # speakers here), with the embedded run loop and a TLV in RAM for this test; the linker keeps only what is used
    s += [os.path.join(b, f) for f in (
        'ad_parser.c', 'btstack_crypto.c', 'btstack_hid.c', 'btstack_hid_parser.c', 'btstack_linked_list.c',
        'btstack_memory.c', 'btstack_memory_pool.c', 'btstack_ring_buffer.c', 'btstack_run_loop.c',
        'btstack_run_loop_base.c', 'btstack_tlv.c', 'btstack_tlv_none.c', 'btstack_util.c', 'hci.c', 'hci_cmd.c',
        'hci_dump.c', 'hci_event.c', 'l2cap.c', 'l2cap_signaling.c')]
    s += [os.path.join(b, 'ble', f) for f in ('att_db.c', 'att_dispatch.c', 'att_server.c', 'gatt_client.c',
                                              'le_device_db_tlv.c', 'sm.c')]
    s += [os.path.join(b, 'classic', f) for f in ('btstack_link_key_db_tlv.c', 'sdp_client.c', 'sdp_util.c',
                                                  'sdp_server.c', 'hid_host.c')]
    for f in ('hci_event_builder.c',):
        if os.path.exists(os.path.join(b, f)):
            s.append(os.path.join(b, f))
    s += [os.path.join(BTSTACK, 'platform', 'embedded', f) for f in (
        'btstack_tlv_flash_bank.c', 'hal_flash_bank_memory.c', 'hci_dump_embedded_stdout.c')]
    s += [os.path.join(BTSTACK, '3rd-party', 'micro-ecc', 'uECC.c'),
          os.path.join(BTSTACK, '3rd-party', 'rijndael', 'rijndael.c')]
    return s


def main():
    wifi_make.fetch(CYW43, wifi_make.CYW43_URL, wifi_make.CYW43_COMMIT)
    wifi_make.fetch(BTSTACK, BTSTACK_URL, BTSTACK_COMMIT)
    os.makedirs(OUT, exist_ok=True)
    incs = [os.path.join(HERE, 'port'), os.path.join(WIFI, 'port'), os.path.join(CYW43, 'src'),
            os.path.join(CYW43, 'firmware'), os.path.join(BTSTACK, 'src'), os.path.join(BTSTACK, 'platform', 'embedded'),
            os.path.join(BTSTACK, '3rd-party', 'micro-ecc'), os.path.join(BTSTACK, '3rd-party', 'rijndael'),
            os.path.join(REPO, 'Drivers', 'CMSIS', 'Device', 'ST', 'STM32H7xx', 'Include'),
            os.path.join(REPO, 'Drivers', 'CMSIS', 'Include')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(OUT, 'bt.app')] + sources() + \
          ['--api', '2', '--cflag=-DSTM32H743xx', '--cflag=-DCYW43_LWIP=0', '--cflag=-DCYW43_ENABLE_BLUETOOTH=1',
           '--cflag=-DPICOMITEBTH', '--cflag=-DENABLE_BLE=1', '--cflag=-DENABLE_CLASSIC=1', '-j', '16']
    if 'debug' in sys.argv[1:]:                 # (python make.py debug: the glue's debug messages, BTstack's log and its
        cmd += ['--cflag=-DCYBT_DEBUG_ENABLED=1', '--cflag=-DENABLE_LOG_INFO', '--cflag=-DENABLE_LOG_ERROR',
                '--cflag=-DBT_APP_DUMP']        # HCI packets, as text)
    for i in incs:
        cmd += ['-I', i]
    subprocess.run(cmd, check=True)


if __name__ == '__main__':
    main()
