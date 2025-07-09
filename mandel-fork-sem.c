#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

int y_chars = 50;
int x_chars = 90;
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
double xstep, ystep;

sem_t *sync_array;

void compute_mandel_line(int line, int color_val[]) {
    double x, y;
    int val;

    y = ymax - line * ystep;
    for (int i = 0; i < x_chars; i++) {
        x = xmin + i * xstep;
        val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
        if (val > 255) val = 255;
        color_val[i] = xterm_color(val);
    }
}

void output_mandel_line(int fd, int color_val[]) {
    char symbol = '@';
    char newline = '\n';
    for (int i = 0; i < x_chars; i++) {
        set_xterm_color(fd, color_val[i]);
        if (write(fd, &symbol, 1) != 1) { perror("write point"); exit(1); }
    }
    if (write(fd, &newline, 1) != 1) { perror("write newline"); exit(1); }
}

void *create_shared_memory_area(unsigned int numbytes) {
    if (numbytes == 0) {
        fprintf(stderr, "%s: internal error\n", __func__);
        exit(1);
    }

    int pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;
    void *area = mmap(NULL, pages * sysconf(_SC_PAGE_SIZE),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (area == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return area;
}

void destroy_shared_memory_area(void *addr, unsigned int numbytes) {
    int pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;
    if (munmap(addr, pages * sysconf(_SC_PAGE_SIZE)) == -1) {
        perror("munmap failed");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int n_processes;
    pid_t child_pid;
    struct timeval start, end;

    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <NPROCS>\n", argv[0]);
        exit(1);
    }

    n_processes = atoi(argv[1]);
    if (n_processes < 1 || n_processes > 32) {
        fprintf(stderr, "Invalid thread number\n");
        exit(1);
    }

    size_t sync_size = y_chars * sizeof(sem_t);
    void *shared_sync = create_shared_memory_area(sync_size);
    sync_array = (sem_t *)shared_sync;

    for (int i = 0; i < y_chars; i++) {
        if (sem_init(&sync_array[i], 1, 0) == -1) {
            perror("sem_init");
            destroy_shared_memory_area(shared_sync, sync_size);
            exit(1);
        }
    }

    sem_post(&sync_array[0]);

    gettimeofday(&start, NULL);

    for (int i = 0; i < n_processes; i++) {
        child_pid = fork();
        if (child_pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (child_pid == 0) {
            int local_colors[x_chars];
            for (int row = i; row < y_chars; row += n_processes) {
                compute_mandel_line(row, local_colors);
                sem_wait(&sync_array[row]);
                output_mandel_line(1, local_colors);
                if (row + 1 < y_chars)
                    sem_post(&sync_array[row + 1]);
            }
            _exit(0);
        }
    }

    for (int i = 0; i < n_processes; i++) wait(NULL);
    for (int i = 0; i < y_chars; i++) sem_destroy(&sync_array[i]);

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_usec - start.tv_usec) / 1e6;
    printf("Elapsed time: %.6f seconds\n", elapsed);

    destroy_shared_memory_area(shared_sync, sync_size);
    reset_xterm_color(1);
    return 0;
}