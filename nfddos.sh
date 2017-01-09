#!/bin/sh

export DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}:/Users/tpoder/nfddos/libnf/src/.libs/:$(pwd)/../libnf/src/.libs:
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/Users/tpoder/nfddos/libnf/src/.libs/:$(pwd)/../libnf/src/.libs:

./nfddos -F -d 1 -p nfddos.pid $@


