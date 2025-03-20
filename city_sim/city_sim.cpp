#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <random>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <iomanip>
#include <deque>

// Screen dimensions
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// Grid settings
const int CELL_SIZE = 16; // Increased size for better visibility
const int GRID_WIDTH = SCREEN_WIDTH / CELL_SIZE;
const int GRID_HEIGHT = SCREEN_HEIGHT / CELL_SIZE;

// Simulation constants
const int INITIAL_ROADS = 30;
const int MAX_SIMULATION_STEPS = 10000;
const Uint32 SIMULATION_DELAY = 300;  // Increased delay to slow down growth

// Cell types
enum CellType {
    EMPTY = 0,
    ROAD = 1,
    RESIDENTIAL = 2,
    COMMERCIAL = 3,
    INDUSTRIAL = 4,
    WATER = 5,
    PARK = 6,
    POWER_PLANT = 7,
    GOVERNMENT = 8,
    FOREST = 9,
    FARM = 10
};

// Building style
enum BuildingStyle {
    BASIC = 0,
    MODERN = 1,
    HISTORIC = 2,
    FANCY = 3
};

// Color definitions with better visual distinction
const SDL_Color COLOR_EMPTY = {50, 70, 30, 255};
const SDL_Color COLOR_ROAD = {80, 80, 80, 255};
const SDL_Color COLOR_RESIDENTIAL_BASE = {220, 180, 150, 255};
const SDL_Color COLOR_COMMERCIAL_BASE = {130, 170, 210, 255};
const SDL_Color COLOR_INDUSTRIAL_BASE = {200, 170, 100, 255};
const SDL_Color COLOR_WATER = {30, 120, 200, 255};
const SDL_Color COLOR_PARK = {60, 180, 60, 255};
const SDL_Color COLOR_POWER_PLANT = {180, 60, 60, 255};
const SDL_Color COLOR_GOVERNMENT = {180, 120, 200, 255};
const SDL_Color COLOR_FOREST = {40, 140, 40, 255};
const SDL_Color COLOR_FARM = {180, 210, 100, 255};

// Direction vectors for neighbors
const int dx[] = {-1, 0, 1, 0, -1, -1, 1, 1};
const int dy[] = {0, 1, 0, -1, -1, 1, 1, -1};

// Structure to represent a building
struct Building {
    CellType type;
    int density;
    int age;
    BuildingStyle style;
    int variant;
    bool hasTree;
    bool hasCar;
};

// Structure to represent a car
struct Car {
    float x, y;
    float speed;
    int direction;
    int roadIndex;
    SDL_Color color;
};

// City grid and related data
std::vector<std::vector<CellType>> grid;
std::vector<std::vector<Building>> buildings;
std::vector<Car> cars;
std::vector<std::pair<int, int>> roads;
std::vector<std::pair<int, int>> waterCells;
int currentStep = 0;
Uint32 lastWaterAnimTime = 0;
int waterAnimPhase = 0;

// Water animation colors
const int WATER_ANIM_PHASES = 8;
SDL_Color waterColors[WATER_ANIM_PHASES];

// Random number generation
std::random_device rd;
std::mt19937 gen(rd());

// Function prototypes
void initializeGrid();
void simulationStep();
void drawGrid(SDL_Renderer* renderer);
void generateInitialRoads();
void generateTerrain();
bool isValidCell(int x, int y);
int countNeighborsOfType(int x, int y, CellType type);
int countNeighborsOfTypeInRadius(int x, int y, CellType type, int radius);
void growCity();
void updateCars();
void drawCars(SDL_Renderer* renderer);
void addRandomCar();
SDL_Color getBuildingColor(const Building& building);
void drawBuilding(SDL_Renderer* renderer, int x, int y, const Building& building);
void drawWater(SDL_Renderer* renderer, int x, int y);
void drawRoad(SDL_Renderer* renderer, int x, int y);
void drawTree(SDL_Renderer* renderer, int x, int y, int size);
void initializeWaterAnimation();
void updateWaterAnimation();

// Initialize water animation colors
void initializeWaterAnimation() {
    waterColors[0] = {20, 100, 200, 255};
    waterColors[1] = {25, 110, 205, 255};
    waterColors[2] = {30, 120, 210, 255};
    waterColors[3] = {35, 130, 215, 255};
    waterColors[4] = {40, 140, 220, 255};
    waterColors[5] = {35, 130, 215, 255};
    waterColors[6] = {30, 120, 210, 255};
    waterColors[7] = {25, 110, 205, 255};
}

// Update water animation phase
void updateWaterAnimation() {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastWaterAnimTime > 200) {
        waterAnimPhase = (waterAnimPhase + 1) % WATER_ANIM_PHASES;
        lastWaterAnimTime = currentTime;
    }
}

// Check if coordinates are within grid bounds
bool isValidCell(int x, int y) {
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

// Count neighbors of a specific type (using 4 directions)
int countNeighborsOfType(int x, int y, CellType type) {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (isValidCell(nx, ny) && grid[nx][ny] == type) {
            count++;
        }
    }
    return count;
}

// Count neighbors of a specific type within a radius
int countNeighborsOfTypeInRadius(int x, int y, CellType type, int radius) {
    int count = 0;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if (i == 0 && j == 0) continue;
            int nx = x + i;
            int ny = y + j;
            if (isValidCell(nx, ny) && grid[nx][ny] == type) {
                count++;
            }
        }
    }
    return count;
}

