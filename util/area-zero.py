#!/bin/env python3

"""
area-zero.py - Helper script for Multirole
==========================================

Runs the main server executable and manages its lifetime.

When initially executed, this script will launch an instance of Multirole
and will wait until it exits (gracefully or not), after that, it will try
to launch another instance and wait for that one, repeating the process.

A signal handler for SIGTERM is installed, when the signal is received it is
forwarded to the currently running server instance, and after a small grace
period this script will automatically launch another server executable,
resuming the waiting process described previously.

This script is never meant to exit gracefully, therefore if you want to
terminate it, signal the process with SIGKILL (which cannot be caught),
this will relinquish the ownership of the currently running server instance
to the operating system. After that, it's just a matter of signaling Multirole
with SIGTERM so it can finish its execution gracefully.
"""

import asyncio
from datetime import datetime
import subprocess
import signal

MULTIROLE_EXEC = './multirole'
SIGNALING_PERIOD_IN_SECONDS = 3
POLL_RATE_IN_SECONDS = 1

def TPrint(s):
	print('[' + str(datetime.now()) + '] ' + s)

class AreaZero():
	def __init__(self):
		self.process = None
		self.wait_task = None

	async def Start(self):
		await self.Launch()
		self.wait_task = asyncio.create_task(self.Wait())
		await self.wait_task

	async def Launch(self):
		TPrint('Launching ' + "'"+ MULTIROLE_EXEC + "'")
		self.process = await asyncio.create_subprocess_exec(MULTIROLE_EXEC)

	async def Wait(self):
		try:
			while True:
				try:
					await asyncio.wait_for(self.process.wait(), POLL_RATE_IN_SECONDS)
					TPrint('Multirole exited without being signaled!')
					await self.Launch()
				except asyncio.TimeoutError:
					pass
		except asyncio.exceptions.CancelledError:
			pass

	async def Signal(self): # NOTE: Assumes that self.Start() was called before.
		TPrint('Signaling multirole...')
		self.wait_task.cancel()
		self.process.send_signal(signal.SIGTERM)
		TPrint('Waiting signaling period (' + str(SIGNALING_PERIOD_IN_SECONDS) + ' seconds)...')
		await asyncio.sleep(SIGNALING_PERIOD_IN_SECONDS)
		await self.Start()

def SigHandler(loop, areaZero):
	loop.create_task(areaZero.Signal())

if __name__ == '__main__':
	areaZero = AreaZero()
	loop = asyncio.get_event_loop()
	loop.add_signal_handler(signal.SIGTERM, SigHandler, loop, areaZero)
	loop.create_task(areaZero.Start())
	loop.run_forever()
