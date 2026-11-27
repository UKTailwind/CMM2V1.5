"""make.py: the web browser's programs (docs/Web_Browser_Study.md). For now step 1: out/fetch.app, the curl tool
(curl 8.23.0) with libcurl, mbedTLS 3.6.8 for TLS and zlib 1.3.1 for compressed pages, on the firmware's Wi-Fi through
services version 21 (port/cmm2_net.c gives the BSD socket calls). The three are fetched the first time, as their
release archives (SHA-256 checked), into apps/browser/src (git ignores it), and built as they are: the CMM2's side is
all in port/ and fetch_main.c.

    python apps/browser/make.py
then put out/fetch.app on the card, and curl's CA bundle (https://curl.se/ca/cacert.pem) as A:/cacert.pem. At the
CMM2's prompt, with OPTION WIFI set:
    RUN "fetch.app", -o page.html https://example.com/
"""
import glob, hashlib, os, re, subprocess, sys, tarfile, urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'src')
OUT = os.path.join(HERE, 'out')
PORT = os.path.join(HERE, 'port')

PACKAGES = {                                    # folder: (url, SHA-256 of the archive)
    'curl-8.23.0': ('https://github.com/curl/curl/releases/download/curl-8_23_0/curl-8.23.0.tar.xz',
                    '8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5'),
    'mbedtls-3.6.8': ('https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.8/mbedtls-3.6.8.tar.bz2',
                      'bc54b818e87f49c0b3281dd85d24adaede0e63b4e822e3fbd49c005a38b9595b'),
    'zlib-1.3.1': ('https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.xz',
                   '38ef96b8dfe510d42707d9c781877914792541133e1870841463bfa73f883e32'),
}
CURL = os.path.join(SRC, 'curl-8.23.0')
MBEDTLS = os.path.join(SRC, 'mbedtls-3.6.8')
ZLIB = os.path.join(SRC, 'zlib-1.3.1')


def fetch(name):
    """the package's folder, from its release archive the first time"""
    url, sha = PACKAGES[name]
    folder = os.path.join(SRC, name)
    if os.path.isdir(folder):
        return
    os.makedirs(SRC, exist_ok=True)
    arc = os.path.join(SRC, os.path.basename(url))
    if not os.path.exists(arc):
        print('fetching', url, flush=True)
        urllib.request.urlretrieve(url, arc)
    got = hashlib.sha256(open(arc, 'rb').read()).hexdigest()
    if got != sha:
        sys.exit('%s: SHA-256 %s, not %s' % (arc, got, sha))
    with tarfile.open(arc) as t:
        t.extractall(SRC, filter='data')


def makefile_list(path, var):
    """the files a Makefile.inc variable lists (VAR = a.c b.c \\ ...), with $(OTHER) expanded"""
    text = open(path).read().replace('\\\n', ' ')
    m = re.search(r'^%s\s*=(.*)$' % var, text, re.M)
    if not m:
        sys.exit('%s: no %s' % (path, var))
    out = []
    for w in m.group(1).split():
        sub = re.fullmatch(r'\$\((\w+)\)', w)
        out += makefile_list(path, sub.group(1)) if sub else [w]
    return out


def sources():
    lib = os.path.join(CURL, 'lib')
    s = []
    for var in ('LIB_CFILES', 'LIB_VAUTH_CFILES', 'LIB_VDNS_CFILES', 'LIB_VTLS_CFILES', 'LIB_VQUIC_CFILES', 'LIB_CURLX_CFILES'):
        s += [os.path.join(lib, f) for f in makefile_list(os.path.join(lib, 'Makefile.inc'), var)]
    src = os.path.join(CURL, 'src')
    s += [os.path.join(src, f) for f in makefile_list(os.path.join(src, 'Makefile.inc'), 'CURL_CFILES')]
    s += [os.path.join(src, 'tool_hugehelp.c')]
    s += [os.path.join(ZLIB, f) for f in ('adler32.c', 'crc32.c', 'inflate.c', 'inftrees.c', 'inffast.c', 'zutil.c')]
    s += sorted(glob.glob(os.path.join(MBEDTLS, 'library', '*.c')))
    s += [os.path.join(PORT, f) for f in ('cmm2_net.c', 'cmm2_tls.c', 'cmm2_stdio.c')]
    s += [os.path.join(HERE, 'fetch_main.c')]
    return s


def main():
    for name in PACKAGES:
        fetch(name)
    os.makedirs(OUT, exist_ok=True)
    listfile = os.path.join(OUT, 'fetch.sources')
    open(listfile, 'w').write('\n'.join(sources()) + '\n')
    incs = [os.path.join(PORT, 'include'), PORT, os.path.join(CURL, 'include'), os.path.join(CURL, 'lib'),
            os.path.join(CURL, 'src'), ZLIB, os.path.join(MBEDTLS, 'include'),
            os.path.join(REPO, 'Drivers', 'CMSIS', 'Device', 'ST', 'STM32H7xx', 'Include'),
            os.path.join(REPO, 'Drivers', 'CMSIS', 'Include')]
    flags = ['-DHAVE_CONFIG_H', '-DBUILDING_LIBCURL', '-DCURL_STATICLIB', '-DSTM32H743xx',
             '-DMBEDTLS_USER_CONFIG_FILE=<mbedtls_user_config.h>',
             '-Wno-format']                     # (curl prints uint32_t, unsigned long here, with %u: the same 32 bits)
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(OUT, 'fetch.app'),
           '--sources', listfile, '--api', '21', '--stack', str(128 * 1024)] + \
          ['--cflag=' + f for f in flags] + sum([['-I', i] for i in incs], []) + \
          ['--ldflag=-Wl,--wrap=fopen', '--ldflag=-Wl,--wrap=fdopen']   # (port/cmm2_stdio.c: 16 KB file buffers)
    subprocess.run(cmd, check=True)


if __name__ == '__main__':
    main()