// Initialize the grid with empty cells and prepare related data structures
void initializeGrid() {
    grid.resize(GRID_WIDTH, std::vector<CellType>(GRID_HEIGHT, EMPTY));
    buildings.resize(GRID_WIDTH, std::vector<Building>(GRID_HEIGHT));
    
    // Initialize all buildings to empty
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            buildings[x][y] = {EMPTY, 0, 0, BASIC, 0, false, false};
        }
    }
    
    // Initialize water animation
    initializeWaterAnimation();
    
    // Generate terrain features first
    generateTerrain();
    
    // Generate initial roads
    generateInitialRoads();
}

// Generate terrain features like water bodies, forests, etc.
void generateTerrain() {
    // Generate water bodies (rivers and lakes)
    std::uniform_int_distribution<int> dist_water_count(1, 3);
    int waterBodies = dist_water_count(gen);
    
    for (int i = 0; i < waterBodies; i++) {
        std::uniform_int_distribution<int> dist_x(5, GRID_WIDTH - 5);
        std::uniform_int_distribution<int> dist_y(5, GRID_HEIGHT - 5);
        int startX = dist_x(gen);
        int startY = dist_y(gen);
        
        // Generate a river or lake
        std::uniform_int_distribution<int> dist_type(0, 1);
        if (dist_type(gen) == 0) {
            // River
            int length = 15 + gen() % 20;
            int dir = gen() % 4;
            int curX = startX;
            int curY = startY;
            
            for (int j = 0; j < length; j++) {
                // Occasionally change direction slightly
                if (gen() % 5 == 0) {
                    dir = (dir + (gen() % 3 - 1)) % 4;
                    if (dir < 0) dir += 4;
                }
                
                // Create river segment and some surrounding water
                for (int ox = -1; ox <= 1; ox++) {
                    for (int oy = -1; oy <= 1; oy++) {
                        int nx = curX + ox;
                        int ny = curY + oy;
                        if (isValidCell(nx, ny) && grid[nx][ny] == EMPTY) {
                            grid[nx][ny] = WATER;
                            waterCells.push_back({nx, ny});
                        }
                    }
                }
                
                // Move to next position
                curX += dx[dir];
                curY += dy[dir];
                if (!isValidCell(curX, curY)) break;
            }
        } else {
            // Lake
            std::deque<std::pair<int, int>> queue;
            queue.push_back({startX, startY});
            std::uniform_int_distribution<int> dist_size(20, 40);
            int size = dist_size(gen);
            
            while (!queue.empty() && size > 0) {
                auto [x, y] = queue.front();
                queue.pop_front();
                
                if (!isValidCell(x, y) || grid[x][y] != EMPTY) continue;
                
                grid[x][y] = WATER;
                waterCells.push_back({x, y});
                size--;
                
                for (int d = 0; d < 4; d++) {
                    std::uniform_int_distribution<int> dist_expand(0, 100);
                    if (dist_expand(gen) < 70) { // 70% chance to expand
                        queue.push_back({x + dx[d], y + dy[d]});
                    }
                }
            }
        }
    }
    
    // Generate forests
    std::uniform_int_distribution<int> dist_forest_count(2, 5);
    int forestCount = dist_forest_count(gen);
    
    for (int i = 0; i < forestCount; i++) {
        std::uniform_int_distribution<int> dist_x(5, GRID_WIDTH - 5);
        std::uniform_int_distribution<int> dist_y(5, GRID_HEIGHT - 5);
        int startX = dist_x(gen);
        int startY = dist_y(gen);
        
        std::deque<std::pair<int, int>> queue;
        queue.push_back({startX, startY});
        std::uniform_int_distribution<int> dist_size(10, 30);
        int size = dist_size(gen);
        
        while (!queue.empty() && size > 0) {
            auto [x, y] = queue.front();
            queue.pop_front();
            
            if (!isValidCell(x, y) || grid[x][y] != EMPTY) continue;
            
            grid[x][y] = FOREST;
            buildings[x][y].type = FOREST;
            buildings[x][y].variant = gen() % 3; // Different tree types
            size--;
            
            for (int d = 0; d < 4; d++) {
                std::uniform_int_distribution<int> dist_expand(0, 100);
                if (dist_expand(gen) < 60) { // 60% chance to expand
                    queue.push_back({x + dx[d], y + dy[d]});
                }
            }
        }
    }
    
    // Generate farms in some areas
    std::uniform_int_distribution<int> dist_farm_count(1, 3);
    int farmCount = dist_farm_count(gen);
    
    for (int i = 0; i < farmCount; i++) {
        std::uniform_int_distribution<int> dist_x(5, GRID_WIDTH - 5);
        std::uniform_int_distribution<int> dist_y(5, GRID_HEIGHT - 5);
        int startX = dist_x(gen);
        int startY = dist_y(gen);
        
        std::uniform_int_distribution<int> dist_width(5, 10);
        std::uniform_int_distribution<int> dist_height(5, 10);
        int width = dist_width(gen);
        int height = dist_height(gen);
        
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                int nx = startX + x;
                int ny = startY + y;
                if (isValidCell(nx, ny) && grid[nx][ny] == EMPTY) {
                    grid[nx][ny] = FARM;
                    buildings[nx][ny].type = FARM;
                    buildings[nx][ny].variant = gen() % 3; // Different farm types
                }
            }
        }
    }
}

