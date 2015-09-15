#!/bin/sh
set -e

cp -r /dbuild/sources /dbuild/building
cd /dbuild/building
make -j clean

make -j package-debian-jessie-build-install-dependencies

make -j package-debian-jessie-build
chmod 644 packages/debian-jessie/*.deb
cp packages/debian-jessie/*.deb /dbuild/sources/packages/debian-jessie/

