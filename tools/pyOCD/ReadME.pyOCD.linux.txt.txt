Install Step:

1. install python 2.7 32 bit or 3.10 64bit
sudo apt install python2.7 or python3.10

2. update pip and install required packages

python -m pip install --upgrade pip
pip install pyusb enum34 cmsis-pack-manager colorama intelhex intervaltree prettytable pyelftools pyyaml six

3. install pyOCD

sudo pythom -m easy_install tools/pyOCD/pyocd-0.21.1.dev36-py2.7.egg for python 2.7
or
sudo pythom -m pip install tools/pyOCD/pyocd-0.34.1.tar.gz for python 3.10

Usage:

pyocd-gdbserver -p 2331
or
python -m pyocd gdbserver -p 2331