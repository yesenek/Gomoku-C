#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 19

typedef enum { EMPTY = 0, WHITE = 1, BLACK = 2 } Color;

typedef struct {
    Color map[SIZE][SIZE];
} Board;

static inline bool is_valid(int x, int y) {
    return (x >= 0 && x < SIZE && y >= 0 && y < SIZE);
}

void Board_init(Board* b) {
    memset(b->map, EMPTY, sizeof(b->map));
}

bool make_move(Board* b, int x, int y, Color c) {
    if (!is_valid(x, y))
        return false;

    if (b->map[y][x] != EMPTY)
        return false;

    b->map[y][x] = c;
    return true;
}

static inline Color opposite_color(Color c) {
    return 3 - c;
}

static inline bool has_neighbors(Board* b, int x, int y, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx == 0 && dy == 0) continue;

            int nx = x + dx;
            int ny = y + dy;

            if (is_valid(nx, ny) && b->map[ny][nx] != EMPTY) {
                return true;
            }
        }
    }
    return false;
}

static inline int count_one_way(Board* b, Color c, int x, int y, int dx, int dy) {
    Color (*map)[SIZE] = b->map;
    int count = 0;
    int cur_x = x + dx;
    int cur_y = y + dy;

    while (cur_x >= 0 && cur_x < SIZE && cur_y >= 0 && cur_y < SIZE && map[cur_y][cur_x] == c) {
        count++;
        cur_x += dx;
        cur_y += dy;
    }

    return count;
}

static inline int evaluate_stones(Board* b, Color c, int x, int y) {
    static const int dirs[4][2] = {
        { 1, 0 },
        { 0, 1 },
        { 1, 1 },
        { 1, -1 }
    };

    int total_score = 0;
    int best_priority = 0;
    int open_three_count = 0;
    int open_four_count = 0;
    int broken_four_count = 0;
    Color enemy = opposite_color(c);

    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0];
        int dy = dirs[d][1];

        int forward = count_one_way(b, c, x, y, dx, dy);
        int backward = count_one_way(b, c, x, y, -dx, -dy);

        int temp_score = forward + backward + 1;
        int fx1 = x + (forward + 1) * dx;
        int fy1 = y + (forward + 1) * dy;
        int bx1 = x - (backward + 1) * dx;
        int by1 = y - (backward + 1) * dy;
        int fx2 = x + (forward + 2) * dx;
        int fy2 = y + (forward + 2) * dy;
        int bx2 = x - (backward + 2) * dx;
        int by2 = y - (backward + 2) * dy;

        bool f_open = is_valid(fx1, fy1) && b->map[fy1][fx1] == EMPTY;
        bool b_open = is_valid(bx1, by1) && b->map[by1][bx1] == EMPTY;
        bool f2_c = is_valid(fx2, fy2) && b->map[fy2][fx2] == c;
        bool b2_c = is_valid(bx2, by2) && b->map[by2][bx2] == c;

        if (temp_score >= 5) {
            if (best_priority < 100) {
                best_priority = 100;
                total_score = 1000000;
            }
        } else if (temp_score == 4 && f_open && b_open) {
            open_four_count++;
            if (best_priority < 95) {
                best_priority = 95;
                total_score = 500000;
            }
        } else if (temp_score == 4 && (f_open || b_open)) {
            if (best_priority < 85) {
                best_priority = 85;
                total_score = 50000;
            }
        } else if (temp_score == 3 && ((f_open && f2_c) || (b_open && b2_c))) {
            broken_four_count++;
            if (best_priority < 90) {
                best_priority = 90;
                total_score = 80000;
            }
        } else if (temp_score == 3 && f_open && b_open) {
            open_three_count++;
            if (best_priority < 70) {
                best_priority = 70;
                total_score = 10000;
            }
        } else if (temp_score == 3 && (f_open || b_open)) {
            if (best_priority < 60) {
                best_priority = 60;
                total_score = 5000;
            }
        } else if (temp_score == 2 && f_open && b_open) {
            if (best_priority < 30) {
                best_priority = 30;
                total_score = 1000;
            }
        } else if (temp_score == 2 && (f_open || b_open)) {
            if (best_priority < 20) {
                best_priority = 20;
                total_score = 500;
            }
        }
    }

    if (open_four_count >= 2) {
        total_score += 900000;
    }

    if (open_four_count >= 1 && broken_four_count >= 1) {
        total_score += 300000;
    }

    if (broken_four_count >= 2) {
        total_score += 120000;
    }

    if (open_three_count >= 2) {
        total_score += 80000;
    }

    if (open_three_count >= 1 && broken_four_count >= 1) {
        total_score += 150000;
    }

    int dist = abs(x - SIZE / 2) + abs(y - SIZE / 2);
    total_score += (SIZE - dist);

    return total_score;
}

