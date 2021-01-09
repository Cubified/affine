## affine

A dependency-free terminal-based image viewer.  Capable of displaying images in the terminal using truecolor (16 million color) support, or 256-color support as a fallback.  Also includes a SNES Mode 7-inspired viewing mode.

### Screenshots

View mode:

![View mode](https://github.com/Cubified/affine/blob/main/screenshots/view.png)

Affine mode (still a work in progress):

![Affine mode](https://github.com/Cubified/affine/blob/main/screenshots/affine.gif)

### Compiling and Running

`affine` does not depend on anything other than the C standard library and [stb_image](https://github.com/nothings/stb), meaning it can be compiled with:

     $ make

And run with:

     $ ./affine [image file] [mode]

Where `mode` is one of:

- `view`:  View the image natively
- `affine`:  View the image using a Mode 7 perspective transform, with WASD to navigate and Q to quit
