#!/bin/sh

export DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}:/Users/tpoder/nfddos/libnf/src/.libs/

./nfddos -F -p nfddos.pid $@


