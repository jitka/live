#!/bin/bash

testuj() {
	./live-opt tests/$1 $2 vysl/$1
}

make
rm vysl/*

time=0
time=$((time+`testuj turingmachine 50`))
time=$((time+`testuj gosper_glider_gun 2100`))
time=$((time+`testuj dart 2300`))
time=$((time+`testuj pulsar.in 9000`))
echo "$time"

for i in `ls vysl`
do
	diff -q vysl/$i vysl2/$i
done
