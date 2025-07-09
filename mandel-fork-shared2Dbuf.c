#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

int y_chars = 50;
int x_chars = 90;
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
double xstep, ystep;

void compute_mandel_line(int line, int color_val[]) {
    double x, y;
    int val;
    y = ymax - line * ystep;
    for (int col = 0; col < x_chars; col++) {
        x = xmin + col * xstep;
        val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
        if (val > 255) val = 255;
        color_val[col] = xterm_color(val);
    }
}

void output_mandel_line(int fd, int color_val[]) {
    char symbol = '@';
    char newline = '\n';
    for (int i = 0; i < x_chars; i++) {
        set_xterm_color(fd, color_val[i]);
        if (write(fd, &symbol, 1) != 1) {
            perror("write point");
            exit(1);
        }
    }
    if (write(fd, &newline, 1) != 1) {
        perror("write newline");
        exit(1);
    }
}

void *create_shared_memory_area(unsigned int numbytes) {
    if (numbytes == 0) {
        fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
        exit(1);
    }

    int pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;
    void *ptr = mmap(NULL, pages * sysconf(_SC_PAGE_SIZE),
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return ptr;
}

void destroy_shared_memory_area(void *addr, unsigned int numbytes) {
    int pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;
    if (munmap(addr, pages * sysconf(_SC_PAGE_SIZE)) == -1) {
        perror("munmap");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    struct timeval start, end;
    gettimeofday(&start, NULL);

    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <NPROCS>\n", argv[0]);
        exit(1);
    }

    int NPROCS = atoi(argv[1]);
    if (NPROCS < 1 || NPROCS > 32) {
        fprintf(stderr, "Invalid thread number\n");
        exit(1);
    }

    size_t total_size = y_chars * x_chars * sizeof(int);
    void *buffer_ptr = create_shared_memory_area(total_size);
    int (*buffer)[x_chars] = (int (*)[x_chars])buffer_ptr;

    for (int i = 0; i < NPROCS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            int temp_colors[x_chars];
            for (int row = i; row < y_chars; row += NPROCS) {
                compute_mandel_line(row, temp_colors);
                memcpy(buffer[row], temp_colors, x_chars * sizeof(int));
            }
            _exit(0);
        }
    }

    for (int i = 0; i < NPROCS; i++) wait(NULL);
    for (int row = 0; row < y_chars; row++) output_mandel_line(1, buffer[row]);

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_usec - start.tv_usec) / 1e6;
    printf("Elapsed time: %.6f seconds\n", elapsed);

    destroy_shared_memory_area(buffer_ptr, total_size);
    reset_xterm_color(1);
    return 0;
}