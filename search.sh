#!/bin/bash

TRACE=$1
OUTPUTDIR=$2
nConfig=1

for C in {12..15}
do
  for B in {5..7}
  do
    for P in $(seq 12 $(($C>14 ? 14 : $C)))
    do
      for S in $(seq $(($C-$P)) $(($C-$B-5)))
      do
        for T in $(seq 5 $(($C-$B-$S)))
        do
          M=$(($((32-$P))>20 ? 20 : $((32-$P))))
          echo "run #$nConfig"
          ./cachesim "-v" "-c $C" "-b $B" "-p $P" "-s $S" "-t $T" "-m $M" < $TRACE > "${OUTPUTDIR}/out_${nConfig}.out"
          nConfig=$(($nConfig+1))
        done
      done
    done
  done
done