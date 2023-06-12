/*
 * file: cats_cradle.c
 * -------------------
 * Game created in 1 day for experimentaljams.com, theme was "loop".
 *
 * Author: Mason Armand
 * Date Created: Jun 10, 2023
 * Last Modified: Jun 11, 2023
 */
#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define POINT_RADIUS 3.0f

typedef struct {
        double start_time;
        double life_time;
        bool started;
} Timer;

typedef struct {
        Vector2 pos;
        float timestamp;
        float radius;
} Point;

typedef struct {
        Point* points;
        unsigned int size;
        unsigned int capacity;
} Path;

typedef struct {
        Point start;
        Point end;
} LineSegment;

typedef struct {
        Vector2 pos;
        Texture2D tex;
        Path path;
} Player;

typedef struct {
        Vector2 pos;
        Vector2 dir;
        float scale;
        float dir_timer;
        float dir_threshold;
        Color color;
        Timer death_timer;
        unsigned char start_alpha;
} Enemy;

typedef struct {
        Enemy* enemies;
        unsigned int size;
        unsigned int capacity;
} EnemyList;

typedef struct {
        unsigned int num;
        Timer timer;
} EnemyWave;

typedef enum {
        TUTORIAL,
        GAME,
        GAMEOVER,
        PAUSED
} GameState;

void draw_centered_text(const char* text, int font_size, Color color);
void spawn_enemies(EnemyList* list, EnemyWave* wave);

void init_player(Player* player);
void free_player(Player* player);
void player_input(Player* player);
void draw_player(Player* player);

void init_enemy(Enemy* enemy);
void kill_enemy(Enemy* enemy, Sound snd_death);
void update_enemy(Enemy* enemy, float delta);
void init_enemy_list(EnemyList* list, unsigned int initial_capacity);
void add_enemy(EnemyList* list, Enemy enemy);
void remove_enemy(EnemyList* list, unsigned int index);
void remove_dead_enemies(EnemyList* list);
void free_enemy_list(EnemyList* list);
void draw_enemy(Enemy* enemy, Texture2D* texture);
void draw_enemy_list(EnemyList* list, Texture2D* texture);

void add_point(Path* path, Vector2 pos);
void add_interpolated_points(Path* path, float x1, float y1, float x2, float y2);
void remove_excess_points(Path* path, unsigned int max_points);
void draw_point(Point* point);
void draw_path(Path* path);
bool line_segments_intersect(LineSegment* a, LineSegment* b);
bool has_made_loop(Path* path);
bool is_in_loop(Enemy* enemy, Path* path);

void start_timer(Timer* timer, double lifetime);
void reset_timer(Timer* timer);
bool timer_done(Timer timer);
double get_remaining_time(Timer timer);

bool is_enemy_collision(Player* player, Enemy* enemy, Texture2D* enemy_tex);
void draw_wave(EnemyWave* wave);
float randf(float min, float max);


