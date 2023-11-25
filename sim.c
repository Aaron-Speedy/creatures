#define TB_IMPL
#include "termbox2.h"

#include <time.h>
#include <assert.h>

typedef enum {
  NOT_FOOD = 0,
  FOOD_A,
  FOOD_B,
  NUM_FOOD_TYPES,
} Food;
char food_display[NUM_FOOD_TYPES] = { '.', 'O', '#' };

#define NUM_NEURONS 10
#define MOVEMENT_NEURON 0
#define ROTATION_NEURON 1
#define ROTATION_DIR_NEURON 2
#define NEURON_IS_ON(brain, i) (brain->values[i] >= brain->thresholds[i])
typedef struct {
  float values[NUM_NEURONS];
  float periods[NUM_NEURONS];
  float thresholds[NUM_NEURONS];
  float synapses[NUM_NEURONS][NUM_NEURONS];
} Brain;

typedef struct {
  int hunger;
  Food need;
  enum {
    NORTH,
    EAST,
    SOUTH,
    WEST,
    NUM_DIRS,
  } dir;
  Brain brain;
} Agent;
char direction_display[NUM_DIRS] = {'^', '>', 'V', '<'};

typedef struct {
  union {
    Food food;
    Agent *agent;
  };
  enum {
    TYPE_FOOD = 0,
    TYPE_AGENT,
    NUM_SPOT_TYPES,
  } type;
} Spot;

int main(void) {
  srand(time(NULL));

  #define MAX_AGENTS 40
  Agent agent_arena[MAX_AGENTS];
  size_t dead_list[MAX_AGENTS] = { 0 };
  size_t dead_list_index = 0;

  #define GRID_WIDTH 80
  #define GRID_HEIGHT 40
  Spot grid_1[GRID_WIDTH][GRID_HEIGHT] = { 0 };
  Spot grid_2[GRID_WIDTH][GRID_HEIGHT] = { 0 };
  Spot (*grid)[GRID_HEIGHT] = grid_1;
  Spot (*new_grid)[GRID_HEIGHT] = grid_2;
  Spot (*temp_grid)[GRID_HEIGHT];

  for (int i = 0; i < 100; i++) {
    size_t x = rand() % GRID_WIDTH;
    size_t y = rand() % GRID_HEIGHT;

    grid[x][y].type = TYPE_FOOD;
    grid[x][y].food = rand() % NUM_FOOD_TYPES;
  }

  for (int i = 0; i < MAX_AGENTS - 1; i++) {
    size_t x = rand() % GRID_WIDTH;
    size_t y = rand() % GRID_HEIGHT;

    Agent *new_agent = &agent_arena[dead_list[dead_list_index]];
    new_agent->hunger = 100;
    new_agent->need = rand() % NUM_FOOD_TYPES;
    new_agent->dir = rand() % NUM_DIRS;

    for (int i = 0; i < NUM_NEURONS; i++) {
      new_agent->brain.values[i] = (float) rand() / (float) RAND_MAX;
      new_agent->brain.periods[i] = 1 + rand() % 20;
      new_agent->brain.thresholds[i] = 0.5;
      for (int j = 0; j < NUM_NEURONS; j++) {
        new_agent->brain.synapses[i][j] = 2 * ((float) rand() /
                                          (float) RAND_MAX) - 1;
      }
    }

    grid[x][y].type = TYPE_AGENT;
    grid[x][y].agent = new_agent;

    if (dead_list_index == 0) {
      dead_list[0] += 1;
    } else {
      dead_list_index -= 1; 
    }
  }

  tb_init();

  while (1) {
    for (int x = 0; x < GRID_WIDTH; x++) {
      for (int y = 0; y < GRID_HEIGHT; y++) {
        if (grid[x][y].type == TYPE_FOOD) {
          tb_set_cell(
            x, y,
            food_display[grid[x][y].food],
            0, 0
          );

          if (new_grid[x][y].type == TYPE_FOOD) { // Maybe in the future change to using a modified flag
            new_grid[x][y].food = grid[x][y].food;
          }
        } 

        else {
          Agent *agent = grid[x][y].agent;

          tb_set_cell(
            x, y,
            direction_display[agent->dir],
            0, TB_RED 
          );

          // agent->hunger -= 1;
          // if (agent->hunger < 0) {
          //   dead_list_index += 1;
          //   dead_list[dead_list_index] = (agent - agent_arena) /
          //                                sizeof(Agent);
          //   new_grid[x][y].type = TYPE_FOOD;
          //   new_grid[x][y].food = NOT_FOOD;
          //   continue;
          // }

          Brain *brain = &agent->brain;
          for (int i = 0; i < NUM_NEURONS; i++) {
            tb_printf(GRID_WIDTH + 2, i, 0, 0, "%d", NEURON_IS_ON(brain, i));
            if (NEURON_IS_ON(brain, i)) {
              for (int j = 0; j < NUM_NEURONS; j++) {
                brain->values[j] += brain->values[i] * brain->synapses[i][j];
                if (brain->values[j] > 1) {
                  brain->values[j] = 1;
                }
                if (brain->values[j] < -1) {
                  brain->values[j] = 1;
                }

                if (NEURON_IS_ON(brain, j)) {
                  brain->synapses[i][j] += 0.05; // TODO: Put as constant
                } else {
                  brain->synapses[i][j] -= 0.05;
                }
                if (brain->synapses[i][j] > 1) {
                  brain->synapses[i][j] = 1;
                }
                if (brain->synapses[i][j] < -1) {
                  brain->synapses[i][j] = -1;
                }
              }
            }
          }

          if (NEURON_IS_ON(brain, ROTATION_NEURON)) {
            agent->dir = (agent->dir + 2 * NEURON_IS_ON(brain, ROTATION_DIR_NEURON) - 1) % NUM_DIRS;
          }

          int new_x = x;
          int new_y = y;
          switch (agent->dir) {
            case NORTH:
              new_y -= 1;
              break;
            case EAST:
              new_x += 1;
              break;
            case SOUTH:
              new_y += 1;
              break;
            case WEST:
              new_x -= 1;
              break;
            case NUM_DIRS:
              assert(!"NOT_DIRS is not a valid direction");
          }

          if (!NEURON_IS_ON(brain, MOVEMENT_NEURON) ||
              new_x < 0 || new_x >= GRID_WIDTH ||
              new_y < 0 || new_y >= GRID_HEIGHT ||
              grid[new_x][new_y].type == TYPE_AGENT ||
              new_grid[new_x][new_y].type == TYPE_AGENT) {
            new_x = x;
            new_y = y;
          }

          new_grid[x][y].type = TYPE_FOOD;
          new_grid[x][y].food = NOT_FOOD;
          new_grid[new_x][new_y].type = TYPE_AGENT;
          new_grid[new_x][new_y].agent = agent;
        }
      }
    }

    temp_grid = grid;
    grid = new_grid;
    new_grid = temp_grid;
    for (int x = 0; x < GRID_WIDTH; x++) {
      for (int y = 0; y < GRID_HEIGHT; y++) {
        new_grid[x][y].type = TYPE_FOOD;
        new_grid[x][y].food = NOT_FOOD;
      }
    }

    tb_present();
    usleep(1000000/60);
  }

  tb_shutdown();
  return 0;
}
