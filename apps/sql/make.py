"""make.py: builds apps/sql/out/sql.app, SQLite for the CMM2 run from MMBasic's RUN (docs/SQL_Plan.md, step 1).

SQLite 3.47.0, the version the MicroPython port builds and tested on a G2 (its lib/usqlite/sqlite3.c and sqlite3.h are
byte for byte the amalgamation's), is fetched the first time from sqlite.org into apps/sql/sqlite (git ignores it) and
checked against the zip's SHA-256. SQLite is in the public domain. sqlite_cmm2.h is its settings (usqlite's, MIT),
sqlite_unit.c compiles it (as usqlite's usqlite.c does), sqlcore.c is its OS layer on the firmware's services, and
sql_app.c the program (MIT).

    python apps/sql/make.py
then put out/sql.app in A:/sql and, at the CMM2's prompt:
    RUN "A:/sql/sql.app", club.db "SELECT * FROM member"
"""
import hashlib, io, os, subprocess, sys, urllib.request, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'sqlite')
URL = 'https://www.sqlite.org/2024/sqlite-amalgamation-3470000.zip'
SHA256 = '2842fddbb1cc33f66c7da998a57535f14a6bfee159676a07bb4bf3e59375d93e'
FILES = ('sqlite3.c', 'sqlite3.h')


def fetch():
    """sqlite3.c and sqlite3.h from the amalgamation's zip, once"""
    if all(os.path.exists(os.path.join(SRC, f)) for f in FILES):
        return
    data = urllib.request.urlopen(URL, timeout=120).read()
    got = hashlib.sha256(data).hexdigest()
    if got != SHA256:
        sys.exit('%s: SHA-256 %s, not %s' % (URL, got, SHA256))
    os.makedirs(SRC, exist_ok=True)
    z = zipfile.ZipFile(io.BytesIO(data))
    for f in FILES:
        open(os.path.join(SRC, f), 'wb').write(z.read('sqlite-amalgamation-3470000/' + f))


# The one change to SQLite (13 Nov 2026): its sorter keeps rows in memory up to the cache's size before it writes
# them out, capped by SQLITE_MAX_PMASZ, which sqlite3.c defines whatever is set before it. Guarded, sqlite_cmm2.h's
# 256 KB holds, and a big page cache no longer makes a big sorter (a 1.5 MB cache in a 3 MB pool: CREATE INDEX ran out
# of memory on the G2).
PATCH_OLD = b'#define SQLITE_MAX_PMASZ    (1<<29)\n'
PATCH_NEW = b'#ifndef SQLITE_MAX_PMASZ            /* (CMM2: apps/sql/make.py) */\n# define SQLITE_MAX_PMASZ    (1<<29)\n#endif\n'


def patch():
    p = os.path.join(SRC, 'sqlite3.c')
    s = open(p, 'rb').read()
    if PATCH_NEW in s:
        return
    if s.count(PATCH_OLD) != 1:
        sys.exit('%s: SQLITE_MAX_PMASZ is not as make.py expects' % p)
    open(p, 'wb').write(s.replace(PATCH_OLD, PATCH_NEW))


def build(out, srcs, extra):
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), out] + srcs + extra
    cmd += ['--stack', '0x10000', '-I', SRC, '-I', HERE, '--cflag=-include' + os.path.join(HERE, 'sqlite_cmm2.h')]
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    if subprocess.run(cmd).returncode:
        sys.exit(1)


def main():
    fetch()
    patch()
    out = os.path.join(HERE, 'out')
    os.makedirs(os.path.join(out, 'mod'), exist_ok=True)
    core = [os.path.join(HERE, 'sqlite_unit.c'), os.path.join(HERE, 'sqlcore.c')]
    # sql.app: services version 10 (dir_current, for names after the current directory); 1 MB of heap at least
    build(os.path.join(out, 'sql.app'), core + [os.path.join(HERE, 'sql_app.c')], ['--api', '10', '--heap', str(1 << 20)])
    # sql.mod, the SQL command's module (services version 19: the file cut short), in a folder of its own (build.py's objects
    # and map are named after it); its heap is what SQL OPEN gives it
    build(os.path.join(out, 'mod', 'sql.mod'), core + [os.path.join(HERE, 'sql_mod.c')], ['--api', '19', '--module'])


if __name__ == '__main__':
    main()