// Generate initial road layout
void generateInitialRoads() {
    // Create a main horizontal road
    int mainRoadY = GRID_HEIGHT / 2;
    for (int x = 0; x < GRID_WIDTH; x++) {
        if (grid[x][mainRoadY] == EMPTY) {
            grid[x][mainRoadY] = ROAD;
            roads.push_back({x, mainRoadY});
        }
    }
    
    // Create a main vertical road
    int mainRoadX = GRID_WIDTH / 2;
    for (int y = 0; y < GRID_HEIGHT; y++) {
        if (grid[mainRoadX][y] == EMPTY) {
            grid[mainRoadX][y] = ROAD;
            roads.push_back({mainRoadX, y});
        }
    }
    
    // Add some random roads branching from main roads
    std::uniform_int_distribution<int> dist_width(0, GRID_WIDTH - 1);
    std::uniform_int_distribution<int> dist_height(0, GRID_HEIGHT - 1);
    std::uniform_int_distribution<int> dist_length(5, 15);
    std::uniform_int_distribution<int> dist_direction(0, 3);
    
    for (int i = 0; i < INITIAL_ROADS; i++) {
        // Start from an existing road
        int x, y;
        if (i % 2 == 0) {
            // Start from main horizontal road
            x = dist_width(gen);
            y = mainRoadY;
        } else {
            // Start from main vertical road
            x = mainRoadX;
            y = dist_height(gen);
        }
        
        // Pick a direction (0: left, 1: down, 2: right, 3: up)
        int direction = dist_direction(gen);
        int length = dist_length(gen);
        
        for (int j = 0; j < length; j++) {
            x += dx[direction];
            y += dy[direction];
            
            if (isValidCell(x, y) && grid[x][y] == EMPTY) {
                grid[x][y] = ROAD;
                roads.push_back({x, y});
            } else {
                break;
            }
        }
    }
    
    // Add some cars
    for (int i = 0; i < 15; i++) {
        addRandomCar();
    }
}

// Add a new random car to the simulation
void addRandomCar() {
    if (roads.empty()) return;
    
    std::uniform_int_distribution<int> dist_road(0, roads.size() - 1);
    int roadIndex = dist_road(gen);
    auto [x, y] = roads[roadIndex];
    
    std::uniform_real_distribution<float> dist_speed(0.05f, 0.2f);
    std::uniform_int_distribution<int> dist_direction(0, 3);
    std::uniform_int_distribution<int> dist_r(150, 250);
    std::uniform_int_distribution<int> dist_g(150, 250);
    std::uniform_int_distribution<int> dist_b(150, 250);
    
    Car car;
    car.x = x;
    car.y = y;
    car.speed = dist_speed(gen);
    car.direction = dist_direction(gen);
    car.roadIndex = roadIndex;
    car.color = {
        static_cast<Uint8>(dist_r(gen)), 
        static_cast<Uint8>(dist_g(gen)), 
        static_cast<Uint8>(dist_b(gen)), 
        255
    };
    
    cars.push_back(car);
}

// Update car positions
void updateCars() {
    if (roads.empty()) return;
    
    for (auto& car : cars) {
        // Move car along current direction
        float nextX = car.x + dx[car.direction] * car.speed;
        float nextY = car.y + dy[car.direction] * car.speed;
        
        // Check if we need to change direction or find a new road
        if (!isValidCell(static_cast<int>(nextX), static_cast<int>(nextY)) || 
            grid[static_cast<int>(nextX)][static_cast<int>(nextY)] != ROAD) {
            
            // Find adjacent roads
            std::vector<int> possibleDirs;
            for (int d = 0; d < 4; d++) {
                if (d == (car.direction + 2) % 4) continue; // Don't go backwards
                
                int nx = static_cast<int>(car.x) + dx[d];
                int ny = static_cast<int>(car.y) + dy[d];
                
                if (isValidCell(nx, ny) && grid[nx][ny] == ROAD) {
                    possibleDirs.push_back(d);
                }
            }
            
            if (!possibleDirs.empty()) {
                // Choose a random valid direction
                std::uniform_int_distribution<int> dist_dir(0, possibleDirs.size() - 1);
                car.direction = possibleDirs[dist_dir(gen)];
                
                // Update next position
                nextX = car.x + dx[car.direction] * car.speed;
                nextY = car.y + dy[car.direction] * car.speed;
            } else {
                // No valid direction, teleport to another road
                std::uniform_int_distribution<int> dist_road(0, roads.size() - 1);
                car.roadIndex = dist_road(gen);
                auto [newX, newY] = roads[car.roadIndex];
                car.x = newX;
                car.y = newY;
                
                std::uniform_int_distribution<int> dist_dir(0, 3);
                car.direction = dist_dir(gen);
                continue;
            }
        }
        
        // Update position
        car.x = nextX;
        car.y = nextY;
    }
    
    // Occasionally add a new car
    if (gen() % 100 < 5 && cars.size() < roads.size() / 5) {
        addRandomCar();
    }
}

// Draw cars
void drawCars(SDL_Renderer* renderer) {
    for (const auto& car : cars) {
        SDL_SetRenderDrawColor(renderer, car.color.r, car.color.g, car.color.b, car.color.a);
        
        SDL_Rect carRect;
        carRect.x = static_cast<int>(car.x * CELL_SIZE) + CELL_SIZE / 3;
        carRect.y = static_cast<int>(car.y * CELL_SIZE) + CELL_SIZE / 3;
        carRect.w = CELL_SIZE / 3;
        carRect.h = CELL_SIZE / 3;
        
        SDL_RenderFillRect(renderer, &carRect);
    }
}

