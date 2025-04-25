#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>

#ifndef OUTPUT_PATH
#define OUTPUT_PATH "/tmp/echo-server-out.log"
#endif

#ifndef N
#define N 10
#endif

#ifndef INPUT_PATH
#define INPUT_PATH "/tmp/echo_fifo"
#endif

#define BUFFER_SIZE 1024

volatile sig_atomic_t run = true;
volatile sig_atomic_t immediate_shutdown = false;
volatile sig_atomic_t alarmed = 0;
volatile sig_atomic_t sigusr = 0;
volatile sig_atomic_t sighup = 0;

FILE *out;
int messages = 0;
int total_size = 0;
int last_size = 0;
int alarms = 0;
int is_daemon = 0;

void daemonize() {
    is_daemon = 1;
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork1");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) {
        perror("fork2");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    if (chdir("/") < 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    out = fopen(OUTPUT_PATH, "a");
    if (!out) {
        perror("fopen log");
        exit(EXIT_FAILURE);
    }

    if (alarm(N) == (unsigned int) -1) {
        fprintf(out, "Error setting alarm\n");
        exit(EXIT_FAILURE);
    }
}


static void handle_alarm(int signal) {
    alarm(N);
    alarmed = 1;
}


static void handle_sigusr(int signal) {
    sigusr = 1;
}


static void handle_sighup(int signal) {
    sighup = 1;
}


static void handle_terminate(int signal) {
    if (signal == SIGTERM) {
        immediate_shutdown = true;
    }
    run = false;
}

void print_statistics() {
    fprintf(out, "current statistics:\nmessages read:\t%d\nsize total:\t%d\nsize last:\t%d\ntotal alarms:\t%d\n",
            messages, total_size, last_size, alarms);
    if (fflush(out) != 0) {
        perror("fflush");
    }
}

void check_signals() {
    if (alarmed) {
        alarmed = 0;
        alarms++;
        fprintf(out, "echo-server is still running and ready to read messages\n");
        fflush(out);
    }
    if (sigusr) {
        sigusr = 0;
        print_statistics();
    }
    if (sighup) {
        sighup = 0;
        if (is_daemon == 0) {
            daemonize();
            fprintf(out, "daemonized by sighup\n");
            print_statistics();
        }
    }
}


int main(int argc, char *argv[]) {
    struct sigaction sa_alarm;
    sa_alarm.sa_handler = handle_alarm;
    sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags = 0;
    if (sigaction(SIGALRM, &sa_alarm, NULL) == -1) {
        perror("sigaction SIGALRM");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_usr;
    sa_usr.sa_handler = handle_sigusr;
    sigemptyset(&sa_usr.sa_mask);
    sa_usr.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa_usr, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_hup;
    sa_hup.sa_handler = handle_sighup;
    sigemptyset(&sa_hup.sa_mask);
    sa_hup.sa_flags = 0;
    if (sigaction(SIGHUP, &sa_hup, NULL) == -1) {
        perror("sigaction SIGHUP");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_term;
    sa_term.sa_handler = handle_terminate;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    if (sigaction(SIGTERM, &sa_term, NULL) == -1) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGQUIT, &sa_term, NULL) == -1) {
        perror("sigaction SIGQUIT");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_int;
    sa_int.sa_handler = SIG_IGN;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    if (argc > 1 && strcmp(argv[1], "-daemon") == 0) {
        daemonize();
    }

    struct stat st;
    int fifo_fd;

    if (mkfifo(INPUT_PATH, 0600) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }

        if (stat(INPUT_PATH, &st) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }

        if (!S_ISFIFO(st.st_mode)) {
            fprintf(stderr, "Файл %s существует и не является FIFO\n", INPUT_PATH);
            exit(EXIT_FAILURE);
        }
    }

    out = stdout;
    if (is_daemon) {
        out = fopen(OUTPUT_PATH, "w");
        if (!out) {
            perror("fopen log");
            exit(EXIT_FAILURE);
        }
    }
    if (alarm(N) == (unsigned int) -1) {
        fprintf(out, "Error setting alarm\n");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    bool eof = false;
    char last = 0;
    ssize_t n;

    while (run) {
        fifo_fd = -1;
        do {
            check_signals();
            if(!immediate_shutdown && run) {
                fifo_fd = open(INPUT_PATH, O_RDONLY);
            }
            if (immediate_shutdown) {
                fprintf(out, "received SIGTERM\n");
            }
        } while (fifo_fd == -1 && errno == EINTR && !immediate_shutdown && run);
        if (immediate_shutdown) {
            print_statistics();
            if (unlink(INPUT_PATH) == -1) {
                perror("unlink");
            }
            if (fifo_fd != -1) {
                if (close(fifo_fd) == -1) {
                    perror("close");
                }
            }
            exit(0);
        }
        if (fifo_fd == -1) {
            perror("open");
            unlink(INPUT_PATH);
            exit(EXIT_FAILURE);
        }
        last_size = 0;
        messages++;
        while (!eof) {
            do {
                check_signals();
                if(!immediate_shutdown && run) {
                    n = read(fifo_fd, buffer, BUFFER_SIZE - 1);
                }
            } while (n == -1 && errno == EINTR && !immediate_shutdown);
            if (immediate_shutdown) {
                print_statistics();
                if (close(fifo_fd) == -1) {
                    perror("close");
                }
                if (unlink(INPUT_PATH) == -1) {
                    perror("unlink");
                }
                exit(0);
            }
            if (n > 0) {
                last_size += n;
                total_size += n;
                last = buffer[n - 1];
                buffer[n] = '\0';
                fprintf(out, "Received: %s\n", buffer);
                fflush(out);

            } else if (n == 0) {
                if (last != '\n') {
                    fprintf(out, "\n");
                }
                eof = true;
            } else {
                perror("read");
                close(fifo_fd);
                break;
            }
        }
        eof = false;
        if (close(fifo_fd) == -1) {
            perror("close");
        }
    }
    print_statistics();
    if (is_daemon) {
        fclose(out);
    }

    if (unlink(INPUT_PATH) == -1) {
        perror("unlink");
    }
    return 0;
}