int main(void)
{
        Player cat;
        GameState game_state = TUTORIAL;
        EnemyList enemy_list = { 0 };
        Texture2D etex;
        Texture2D bg;
        Texture2D howto;
        Sound snd_edeath;
        EnemyWave wave = { 0 };
        float prev_mouse_x;
        float prev_mouse_y;
        unsigned int i;
        bool enemies_spawned = false;

        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "cat's cradle");
        InitAudioDevice();
        SetExitKey(KEY_Q);
        SetTargetFPS(60);

        init_player(&cat);
        etex = LoadTexture("res/mouse.png");
        bg = LoadTexture("res/grass.png");
        howto = LoadTexture("res/howto.png");
        snd_edeath = LoadSound("res/enemy_death.mp3");
        prev_mouse_x = cat.pos.x;
        prev_mouse_y = cat.pos.y;

        reset_timer(&wave.timer);

        HideCursor();
        SetMousePosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

        while (!WindowShouldClose()) {
                switch (game_state) {
                case TUTORIAL:
                        if (IsMouseButtonPressed(0)
                        || IsKeyPressed(KEY_ENTER)
                        || IsKeyPressed(KEY_SPACE))
                                game_state = GAME;
                        BeginDrawing();
                                DrawTexture(howto, 0, 0, WHITE);
                        EndDrawing();
                        break;
                case GAME:
                        spawn_enemies(&enemy_list, &wave);

                        player_input(&cat);

                        add_interpolated_points(&cat.path, prev_mouse_x, prev_mouse_y, cat.pos.x, cat.pos.y);
                        remove_excess_points(&cat.path, 600);
                        prev_mouse_x = cat.pos.x;
                        prev_mouse_y = cat.pos.y;

                        if (has_made_loop(&cat.path)) {
                                for (i = 0; i < enemy_list.size; i++) {
                                        if (is_in_loop(&enemy_list.enemies[i], &cat.path)) {
                                                kill_enemy(&enemy_list.enemies[i], snd_edeath);
                                        }
                                }
                        }

                        remove_dead_enemies(&enemy_list);

                        for (i = 0; i < enemy_list.size; i++) {
                                update_enemy(&enemy_list.enemies[i], GetFrameTime());
                        }

                        for (i = 0; i < enemy_list.size; i++) {
                                if (is_enemy_collision(&cat, &enemy_list.enemies[i], &etex)) {
                                        game_state = GAMEOVER;
                                }
                        }

                        if (IsKeyPressed(KEY_ESCAPE))
                                game_state = PAUSED;

                        BeginDrawing();
                                DrawTexture(bg, 0, 0, WHITE);
                                draw_wave(&wave);
                                draw_player(&cat);
                                draw_enemy_list(&enemy_list, &etex);
                        EndDrawing();

                        break;
                case GAMEOVER:
                        BeginDrawing();
                                DrawTexture(bg, 0, 0, WHITE);
                                draw_wave(&wave);
                                draw_player(&cat);
                                draw_enemy_list(&enemy_list, &etex);
                                draw_centered_text("You died, click to restart!", 40, WHITE);
                                if (IsMouseButtonPressed(0)) {
                                        wave.num = 0;
                                        free_enemy_list(&enemy_list);
                                        game_state = GAME;
                                }
                        EndDrawing();
                        break;
                case PAUSED:
                        BeginDrawing();
                                DrawTexture(bg, 0, 0, WHITE);
                                draw_wave(&wave);
                                draw_player(&cat);
                                draw_enemy_list(&enemy_list, &etex);
                                draw_centered_text("Paused", 40, WHITE);
                                if (IsKeyPressed(KEY_ESCAPE))
                                        game_state = GAME;
                        EndDrawing();
                        break;
                }
        }
        UnloadTexture(etex);
        UnloadTexture(bg);
        UnloadTexture(howto);
        UnloadSound(snd_edeath);
        free_enemy_list(&enemy_list);
        free_player(&cat);
        CloseAudioDevice();
        CloseWindow();
        return 0;
}


void draw_centered_text(const char* text, int font_size, Color color)
{
        int text_width = MeasureText(text, font_size);
        int x = (SCREEN_WIDTH - text_width) / 2;
        int y = (SCREEN_HEIGHT - font_size) / 2;
        DrawText(text, x, y, font_size, color);
}


void spawn_enemies(EnemyList* list, EnemyWave* wave)
{
        unsigned int i;

        if (!list->enemies) {
                init_enemy_list(list, 10);
                start_timer(&wave->timer, 3.0f);
        }

        if (list->size == 0 && wave->timer.start_time == 0)
                start_timer(&wave->timer, 3.0f);

        if (list->size == 0 && timer_done(wave->timer)) {
                unsigned int enemy_count = 5 + wave->num * 5;
                for (i = 0; i < enemy_count; i++) {
                        Enemy enemy;
                        init_enemy(&enemy);
                        add_enemy(list, enemy);
                }
                wave->num += 1;
                reset_timer(&wave->timer);
        }
}


