#!/bin/sh
#
# End-to-end screen tests.
#
# For every tests/e2e/<case>.in, run the built binary with that file on
# stdin inside a temporary copy of the repository's data/ directory, strip
# the ANSI clear-screen codes, and diff the output against
# tests/e2e/<case>.expected.
#
# Usage (from the repository root, normally via "make test"):
#     sh tests/e2e/run.sh [path-to-binary]
#
# The program writes to data/ immediately on any change, which is why each
# case runs in its own fresh copy. The repository's data/ is never touched.
#
# To add a case: write <case>.in, run the binary by hand with it, check the
# output against calc.c and docs/CODEBASE.md, then save it as
# <case>.expected. Never generate an .expected file without checking it.

set -u

DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$DIR/../.." && pwd)

# a sale is dated today; mask it so sale cases compare equal on any day
today=$(date +%F)

BIN=${1:-"$ROOT/dealerdesk"}
if [ ! -x "$BIN" ] && [ -x "$BIN.exe" ]; then
    BIN="$BIN.exe"
fi
if [ ! -x "$BIN" ]; then
    echo "e2e: binary not found: $BIN (run make first)" >&2
    exit 2
fi
case "$BIN" in
    /*) ;;
    *)  BIN="$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")" ;;
esac

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cases=0
failed=0

for input in "$DIR"/*.in; do
    name=$(basename "$input" .in)
    cases=$((cases + 1))

    rm -rf "$TMP/data"
    cp -r "$ROOT/data" "$TMP/data"

    # tr removes ESC and CR (CR appears when stdout is a pipe on Windows);
    # sed then removes the remaining "[2J[H" of the clear-screen sequence.
    (cd "$TMP" && "$BIN" < "$input") \
        | tr -d '\033\r' \
        | sed 's/\[2J\[H//g' \
        | sed "s/$today/<TODAY>/g" \
        > "$TMP/$name.out"

    # the expected file is also CR-stripped in case it was checked out as CRLF
    tr -d '\r' < "$DIR/$name.expected" \
        | sed "s/$today/<TODAY>/g" \
        > "$TMP/$name.expected"
    if diff -u "$TMP/$name.expected" "$TMP/$name.out" > "$TMP/$name.diff"; then
        echo "PASS $name"
    else
        echo "FAIL $name"
        cat "$TMP/$name.diff"
        failed=$((failed + 1))
    fi
done

echo "e2e: $cases case(s), $failed failed"
[ "$failed" -eq 0 ]
