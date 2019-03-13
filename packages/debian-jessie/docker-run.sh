#!/bin/sh
set -e

cp --recursive /dbuild/sources /dbuild/building
cd /dbuild/building
make -j${CONCURRENCY} clean

make -j${CONCURRENCY} package-debian-jessie-build-install-dependencies

make -j${CONCURRENCY} package-debian-jessie-build
chmod 644 packages/debian-jessie/*.deb
cp packages/debian-jessie/*.deb /dbuild/sources/packages/debian-jessie/

