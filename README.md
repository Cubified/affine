## affine

A dependency-free terminal-based image viewer, built with pure ANSI escape sequences (no ncurses).  Capable of displaying images in the terminal using truecolor (16 million color) support, or 256-color support as a fallback.  Also includes a SNES Mode 7-inspired viewing mode.

### Screenshots

View mode:

![View mode](https://github.com/Cubified/affine/blob/main/screenshots/view.gif)

Affine mode:

![Affine mode](https://github.com/Cubified/affine/blob/main/screenshots/affine.gif)

### Compiling and Running

`affine` does not depend on anything other than the C standard library and [stb_image](https://github.com/nothings/stb), meaning it can be compiled with:

     $ make

And run with:

     $ ./affine [image file] [mode]

Where `mode` is one of:

- `view`:  View the image natively, with arrow keys to move and +/- to zoom -- if no mode is specified, this one is used
- `affine`:  View the image using a Mode 7 perspective transform, with arrow keys or WASD to navigate, E/C to adjust tilt, R/V to adjust height, and Q to quit

### To-do
- Animated GIF support
- Rendering optimizations
