#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

template<typename... Args>
void Assert(const bool expr, Args &&... args) {
    if (!expr) {
        (std::cerr << ... << args) << '\n';
        std::exit(1);
    }
}

std::random_device rd;
std::mt19937 gen(rd());

constexpr short SCREEN_WIDTH{800}, SCREEN_HEIGHT{600};
bool running = true;
bool isMainMenu = true;

enum class ScreenState { MENU, PLAY, GAME_OVER };

auto currentState = ScreenState::MENU;

struct App {
    SDL_Renderer *renderer;
    SDL_Window *window;
    TTF_Font *font;
};

struct Position {
    int x, y;
};

struct Size {
    int width, height;
};

struct Segment {
    Position position;
    Size size;
};

void renderText(const App &app, const std::string &text, int x, int y) {
    constexpr SDL_Color color{255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(app.font, text.c_str(), color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(app.renderer, surface);
    const SDL_Rect rect{x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderCopy(app.renderer, texture, nullptr, &rect);
    SDL_DestroyTexture(texture);
}

class Food {
public:
    Food() {
        reset();
    }

    void renderFood(SDL_Renderer &renderer) const {
        const SDL_Rect rect{position.x, position.y, size.width, size.height};
        SDL_SetRenderDrawColor(&renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(&renderer, &rect);
    }

    [[nodiscard]] Size getSize() const {
        return size;
    }

    [[nodiscard]] Position getPosition() const {
        return position;
    }

    void reset() {
        std::uniform_int_distribution dist_x(0, SCREEN_WIDTH - size.width);
        std::uniform_int_distribution dist_y(0, SCREEN_HEIGHT - size.height);
        position.x = dist_x(gen);
        position.y = dist_y(gen);
    }

private:
    Size size{35, 35};
    Position position{};
};

class Snake {
public:
    Snake() {
        segments.push_back({{SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, {35, 35}});
    }

    void renderSnake(SDL_Renderer &renderer) const {
        for (const auto &[position, size]: segments) {
            const SDL_Rect rect{position.x, position.y, size.width, size.height};
            SDL_SetRenderDrawColor(&renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(&renderer, &rect);
        }
    }

    void updateSnakeInput(const SDL_KeyboardEvent *event) {
        if (!isAlive) return;

        const bool movingLeft = (direction.x == -1 && direction.y == 0);
        const bool movingRight = (direction.x == 1 && direction.y == 0);
        const bool movingUp = (direction.x == 0 && direction.y == -1);

        if (const bool movingDown = (direction.x == 0 && direction.y == 1); event->keysym.sym == SDLK_w && !movingDown) {
            direction = {0, -1};
        } else if (event->keysym.sym == SDLK_s && !movingUp) {
            direction = {0, 1};
        } else if (event->keysym.sym == SDLK_a && !movingRight) {
            direction = {-1, 0};
        } else if (event->keysym.sym == SDLK_d && !movingLeft) {
            direction = {1, 0};
        }
    }


    void updateSnake() {
        if (!isAlive) return;

        std::vector<Position> oldPositions;
        for (const auto &segment: segments) {
            oldPositions.push_back(segment.position);
        }

        for (auto i = segments.size() - 1; i > 0; --i) {
            segments[i].position = oldPositions[i - 1];
        }

        segments[0].position.x += direction.x * speed;
        segments[0].position.y += direction.y * speed;

        for (int i = 1; i < segments.size(); ++i) {
            if (segments[0].position.x == oldPositions[i].x &&
                segments[0].position.y == oldPositions[i].y) {
                isAlive = false;
            }
        }

        if (segments[0].position.x < 0) { isAlive = false; }
        if (segments[0].position.x > SCREEN_WIDTH - segments[0].size.width) { isAlive = false; }
        if (segments[0].position.y < 0) { isAlive = false; }
        if (segments[0].position.y > SCREEN_HEIGHT - segments[0].size.height) { isAlive = false; }
    }

    void eatFood(Food &food) {
        if (isColliding(food)) {
            segments.push_back({segments.back().position, {35, 35}});
            food.reset();
            points++;
        }
    }

    [[nodiscard]] bool isColliding(const Food &food) const {
        return segments[0].position.x + segments[0].size.width >= food.getPosition().x &&
               segments[0].position.x <= food.getPosition().x + food.getSize().width &&
               segments[0].position.y + segments[0].size.height >= food.getPosition().y &&
               segments[0].position.y <= food.getPosition().y + food.getSize().height;
    }

    [[nodiscard]] bool getIsAlive() const {
        return isAlive;
    }

    [[nodiscard]] int getPoints() const {
        return points;
    }

    void reset(Food &food) {
        segments.clear();
        segments.push_back({{SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, {35, 35}});
        direction = {1, 0};
        isAlive = true;
        food.reset();
        points = 0;
    }

private:
    std::pmr::vector<Segment> segments;
    Position direction{1, 0};
    int speed{4}, points{0};
    bool isAlive{true};
};


Food food{};
Snake snake{};

void initSDL(App &app) {
    constexpr int rendererFlags{SDL_RENDERER_ACCELERATED};
    constexpr int windowFlags{0};

    Assert(SDL_Init(SDL_INIT_VIDEO) >= 0, "Failed to initialize SDL:", SDL_GetError());

    app.window = SDL_CreateWindow("SNAKE!!!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                  SCREEN_HEIGHT,
                                  windowFlags);
    Assert(app.window != nullptr, "Failed to create window: ", SDL_GetError());

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    app.renderer = SDL_CreateRenderer(app.window, -1, rendererFlags);
    Assert(app.renderer != nullptr, "Failed to create renderer: ", SDL_GetError());

    Assert(TTF_Init() >= 0, "Failed to initialize SDL_ttf:", TTF_GetError());

    app.font = TTF_OpenFont("assets/Minecraft.ttf", 32);
    Assert(app.font != nullptr, "Failed to load font: ", SDL_GetError());
}

void input() {
    SDL_Event event{};

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_q) running = false;
                if (event.key.keysym.sym == SDLK_SPACE && isMainMenu) {
                    snake.reset(food);
                    currentState = ScreenState::PLAY;
                }
                if (event.key.keysym.sym == SDLK_SPACE && !isMainMenu) {
                    isMainMenu = true;
                    snake.reset(food);
                    currentState = ScreenState::MENU;
                }
                snake.updateSnakeInput(&event.key);
                break;
            default:
                break;
        }
    }
}

void cleanup(const App &app) {
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}

int main() {
    App app{};
    initSDL(app);

    while (running) {
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app.renderer);
        switch (currentState) {
            case ScreenState::MENU:
                renderText(app, "SNAKE!!!", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 50);
                break;
            case ScreenState::PLAY:
                snake.renderSnake(*app.renderer);
                snake.updateSnake();
                food.renderFood(*app.renderer);
                snake.eatFood(food);
                renderText(app, "Points: " + std::to_string(snake.getPoints()), 10, 10);
                break;
            case ScreenState::GAME_OVER:
                renderText(app, "YOU LOST: " + std::to_string(snake.getPoints()), SCREEN_WIDTH / 2 - 100,
                           SCREEN_HEIGHT / 2 - 50);
                break;
        }
        SDL_RenderPresent(app.renderer);
        input();

        if (!snake.getIsAlive()) currentState = ScreenState::GAME_OVER;

        SDL_Delay(16);
    }

    cleanup(app);

    return EXIT_SUCCESS;
}