void init_player(Player* player)
{
        player->tex = LoadTexture("res/cat.png");
        player->pos = GetMousePosition();
        player->path.points = malloc(10 * sizeof(Point));
        player->path.size = 0;
        player->path.capacity = 10;
}


void free_player(Player* player)
{
        UnloadTexture(player->tex);
        free(player->path.points);
}


void player_input(Player* player)
{
        player->pos = GetMousePosition();
}


void draw_player(Player* player)
{
        float x = player->pos.x - (player->tex.width / 2);
        float y = player->pos.y - (player->tex.height / 2);

        draw_path(&player->path);
        DrawTexture(player->tex, x, y, WHITE);
}


void init_enemy(Enemy* enemy)
{
        bool left_x = GetRandomValue(0, 1);
        unsigned char gray_value = GetRandomValue(180, 255);

        enemy->color = (Color) { gray_value, gray_value, gray_value, 255 };
        enemy->pos.x = (left_x) ? GetRandomValue(-10, -5) : SCREEN_WIDTH + GetRandomValue(5, 10);
        enemy->pos.y = GetRandomValue(-10, SCREEN_HEIGHT);
        enemy->scale = randf(0.5f, 1.2f);
        enemy->dir_timer = 0.0f;
        enemy->dir_threshold = randf(0.3f, 1.0f);

        reset_timer(&enemy->death_timer);

        if (enemy->pos.x < 0)
                enemy->dir.x = 1;
        else if (enemy->pos.x > SCREEN_WIDTH)
                enemy->dir.x = -1;
        else
                enemy->dir.x = randf(-1.0f, 1.0f);

        if (enemy->pos.y < 0)
                enemy->dir.y = 1;
        else if (enemy->pos.y > SCREEN_HEIGHT)
                enemy->dir.y = -1;
        else
                enemy->dir.y = randf(-1.0f, 1.0f);
}


void kill_enemy(Enemy* enemy, Sound snd_death)
{
        if (enemy->death_timer.started)
                return;
        if (!IsSoundPlaying(snd_death))
                PlaySound(snd_death);
        start_timer(&enemy->death_timer, 0.5f);
        enemy->color = RED;
        enemy->start_alpha = 255;
}


void update_enemy(Enemy* enemy, float delta)
{
        float speed = 100.0f;

        enemy->pos.x += enemy->dir.x * speed * delta;
        enemy->pos.y += enemy->dir.y * speed * delta;

        enemy->dir_timer += delta;

        if (enemy->dir_timer < enemy->dir_threshold) {
                return;
        }

        if (enemy->pos.x < -enemy->scale)
                enemy->dir.x = randf(0.0f, 1.0f);
        else if (enemy->pos.x > SCREEN_WIDTH + enemy->scale)
                enemy->dir.x = randf(-1.0f, 0.0f);
        else if (enemy->pos.y < -enemy->scale)
                enemy->dir.y = randf(0.0f, 1.0f);
        else if (enemy->pos.y > SCREEN_HEIGHT + enemy->scale)
                enemy->dir.y = randf(-1.0f, 0.0f);

        enemy->dir.x += randf(-1.0f, 1.0f) * 0.05f;
        enemy->dir.y += randf(-1.0f, 1.0f) * 0.05f;

        enemy->dir_timer = 0.0f;
}


void init_enemy_list(EnemyList* list, unsigned int initial_capacity)
{
        list->enemies = malloc(initial_capacity * sizeof(Enemy));
        if (!list->enemies) {
                fprintf(stderr, "Failed to allocate memory\n");
                exit(1);
        }
        list->size = 0;
        list->capacity = initial_capacity;
}


void add_enemy(EnemyList* list, Enemy enemy)
{
        if (list->size == list->capacity) {
                list->capacity *= 2;
                list->enemies = realloc(list->enemies, list->capacity * sizeof(Enemy));
                if (!list->enemies) {
                        fprintf(stderr, "Failed to allocate memory\n");
                        exit(1);
                }
        }

        list->enemies[list->size] = enemy;
        list->size++;
}


