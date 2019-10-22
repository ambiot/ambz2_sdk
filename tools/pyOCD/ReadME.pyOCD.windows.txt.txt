Install Step:

1. install python 2.7
download latest python2.7 "Windows x86 MSI" from https://www.python.org/downloads/release/python-2716/
current version is 2.7.16

setup system path for python and its script
Advanced system settings -> environment variables -> Add python install path and script to PATH
eg. 
C:\Python27 
C:\Python27\Scripts

2. update pip and install required packages

python -m pip install --upgrade pip
pip install pywinusb enum34 cmsis-pack-manager colorama intelhex intervaltree prettytable pyelftools pyyaml six

3. install pyOCD

install tools/pyOCD/pyocd-0.21.1.dev35+dirty.win32.exe



Usage:

pyocd-gdbserver -p 2331 