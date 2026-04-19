# dyneeded: analyze the dynamic dependencies of an executable
## Description
A cross platform and fancier `ldd` / `dumpbin` that's meant for those who are trying to ship native binaries.

It's **killer features** are:
- Tells you the **minimum required versions** of the libraries (GLIBC, GLIBCXX, CXXABI, GCC) to run the executable
- Works with **PE**, **ELF** and **MachO** binaries, and you don't even need to be on the same platform as the executable you're analyzing
- Fancy **color** output for more efficient **LARPing**

## Simple usage
99% of the time you'll be doing
```bash
dyneeded <executable>
```
And will get output like
```bash
╭───────────────────────────────────────────────────────────────────────╮
│ Executable: /home/kingdom_of_cpp/Téléchargements/dyneeded/bin/dyneeded│
├───────────────────────────────────────────────────────────────────────┤
│ Dynamic dependencies                                                  │
│  libstdc++.so.6       => /lib/libstdc++.so.6                          │
│  ld-linux-x86-64.so.2 => /lib64/ld-linux-x86-64.so.2                  │
│  libm.so.6            => /lib/libm.so.6                               │
│  libgcc_s.so.1        => /lib/libgcc_s.so.1                           │
│  libc.so.6            => /lib/libc.so.6                               │
│  ld-linux.so.2        => /lib/ld-linux.so.2                           │
├───────────────────────────────────────────────────────────────────────┤
│ Minimum directly required versions                                    │
│  GCC      3.0                                                         │
│  GLIBC    2.34                                                        │
│  CXXABI   1.3.9                                                       │
│  GLIBCXX  3.4.32                                                      │
╰───────────────────────────────────────────────────────────────────────╯
```
In a real terminal its colored with a bunch of other graphical effects that may or may not harm readability!

## Advanced usage
There's some more flags if you ever need to use this thing
seriously
- `-r` or `--recursive` to also analyze the shared objects and their dependencies
- `-j` or `--json` to get the output in JSON format
- `-tr` or `--tree` to get the output in a tree format
- `-t` or `--text` to get the output in a human readable but not too fancy format (for centrists)
- `-c` or `--classic` to get the same output format as `ldd` (for boomers)

## LARPing with dyneeded
To please the LARPers I could've written this thing in Rust
but I decided to add LARP options. You must find them thoughbeit.

## Being based with dyneeded
dyneeded is actually gem. Try to find the funny options.

## Reusing dyneeded code
At it's `/core/`, it's a small wrapper around the `LIEF` library, so you might want to use `LIEF` directly.
But if for whatever reason you want to then just plug the `/core/` directory into your project, with its 
dependencies.

## Building
1. Make sure to have a compiler and git installed
2. Make sure to have `xmake` installed
3. `cd` into the project directory and run `xmake` to build the project
4. It will ask you to install libraries, just press `y` to install them
5. The executable will be located in the `build/{os}/{arch}/release` directory