int find_move(Board* b, Color c, int x, int y, int depth) {
    if (depth <= 0)
        return 0;

    if (!has_neighbors(b, x, y, 3))
        return 0;

    int total_score = 0;
    bool placed = false;

    if (b->map[y][x] == EMPTY) {
        b->map[y][x] = c;
        placed = true;
    }

    int evaluated = evaluate_stones(b, c, x, y);

    int rx = -1;
    int ry = -1;
    int best_score = -999999;

    for (int sy = 0; sy < SIZE; sy++) {
        for (int sx = 0; sx < SIZE; sx++) {
            if (b->map[sy][sx] == EMPTY)
                continue;

            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int tx = sx + dx;
                    int ty = sy + dy;

                    if (!is_valid(tx, ty))
                        continue;

                    if (b->map[ty][tx] != EMPTY)
                        continue;

                    if (!has_neighbors(b, tx, ty, 2))
                        continue;

                    b->map[ty][tx] = opposite_color(c);

                    int score = evaluate_stones(b, opposite_color(c), tx, ty);

                    b->map[ty][tx] = EMPTY;

                    if (score > best_score) {
                        best_score = score;
                        rx = tx;
                        ry = ty;
                    }

                    if (best_score >= 500000)
                        break;
                }
            }
        }
    }

    if (rx != -1) {
        b->map[ry][rx] = opposite_color(c);
        int evaluated2 = evaluate_stones(b, opposite_color(c), rx, ry);
        total_score = evaluated - evaluated2 - (find_move(b, opposite_color(c), rx, ry, depth - 1) / 2);
        b->map[ry][rx] = EMPTY;
    }

    if (placed) {
        b->map[y][x] = EMPTY;
    }

    return total_score;
}

void print_board(Board* b) {
    printf("\n   ");
    for (int x = 0; x < SIZE; x++) {
        printf(" %c", 'A' + x);
    }
    printf("\n");

    for (int y = 0; y < SIZE; y++) {
        printf("%2d ", SIZE - y);
        for (int x = 0; x < SIZE; x++) {
            char c = '.';
            if (b->map[y][x] == WHITE)
                c = 'W';
            else if (b->map[y][x] == BLACK)
                c = 'B';
            printf(" %c", c);
        }
        printf("\n");
    }
    printf("\n");
}

bool parse_move(char* input, int* x, int* y) {
    char col;
    int row;

    if (sscanf(input, "%c%d", &col, &row) != 2)
        return false;

    if (col >= 'a' && col <= 'z')
        col -= 32;

    *x = col - 'A';
    *y = SIZE - row;

    if (!is_valid(*x, *y))
        return false;

    return true;
}

bool check_win(Board* b, Color c, int x, int y) {
    static const int dirs[4][2] = {
        {1, 0}, {0, 1}, {1, 1}, {1, -1}
    };

    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0];
        int dy = dirs[d][1];
        int count = 1;

        count += count_one_way(b, c, x, y, dx, dy);
        count += count_one_way(b, c, x, y, -dx, -dy);

        if (count >= 5) {
            return true;
        }
    }

    return false;
}

int main() {
    Board board;
    Board_init(&board);
    srand(time(NULL));

    while (1) {
        print_board(&board);

        char input[32];
        int x;
        int y;

        printf("Your move (example: K10): ");
        scanf("%31s", input);

        if (!parse_move(input, &x, &y)) {
            printf("Invalid move.\n");
            continue;
        }

        if (!make_move(&board, x, y, WHITE)) {
            printf("Cell occupied.\n");
            continue;
        }

        if (check_win(&board, WHITE, x, y)) {
            print_board(&board);
            printf("WHITE wins!\n");
            break;
        }

        int best_score = -999999999;
        int best_x = -1;
        int best_y = -1;

        for (int yy = 0; yy < SIZE; yy++) {
            for (int xx = 0; xx < SIZE; xx++) {
                if (board.map[yy][xx] != EMPTY)
                    continue;

                if (!has_neighbors(&board, xx, yy, 3))
                    continue;

                int score = find_move(&board, BLACK, xx, yy, 200);

                if (score > best_score) {
                    best_score = score;
                    best_x = xx;
                    best_y = yy;
                }
            }
        }

        if (best_x != -1) {
            make_move(&board, best_x, best_y, BLACK);
            printf("AI played: %c%d\n", 'A' + best_x, SIZE - best_y);

            if (check_win(&board, BLACK, best_x, best_y)) {
                print_board(&board);
                printf("BLACK wins!\n");
                break;
            }
        } else {
            printf("AI could not find a move.\n");
            break;
        }
    }

    return 0;
}
