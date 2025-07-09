## Mandelbrot Terminal Renderer

This project visualizes the Mandelbrot Set directly in the terminal using 256-color ANSI output. Written in C, it uses low-level system programming techniques (e.g., `fork()`, `mmap()`, `write()`) to compute and render the image in parallel across multiple processes.

Two versions are included:
- **With semaphores**: Synchronizes output line-by-line using POSIX semaphores.
- **Without semaphores**: Child processes compute rows into a shared memory buffer, and the parent handles all output.

No graphical libraries are used — all output is printed directly to the terminal via standard output using ANSI escape codes.