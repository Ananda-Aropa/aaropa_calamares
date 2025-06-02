#!/bin/bash

cd "$(dirname "$0")"

current_ver=$(head -1 debian/changelog | grep -Eo '[0-9]+(\.[0-9]+){2,}')

UPSTREAM=https://github.com/calamares/calamares

upstream_version=$(wget -O /dev/null "$UPSTREAM/releases/latest" 2>&1 | grep "$UPSTREAM/releases/tag/" | head -1 | awk '{print $2}')
upstream_version=${upstream_version##*/}
upstream_version=${upstream_version#v}

sed -i -r "s/aaropa-calamares \([0-9]+(\.[0-9]+)+/aaropa-calamares ($upstream_version/g" debian/changelog

find . -type f -exec sed -i "s/$current_ver/$upstream_version/g" {} +
