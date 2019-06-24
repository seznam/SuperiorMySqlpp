#!/bin/sh
set -e

cp --recursive /dbuild/sources /dbuild/building
cd /dbuild/building
make -j${CONCURRENCY} clean

make -j${CONCURRENCY} package-debian-buster-build-install-dependencies

make -j${CONCURRENCY} package-debian-buster-build
chmod 644 packages/debian-buster/*.deb
cp packages/debian-buster/*.deb /dbuild/sources/packages/debian-buster/

