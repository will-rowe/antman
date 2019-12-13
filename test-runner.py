#!/usr/bin/env python3

"""
    This is a simple runner to test antman functionality.
    (unit tests are handled by automake)
"""

import subprocess, sys

# check the program help can be called
print("checking for program help...")
try:
    output = subprocess.check_output(['antman', '-h'])
except subprocess.CalledProcessError as e:
    errorCode = e.returncode
    print("---\nfailed to call antman help (error code: {})." .format(errorCode))

# check the version number
print("checking version number...")
try:
    output = subprocess.run(['antman', '-v'], stdout=subprocess.PIPE)
except subprocess.CalledProcessError as e:
    errorCode = e.returncode
    print("---\nerror: failed to call antman version (error code: {})." .format(errorCode))
progVersion = output.stdout.decode('utf-8')
codeVersion = ""
with open('./VERSION') as f:
    codeVersion = f.readline()
if progVersion.rstrip('\n') != codeVersion:
    sys.exit("---\nerror: compiled version number does not match codebase ({} vs. {})." .format(progVersion.rstrip('\n'), codeVersion))

# check daemonization

print("---\npassed all tests.")