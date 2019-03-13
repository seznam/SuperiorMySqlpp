#!/bin/sh
set -e

cp --recursive /dbuild/sources /dbuild/building
cd /dbuild/building
make -j${CONCURRENCY} clean

make -j${CONCURRENCY} package-debian-stretch-build-install-dependencies

make -j${CONCURRENCY} package-debian-stretch-build
chmod 644 packages/debian-stretch/*.deb
cp packages/debian-stretch/*.deb /dbuild/sources/packages/debian-stretch/