void remove_enemy(EnemyList* list, unsigned int index)
{
        if (index < list->size) {
                unsigned int i;
                for (i = index; i < list->size - 1; i++) {
                        list->enemies[i] = list->enemies[i + 1];
                }
                list->size --;
        }
}


void remove_dead_enemies(EnemyList* list)
{
        int i;
        for (i = list->size - 1; i >= 0; i--) {
                if (timer_done(list->enemies[i].death_timer)) {
                        remove_enemy(list, i);
                }
        }
}


void free_enemy_list(EnemyList* list)
{
        free(list->enemies);
        list->enemies = NULL;
        list->size = 0;
        list->capacity = 0;
}


void draw_enemy(Enemy* enemy, Texture2D* texture)
{
        Vector2 scale = { 1.0f, 1.0f };
        Vector2 origin = { texture->width / 2, texture->height / 2 };

        if (enemy->dir.x < 0) scale.x = -1.0f;

        Rectangle source = {0, 0, texture->width * scale.x, texture->height * scale.y};
        if (enemy->death_timer.started) {
                double elapsed_time = GetTime() - enemy->death_timer.start_time;;
                enemy->color.a = enemy->start_alpha * (1.0 - elapsed_time / enemy->death_timer.life_time);
                if (enemy->color.a < 0) enemy->color.a = 0;

        }

        DrawTexturePro(*texture, source, (Rectangle){enemy->pos.x, enemy->pos.y, texture->width * enemy->scale, texture->height * enemy->scale}, origin, 0, enemy->color);
}


void draw_enemy_list(EnemyList* list, Texture2D* texture)
{
        unsigned int i;
        for (i = 0; i < list->size; i++) {
                draw_enemy(&list->enemies[i], texture);
        }
}


void add_point(Path* path, Vector2 pos)
{
        float current_time = (float) GetTime();
        if (path->size >= path->capacity) {
                path->capacity *= 2;
                path->points = realloc(path->points, path->capacity * sizeof(Point));
                if (!path->points) {
                        fprintf(stderr, "Memory Allocation Failed.\n");
                        exit(1);
                }
        }

        path->points[path->size].pos = pos;
        path->points[path->size].timestamp = current_time;
        path->points[path->size].radius = POINT_RADIUS;
        path->size++;
}


void add_interpolated_points(Path* path, float x1, float y1, float x2, float y2)
{
        float dx = x2 - x1;
        float dy = y2 - y1;

        float dist = sqrtf(dx * dx + dy * dy);

        if (dist >= POINT_RADIUS * 2) {
                unsigned int n = (unsigned int) dist;
                unsigned int i;
                for (i = 0; i <= n; i++) {
                        float t = (float) i / n;
                        float x = x1 + t * dx;
                        float y = y1 + t * dy;
                        Vector2 pos = {x, y};
                        add_point(path, pos);
                }
        }
        else {
                Vector2 pos = {x2, y2};
                add_point(path, pos);
        }
}


void remove_excess_points(Path* path, unsigned int max_points)
{
        if (path->size > max_points) {
                unsigned int excess_points = path->size - max_points;
                memmove(path->points, path->points + excess_points, max_points * sizeof(Point));
                path->size = max_points;
        }
}


void draw_point(Point* point)
{
        DrawCircle(point->pos.x, point->pos.y, point->radius, BLACK);
}


void draw_path(Path* path)
{
        unsigned int i;
        for (i = 0; i < path->size; i++) {
                draw_point(&path->points[i]);
        }
}


