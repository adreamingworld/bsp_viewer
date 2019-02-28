# Quake 3 BSP Viewer
A simple Quake 3 BSP viewer.

See a list of [current issues](docs/ISSUES.md) with the project.

![Screenshot](resources/screenshot.png?raw=true)

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
bsp -b <bsp-file-name>

```
### Command line options
```
-b <file name>		- BSP file to load.
-d <display number>	- Which display to use. Defaults to 0.
```

## Controls
* WASD		 	- move around
* Mouse		 	- look around
* r 			- Respawn in next spawn point
* p 			- Toggle PVS culling
* Up arrow		- Increase bezier patch detail level
* Down arrow		- Decrease bezier patch detail level 
* F1			- Take a screenshot (Currently saves to working directory)


