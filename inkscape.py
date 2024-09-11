#!/usr/bin/env python3
import os, sys, subprocess

if '--install' in sys.argv:
	cmd = 'sudo apt-get install libxslt-dev libboost-all-dev liblcms2-dev libgc-dev libdouble-conversion-dev libpotrace-dev libpangomm-2.48-dev libcairomm-1.16-dev libgtkmm-4.0-dev mm-common'
	print(cmd)
	subprocess.check_call(cmd.split())


_thisdir = os.path.split(os.path.abspath(__file__))[0]
_buildir = os.path.join(_thisdir, 'build')
if not os.path.isdir(_buildir):
	os.mkdir(_buildir)

cmd = ['cmake', os.path.abspath(_thisdir)]
print(cmd)
subprocess.check_call(cmd, cwd=_buildir)