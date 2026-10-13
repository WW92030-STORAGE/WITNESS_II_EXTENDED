#ifndef WITNESS_TESTS
#define WITNESS_TESTS

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <vector>
#include <cstdint>
using namespace std;

#include <esp_random.h>
#include <EEPROM.h>

#include "matrices.h"
#include "panels.h"

#include <WITNESSCLONEII.h>
#include <chrono>

#include "presets.h"

const int NUM_PATHS = 16;

// For testing
void unitTest() {
  srand(1);

  int index = int(rng() * 7);

  Grid grid;
  grid.defaultDiagonal();
  grid.set(2, 0, new Endpoint(true));
  grid.set(0, 1, new Endpoint(true));

  grid.drawLine(0, 0, 2, 0);
  grid.drawLine(2, 0, 2, 8);
  grid.drawLine(2, 8, 8, 8);

  // puzzleSolver.grid = &grid;
  render(grid, EntityColor::RGB_BLACK, EntityColor::RGB_DARK, EntityColor::RGB_GREEN);
  delay(1000);

  std::cout << "END TEST\n";
  
  return;
}

// For testing
void randTest() {
  RandGrid<RotationalGrid> gridgen(9, 9);
  Solver puzzleSolver;

  dispstats();
  std::cout << "BEGIN PATHFIND\n";

  gridgen.pathfind(NUM_PATHS);

  std::cout << "END PATHFIND\n";
  dispstats();

  int index = int(rng() * 7);

  RotationalGrid grid = gridgen.randBlocks();
  dispstats();
  
  std::cout << "GRID CHOSEN\n";
  std::cout << grid.to_string() << "\n";

  render(grid, EntityColor::RGB_BLACK, EntityColor::RGB_DARK, EntityColor::RGB_GREEN);
  delay(1000);

  puzzleSolver.grid = &grid;
  puzzleSolver.solve(1, true, false);
  if (puzzleSolver.solutions.size() <= 0) {
    matrixpanel->fillScreen(hex565(0xFF0000));
    delay(1000);
    return;
  }
  puzzleSolver.apply(0);

  std::cout << "SOLVED!\n";

  render(grid, EntityColor::RGB_BLACK, EntityColor::RGB_DARK, EntityColor::RGB_GREEN);
  delay(1000);
}

// Full animation (not actually a test)
void randTests() {
  RandGrid gridgen;
  RandGrid<RotationalGrid> gridgen2;
  Solver puzzleSolver;

  dispstats();
  std::cout << "BEGIN PATHFIND\n";

  gridgen.pathfind(NUM_PATHS);
  gridgen2.pathfind(NUM_PATHS);

  std::cout << "END PATHFIND\n";
  dispstats();

  int index = int(rng() * 12);

  Grid* grid = 0;
  std::cout << "INDEX " << index << "\n";
  dispstats();
  if (index == 0) grid = new RotationalGrid(gridgen2.randBlobs(9, 3, 2));
  else if (index == 1) grid = new RotationalGrid(gridgen2.randMaze());
  else if (index == 2) grid = new Grid(gridgen2.randStars());
  else if (index == 3) grid = new Grid(gridgen.randBlobs(12, 2, 0));
  else if (index == 4) grid = new RotationalGrid(gridgen2.randTriangles(8, 0));
  else if (index == 5) grid = new RotationalGrid(gridgen2.randBlocks(4, int(rng() * 4)));
  else if (index == 6) grid = new RotationalGrid(gridgen2.randDots(4 + int(rng() * 4), int(rng() * 8) ));
  else if (index == 7) grid = new RotationalGrid(gridgen2.randStarsGeneral(4));
  else if (index == 8) grid = new Grid(gridgen.randTriangles(12, 2));
  else if (index == 9) grid = new Grid(gridgen.randBlobs(9, 3, 0));
  else if (index == 10) grid = new Grid(gridgen.randMaze());
  else if (index == 11) grid = new Grid(gridgen.randDots(4 + int(rng() * 4), int(rng() * 8) ));
  dispstats();
  std::cout << "GRID CHOSEN\n";

  puzzleSolver.grid = grid;
  render(*grid, EntityColor::RGB_BLACK, EntityColor::RGB_DARK, EntityColor::RGB_GREEN);
  delay(1000);

  puzzleSolver.solve(1);
  if (puzzleSolver.solutions.size() == 0) {
    cout << "NO SOLUTION :(\n";
    matrixpanel->fillScreenRGB888(255, 0, 0);
  }
  else {
    puzzleSolver.apply(0);
    render(*grid, EntityColor::RGB_BLACK, EntityColor::RGB_DARK, EntityColor::RGB_GREEN);
  }

  std::cout << "TRAVERSED " << puzzleSolver.ITERS << " NODES\n";

  delete grid;
  delay(1000);
}


#endif