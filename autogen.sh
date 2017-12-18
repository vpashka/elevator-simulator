#! /bin/sh

autoreconf -fiv

./configure --enable-maintainer-mode --prefix=/usr $*
