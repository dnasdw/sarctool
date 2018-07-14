# sarctool

A tool for extracting/creating sarc file.

## History

- v1.2.0 @ 2018.07.05 - Support more alignment
- v1.2.1 @ 2018.07.14 - Support modify .bffnt alignment

### v1.1

- v1.1.0 @ 2018.01.04 - A new beginning

### v1.0

- v1.0.0 @ 2015.03.02 - First release
- v1.0.1 @ 2017.06.16 - Refactoring
- v1.0.2 @ 2017.08.01 - Fix sarc header
- v1.0.3 @ 2017.08.23 - Fix typo

## Platforms

- Windows
- Linux
- macOS

## Building

### Dependencies

- cmake
- libiconv

### Compiling

- make 64-bit version
~~~
mkdir project
cd project
cmake ..
make
~~~

- make 32-bit version
~~~
mkdir project
cd project
cmake -DBUILD64=OFF ..
make
~~~

### Installing

~~~
make install
~~~

## Usage

~~~
sarctool [option...] [option]...
~~~

## Options

See `sarctool --help` messages.
