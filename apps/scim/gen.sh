#!/bin/bash
# gen.sh SRC GEN: sc-im's grammar made as its src/Makefile makes it: gram.c and y.tab.h from gram.y by bison -y -d,
# experres.h and statres.h from gram.y by eres.sed and sres.sed. make.py runs it in WSL when gram.y or the sed
# scripts change, and the four files are kept in apps/scim/gen, so that a build needs no bison.
# bison: the PATH's, else one unpacked without root into ~/tools/bison (apt-get download bison m4; dpkg -x each .deb).
set -e
src=$1
gen=$2
BISON=${BISON:-bison}
if ! command -v "$BISON" >/dev/null 2>&1 && [ -x "$HOME/tools/bison/usr/bin/bison" ]; then
    BISON=$HOME/tools/bison/usr/bin/bison
    export BISON_PKGDATADIR=$HOME/tools/bison/usr/share/bison M4=$HOME/tools/bison/usr/bin/m4
fi
tmp=$(mktemp -d)
cp "$src/gram.y" "$tmp/"
(cd "$tmp" && "$BISON" -y -d gram.y)
cp "$tmp/y.tab.c" "$gen/gram.c"
cp "$tmp/y.tab.h" "$gen/y.tab.h"
sed -f "$src/eres.sed" < "$src/gram.y" > "$gen/experres.h"
sed -f "$src/sres.sed" < "$src/gram.y" > "$gen/statres.h"
"$BISON" --version | head -1 > "$gen/bison-version.txt"
rm -rf "$tmp"
