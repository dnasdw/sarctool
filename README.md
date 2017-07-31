# sarctool

A tool for extracting/creating sarc file.

## History

- v1.0.0 @ 2015.03.02 - First release
- v1.0.1 @ 2017.06.16 - Refactoring
- v1.0.2 @ 2017.08.01 - Fix sarc header

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
