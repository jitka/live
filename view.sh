#!/bin/sh

help() {
        echo "prohlizi mrizku hry live"
	echo "nejdriv zadejte jmeno souboru"
	echo "dva parametry urcuji levy harni roh"
	echo "pokud zadate ctyry mate urcenou i sirku a vysku zobrazovaneho"
	exit
}

vyska=22
sirka=60

if [ "$#" -eq 5 ]; then
	vyska=$4
	sirka=$5
fi


if [ "$#" -eq 3 -o "$#" -eq 5 ]; then
	x=$(( $2+$vyska))
	y=$(( $3+$sirka))
	sed -n "$2,$x p" $1 | cut -c $3-$y
	exit
fi

help()

