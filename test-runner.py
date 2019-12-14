#!/usr/bin/env python3

"""
    This is a simple runner to test antman functionality.
    (unit tests are handled by automake)
"""

import os, subprocess, sys

# check the program help can be called
print("checking for program help...")
try:
    output1 = subprocess.check_output(['antman', '-h'])
except subprocess.CalledProcessError as e:
    errorCode = e.returncode
    print("---\nfailed to call `antman -h` (error code: {})." .format(errorCode))

# check the version number
print("checking version number...")
try:
    output2 = subprocess.run(['antman', '-v'], stdout=subprocess.PIPE)
except subprocess.CalledProcessError as e:
    errorCode = e.returncode
    sys.exit("---\nerror: failed to call `antman -v` (error code: {})." .format(errorCode))
progVersion = output2.stdout.decode('utf-8')
codeVersion = ""
with open('./VERSION') as f:
    codeVersion = f.readline()
if progVersion.rstrip('\n') != codeVersion:
    sys.exit("---\nerror: compiled version number does not match codebase ({} vs. {})." .format(progVersion.rstrip('\n'), codeVersion))

# check daemonization
print("checking daemonization...")
# first make sure we can get pid
try:
    output3 = subprocess.run(['antman', '--getPID'], stdout=subprocess.PIPE)
except subprocess.CalledProcessError as e:
    errorCode = e.returncode
    sys.exit("---\nerror: failed to call `antman --getPID` (error code: {})." .format(errorCode))
# next make sure it is -1 as antman shouldn't be running
pid = output3.stdout.decode('utf-8').rstrip('\n')
if pid != "-1":
    sys.exit("---\nerror: antman daemon shouldn't have a PID yet (got: {})." .format(pid))
# now start antman and get pid
try:
    output4 = subprocess.run(['antman', '--start'], stdout=subprocess.PIPE)
except subprocess.CalledProcessError as e:
    errorCode = e.returncode
    sys.exit("---\nerror: failed to start antman (error code: {})." .format(errorCode))
try:
    output5 = subprocess.run(['antman', '--getPID'], stdout=subprocess.PIPE)
except subprocess.CalledProcessError as e:
    errorCode = e.returncode
    sys.exit("---\nerror: failed to call `antman --getPID` (error code: {})." .format(errorCode))
pid = output5.stdout.decode('utf-8').rstrip('\n')
# check the PID is in use
try:
    os.kill(int(pid), 0)
except OSError:
    sys.exit("---\nerror: no program running using registered PID (PID: {})." .format(pid))
# check antman is using the registered PID
else:
    output6 = subprocess.run(['ps', '-p', pid, '-o', 'comm='], stdout=subprocess.PIPE)
    runningProg = output6.stdout.decode('utf-8').rstrip('\n')
    if runningProg != "antman":
        sys.exit("---\nerror: another program is running using registered PID (program: {}, PID: {})." .format(runningProg, pid))

# now stop antman
try:
    output7 = subprocess.run(['antman', '--stop'], stdout=subprocess.PIPE)
except subprocess.CalledProcessError as e:
    errorCode = e.returncode
    sys.exit("---\nerror: failed to stop antman (error code: {})." .format(errorCode))

print("---\npassed all tests.")
