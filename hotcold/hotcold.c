#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_ATTEMPTS 8
#define MAX_MESSAGES 3

typedef enum {
    WELCOME_SCREEN,
    GAME_SCREEN
} GameScreen;

typedef struct {
    int target;
    int current_guess;
    int last_guess;
    int attempts;
    bool game_over;
    GameScreen current_screen;
    char messages[MAX_MESSAGES][256];
    SDL_Color message_colors[MAX_MESSAGES];
    SDL_GameController *controller;
} GameState;

void init_game(GameState *game) {
    game->target = rand() % 100 + 1;
    game->current_guess = 50;
    game->last_guess = -1;
    game->attempts = 0;
    game->game_over = false;
    game->current_screen = WELCOME_SCREEN;
    
    snprintf(game->messages[0], sizeof(game->messages[0]), 
             "Welcome to Hot/Cold!");
    snprintf(game->messages[1], sizeof(game->messages[1]), 
             "Try to guess the secret number between 1 and 100");
    snprintf(game->messages[2], sizeof(game->messages[2]), 
             "Press any key or button to start!");
    
    game->message_colors[0] = (SDL_Color){255, 215, 0, 255};  // Gold
    game->message_colors[1] = (SDL_Color){135, 206, 235, 255};  // Sky Blue
    game->message_colors[2] = (SDL_Color){50, 255, 50, 255};  // Bright Green
}

void get_temperature(GameState *game, char *temp_text, SDL_Color *color) {
    int diff = abs(game->last_guess - game->target);
    
    if (diff == 0) {
        snprintf(temp_text, 32, "EXACT!");
        *color = (SDL_Color){0, 255, 0, 255};
    } else if (diff <= 5) {
        snprintf(temp_text, 32, "BURNING HOT!");
        *color = (SDL_Color){255, 0, 0, 255};
    } else if (diff <= 10) {
        snprintf(temp_text, 32, "Very Warm!");
        *color = (SDL_Color){255, 128, 0, 255};
    } else if (diff <= 20) {
        snprintf(temp_text, 32, "Warm");
        *color = (SDL_Color){255, 200, 0, 255};
    } else if (diff <= 30) {
        snprintf(temp_text, 32, "Cool");
        *color = (SDL_Color){0, 200, 255, 255};
    } else {
        snprintf(temp_text, 32, "Freezing!");
        *color = (SDL_Color){0, 100, 255, 255};
    }
}

void make_guess(GameState *game) {
    game->attempts++;
    game->last_guess = game->current_guess;
    
    char *message = game->messages[0];
    if (game->current_guess == game->target) {
        snprintf(message, 256, "You won in %d attempts! Press Enter/A to play again", 
                game->attempts);
        game->game_over = true;
        game->message_colors[0] = (SDL_Color){0, 255, 0, 255};
    } else if (game->attempts >= MAX_ATTEMPTS) {
        snprintf(message, 256, "Game Over! The number was %d. Press Enter/A to play again", 
                game->target);
        game->game_over = true;
        game->message_colors[0] = (SDL_Color){255, 0, 0, 255};
    } else {
        snprintf(message, 256, "%s", 
                game->current_guess < game->target ? "Higher..." : "Lower...");
    }
}