bool line_segments_intersect(LineSegment* a, LineSegment* b)
{
        float denominator = ((b->end.pos.y - b->start.pos.y) * (a->end.pos.x - a->start.pos.x))
                          - ((b->end.pos.x - b->start.pos.x) * (a->end.pos.y - a->start.pos.y));
        if (denominator == 0.0f) {
                return false;
        }

        float numerator1 = ((b->start.pos.x - a->start.pos.x) * (a->end.pos.y - a->start.pos.y))
                         - ((b->start.pos.y - a->start.pos.y) * (a->end.pos.x - a->start.pos.x));
        float numerator2 = ((a->start.pos.y - b->start.pos.y) * (b->end.pos.x - b->start.pos.x))
                         - ((a->start.pos.x - b->start.pos.x) * (b->end.pos.y - b->start.pos.y));

        if (numerator1 == 0.0f || numerator2 == 0.0f) {
                return false;
        }

        float r = numerator1 / denominator;
        float s = numerator2 / denominator;

        return (r > 0 && r < 1) && (s > 0 && s < 1);
}


bool has_made_loop(Path* path)
{
        unsigned int i;
        unsigned int j;

        if (path->size < 4) {
                return false;
        }

        for (i = 0; i < path->size - 2; i++) {
                for (j = i + 2; j < path->size - 1; j++) {
                        LineSegment segment1 = { path->points[i], path->points[i + 1] };
                        LineSegment segment2 = { path->points[j], path->points[j + 1] };
                        if (line_segments_intersect(&segment1, &segment2)) {
                                return true;
                        }
                }
        }
        return false;
}


bool is_in_loop(Enemy* enemy, Path* path)
{
        Vector2 pos = enemy->pos;
        bool result = false;
        unsigned int j = path->size - 1;
        unsigned int i;
        for (i = 0; i < path->size; i++) {
                if ((path->points[i].pos.y < pos.y && path->points[j].pos.y >= pos.y)
                || (path->points[j].pos.y < pos.y && path->points[i].pos.y >= pos.y)) {
                        if (path->points[i].pos.x + (pos.y - path->points[i].pos.y) / (path->points[j].pos.y - path->points[i].pos.y) * (path->points[j].pos.x - path->points[i].pos.x) < pos.x)
                                result = !result;
                }
                j = i;
        }
        return result;
}


/* timer */
void start_timer(Timer* timer, double lifetime)
{
        timer->start_time = GetTime();
        timer->life_time = lifetime;
        timer->started = true;
}


void reset_timer(Timer* timer)
{
        timer->start_time = 0;
        timer->life_time = 0;
        timer->started = false;
}


bool timer_done(Timer timer)
{
        return (GetTime() - timer.start_time >= timer.life_time) && (timer.started);
}


double get_remaining_time(Timer timer)
{
        double elapsed_time = GetTime() - timer.start_time;
        double remaining_time = timer.life_time - elapsed_time;
        return remaining_time > 0 ? remaining_time : 0;
}


bool is_enemy_collision(Player* player, Enemy* enemy, Texture2D* enemy_tex)
{
        if (enemy->death_timer.started)
                return false;
        Rectangle player_rect = {
                player->pos.x,
                player->pos.y,
                player->tex.width * 0.5f,
                player->tex.height * 0.5f
        };

        Rectangle enemy_rect = {
                enemy->pos.x,
                enemy->pos.y,
                (enemy_tex->width * enemy->scale) * 0.5f,
                (enemy_tex->height * enemy->scale) * 0.5f
        };

        return CheckCollisionRecs(player_rect, enemy_rect);
}


void draw_wave(EnemyWave* wave)
{
        const char* str = TextFormat("%d", wave->num);
        int width = MeasureText(str, 40);
        DrawText(str, (SCREEN_WIDTH - width) / 2, 40, 40, WHITE);

        if (!timer_done(wave->timer) && wave->timer.started) {
                int countdown = get_remaining_time(wave->timer) + 1;
                const char* countdown_str = TextFormat("%d", countdown);
                draw_centered_text(countdown_str, 80, WHITE);
        }
}


float randf(float min, float max)
{
        float r = ((float) rand()) / (float) RAND_MAX;
        float diff = max - min;
        return (min + (r * diff));
}
