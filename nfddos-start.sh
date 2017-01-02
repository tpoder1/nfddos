#!/bin/bash 

set -x 

echo $@ >> xx-out

aname=$(cat "$2/profile" | grep action_name: | cut -f2-100 -d:)

#cat "$2/profile" | mail -s "[NFDDOS] $1 - profile: $aname" tpoder@cis.vutbr.cz