void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, 
                int y, SDL_Color color) {
    if (text[0] == '\0') return;
    
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect dest = {
        .x = (WINDOW_WIDTH - surface->w) / 2,
        .y = y,
        .w = surface->w,
        .h = surface->h
    };
    
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_title(SDL_Renderer *renderer, TTF_Font *font_big) {
    SDL_Color colors[] = {
        {255, 0, 0, 255},    // Hot
        {0, 150, 255, 255}   // Cold
    };
    
    const char* words[] = {"HOT", "COLD"};
    int x_pos = WINDOW_WIDTH / 2 - 120;
    
    for (int i = 0; i < 2; i++) {
        SDL_Surface *surface = TTF_RenderText_Blended(font_big, words[i], colors[i]);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        
        SDL_Rect dest = {
            .x = x_pos + (i * 130),
            .y = 30,
            .w = surface->w,
            .h = surface->h
        };
        
        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void render_attempts(SDL_Renderer *renderer, TTF_Font *font, int attempts) {
    char attempt_text[32];
    snprintf(attempt_text, sizeof(attempt_text), "Attempts: %d/%d", attempts, MAX_ATTEMPTS);
    
    SDL_Color color = {200, 200, 200, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(font, attempt_text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect dest = {
        .x = 20,
        .y = 20,
        .w = surface->w,
        .h = surface->h
    };
    
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_game_screen(SDL_Renderer *renderer, TTF_Font *font, TTF_Font *font_big, GameState *game) {
    render_title(renderer, font_big);
    render_attempts(renderer, font, game->attempts);
    
    // Render current guess
    char guess_text[32];
    snprintf(guess_text, sizeof(guess_text), "%d", game->current_guess);
    SDL_Color guess_color = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(font_big, guess_text, guess_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    // Draw guess background
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
    SDL_Rect guess_bg = {
        .x = (WINDOW_WIDTH - 200) / 2,
        .y = 220,
        .w = 200,
        .h = 80
    };
    SDL_RenderFillRect(renderer, &guess_bg);
    
    SDL_Rect guess_dest = {
        .x = (WINDOW_WIDTH - surface->w) / 2,
        .y = 230,
        .w = surface->w,
        .h = surface->h
    };
    SDL_RenderCopy(renderer, texture, NULL, &guess_dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    
    // Render temperature for confirmed guesses
    if (game->last_guess != -1 && !game->game_over) {
        char temp_text[32];
        SDL_Color temp_color;
        get_temperature(game, temp_text, &temp_color);
        SDL_Surface *temp_surface = TTF_RenderText_Blended(font, temp_text, temp_color);
        SDL_Texture *temp_texture = SDL_CreateTextureFromSurface(renderer, temp_surface);
        
        SDL_Rect temp_dest = {
            .x = (WINDOW_WIDTH - temp_surface->w) / 2,
            .y = 350,
            .w = temp_surface->w,
            .h = temp_surface->h
        };
        SDL_RenderCopy(renderer, temp_texture, NULL, &temp_dest);
        SDL_FreeSurface(temp_surface);
        SDL_DestroyTexture(temp_texture);
    }
    
    // Render game messages
    if (game->messages[0][0] != '\0') {
        SDL_Surface *msg_surface = TTF_RenderText_Blended(font, game->messages[0], 
                                                        game->message_colors[0]);
        SDL_Texture *msg_texture = SDL_CreateTextureFromSurface(renderer, msg_surface);
        
        SDL_Rect msg_dest = {
            .x = (WINDOW_WIDTH - msg_surface->w) / 2,
            .y = 450,
            .w = msg_surface->w,
            .h = msg_surface->h
        };
        SDL_RenderCopy(renderer, msg_texture, NULL, &msg_dest);
        SDL_FreeSurface(msg_surface);
        SDL_DestroyTexture(msg_texture);
    }
}

void render_welcome_screen(SDL_Renderer *renderer, TTF_Font *font, TTF_Font *font_big, GameState *game) {
    render_title(renderer, font_big);
    
    for (int i = 0; i < MAX_MESSAGES; i++) {
        SDL_Surface *surface = TTF_RenderText_Blended(font, game->messages[i], 
                                                    game->message_colors[i]);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        
        SDL_Rect dest = {
            .x = (WINDOW_WIDTH - surface->w) / 2,
            .y = 200 + (i * 60),
            .w = surface->w,
            .h = surface->h
        };
        
        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // Initialize game controller
    SDL_GameController *controller = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) {
                printf("Found game controller: %s\n", 
                       SDL_GameControllerName(controller));
                break;
            }
        }
    }
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        if (controller) SDL_GameControllerClose(controller);
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Hot/Cold Game",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        if (controller) SDL_GameControllerClose(controller);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        if (controller) SDL_GameControllerClose(controller);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    TTF_Font *font_big = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 48);
    
    if (!font || !font_big) {
        printf("Font loading failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        if (controller) SDL_GameControllerClose(controller);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    srand(time(NULL));
    GameState game;
    game.controller = controller;
    init_game(&game);
    
    bool running = true;
    SDL_Event event;
    Uint32 last_controller_check = 0;
    const int CONTROLLER_CHECK_INTERVAL = 1000;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (game.current_screen == WELCOME_SCREEN) {
                if (event.type == SDL_KEYDOWN || 
                    (event.type == SDL_CONTROLLERBUTTONDOWN)) {
                    game.current_screen = GAME_SCREEN;
                    game.messages[0][0] = '\0';
                }
            } else if (!game.game_over) {
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                            game.current_guess = game.current_guess < 100 ? 
                                               game.current_guess + 1 : 100;
                            break;
                        case SDLK_DOWN:
                            game.current_guess = game.current_guess > 1 ? 
                                               game.current_guess - 1 : 1;
                            break;
                        case SDLK_RETURN:
                            make_guess(&game);
                            break;
                    }
                } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                    switch (event.cbutton.button) {
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:
                            game.current_guess = game.current_guess < 100 ? 
                                               game.current_guess + 1 : 100;
                            break;
                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                            game.current_guess = game.current_guess > 1 ? 
                                               game.current_guess - 1 : 1;
                            break;
                        case SDL_CONTROLLER_BUTTON_A:
                            make_guess(&game);
                            break;
                    }
                }
            } else if ((event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) ||
                       (event.type == SDL_CONTROLLERBUTTONDOWN && 
                        event.cbutton.button == SDL_CONTROLLER_BUTTON_A)) {
                init_game(&game);
            } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
                if (!game.controller) {
                    game.controller = SDL_GameControllerOpen(event.cdevice.which);
                    if (game.controller) {
                        printf("Controller connected: %s\n", 
                               SDL_GameControllerName(game.controller));
                    }
                }
            } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                if (game.controller && 
                    event.cdevice.which == SDL_JoystickInstanceID(
                        SDL_GameControllerGetJoystick(game.controller))) {
                    SDL_GameControllerClose(game.controller);
                    game.controller = NULL;
                    printf("Controller disconnected\n");
                }
            }
        }

        // Check for new controllers periodically
        Uint32 current_time = SDL_GetTicks();
        if (!game.controller && (current_time - last_controller_check > CONTROLLER_CHECK_INTERVAL)) {
            for (int i = 0; i < SDL_NumJoysticks(); i++) {
                if (SDL_IsGameController(i)) {
                    game.controller = SDL_GameControllerOpen(i);
                    if (game.controller) {
                        printf("Found game controller: %s\n", 
                               SDL_GameControllerName(game.controller));
                        break;
                    }
                }
            }
            last_controller_check = current_time;
        }
        
        // Render game
        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);
        
        if (game.current_screen == WELCOME_SCREEN) {
            render_welcome_screen(renderer, font, font_big, &game);
        } else {
            render_game_screen(renderer, font, font_big, &game);
        }
        
        SDL_RenderPresent(renderer);
    }
    
    if (game.controller) {
        SDL_GameControllerClose(game.controller);
    }
    TTF_CloseFont(font_big);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}