#!/bin/bash

f=""
src=$1

for filename in $(echo ./t_results/$src/*.out); do
  params=$(grep 'AAT\|(C,B,S)\|log2' $filename | awk -F ' ' '{ print $NF }' | tail -n 5)
  CBS=$(echo "$params" | head -n 1 | tail -n 1 | tr ',' ';')
  P=$(echo "$params" | head -n 2 | tail -n 1)
  T=$(echo "$params" | head -n 3 | tail -n 1)
  M=$(echo "$params" | head -n 4 | tail -n 1)
  AAT=$(echo "$params" | head -n 5 | tail -n 1)
  f="${f}$filename,$CBS,$P,$T,$M,$AAT\n"
done


echo -n "filename,(C;B;S),P,T,M,AAT" > "./reports/$src.csv"
echo -e $f | sort -V -s -k 6 --field-separator=',' >> "./reports/$src.csv"
