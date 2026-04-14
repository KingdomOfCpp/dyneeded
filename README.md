# dyneeded: analyze the dynamic dependencies of an executable
## Description
To know what shared objects an executable is using, on Linux you use 
`ldd` and on Windows you use `dumpbin`.
Except they're quite basic, don't work the same way and i 
didn't make them myself.

So i made `dyneeded` !
## Simple usage
99% of the time you'll be doing
```bash
dyneeded <executable>
```
And will get output like
```bash
Executable: /path/to/executable

Shared objects:
- /path/to/shared/object1.so
- /path/to/shared/object2.so
- /path/to/shared/object3.so

Missing shared objects:
- name of any shared object
- dyneeded could not find 

Minimal GLIBC: 2.27
Minimal GLIBCXX: 3.4.22
```
(On Windows the last two fields is the minium C++ runtime version)

## Advanced usage
There's some more flags if you ever need to use this thing
seriously
- `-r` or `--recursive` to also analyze the shared objects and their dependencies
- `-j` or `--json` to get the output in JSON format
- '-tr' or `--tree` to get the output in a tree format
- '-t' or `--text` to get the output in a human readable but not too fancy format (for centrists)
- '-c' or '--classic' to get the exact same output format as `ldd` (for boomers)

## LARPing with dyneeded
To please the LARPers I could've written this thing in Rust
but I decided to add LARP options. You must find them thoughbeit.

## Being based with dyneeded
dyneeded is actually gem. Try to find the funny options.

## Building
1. Make sure to gave a compiler and git installed
2. Make sure to have `xmake` installed
3. `cd` into the project directory and run `xmake` to build the project
4. It will ask you to install libraries, just press `y` to install them
5. The executable will be located in the `build/{os}/{arch}/release` directory