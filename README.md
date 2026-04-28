# dyneeded: analyze the dynamic dependencies of an executable
## Description
A cross platform and fancier `ldd` / `dumpbin` that's meant for those who are trying to ship native binaries and want to know
what dll's to include and what system libraries are needed.

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
│ Executable: /home/kingdom_of_cpp/Downloads/dyneeded/bin/dyneeded      │
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
- `-j` or `--json` to get the output in JSON format
- `-tr` or `--tree` to get the output in a tree format
- `-t` or `--text` to get the output in a human readable but not too fancy format (for centrists)
- `-c` or `--classic` to get the same output format as `ldd` (for boomers)

## Installing
### Installing on Windows
Head over to releases, download the setup from the latest version, run it, tick "add to path", and you're good to go you now can use dyneeded in the terminal

### Installing on Linux
On fedora based distros, download the .rpm, on debian based distros (including Ubuntu), download the .deb and on Arch based distros, download the .pkg.tar.zst. And then install
with your distro's package manager.

## LARPing with dyneeded
To please the LARPers I could've written this thing in Rust
but I decided to add LARP options. You must find them thoughbeit.

## Being based with dyneeded
dyneeded is actually gem. Try to find the funny options.

## Helping me develop it
If it does an error or takes too much time to compute (because yes, lag is a bug), then open an issue! 

You can also try to write code but i'll probably deny your PR and write it myself.

## Reusing dyneeded code
At it's `/core/`, it's a small wrapper around the `LIEF` library, so you might want to use `LIEF` directly.
But if for whatever reason you want to then just plug the `/core/` directory into your project, with its 
dependencies.

## Building
It builds with `CMake` and `vcpkg` with no extra steps.
If you don't know how to then search online
