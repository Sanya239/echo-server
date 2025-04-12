#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>

#define FIFO_PATH "/tmp/echo_fifo"
#define BUFFER_SIZE 1024

bool running = true;

static void handle_terminate(int signal) {
    running = false;
}


int main(int argc, char **argv) {
    struct sigaction sa_term;
    sa_term.sa_handler = handle_terminate;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);
    sigaction(SIGINT, &sa_term, NULL);
    sigaction(SIGQUIT, &sa_term, NULL);

    bool descret;
    if (argc > 1) {
        if (strcmp(argv[1], "-descret") == 0) {
            descret = true;
        }
    }
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("open FIFO for writing");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];

    printf("Echo client. Please enter messages (Ctrl+C to exit):\n");

    while (running) {
        int res = 0;
        do {
            res = fgets(buffer, BUFFER_SIZE, stdin)!=0;
            if (!running) {
                close(fifo_fd);
                exit(0);
            }
        }  while (res == 0 && errno == EINTR);
        if(res == 0){
            printf("closing\n");

            break;
        }
        size_t len = strlen(buffer);
        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        if (write(fifo_fd, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
        if (descret) {
            close(fifo_fd);

            fifo_fd = open(FIFO_PATH, O_WRONLY);
            if (fifo_fd == -1) {
                perror("open FIFO for writing");
                exit(EXIT_FAILURE);
            }
        }
    }

    close(fifo_fd);
    return 0;
}
