"""zip_apps_check.py ZIP [--since TAG]: is each program in a programs zip the build of its source as it is now?

For every .app and .mod in ZIP: its apps/<folder> (the zip's folder name, or the table below), the newest local build of
it (apps/<folder>/out/...), whether the zip holds exactly that build, and the commits to its source made after that
build. With --since TAG (the previous release), a program whose source has no commit since that release is UNCHANGED:
its copy carries over, as checked then. Otherwise it is OK when the zip holds its newest build and nothing was
committed to its source more than an hour after that build (builds are often committed a little after they are
made), and REVIEW if not: rebuild and retest it, or say why the zip's copy stands (a commit that only moved comments,
a build made in a worktree...). Programs built outside this repository are EXTERNAL, with where to look.

--since TAG also lists every commit since that tag (the previous release) to apps/ and tools/sdk, for the notes: each
one is in them, or is not visible to a user. (docs/Release_Checklist.md, sections 7 and 8; 18 Nov 2026, after
8.00.00b2 went out without zeptoforth 1.17.2.1 and with a zeptoforth_g1.app that did not start under version 8.)
"""
import glob, hashlib, os, subprocess, sys, zipfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FOLDER = {'quest': 'scummvm', 'Doom': 'doom'}          # zip folder -> apps/ folder, where the names differ
EXTERNAL = {'micropython.app': "WSL ~/src/micropython, branch cmm2 (ports/cmm2): git log since the last release"}
NOTE = {'cc/cc.app': "(and FUZIX's branch cmm2, the compiler's source: its commits since the last release)"}
SLACK = 3600                                           # seconds a commit may follow its build


def git(*a):
    return subprocess.run(['git', '-C', REPO] + list(a), capture_output=True, text=True).stdout


def main():
    args = sys.argv[1:]
    since = None
    since_t = 0
    if '--since' in args:
        i = args.index('--since')
        since = args[i + 1]
        del args[i:i + 2]
    if len(args) != 1:
        sys.exit(__doc__)
    if since:
        since_t = int(git('log', '-1', '--format=%ct', since).strip() or 0)
        if not since_t:
            sys.exit('no such tag or commit: ' + since)
    z = zipfile.ZipFile(args[0])
    sha = lambda b: hashlib.sha256(b).hexdigest()[:12]
    review = 0
    for n in sorted(z.namelist()):
        if not n.endswith(('.app', '.mod')):
            continue
        data = z.read(n)
        top = n.split('/')[0] if '/' in n else ''
        folder = FOLDER.get(top, top.lower())
        if n in EXTERNAL or not folder or not os.path.isdir(os.path.join(REPO, 'apps', folder)):
            print('EXTERNAL %-30s zip %s  %s' % (n, sha(data), EXTERNAL.get(n, 'not built in apps/: find its source')))
            continue
        log = [l for l in git('log', '--format=%ct %h %s', '--', 'apps/' + folder).splitlines() if l.strip()]
        if since and not [l for l in log if int(l.split()[0]) > since_t]:
            print('UNCHANGED %-29s zip %s  no commit to apps/%s since %s %s' % (n, sha(data), folder, since, NOTE.get(n, '')))
            continue
        builds = [p for p in glob.glob(os.path.join(REPO, 'apps', folder, 'out', '**', os.path.basename(n)), recursive=True)]
        builds.sort(key=os.path.getmtime)
        if not builds:
            print('REVIEW   %-30s zip %s  no local build in apps/%s/out (built in a worktree or elsewhere?)' % (n, sha(data), folder))
            review += 1
            continue
        newest = builds[-1]
        same = open(newest, 'rb').read() == data
        built = int(os.path.getmtime(newest))
        later = [l for l in log if int(l.split()[0]) > built + SLACK]
        ok = same and not later
        review += not ok
        print('%-8s %-30s zip %s  build %s (%s)%s' % ('OK' if ok else 'REVIEW', n, sha(data),
              os.path.relpath(newest, REPO), 'the zip holds it' if same else 'NOT the zip\'s',
              '' if not later else '; committed since: ' + '; '.join(' '.join(l.split()[1:])[:70] for l in later)) + ' ' + NOTE.get(n, ''))
    print('%d to review' % review)
    if since:
        print()
        print('Commits since %s to apps/ and tools/sdk (each in the notes, or not visible to a user):' % since)
        print(git('log', '--format=%h %ad %s', '--date=format:%d %b', since + '..HEAD', '--', 'apps', 'tools/sdk'), end='')


if __name__ == '__main__':
    main()
