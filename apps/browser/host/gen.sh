#!/bin/bash
# gen.sh DEST PATCHES: NetSurf and its libraries for apps/browser, run in WSL by make.py (docs/Web_Browser_Study.md).
#   1. Each repository fetched at its pinned commit into ~/src/ns-cmm2 (a Linux folder: the build is quicker there and
#      Dropbox is spared the history), and PATCHES/<repo>-*.patch applied.
#   2. Built natively, the framebuffer front end with the RAM surface and no optional library that needs flex or bison
#      (JavaScript, libnslog) or a host package WSL lacks: this makes every generated source (libcss's property
#      parsers, hubbub's tables, the front end's font and images, the version header) and the en Messages file. GNU
#      gperf, which hubbub needs, is built from source into ~/src/ns-cmm2/inst first (no root needed).
#   3. manifest.py records each component's sources, generated files and compile flags in DEST/manifest.json.
#   4. The trees (without .git, the native build, or the other front ends) go to DEST/<repo>, with the generated sources,
#      and the front end's resources, links followed, to DEST/res for the card.
# MIT License, as Inc/cmm2img.h.
set -e
DEST=$1
PATCHES=$2
HERE=$(cd "$(dirname "$0")" && pwd)
W=~/src/ns-cmm2
INST=$W/inst
mkdir -p $W $DEST
cd $W

REPOS="buildsystem libwapcaplet libparserutils libhubbub libcss libdom libnsbmp libnsgif libnsfb libnsutils netsurf"
declare -A PIN=(
  [buildsystem]=b38fe7146aa3af08881ba9d6817f19ee7abe4a86
  [libwapcaplet]=74e49210e9c56381c1e60aea309dc80074a54ede
  [libparserutils]=c9971265064b194505c6cd398dbdef8788725de7
  [libhubbub]=68989e60c786f1ee490db95c44769c368d135262
  [libcss]=0c4f51901a152ae551987aabdb7881c4f55cdc08
  [libdom]=e4150b65e4e77d6e028912c5cdae084b3a564c93
  [libnsbmp]=3406e7e0bfb5a8edc73af1cd4ae39553907514fb
  [libnsgif]=b9e3b0aadce374cce45c34feb55ad73a3381ff98
  [libnsfb]=a287525eeafa4b9e721822670e1e03eeeef6b31c
  [libnsutils]=4f0b88241304d97e10c363b81f3cf959404da82b
  [netsurf]=170d89462442c286f80c0604f30e4c7f2a801265
)

# 1. the sources, and our patches (a repository at another commit is fetched again)
for r in $REPOS; do
  if [ -d $r/.git ] && [ "$(git -C $r rev-parse HEAD)" != "${PIN[$r]}" ]; then rm -rf $r; fi
  if [ ! -d $r/.git ]; then
    git init -q $r
    git -C $r fetch -q --depth 1 https://github.com/netsurf-browser/$r.git ${PIN[$r]}
    git -C $r checkout -q FETCH_HEAD
  fi
  git -C $r checkout -q -- .                    # (the patches below applied afresh each time)
  for p in $PATCHES/$r-*.patch; do
    [ -f "$p" ] || continue
    git -C $r apply "$p"
    echo "$r: $(basename $p)"
  done
done

# 2. gperf, then the native build
if [ ! -x $INST/bin/gperf ]; then
  mkdir -p $W/tools && cd $W/tools
  [ -f gperf-3.1.tar.gz ] || curl -sSLO https://ftp.gnu.org/gnu/gperf/gperf-3.1.tar.gz
  echo "588546b945bba4b70b6a3a616e80b4ab466e3f33024a352fc2198112cdbb3ae2  gperf-3.1.tar.gz" | sha256sum -c --quiet
  rm -rf gperf-3.1 && tar xzf gperf-3.1.tar.gz && cd gperf-3.1
  ./configure --prefix=$INST > ../gperf_configure.log 2>&1
  make -j8 > ../gperf_make.log 2>&1 && make install > ../gperf_install.log 2>&1
  cd $W
fi
export PKG_CONFIG_PATH=$INST/lib/pkgconfig PATH=$INST/bin:$PATH LD_LIBRARY_PATH=$INST/lib
make -C buildsystem PREFIX=$INST install > $W/log_buildsystem.txt 2>&1
for l in libwapcaplet libparserutils libhubbub libcss libdom libnsbmp libnsgif libnsfb libnsutils; do
  # a full build each time, so that the log has every compile line (clean needs the same variables as the build)
  make -C $l PREFIX=$INST NSSHARED=$INST/share/netsurf-buildsystem clean > /dev/null 2>&1 || true
  make -C $l PREFIX=$INST NSSHARED=$INST/share/netsurf-buildsystem Q= install > $W/log_$l.txt 2>&1 \
    || { tail -30 $W/log_$l.txt; exit 1; }
done
rm -rf netsurf/build                            # (a full build: objects made with other options would be kept)
make -C netsurf TARGET=framebuffer PREFIX=$INST NSSHARED=$INST/share/netsurf-buildsystem Q= \
  NETSURF_USE_CURL=NO NETSURF_USE_OPENSSL=NO NETSURF_USE_JPEG=NO NETSURF_USE_JPEGXL=NO NETSURF_USE_PNG=NO \
  NETSURF_USE_WEBP=NO NETSURF_USE_RSVG=NO NETSURF_USE_NSSVG=NO NETSURF_USE_ROSPRITE=NO NETSURF_USE_DUKTAPE=NO \
  NETSURF_USE_NSLOG=NO NETSURF_USE_NSPSL=NO NETSURF_USE_UTF8PROC=NO NETSURF_USE_VIDEO=NO \
  NETSURF_FB_FONTLIB=internal > $W/log_netsurf.txt 2>&1 || { tail -40 $W/log_netsurf.txt; exit 1; }

# 3. the manifest
python3 $HERE/manifest.py $W $DEST/manifest.json

# 4. the trees, the generated sources and the resources, copied over what is there (Dropbox can refuse WSL a folder's
#    removal); and the libraries' headers as they are installed (dom/bindings/hubbub/parser.h...), for their users
for r in $REPOS; do
  [ $r = buildsystem ] && continue
  mkdir -p $DEST/$r
  if [ $r = netsurf ]; then
    tar -C $W/$r --exclude=.git --exclude=./build --exclude=./docs --exclude=./test --exclude=./resources \
        --exclude=./frontends/amiga --exclude=./frontends/atari --exclude=./frontends/beos --exclude=./frontends/gtk \
        --exclude=./frontends/monkey --exclude=./frontends/qt --exclude=./frontends/riscos \
        --exclude=./frontends/windows --exclude=./frontends/framebuffer/res -cf - . | tar -C $DEST/$r -xf -
    mkdir -p $DEST/$r/build/Linux-framebuffer
    cp $W/$r/build/Linux-framebuffer/*.c $W/$r/build/Linux-framebuffer/*.h $DEST/$r/build/Linux-framebuffer/
  else
    tar -C $W/$r --exclude=.git --exclude='./build-*' --exclude=./test -cf - . | tar -C $DEST/$r -xf -
  fi
done
mkdir -p $DEST/include $DEST/res
tar -C $INST/include -cf - . | tar -C $DEST/include -xf -
tar -C $W/netsurf/frontends/framebuffer/res -h --exclude=./fonts --exclude=./icons --exclude=./pointers \
    --exclude=./throbber --exclude=./de --exclude=./fr --exclude=./it --exclude=./ja --exclude=./nl --exclude=./zh_CN \
    -cf - . | tar -C $DEST/res -xf -
echo "gen.sh: done"