// Grow the city by adding new buildings
void growCity() {
    // Find potential spots for new buildings near roads
    std::vector<std::pair<int, int>> buildingSpots;
    
    for (const auto& [rx, ry] : roads) {
        for (int d = 0; d < 4; d++) {
            int nx = rx + dx[d];
            int ny = ry + dy[d];
            
            if (isValidCell(nx, ny) && grid[nx][ny] == EMPTY) {
                buildingSpots.push_back({nx, ny});
            }
        }
    }
    
    // Randomly select some spots to build on
    std::shuffle(buildingSpots.begin(), buildingSpots.end(), gen);
    int maxBuildingsPerStep = 1 + currentStep / 50; // Gradually increase building rate
    int newBuildings = std::min(maxBuildingsPerStep, static_cast<int>(buildingSpots.size()));
    
    std::uniform_int_distribution<int> dist_type(0, 100);
    std::uniform_int_distribution<int> dist_style(0, 3);
    std::uniform_int_distribution<int> dist_variant(0, 4);
    std::uniform_int_distribution<int> dist_tree(0, 100);
    
    for (int i = 0; i < newBuildings; i++) {
        if (i >= buildingSpots.size()) break;
        
        int x = buildingSpots[i].first;
        int y = buildingSpots[i].second;
        
        CellType type;
        int randType = dist_type(gen);
        
        // Determine building type based on surroundings and random chance
        if (randType < 60) {
            type = RESIDENTIAL;
        } else if (randType < 85) {
            type = COMMERCIAL;
        } else {
            type = INDUSTRIAL;
        }
        
        // Check for water nearby to prefer residential
        if (countNeighborsOfTypeInRadius(x, y, WATER, 3) > 0 && randType < 80) {
            type = RESIDENTIAL;
        }
        
        // Check for parks or forests nearby to prefer residential
        if ((countNeighborsOfTypeInRadius(x, y, PARK, 3) > 0 || 
             countNeighborsOfTypeInRadius(x, y, FOREST, 3) > 0) && randType < 75) {
            type = RESIDENTIAL;
        }
        
        // Occasionally create a park instead
        if (randType > 95 && currentStep > 50) {
            type = PARK;
        }
        
        // Update grid and building info
        grid[x][y] = type;
        buildings[x][y].type = type;
        buildings[x][y].density = 1;
        buildings[x][y].age = 0;
        buildings[x][y].style = static_cast<BuildingStyle>(dist_style(gen));
        buildings[x][y].variant = dist_variant(gen);
        
        // Sometimes add a tree to residential or commercial buildings
        if ((type == RESIDENTIAL || type == COMMERCIAL) && dist_tree(gen) < 40) {
            buildings[x][y].hasTree = true;
        }
    }
    
    // Mature existing buildings
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            if (grid[x][y] != EMPTY && grid[x][y] != ROAD && grid[x][y] != WATER) {
                buildings[x][y].age++;
                
                // Increase density for some buildings as they age
                if (buildings[x][y].age % 20 == 0 && buildings[x][y].density < 3) {
                    if (grid[x][y] == RESIDENTIAL || grid[x][y] == COMMERCIAL || grid[x][y] == INDUSTRIAL) {
                        if (gen() % 5 < 3) { // 60% chance to increase density
                            buildings[x][y].density++;
                        }
                    }
                }
                
                // Add a tree to some residential buildings over time
                if (!buildings[x][y].hasTree && grid[x][y] == RESIDENTIAL && buildings[x][y].age % 30 == 0) {
                    if (gen() % 10 < 4) { // 40% chance to add a tree
                        buildings[x][y].hasTree = true;
                    }
                }
            }
        }
    }
    
    // Add new roads as the city grows
    if (currentStep % 10 == 0) {
        // Find potential spots for new roads near buildings
        std::vector<std::pair<int, int>> roadSpots;
        
        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                if (grid[x][y] != EMPTY && grid[x][y] != ROAD && grid[x][y] != WATER) {
                    for (int d = 0; d < 4; d++) {
                        int nx = x + dx[d];
                        int ny = y + dy[d];
                        
                        if (isValidCell(nx, ny) && grid[nx][ny] == EMPTY) {
                            // Check if there's a road nearby
                            bool nearRoad = false;
                            for (int d2 = 0; d2 < 4; d2++) {
                                int nnx = nx + dx[d2];
                                int nny = ny + dy[d2];
                                if (isValidCell(nnx, nny) && grid[nnx][nny] == ROAD) {
                                    nearRoad = true;
                                    break;
                                }
                            }
                            
                            if (nearRoad) {
                                roadSpots.push_back({nx, ny});
                            }
                        }
                    }
                }
            }
        }
        
        // Add some new roads
        std::shuffle(roadSpots.begin(), roadSpots.end(), gen);
        int maxRoadsPerStep = 1 + currentStep / 100; // Gradually increase road building rate
        int newRoads = std::min(maxRoadsPerStep, static_cast<int>(roadSpots.size()));
        
        for (int i = 0; i < newRoads; i++) {
            if (i >= roadSpots.size()) break;
            
            int x = roadSpots[i].first;
            int y = roadSpots[i].second;
            
            grid[x][y] = ROAD;
            roads.push_back({x, y});
        }
    }
}

