Install Step:

1. install python 2.7
sudo apt install python2.7

2. update pip and install required packages

python -m pip install --upgrade pip
pip install pyusb enum34 cmsis-pack-manager colorama intelhex intervaltree prettytable pyelftools pyyaml six

3. install pyOCD

sudo pythom -m easy_install tools/pyOCD/pyocd-0.21.1.dev36-py2.7.egg


Usage:

pyocd-gdbserver -p 2331 