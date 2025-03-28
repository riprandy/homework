#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_PLAYERS 10
#define MAX_GRID_SIZE 10

typedef struct {
    int x, y;
} coordinate;

typedef enum { START, MARK } cmt;
typedef struct { cmt type; coordinate position; } client_message;
-
typedef enum { END, RESULT } smt;
typedef struct { smt type; int success; int filled_count; } server_message;

typedef struct {
    coordinate position;
    char character;
} grid_update;

int pipes[MAX_PLAYERS][2];
char grid[MAX_GRID_SIZE][MAX_GRID_SIZE];
int grid_width, grid_height, streak_size, player_count;
char player_chars[MAX_PLAYERS];

void setup_pipes() {
    for (int i = 0; i < player_count; i++) {
        if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, pipes[i]) == -1) {
            perror("socketpair failed");
            exit(1);
        }
    }
}

void fork_players(char *players[MAX_PLAYERS], char *args[MAX_PLAYERS][10]) {
    for (int i = 0; i < player_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            dup2(pipes[i][0], STDIN_FILENO);
            dup2(pipes[i][0], STDOUT_FILENO);
            close(pipes[i][1]);
            execvp(players[i], args[i]);
            perror("execvp failed");
            exit(1);
        } else {
            close(pipes[i][0]);
        }
    }
}

int check_winner() {
    for (int i = 0; i < grid_height; i++) {
        for (int j = 0; j < grid_width; j++) {
            if (grid[i][j] == ' ') continue;
            char c = grid[i][j];
            int win = 1;
            for (int k = 0; k < streak_size; k++) {
                if (j + k >= grid_width || grid[i][j + k] != c) { win = 0; break; }
            }
            if (win) return c;
            win = 1;
            for (int k = 0; k < streak_size; k++) {
                if (i + k >= grid_height || grid[i + k][j] != c) { win = 0; break; }
            }
            if (win) return c;
            win = 1;
            for (int k = 0; k < streak_size; k++) {
                if (i + k >= grid_height || j + k >= grid_width || grid[i + k][j + k] != c) { win = 0; break; }
            }
            if (win) return c;
        }
    }
    return 0;
}

void handle_messages() {
    fd_set read_fds;
    int max_fd = 0;
    for (int i = 0; i < player_count; i++) {
        if (pipes[i][1] > max_fd) max_fd = pipes[i][1];
    }
    while (1) {
        FD_ZERO(&read_fds);
        for (int i = 0; i < player_count; i++) {
            FD_SET(pipes[i][1], &read_fds);
        }
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) > 0) {
            for (int i = 0; i < player_count; i++) {
                if (FD_ISSET(pipes[i][1], &read_fds)) {
                    client_message msg;
                    read(pipes[i][1], &msg, sizeof(msg));
                    if (msg.type == MARK) {
                        if (grid[msg.position.y][msg.position.x] == ' ') {
                            grid[msg.position.y][msg.position.x] = player_chars[i];
                            int winner = check_winner();
                            if (winner) {
                                printf("Winner: Player%c\n", winner);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
}

int main() {
    scanf("%d %d %d %d", &grid_width, &grid_height, &streak_size, &player_count);
    char *players[MAX_PLAYERS], *args[MAX_PLAYERS][10];
    for (int i = 0; i < player_count; i++) {
        scanf(" %c", &player_chars[i]);
    }
    setup_pipes();
    fork_players(players, args);
    handle_messages();
    return 0;
}