// Get building color based on type, density, and style
SDL_Color getBuildingColor(const Building& building) {
    SDL_Color color;
    
    switch (building.type) {
        case RESIDENTIAL:
            color = COLOR_RESIDENTIAL_BASE;
            // Adjust color based on style and variant
            switch (building.style) {
                case BASIC:
                    // No change for basic
                    break;
                case MODERN:
                    color.r = 200 + building.variant * 10;
                    color.g = 200 + building.variant * 10;
                    color.b = 220 + building.variant * 5;
                    break;
                case HISTORIC:
                    color.r = 190 + building.variant * 10;
                    color.g = 150 + building.variant * 8;
                    color.b = 130 + building.variant * 5;
                    break;
                case FANCY:
                    color.r = 240 - building.variant * 5;
                    color.g = 180 + building.variant * 10;
                    color.b = 180 + building.variant * 10;
                    break;
            }
            break;
        
        case COMMERCIAL:
            color = COLOR_COMMERCIAL_BASE;
            // Adjust color based on style and variant
            switch (building.style) {
                case BASIC:
                    // No change for basic
                    break;
                case MODERN:
                    color.r = 100 + building.variant * 5;
                    color.g = 180 + building.variant * 10;
                    color.b = 230 + building.variant * 5;
                    break;
                case HISTORIC:
                    color.r = 150 + building.variant * 8;
                    color.g = 160 + building.variant * 5;
                    color.b = 180 + building.variant * 10;
                    break;
                case FANCY:
                    color.r = 130 + building.variant * 10;
                    color.g = 200 + building.variant * 5;
                    color.b = 230 + building.variant * 5;
                    break;
            }
            break;
        
        case INDUSTRIAL:
            color = COLOR_INDUSTRIAL_BASE;
            // Adjust color based on variant
            color.r = color.r - building.variant * 10;
            color.g = color.g - building.variant * 5;
            break;
        
        case PARK:
            color = COLOR_PARK;
            // Slightly vary park colors
            color.g = 160 + building.variant * 10;
            break;
        
        case GOVERNMENT:
            color = COLOR_GOVERNMENT;
            break;
        
        case POWER_PLANT:
            color = COLOR_POWER_PLANT;
            break;
        
        case FOREST:
            color = COLOR_FOREST;
            // Vary forest colors
            color.g = 120 + building.variant * 10;
            break;
        
        case FARM:
            color = COLOR_FARM;
            // Vary farm colors
            color.g = 190 + building.variant * 10;
            color.r = 170 + building.variant * 8;
            break;
        
        default:
            color = {150, 150, 150, 255};
    }
    
    // Adjust for building density
    if (building.type == RESIDENTIAL || building.type == COMMERCIAL || building.type == INDUSTRIAL) {
        int densityFactor = building.density - 1;
        // Darken colors for higher density
        color.r = std::max(50, color.r - densityFactor * 20);
        color.g = std::max(50, color.g - densityFactor * 15);
        color.b = std::max(50, color.b - densityFactor * 15);
    }
    
    return color;
}

