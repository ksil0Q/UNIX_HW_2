#!/bin/bash

rm ./filename.lck 2>>/dev/null
rm ./filename 2>>/dev/null
rm ./statistics.log 2>>/dev/null
rm ./nohup.out 2>>/dev/null


make build
make config

pids=();
for ((i = 0; i < 10; i++))
do
    nohup ./task -f filename &
    pid=$!
    pids+=("$pid")
done

sleep 30

for pid in "${pids[@]}"
do
    kill -2 "$pid" 2>>/dev/null
done
