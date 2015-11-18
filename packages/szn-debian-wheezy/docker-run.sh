#!/bin/sh
set -e

cp -r /dbuild/sources /dbuild/building
cd /dbuild/building
make -j${CONCURRENCY} clean

make -j${CONCURRENCY} package-szn-debian-wheezy-build-install-dependencies

make -j${CONCURRENCY} package-szn-debian-wheezy-build
chmod 644 packages/szn-debian-wheezy/*.deb
cp packages/szn-debian-wheezy/*.deb /dbuild/sources/packages/szn-debian-wheezy/