// Draw a building
void drawBuilding(SDL_Renderer* renderer, int x, int y, const Building& building) {
    SDL_Rect buildingRect;
    buildingRect.x = x * CELL_SIZE;
    buildingRect.y = y * CELL_SIZE;
    buildingRect.w = CELL_SIZE;
    buildingRect.h = CELL_SIZE;
    
    SDL_Color color = getBuildingColor(building);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(renderer, &buildingRect);
    
    // Draw building details based on type
    switch (building.type) {
        case RESIDENTIAL: {
            // Draw house details based on density and style
            if (building.density == 1) {
                // Small house
                SDL_Rect roofRect;
                roofRect.x = x * CELL_SIZE + 1;
                roofRect.y = y * CELL_SIZE + 1;
                roofRect.w = CELL_SIZE - 2;
                roofRect.h = CELL_SIZE / 3;
                
                // Roof color
                SDL_SetRenderDrawColor(renderer, 180, 100, 80, 255);
                SDL_RenderFillRect(renderer, &roofRect);
                
                // Window
                SDL_Rect windowRect;
                windowRect.x = x * CELL_SIZE + CELL_SIZE / 3;
                windowRect.y = y * CELL_SIZE + CELL_SIZE / 2;
                windowRect.w = CELL_SIZE / 3;
                windowRect.h = CELL_SIZE / 4;
                
                SDL_SetRenderDrawColor(renderer, 220, 230, 250, 255);
                SDL_RenderFillRect(renderer, &windowRect);
            } else if (building.density == 2) {
                // Medium house/apartment
                // Windows
                for (int i = 0; i < 2; i++) {
                    for (int j = 0; j < 2; j++) {
                        SDL_Rect windowRect;
                        windowRect.x = x * CELL_SIZE + 2 + i * (CELL_SIZE / 2 - 2);
                        windowRect.y = y * CELL_SIZE + 2 + j * (CELL_SIZE / 2 - 2);
                        windowRect.w = CELL_SIZE / 3;
                        windowRect.h = CELL_SIZE / 4;
                        
                        SDL_SetRenderDrawColor(renderer, 220, 230, 250, 255);
                        SDL_RenderFillRect(renderer, &windowRect);
                    }
                }
            } else {
                // Tall apartment building
                for (int i = 0; i < 3; i++) {
                    SDL_Rect windowRowRect;
                    windowRowRect.x = x * CELL_SIZE + 2;
                    windowRowRect.y = y * CELL_SIZE + 2 + i * (CELL_SIZE / 3);
                    windowRowRect.w = CELL_SIZE - 4;
                    windowRowRect.h = 2;
                    
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderFillRect(renderer, &windowRowRect);
                }
            }
            
            // Draw tree if present
            if (building.hasTree) {
                drawTree(renderer, x, y, 1);
            }
            break;
        }
        
        case COMMERCIAL: {
            // Commercial building details
            if (building.density == 1) {
                // Small shop
                SDL_Rect signRect;
                signRect.x = x * CELL_SIZE + 2;
                signRect.y = y * CELL_SIZE + 2;
                signRect.w = CELL_SIZE - 4;
                signRect.h = 4;
                
                SDL_SetRenderDrawColor(renderer, 220, 220, 100, 255);
                SDL_RenderFillRect(renderer, &signRect);
                
                // Window/door
                SDL_Rect windowRect;
                windowRect.x = x * CELL_SIZE + CELL_SIZE / 4;
                windowRect.y = y * CELL_SIZE + CELL_SIZE / 2;
                windowRect.w = CELL_SIZE / 2;
                windowRect.h = CELL_SIZE / 3;
                
                SDL_SetRenderDrawColor(renderer, 200, 220, 240, 255);
                SDL_RenderFillRect(renderer, &windowRect);
            } else if (building.density == 2) {
                // Medium commercial building
                // Windows grid
                for (int i = 0; i < 2; i++) {
                    for (int j = 0; j < 3; j++) {
                        SDL_Rect windowRect;
                        windowRect.x = x * CELL_SIZE + 2 + i * (CELL_SIZE / 2 - 2);
                        windowRect.y = y * CELL_SIZE + 2 + j * (CELL_SIZE / 3 - 1);
                        windowRect.w = CELL_SIZE / 3;
                        windowRect.h = CELL_SIZE / 6;
                        
                        SDL_SetRenderDrawColor(renderer, 180, 210, 240, 255);
                        SDL_RenderFillRect(renderer, &windowRect);
                    }
                }
            } else {
                // Office building
                for (int i = 0; i < 4; i++) {
                    SDL_Rect windowRowRect;
                    windowRowRect.x = x * CELL_SIZE + 2;
                    windowRowRect.y = y * CELL_SIZE + 2 + i * (CELL_SIZE / 4);
                    windowRowRect.w = CELL_SIZE - 4;
                    windowRowRect.h = CELL_SIZE / 8;
                    
                    SDL_SetRenderDrawColor(renderer, 150, 200, 240, 255);
                    SDL_RenderFillRect(renderer, &windowRowRect);
                }
            }
            
            // Draw tree if present
            if (building.hasTree) {
                drawTree(renderer, x, y, 1);
            }
            break;
        }
        
        case INDUSTRIAL: {
            // Industrial building details
            if (building.density == 1) {
                // Small warehouse
                SDL_Rect doorRect;
                doorRect.x = x * CELL_SIZE + CELL_SIZE / 3;
                doorRect.y = y * CELL_SIZE + CELL_SIZE / 2;
                doorRect.w = CELL_SIZE / 3;
                doorRect.h = CELL_SIZE / 2;
                
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_RenderFillRect(renderer, &doorRect);
            } else if (building.density == 2) {
                // Factory with smokestack
                SDL_Rect stackRect;
                stackRect.x = x * CELL_SIZE + CELL_SIZE / 4;
                stackRect.y = y * CELL_SIZE;
                stackRect.w = CELL_SIZE / 6;
                stackRect.h = CELL_SIZE / 2;
                
                SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                SDL_RenderFillRect(renderer, &stackRect);
                
                // Factory windows
                SDL_Rect windowRect;
                windowRect.x = x * CELL_SIZE + CELL_SIZE / 2;
                windowRect.y = y * CELL_SIZE + CELL_SIZE / 3;
                windowRect.w = CELL_SIZE / 3;
                windowRect.h = CELL_SIZE / 6;
                
                SDL_SetRenderDrawColor(renderer, 150, 150, 120, 255);
                SDL_RenderFillRect(renderer, &windowRect);
            } else {
                // Heavy industry
                // Multiple stacks
                for (int i = 0; i < 2; i++) {
                    SDL_Rect stackRect;
                    stackRect.x = x * CELL_SIZE + 2 + i * (CELL_SIZE / 2);
                    stackRect.y = y * CELL_SIZE;
                    stackRect.w = CELL_SIZE / 6;
                    stackRect.h = CELL_SIZE / 2;
                    
                    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
                    SDL_RenderFillRect(renderer, &stackRect);
                }
                
                // Structure
                SDL_Rect structureRect;
                structureRect.x = x * CELL_SIZE + 2;
                structureRect.y = y * CELL_SIZE + CELL_SIZE / 2;
                structureRect.w = CELL_SIZE - 4;
                structureRect.h = CELL_SIZE / 2;
                
                SDL_SetRenderDrawColor(renderer, 130, 130, 100, 255);
                SDL_RenderFillRect(renderer, &structureRect);
            }
            break;
        }
        
        case PARK: {
            // Draw park with trees and path
            drawTree(renderer, x, y, 2);
            
            // Path
            SDL_Rect pathRect;
            pathRect.x = x * CELL_SIZE + CELL_SIZE / 4;
            pathRect.y = y * CELL_SIZE + CELL_SIZE / 2;
            pathRect.w = CELL_SIZE / 2;
            pathRect.h = CELL_SIZE / 6;
            
            SDL_SetRenderDrawColor(renderer, 200, 180, 140, 255);
            SDL_RenderFillRect(renderer, &pathRect);
            break;
        }
        
        case FOREST: {
            // Draw multiple trees for forest
            drawTree(renderer, x, y, 3);
            break;
        }
        
        case FARM: {
            // Draw farm fields
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if ((i + j) % 2 == 0) continue; // Checkerboard pattern
                    
                    SDL_Rect fieldRect;
                    fieldRect.x = x * CELL_SIZE + i * (CELL_SIZE / 3);
                    fieldRect.y = y * CELL_SIZE + j * (CELL_SIZE / 3);
                    fieldRect.w = CELL_SIZE / 3;
                    fieldRect.h = CELL_SIZE / 3;
                    
                    // Darker green for planted fields
                    SDL_SetRenderDrawColor(renderer, 100, 180, 60, 255);
                    SDL_RenderFillRect(renderer, &fieldRect);
                }
            }
            
            // Small farm house
            if (building.variant % 2 == 0) {
                SDL_Rect houseRect;
                houseRect.x = x * CELL_SIZE + CELL_SIZE / 6;
                houseRect.y = y * CELL_SIZE + CELL_SIZE / 6;
                houseRect.w = CELL_SIZE / 3;
                houseRect.h = CELL_SIZE / 3;
                
                SDL_SetRenderDrawColor(renderer, 200, 150, 100, 255);
                SDL_RenderFillRect(renderer, &houseRect);
            }
            break;
        }
    }
}

