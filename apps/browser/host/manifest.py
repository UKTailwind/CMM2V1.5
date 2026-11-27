"""manifest.py W OUT.json: from gen.sh's native build in W, each component's sources, the generated files they use (not
tracked by git), its compile flags (-D, -U, -I, -std from its log's first compile line) and the commit it is at.
MIT License, as Inc/cmm2img.h."""
import glob, json, os, shlex, subprocess, sys

W, OUT = sys.argv[1], sys.argv[2]
INST = os.path.join(W, 'inst')
COMPONENTS = ['libwapcaplet', 'libparserutils', 'libhubbub', 'libcss', 'libdom', 'libnsbmp', 'libnsgif', 'libnsfb',
              'libnsutils', 'netsurf']


def git(repo, *a):
    return subprocess.run(['git', '-C', repo] + list(a), capture_output=True, text=True).stdout


def deps(dfile):
    first = open(dfile).read().replace('\\\n', ' ').split('\n')[0]
    return first.split(':', 1)[1].split() if ':' in first else []


def flags_from_log(log):
    for line in open(log, errors='replace'):
        if ' -c ' in line and line.split()[0] in ('cc', 'gcc'):
            return [t for t in shlex.split(line) if t.startswith(('-D', '-U', '-I', '-std'))]
    return []


man = {}
for comp in COMPONENTS:
    repo = os.path.join(W, comp)
    bdirs = ([os.path.join(repo, 'build', 'Linux-framebuffer')] if comp == 'netsurf' else
             glob.glob(os.path.join(repo, 'build-*-release-lib-static')))
    tracked = set(git(repo, 'ls-files').split('\n'))
    sources, gen = [], set()
    for b in bdirs:
        for d in glob.glob(os.path.join(b, '**', '*.d'), recursive=True):
            for k, p in enumerate(deps(d)):
                a = os.path.normpath(p if os.path.isabs(p) else os.path.join(repo, p))
                if not a.startswith(repo + '/'):
                    continue                    # (system headers, and the other libraries' installed ones)
                rel = os.path.relpath(a, repo)
                if k == 0 and rel.endswith('.c'):
                    sources.append(rel)
                if rel not in tracked:
                    gen.add(rel)
    man[comp] = {'commit': git(repo, 'rev-parse', 'HEAD').strip(), 'sources': sorted(set(sources)),
                 'generated': sorted(gen), 'flags': flags_from_log(os.path.join(W, 'log_%s.txt' % comp))}
    print('%-15s %4d sources, %3d generated, %2d flags' % (comp, len(man[comp]['sources']), len(gen),
                                                         len(man[comp]['flags'])))
json.dump(man, open(OUT, 'w'), indent=1)
