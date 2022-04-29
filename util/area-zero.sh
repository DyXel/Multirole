#!/bin/sh

MULTIROLE_PID=0

spawn_multirole() {
	./multirole &
	MULTIROLE_PID=$!
}

spawn_multirole_if_not_running() {
	kill -s 0 $MULTIROLE_PID 2>/dev/null && return
	spawn_multirole
}

term_and_spawn_multirole() {
	save_traps=$(trap)
	[ $MULTIROLE_PID -ne 0 ] && kill $MULTIROLE_PID >/dev/null
	spawn_multirole
	eval "$save_traps"
}

spawn_multirole
trap "term_and_spawn_multirole" TERM
trap "spawn_multirole_if_not_running" CHLD

while true; do
	sleep 1
done
