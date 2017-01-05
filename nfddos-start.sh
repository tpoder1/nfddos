#!/bin/bash 

cd $2
set -x 

NFDUMPP="../../libnf/bin/nfdumpp"



aname=$(cat profile | grep action_name: | cut -f2-100 -d:)

cat profile > mail 
echo "Action dir: $2" >> mail
echo "" >> mail
echo "FLow report: " >> mail
$NFDUMPP -r nfcap -n 15 -O bytes -A srcip -A dstip -A srcport -A dstport -O first -A first/60 >> mail 

cat mail | mail -s "[NFDDOS] $1 - profile: $aname" tpoder@cis.vutbr.cz

