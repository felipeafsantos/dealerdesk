#!/bin/sh
#
# check-version.sh <base-ref>
#
# Fails unless the version in screen.c is greater than the version on
# <base-ref>, and no tag with the new version exists yet. CI runs it on pull
# requests that change the program; run it locally with:
#
#     sh .github/scripts/check-version.sh origin/master
#
# The version is the "DEALERDESK vX.Y" string in screen.c (see "make version").

set -u

base_ref=${1:?usage: check-version.sh <base-ref>}

extract() {
    grep -o -E 'DEALERDESK v[0-9]+\.[0-9]+' | head -1 | cut -d' ' -f2
}

head_ver=$(extract < screen.c)
base_ver=$(git show "$base_ref:screen.c" 2>/dev/null | extract)

if [ -z "$head_ver" ]; then
    echo "::error::no DEALERDESK vX.Y version string found in screen.c"
    exit 1
fi
if [ -z "$base_ver" ]; then
    echo "::error::could not read screen.c on $base_ref"
    exit 1
fi

# compare major.minor numerically
num() {
    echo "$1" | sed 's/^v//' | awk -F. '{ printf "%d%04d\n", $1, $2 }'
}

if [ "$(num "$head_ver")" -le "$(num "$base_ver")" ]; then
    echo "::error::version is $head_ver but $base_ref is $base_ver; bump the version in screen.c (and the README title) in a pull request that changes the program"
    exit 1
fi

if [ -n "$(git tag -l "$head_ver")" ]; then
    echo "::error::tag $head_ver already exists; pick the next version"
    exit 1
fi

echo "version $base_ver -> $head_ver"
