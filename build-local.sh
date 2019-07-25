#!/bin/sh

set -eax

(cd pkg/src && ./make-build-info.sh)
(cd pkg && autoreconf -i)
(rm -rf built && mkdir -p built && cd built && ../pkg/configure && make clean && make)
