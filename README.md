# PNG Grayscale Converter (C + x86-64 Assembly)

A C project that converts a color PNG image to grayscale using the **Averaging** algorithm, with the performance-critical pixel-processing loop implemented in x86-64 Assembly.

The program is designed to demonstrate the interaction between C and Assembly code, and to show how a small hand-written assembly function can outperform compiler-generated code for a tight numeric loop.

## Algorithm

Each pixel is converted to grayscale using the Averaging method:

```text
Gray = (Red + Green + Blue) / 3
```

The same value is written back to the R, G, and B channels (the alpha channel is preserved), so the resulting image is visually gray while remaining a valid RGB(A) PNG.

## Features

- Reads and writes PNG files via **stb_image** / **stb_image_write** (single-header, no external dependencies).
- Two interchangeable implementations of the pixel-processing function:
  - a pure C reference implementation (`process_c.c`),
  - a hand-written x86-64 Assembly implementation (`process_asm.asm`, NASM syntax).
- Command-line interface: input and output file names are passed as arguments.
- Robust input validation: missing files, unsupported formats, and write errors are all handled gracefully.
- No library functions are used inside the image-processing function.
- The Assembly implementation lives in its own `.asm` file.
- Benchmark results are collected into `results.csv`.
- Optimized for speed: the inner loop is written to work on raw pixel buffers with minimal overhead.

## Project layout

```text
.
├── Makefile
├── main.c
├── process_c.c
├── process_asm.asm
├── process.h
├── stb_image.h
├── stb_image_write.h
├── input.png
└── results.csv
```

| File | Purpose |
|------|---------|
| `Makefile` | Builds both C and Assembly variants, runs benchmarks. |
| `main.c` | CLI, PNG I/O via stb, argument validation, timing. |
| `process_c.c` | Pure C implementation of the Averaging algorithm. |
| `process_asm.asm` | NASM implementation of the Averaging algorithm. |
| `process.h` | Shared declaration of the processing function. |
| `stb_image.h` | Image loading (single-header public-domain library). |
| `stb_image_write.h` | Image saving (single-header public-domain library). |
| `input.png` | Sample input image used for benchmarks. |
| `results.csv` | Timing results collected by the benchmark target. |

## Building

Everything is driven by the `Makefile`.

```bash
make            # build both variants
make c          # build the C variant only
make asm        # build the Assembly variant only
make bench      # run benchmarks and refresh results.csv
make clean      # remove build artifacts
```

Internally the Makefile does roughly this:

```bash
# C variant
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 main.c process_c.c -o png_gray_c -lm

# Assembly variant
nasm -f elf64 process_asm.asm -o process_asm.o
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 main.c process_asm.o -o png_gray_asm -lm
```

> The `.asm` source uses **NASM** syntax. GNU `as` users can port it with minor changes.

### Debug build with sanitizers

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -g -fsanitize=address,undefined \
    main.c process_c.c -o png_gray_asan -lm
```

## Running

```bash
./png_gray_c   input.png output.png
./png_gray_asm input.png output.png
```

Example:

```bash
./png_gray_asm input.png input_gray.png
```

The program prints a short summary on success and a clear error message on failure. It never crashes on malformed input.

## Command-line interface

```text
Usage: png_gray <input.png> <output.png>
```

- `<input.png>` — path to the source color image.
- `<output.png>` — path to the resulting grayscale image (overwritten if it exists).

## Public interface

The shared interface between the C and Assembly implementations lives in `process.h`:

```c
#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

/*
 * Convert an RGB(A) pixel buffer to grayscale in place.
 *
 *   pixels   - pointer to the raw pixel buffer (row-major, 8-bit per channel)
 *   width    - image width in pixels
 *   height   - image height in pixels
 *   channels - number of channels per pixel (3 for RGB, 4 for RGBA)
 *
 * Returns 0 on success, non-zero on error.
 */
int grayscale_average(unsigned char *pixels,
                      size_t width,
                      size_t height,
                      int channels);

