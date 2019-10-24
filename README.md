# dde-devicemanager
Device Manager is a gadget for viewing device information.


深度设备管理器是查看、管理硬件设备的工具软件。

## Depend on 
* lshw
* [dmidecode](http://www.nongnu.org/dmidecode/)
* hciconfig
* hwinfo
* lsusb
* lscpu
* [smartctl](https://www.smartmontools.org/)
* [xrandr](https://www.x.org/wiki/Projects/XRandR/)
* [CUPS](https://www.cups.org/index.html)

## Thridparty lib
* [docx](https://github.com/lpxxn/docx)
* [QtXlsx](http://qtxlsx.debao.me)

## Installation
sudo apt-get install dde-devicemanager

## Build
* mkdir build
* cd build
* qmake ..
* make

## Usage
sudo ./dde-devicemanager