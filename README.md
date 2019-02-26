# Quake 3 BSP Viewer
A simple Quake 3 BSP viewer.

See a list of [current issues](ISSUES.md) with the project.

![Screenshot](screenshots/screenshot.png?raw=true)

## Compiling
### Run-time Dependencies
* [DevIL](http://openil.sourceforge.net)
* [SDL](https://www.libsdl.org) (>= 2.0)

### Build-time Dependencies
* Autotools - automake, autoconf

### Building
```
autoreconf -vi
./configure
make
```

## Executing
```
bsp <bsp-file-name>
```

## Controls
* Left mouse button 	- move forward
* Right mouse button 	- look around
* Middle mouse button	- move backwards
* S 			- Spawn in next spawn point
* Up arrow		- Increase bezier patch detail level
* Down arrow		- Decrease bezier patch detail level 
* F1			- Take a screenshot (Currently saves to working directory)