// Draw water
void drawWater(SDL_Renderer* renderer, int x, int y) {
    SDL_Rect waterRect;
    waterRect.x = x * CELL_SIZE;
    waterRect.y = y * CELL_SIZE;
    waterRect.w = CELL_SIZE;
    waterRect.h = CELL_SIZE;
    
    // Use animated water colors
    SDL_Color waterColor = waterColors[waterAnimPhase];
    SDL_SetRenderDrawColor(renderer, waterColor.r, waterColor.g, waterColor.b, 255);
    SDL_RenderFillRect(renderer, &waterRect);
    
    // Draw wave lines
    SDL_SetRenderDrawColor(renderer, waterColor.r + 20, waterColor.g + 20, waterColor.b + 20, 180);
    for (int i = 0; i < 3; i++) {
        int yOffset = (i * CELL_SIZE / 3 + waterAnimPhase * 2) % CELL_SIZE;
        SDL_RenderDrawLine(renderer, 
                          x * CELL_SIZE, y * CELL_SIZE + yOffset, 
                          x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE + yOffset);
    }
}

// Draw road
void drawRoad(SDL_Renderer* renderer, int x, int y) {
    SDL_Rect roadRect;
    roadRect.x = x * CELL_SIZE;
    roadRect.y = y * CELL_SIZE;
    roadRect.w = CELL_SIZE;
    roadRect.h = CELL_SIZE;
    
    // Road base
    SDL_SetRenderDrawColor(renderer, COLOR_ROAD.r, COLOR_ROAD.g, COLOR_ROAD.b, 255);
    SDL_RenderFillRect(renderer, &roadRect);
    
    // Road markings
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    
    // Check if the road is horizontal or vertical
    bool northRoad = isValidCell(x, y-1) && grid[x][y-1] == ROAD;
    bool southRoad = isValidCell(x, y+1) && grid[x][y+1] == ROAD;
    bool eastRoad = isValidCell(x+1, y) && grid[x+1][y] == ROAD;
    bool westRoad = isValidCell(x-1, y) && grid[x-1][y] == ROAD;
    
    bool vertical = northRoad || southRoad;
    bool horizontal = eastRoad || westRoad;
    
    if (vertical && !horizontal) {
        // Vertical road - draw center line
        SDL_RenderDrawLine(renderer, 
                          x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE, 
                          x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE);
    } else if (horizontal && !vertical) {
        // Horizontal road - draw center line
        SDL_RenderDrawLine(renderer, 
                          x * CELL_SIZE, y * CELL_SIZE + CELL_SIZE / 2, 
                          x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE + CELL_SIZE / 2);
    } else if (vertical && horizontal) {
        // Intersection - draw crossing lines
        SDL_RenderDrawLine(renderer, 
                          x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE, 
                          x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE);
        SDL_RenderDrawLine(renderer, 
                          x * CELL_SIZE, y * CELL_SIZE + CELL_SIZE / 2, 
                          x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE + CELL_SIZE / 2);
    } else {
        // End of road or corner - draw a small square in the middle
        SDL_Rect centerRect;
        centerRect.x = x * CELL_SIZE + CELL_SIZE / 3;
        centerRect.y = y * CELL_SIZE + CELL_SIZE / 3;
        centerRect.w = CELL_SIZE / 3;
        centerRect.h = CELL_SIZE / 3;
        SDL_RenderFillRect(renderer, &centerRect);
    }
}

