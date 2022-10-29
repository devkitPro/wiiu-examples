# gx2_gyro_cube
A cube which gets rotated according to the GamePad's gyroscope angle.  

This example requires installing the `ppc-glm` package using pacman.

## Note about shaders
The bundled shader in `content` has been assembled using [latte-assembler](https://github.com/decaf-emu/decaf-emu) with the following command:  
```
latte-assembler assemble --vsh=shader.vsh --psh=shader.psh content/shader.gsh
```