#endif
```

Both `process_c.c` and `process_asm.asm` export the same `grayscale_average` symbol, so `main.c` does not depend on which one is linked in.

## PNG I/O

PNG handling is delegated to the single-header **stb_image** and **stb_image_write** libraries. The C side:

1. Reads the PNG into an RGBA (or RGB) buffer via `stbi_load`.
2. Calls `grayscale_average` on the raw buffer.
3. Writes the buffer back to a new PNG via `stbi_write_png`.

The buffer is row-major and 8 bits per channel, which is exactly the layout the Assembly function expects.

To keep build times low, the STB implementation macro is defined in exactly one translation unit:

```c
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
```

## Assembly implementation

The Assembly function `grayscale_average` is written in NASM, targeting the **x86-64 System V ABI** (Linux / macOS / BSD). It receives its parameters in:

| Register | Parameter |
|----------|-----------|
| `rdi` | `pixels` |
| `rsi` | `width` |
| `rdx` | `height` |
| `rcx` | `channels` |

The inner loop:

1. Loads three bytes (`R`, `G`, `B`) with `movzx`.
2. Adds them together.
3. Divides by 3 using a fast multiplication (multiply by the magic constant `0xAAAB` and shift right).
4. Stores the result back into all three channels.
5. Advances the pointer by `channels` bytes.
6. Repeats until the whole buffer is processed.

The function is **iterative** (no recursion), uses no external symbols, and does not touch the stack except for saving callee-saved registers it borrows.

> Portability note: the `.asm` file targets x86-64. On other architectures, link the C implementation instead.

## Performance analysis

The assignment requires measuring the program under different optimization levels with the debug mode disabled, and comparing the C and Assembly versions.

### Compiler optimization levels

Build the C version at each level:

```bash
gcc -std=c11 -O0 main.c process_c.c -o png_gray_O0 -lm
gcc -std=c11 -O1 main.c process_c.c -o png_gray_O1 -lm
gcc -std=c11 -O2 main.c process_c.c -o png_gray_O2 -lm
gcc -std=c11 -O3 main.c process_c.c -o png_gray_O3 -lm
gcc -std=c11 -Ofast main.c process_c.c -o png_gray_Ofast -lm
```

Build the Assembly version (the `.asm` file is already hand-optimized, so the C optimizer only affects the surrounding code):

```bash
nasm -f elf64 process_asm.asm -o process_asm.o
gcc -std=c11 -O2 main.c process_asm.o -o png_gray_asm -lm
```

### Measuring

`make bench` runs every built binary several times on `input.png`, averages the timings, and writes them to `results.csv` in the following format:

```csv
build,width,height,channels,run,time_ms
C,-O0,1920,1080,4,1,18.42
C,-O0,1920,1080,4,2,18.37
...
ASM,-O2,1920,1080,4,1,3.21
ASM,-O2,1920,1080,4,2,3.19
```

You can then plot the results or aggregate them with any tool you like.

### Example results table

| Build | Image size | Time (ms) |
|-------|------------|-----------|
| C, `-O0` | 1920×1080 | 18.4 |
| C, `-O1` | 1920×1080 | 6.1 |
| C, `-O2` | 1920×1080 | 4.7 |
| C, `-O3` | 1920×1080 | 4.5 |
| C, `-Ofast` | 1920×1080 | 4.5 |
| ASM, `-O2` | 1920×1080 | 3.2 |

> Numbers above are illustrative. Reproduce them on your machine and replace the table with your own measurements from `results.csv`.

### Observations

- `-O0` is dominated by function-call overhead and unoptimized memory access.
- `-O1` and above already vectorize or unroll the loop to some extent.
- `-O3` / `-Ofast` give diminishing returns for this memory-bound task.
- The Assembly version is consistently faster than the C version at the same optimization level, mostly because it avoids redundant loads and uses an efficient division-by-3 trick.

## Complexity

| Aspect | Value |
|--------|-------|
| Time | O(W · H) — one pass over all pixels |
| Space | O(1) — the image is modified in place |

## Memory management

- The pixel buffer is owned by the C side (`stbi_load` allocates it, `stbi_image_free` releases it).
- `grayscale_average` modifies the buffer in place and does not allocate memory.
- All errors are reported via return codes; the program never calls `exit` from inside the library function.
- The project is intended to be clean under Valgrind and sanitizers.

## Error handling

The program checks:

- the number of command-line arguments,
- existence and readability of the input file,
- whether `stbi_load` succeeded (wrong format, corrupt file, etc.),
- supported channel counts (3 or 4),
- successful write of the output PNG.

On any error it prints a human-readable message and exits with a non-zero status.

## Requirements

- A C11 compiler (`gcc` or `clang`).
- **NASM** for assembling `process_asm.asm`.
- An x86-64 machine for the Assembly version.
- `make` for the build system.
- No external image libraries — STB headers are bundled in the repo.

## License

MIT License. See `LICENSE` if included.
