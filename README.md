# raylib-gravity

Simple 2D gravity simulation built for learning raylib.

## Details

The simulation in itself is pretty simple, using velocity Verlet to integrate the bodies and
Newton's law of universal gravitation to calculate forces between them.

Click and drag with your mouse to spawn new bodies. The initial path of the body will be shown as
a blue path.

May add some graphical effects in the future for testing shaders with raylib.

## Compilation

```bash
mkdir build
cd build
cmake ..
make -j
```

then run `build/src/raylib-gravity`.
