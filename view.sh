#!/bin/sh

help() {
        echo "prohlizi mrizku hry live"
	echo "dva parametry urcuji levy harni roh"
	echo "pokud zadate ctyry mate urcenou i sirku a vysku zobrazovaneho"
	exit
}

vyska=22
sirka=60

if [ "$#" -eq 4 ]; then
	vyska=$3
	sirka=$4
fi


if [ "$#" -eq 2 -o "$#" -eq 4 ]; then
	x=$(( $1+$vyska))
	y=$(( $2+$sirka))
	sed -n "$1,$x p" grid.out | cut -c $2-$y
	exit
fi

help()

