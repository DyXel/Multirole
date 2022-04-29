#!/bin/sh

# area-zero.sh - Helper script for Multirole
# ==========================================
#
# Runs the main server executable and manages its lifetime.
#
# When initially executed, this script will launch an instance of Multirole
# and will wait until it exits (gracefully or not), after that, it will try
# to launch another instance and wait for that one, repeating the process.
#
# A signal handler for SIGTERM is installed, when the signal is received it is
# forwarded to the currently running server instance, and after a small grace
# period this script will automatically launch another server executable,
# resuming the waiting process described previously.
#
# This script is never meant to exit gracefully, therefore if you want to
# terminate it, signal the process with SIGKILL (which cannot be caught),
# this will relinquish the ownership of the currently running server instance
# to the operating system. After that, it's just a matter of signaling Multirole
# with SIGTERM so it can finish its execution gracefully.

MULTIROLE_PID=0

ts_echo() {
	date "+[%Y-%m-%d %T.???] $1"
}

launch_multirole() {
	ts_echo "Launching Multirole..."
	./multirole &
	MULTIROLE_PID=$!
}

launch_multirole_if_not_running() {
	kill -s 0 $MULTIROLE_PID 2>/dev/null && return
	ts_echo "Multirole exited without my signaling! Relaunching..."
	launch_multirole
}

term_and_launch_multirole() {
	ts_echo "Signaling Multirole..."
	disown %%
	kill $MULTIROLE_PID >/dev/null
	launch_multirole
}

trap "launch_multirole_if_not_running" CHLD
launch_multirole
trap "term_and_launch_multirole" TERM

while true; do
	sleep 1
done
