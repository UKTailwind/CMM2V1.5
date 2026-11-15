"""make.py: builds apps/wifi/out/wifi.app, step 1 of docs/Web_Plan.md: the Infineon CYW43439 on Peter's RM2 module
(the rear I/O header: SPI2 half-duplex on PB13 and PB15, CS PI3, WL_ON PI8) driven from a loadable program, with the
firmware as it is. George Robotics' cyw43-driver (MIT) and lwIP (BSD-3-Clause) are fetched the first time, into
apps/wifi/cyw43-driver and apps/wifi/lwip (git ignores them), at the commits the PicoMite's pico-sdk has. The chip's
firmware blob and NVRAM come from cyw43-driver's firmware folder, as the PicoMite's do.

port/ holds the CMM2's side: cyw43_configport.h (pico-sdk's, its RP2040 GPIO and timers replaced), cyw43_bus_cmm2.c
(pico-sdk's cyw43_bus_pio_spi.c: the gSPI framing as it is, the transfer by SPI2 instead of a PIO) and
cyw43_driver_cmm2.c (pico-sdk's cyw43_driver.c, polled, with no async context).

    python apps/wifi/make.py
then put out/wifi.app on the card, and at the CMM2's prompt:
    RUN "wifi.app", scan
"""
import glob, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT = os.path.join(HERE, 'out')
CYW43 = os.path.join(HERE, 'cyw43-driver')
CYW43_URL = 'https://github.com/georgerobotics/cyw43-driver.git'
CYW43_COMMIT = '055d64274b014dd7b1c2fc94d26e8a18face7124'     # pico-sdk's lib/cyw43-driver, as the PicoMite builds it
LWIP = os.path.join(HERE, 'lwip')
LWIP_URL = 'https://github.com/lwip-tcpip/lwip.git'
LWIP_COMMIT = '77dcd25a72509eb83f72b033d219b1d40cd8eb95'      # pico-sdk's lib/lwip: STABLE-2_2_1_RELEASE
MBEDTLS = os.path.join(HERE, 'mbedtls')
MBEDTLS_URL = 'https://github.com/Mbed-TLS/mbedtls.git'
MBEDTLS_COMMIT = '0bebf8b8c7f07abe3571ded48a11aa907a1ffb20'   # pico-sdk's lib/mbedtls: v3.6.6


def fetch(path, url, commit):
    if not os.path.isdir(path):                 # (that commit alone: a whole history will not unpack in a Dropbox folder)
        subprocess.run(['git', 'init', '-q', path], check=True)
        subprocess.run(['git', '-C', path, 'fetch', '-q', '--depth', '1', url, commit], check=True)
        subprocess.run(['git', '-C', path, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', path, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != commit:
        sys.exit('%s is at %s, not %s' % (path, head, commit))


def sources():
    s = [os.path.join(HERE, 'wifi.c'),
         os.path.join(HERE, 'port', 'cyw43_bus_cmm2.c'),
         os.path.join(HERE, 'port', 'cyw43_driver_cmm2.c')]
    s += [os.path.join(HERE, 'port', 'cyw43_arch_cmm2.c')]
    s += [os.path.join(CYW43, 'src', f) for f in ('cyw43_ll.c', 'cyw43_ctrl.c', 'cyw43_stats.c', 'cyw43_lwip.c')]
    lw = os.path.join(LWIP, 'src')
    s += sorted(glob.glob(os.path.join(lw, 'core', '*.c'))) + sorted(glob.glob(os.path.join(lw, 'core', 'ipv4', '*.c')))
    s += [os.path.join(lw, 'netif', 'ethernet.c')]
    # TLS: mbedTLS's library, built with the PicoMite's net/mbedtls_config.h (port/), lwIP's altcp_tls glue (pico-sdk's
    # copy for mbedTLS 3, port/altcp_tls_mbedtls.c, as the PicoMite builds it) and what the config asks of the firmware
    s += [os.path.join(HERE, 'port', 'altcp_tls_mbedtls.c'), os.path.join(HERE, 'port', 'mbedtls_cmm2.c'),
          os.path.join(lw, 'apps', 'altcp_tls', 'altcp_tls_mbedtls_mem.c')]
    s += sorted(glob.glob(os.path.join(MBEDTLS, 'library', '*.c')))
    return s


def main():
    fetch(CYW43, CYW43_URL, CYW43_COMMIT)
    fetch(LWIP, LWIP_URL, LWIP_COMMIT)
    fetch(MBEDTLS, MBEDTLS_URL, MBEDTLS_COMMIT)
    os.makedirs(OUT, exist_ok=True)
    incs = [os.path.join(HERE, 'port'), os.path.join(CYW43, 'src'), os.path.join(CYW43, 'firmware'),
            os.path.join(LWIP, 'src', 'include'), os.path.join(LWIP, 'src', 'apps', 'altcp_tls'),
            os.path.join(MBEDTLS, 'include'), os.path.join(MBEDTLS, 'library'),
            os.path.join(REPO, 'Drivers', 'CMSIS', 'Device', 'ST', 'STM32H7xx', 'Include'),
            os.path.join(REPO, 'Drivers', 'CMSIS', 'Include')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(OUT, 'wifi.app')] + sources() + \
          ['--api', '2', '--cflag=-DSTM32H743xx', '--cflag=-DCYW43_LWIP=1', '--cflag=-DMBEDTLS_CONFIG_FILE=<mbedtls_config.h>',
           '--cflag=-DMBEDTLS_SSL_IN_CONTENT_LEN=16384', '-j', '16']
    for i in incs:
        cmd += ['-I', i]
    subprocess.run(cmd, check=True)


if __name__ == '__main__':
    main()