// Draw tree
void drawTree(SDL_Renderer* renderer, int x, int y, int size) {
    SDL_Rect trunkRect;
    if (size == 1) {
        // Single small tree
        trunkRect.x = x * CELL_SIZE + CELL_SIZE * 3 / 4;
        trunkRect.y = y * CELL_SIZE + CELL_SIZE * 2 / 3;
        trunkRect.w = CELL_SIZE / 8;
        trunkRect.h = CELL_SIZE / 4;
        
        SDL_SetRenderDrawColor(renderer, 120, 80, 40, 255);
        SDL_RenderFillRect(renderer, &trunkRect);
        
        // Tree top
        SDL_SetRenderDrawColor(renderer, 40, 160, 40, 255);
        SDL_Rect leafRect;
        leafRect.x = x * CELL_SIZE + CELL_SIZE * 2 / 3;
        leafRect.y = y * CELL_SIZE + CELL_SIZE / 2;
        leafRect.w = CELL_SIZE / 4;
        leafRect.h = CELL_SIZE / 4;
        SDL_RenderFillRect(renderer, &leafRect);
    } else if (size == 2) {
        // Medium park tree
        trunkRect.x = x * CELL_SIZE + CELL_SIZE / 2 - CELL_SIZE / 10;
        trunkRect.y = y * CELL_SIZE + CELL_SIZE / 2;
        trunkRect.w = CELL_SIZE / 5;
        trunkRect.h = CELL_SIZE / 3;
        
        SDL_SetRenderDrawColor(renderer, 120, 80, 40, 255);
        SDL_RenderFillRect(renderer, &trunkRect);
        
        // Tree top
        SDL_SetRenderDrawColor(renderer, 40, 180, 40, 255);
        int radius = CELL_SIZE / 3;
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dy = -radius; dy <= radius; dy++) {
                if (dx*dx + dy*dy <= radius*radius) {
                    SDL_RenderDrawPoint(renderer, 
                                      x * CELL_SIZE + CELL_SIZE / 2 + dx, 
                                      y * CELL_SIZE + CELL_SIZE / 3 + dy);
                }
            }
        }
    } else {
        // Forest - multiple trees
        // First tree
        trunkRect.x = x * CELL_SIZE + CELL_SIZE / 3;
        trunkRect.y = y * CELL_SIZE + CELL_SIZE / 2;
        trunkRect.w = CELL_SIZE / 8;
        trunkRect.h = CELL_SIZE / 3;
        
        SDL_SetRenderDrawColor(renderer, 100, 70, 30, 255);
        SDL_RenderFillRect(renderer, &trunkRect);
        
        SDL_SetRenderDrawColor(renderer, 30, 130, 30, 255);
        SDL_Rect leaf1Rect;
        leaf1Rect.x = x * CELL_SIZE + CELL_SIZE / 4;
        leaf1Rect.y = y * CELL_SIZE + CELL_SIZE / 4;
        leaf1Rect.w = CELL_SIZE / 4;
        leaf1Rect.h = CELL_SIZE / 4;
        SDL_RenderFillRect(renderer, &leaf1Rect);
        
        // Second tree
        trunkRect.x = x * CELL_SIZE + CELL_SIZE * 2 / 3;
        trunkRect.y = y * CELL_SIZE + CELL_SIZE * 2 / 3;
        trunkRect.w = CELL_SIZE / 8;
        trunkRect.h = CELL_SIZE / 4;
        
        SDL_SetRenderDrawColor(renderer, 110, 75, 35, 255);
        SDL_RenderFillRect(renderer, &trunkRect);
        
        SDL_SetRenderDrawColor(renderer, 35, 140, 35, 255);
        SDL_Rect leaf2Rect;
        leaf2Rect.x = x * CELL_SIZE + CELL_SIZE * 3 / 5;
        leaf2Rect.y = y * CELL_SIZE + CELL_SIZE / 2;
        leaf2Rect.w = CELL_SIZE / 4;
        leaf2Rect.h = CELL_SIZE / 4;
        SDL_RenderFillRect(renderer, &leaf2Rect);
    }
}

// Perform one simulation step
void simulationStep() {
    // Update water animation
    updateWaterAnimation();
    
    // Move cars
    updateCars();
    
    // Only grow the city every few steps to slow down development
    if (currentStep % 3 == 0) {
        growCity();
    }
    
    currentStep++;
}

// Draw the grid to the screen
void drawGrid(SDL_Renderer* renderer) {
    // Draw base terrain
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            SDL_Rect cellRect;
            cellRect.x = x * CELL_SIZE;
            cellRect.y = y * CELL_SIZE;
            cellRect.w = CELL_SIZE;
            cellRect.h = CELL_SIZE;
            
            // Draw base color for empty cells
            if (grid[x][y] == EMPTY) {
                SDL_SetRenderDrawColor(renderer, COLOR_EMPTY.r, COLOR_EMPTY.g, COLOR_EMPTY.b, 255);
                SDL_RenderFillRect(renderer, &cellRect);
            }
        }
    }
    
    // Draw water
    for (const auto& [wx, wy] : waterCells) {
        drawWater(renderer, wx, wy);
    }
    
    // Draw buildings
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            if (grid[x][y] != EMPTY && grid[x][y] != ROAD && grid[x][y] != WATER) {
                drawBuilding(renderer, x, y, buildings[x][y]);
            }
        }
    }
    
    // Draw roads
    for (const auto& [rx, ry] : roads) {
        drawRoad(renderer, rx, ry);
    }
    
    // Draw cars
    drawCars(renderer);
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow("City Simulation", 
                                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                         SCREEN_WIDTH, SCREEN_HEIGHT, 
                                         SDL_WINDOW_SHOWN);
    if (window == NULL) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Load font
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (font == NULL) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        // Try a fallback font
        font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 16);
        if (font == NULL) {
            std::cerr << "Failed to load fallback font! TTF_Error: " << TTF_GetError() << std::endl;
        }
    }
    
    // Initialize simulation
    initializeGrid();
    
    // Main loop flag
    bool quit = false;
    
    // Event handler
    SDL_Event e;
    
    // Main loop
    Uint32 lastTime = SDL_GetTicks();
    
    while (!quit && currentStep < MAX_SIMULATION_STEPS) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
            }
        }
        
        // Perform simulation step at specific intervals
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTime >= SIMULATION_DELAY) {
            simulationStep();
            lastTime = currentTime;
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw city grid
        drawGrid(renderer);
        
        // Update screen
        SDL_RenderPresent(renderer);
        
        // Cap to ~60 FPS for smooth animation
        SDL_Delay(16);
    }
    
    // Clean up
    if (font != NULL) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
