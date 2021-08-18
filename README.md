This is the source code and makefile to build the "PackScan" memory pack scanning tool (packscan). packscan is a command-line utility that performs basic parsing and forensics of data dumped from Super Famicom memory packs. It searches a dump to find content headers, determine which blocks of the pack's flash memory contain data, calculate CRCs on content data, and provide other useful information. 

*** Building the Software ***

Because packscan does not rely on any external dependencies, it should build on any Unix-like platform that understands makefiles and has a C++ compiler. Simply build it by running "make".

*** Using the Software ***

Most users will use packscan to generate a report like this:

$ ./packscan [NAME OF DUMP FILE]

For other options, run packscan with a "-h" option for help on other options:

$ ./packscan -h

*** Credits ***

The Shift-JIS to UTF8 conversion code in packscan is copied from the "apollo-ps3" project: https://github.com/bucanero/apollo-ps3. All other source code in this project is original to packscan.

*** Licenses ***

All source code in this project inherits a GPL v3 license from the apollo-ps3 project.

*** Contact ***

The code in this project was written by Andrew Henderson (hendersa@icculus.org) and is part of the BeagleSatella project. BeagleSatella is an effort to develop a software/hardware platform that can interface with SNES/SFC consoles as a external peripheral. Learn more about BeagleSatella here:

https://www.beaglesatella.org

