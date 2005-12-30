#!/bin/bash
c=0
while true
do
  echo "count = " $c
  dex wall cosmos
  sleep 3 
  dex wall hspace
  sleep 3
  let "c=c+1"
done

