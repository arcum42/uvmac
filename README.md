This is a fork of µvMac 0.37.0, which is a fork of mini vMac, which is a fork of vMac. Forks all around.

To compile, right now, what you'd want to do is:
```bash
mkdir subprojects
meson wrap install spdlog
meson wrap install nlohmann_json
mkdir builddir
meson setup builddir && cd builddir
meson compile
```
Then you'll need to copy the resulting microvmac binary into a directory with a rom called vMac.ROM as well as up to 6 disks named disk1.dsk, etc.

This will probably all change, and may even be out of date as you are reading this.

Feel free to open an issue if it is, but this is very early, and µvMac didn't even have all of vMac's functionality yet. Mostly a personal project at the moment, though that could change. (And I'm open to merging this all back with µvMac, too...)

A lot of what I'm doing is cleanups, moving to full C++, modernizing code, rewriting ui, etc. I'm planning to use imgui, and make the screen be a subwindow at some point, and after SDL 3 comes out, I'll work on porting it. (Or possibly before.)

It'd obviously be nice to get this to the point where it works for more than just a Mac +, and I'd like to unhardcode a lot of things.

# µvMac 0.37.0

Forked from Mini vMac v36.4 by Paul C. Pratt (http://www.gryphel.com/c/minivmac), which was forked from vMac by Philip "PC" Cummins (http://www.vmac.org/main.html)

µvMac (micro vMac) is a small, simple emulator for early Motorola 68000 based emulators. Currently we support systems from the original Macintosh 128K all the way up to the Macintosh II.

This fork was created to clean up and modernize the code base, make the project easier to compile and hack on, and allow for much easier user configuration. The intent of Mini vMac was to create a "emulator collection" of many very optimized "variations" of the same codebase. I consider this much more trouble than it's worth, and intend to focus more on maintainability and accuracy.

µvMac is undergoing substansial and sporadic development and is unlikely to be in an usable state at this time.

## Supported Platforms

µvMac *absolutely requires* SDL2. There are no plans to support platforms that SDL2 does not target. For 99% of users, this should not be a concern. Dropping support for esoteric platforms and exclusively using SDL2 vastly simplifies the complexity of the code.

So far µvMac has only been tested on Windows and Linux. No other operating systems are supported at this time.

## Legal info

You can redistribute µvMac and/or modify it under the terms
of version 2 of the GNU General Public License as published by
the Free Software Foundation.  See the included file COPYING.txt

µvMac is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
license for more details.
