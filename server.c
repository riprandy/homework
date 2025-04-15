#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)

#define MAX_PLAYERS 10
#include "./print_output.h"
#define MAX_GRID 20



int width, height, streak, player_count;
char grid[MAX_GRID][MAX_GRID];
int filled_cells = 0;
int pipes[MAX_PLAYERS][2][2]; // [player_id][0: server_write, 1: server_read][0: read, 1: write]
char player_chars[MAX_PLAYERS];
pid_t player_pids[MAX_PLAYERS];

int directions[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };

int is_within_bounds(int x, int y) {
    return x >= 0 && x < width && y >= 0 && y < height;
}

int check_winner(int x, int y, char ch) {
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0], dy = directions[d][1];
        int count = 1;
        for (int i = 1; i < streak; i++) {
            int nx = x + i*dx, ny = y + i*dy;
            if (!is_within_bounds(nx, ny) || grid[ny][nx] != ch) break;
            count++;
        }
        for (int i = 1; i < streak; i++) {
            int nx = x - i*dx, ny = y - i*dy;
            if (!is_within_bounds(nx, ny) || grid[ny][nx] != ch) break;
            count++;
        }
        if (count >= streak) return 1;
    }
    return 0;
}

void send_result(int p, int success, smt type) {
    sm response = { type, success, filled_cells };
    smp server_msg = { player_pids[p], &response };

    write(pipes[p][0][1], &response, sizeof(response));
    if (type == RESULT) {
        gd updates[filled_cells];
        int k = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (grid[y][x] != '.') {
                    updates[k++] = (gd) { {x, y}, grid[y][x] };
                }
            }
        }
        write(pipes[p][0][1], updates, sizeof(gd) * filled_cells);
        gu grid_updates[filled_cells];
        for (int i = 0; i < filled_cells; i++) {
            grid_updates[i].position = updates[i].position;
            grid_updates[i].character = updates[i].character;
        }
        print_output(NULL, &server_msg, grid_updates, filled_cells);
    } else {
        print_output(NULL, &server_msg, NULL, 0);
    }
}

void notify_end(char winner) {
    for (int i = 0; i < player_count; i++) {
        send_result(i, 0, END);
    }
    if (winner) printf("Winner: Player%c\n", winner);
    else printf("Draw\n");
}

int main() {
    scanf("%d %d %d %d", &width, &height, &streak, &player_count);
    for (int i = 0; i < height; i++) for (int j = 0; j < width; j++) grid[i][j] = '.';

    for (int i = 0; i < player_count; i++) {
        int arg_count;
        scanf(" %c %d", &player_chars[i], &arg_count);
        char *args[arg_count + 2];
        for (int j = 0; j <= arg_count; j++) {
            args[j] = (char*)malloc(100);
            scanf("%s", args[j]);
        }
        args[arg_count + 1] = NULL;

        PIPE(pipes[i][0]);
        PIPE(pipes[i][1]);

        pid_t pid = fork();
        if (pid == 0) {
            dup2(pipes[i][0][0], STDIN_FILENO);
            dup2(pipes[i][1][1], STDOUT_FILENO);
            close(pipes[i][0][1]); close(pipes[i][1][0]);
            execv(args[0], args);
            exit(1);
        } else {
            close(pipes[i][0][0]); close(pipes[i][1][1]);
            player_pids[i] = pid;
        }
    }

    fd_set readfds;
    int maxfd = 0;
    for (int i = 0; i < player_count; i++) {
        if (pipes[i][1][0] > maxfd) maxfd = pipes[i][1][0];
    }

    char winner = 0;
    while (!winner && filled_cells < width * height) {
        FD_ZERO(&readfds);
        for (int i = 0; i < player_count; i++) {
            FD_SET(pipes[i][1][0], &readfds);
        }
        struct timeval timeout = {0, 1000}; // 1 ms
        if (select(maxfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            for (int i = 0; i < player_count; i++) {
                if (FD_ISSET(pipes[i][1][0], &readfds)) {
                    cm msg;
                    if (read(pipes[i][1][0], &msg, sizeof(msg)) <= 0) continue;

                    cmp client_msg = { player_pids[i], &msg };
                    print_output(&client_msg, NULL, NULL, 0);

                    if (msg.type == START) {
                        send_result(i, 0, RESULT);
                    } else if (msg.type == MARK) {
                        int x = msg.position.x, y = msg.position.y;
                        if (is_within_bounds(x, y) && grid[y][x] == '.') {
                            grid[y][x] = player_chars[i];
                            filled_cells++;
                            send_result(i, 1, RESULT);
                            if (check_winner(x, y, player_chars[i])) {
                                winner = player_chars[i];
                            }
                        } else {
                            send_result(i, 0, RESULT);
                        }
                    }
                }
            }
        }
    }

    notify_end(winner);

    for (int i = 0; i < player_count; i++) {
        waitpid(player_pids[i], NULL, 0);
    }

    return 0;
}
