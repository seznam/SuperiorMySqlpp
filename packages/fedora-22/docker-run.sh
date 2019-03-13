#!/bin/sh
set -e

cp --recursive /dbuild/sources /dbuild/building
cd /dbuild/building
make -j${CONCURRENCY} clean

make -j${CONCURRENCY} package-fedora-22-build-install-dependencies

make -j${CONCURRENCY} package-fedora-22-build
chmod 644 packages/fedora-22/*.rpm
cp packages/fedora-22/*.rpm /dbuild/sources/packages/fedora-22/
