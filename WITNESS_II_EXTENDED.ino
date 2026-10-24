#define CONFIG_HEAP_POISONING_LIGHT 1
#define CONFIG_HEAP_TRACING_STANDALONE 1


// Original library and code base - https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <vector>
#include <cstdint>
using namespace std;

#include <esp_random.h>
#include <EEPROM.h>

#include "matrices.h"
#include "panels.h"

#include <WITNESSCLONEII.h>

#include "WITNESS_UTILS.h"
#include "WITNESS_TESTS.h"

#include <chrono>

vector<vector<vector<bool>>> animation{grid0, grid1, grid2, grid3, grid4, grid5, grid6, grid7, grid8};

// EEPROM

#define EEPROM_SIZE 1
uint8_t flashbyte = 0;

// The number of MODES is one more than the number of slides in total -- you have one default face slide and (MODES) custom slides which you flip through by rebooting.
constexpr int MODES = 7;

void eeprom_setup() {
  EEPROM.begin(EEPROM_SIZE);
  flashbyte = EEPROM.read(0);
  EEPROM.write(0, (flashbyte + 1) % (1 + MODES));
  EEPROM.commit();
}

// SETUP

SET_LOOP_TASK_STACK_SIZE(48*1024);

// COMMON VARIABLES

uint32_t timestep = 0;
uint32_t period = (1<<8);

// BEGIN VORONOI VARIABLES

#include <voronoi.h>

vector<vorPoint> points;
vector<vorPoint> points_original;
vector<pair<double, double>> amplitude;
vector<pair<double, double>> freq;
vector<pair<double, double>> phase;


uint16_t TRIANGLE = matrixpanel->color565(0, 0, 0);
uint16_t POINT = matrixpanel->color565(255, 255, 255);
uint16_t VORONOI = matrixpanel->color565(0, 0, 0);
uint16_t CIRC = matrixpanel->color565(0, 0, 0);

void initVorPoints() {
  int N = 10;
  int M = 4;
  int AMP = 5;
  int F = 16;

  int si = -1;
  int ki = -1;

  for (int i = si; i <= N - ki; i++) {
    for (int j = si; j <= M - ki; j++) {
      points_original.push_back({128 * i / N + ((j % 2) * 0.5 * 128/N), 32 * j / M});
      amplitude.push_back({runif(-AMP, AMP), int(runif(-F, F))});
      freq.push_back({runif(-AMP, AMP), int(runif(-F, F))});
      phase.push_back({rng(), rng()});
    }
  }
  points = vector<vorPoint>(points_original);
}

// END VORONOI VARIABLES

// BEGIN DOOMFIRE VARIABLES

#include <DoomFire.h>

DoomFire doomFire(64, 32, 0);
void initDoomFire() {
  doomFire.initPRNG(rand());
  doomFire.SRC = 16;
	doomFire.RIGHT_MARGIN = 0;
	doomFire.defaultSources();

	doomFire.clear();
}

// END DOOMFIRE VARIABLES

// BEGIN CONWAY/CONWAYFACE INFO

#include <Conway.h>

constexpr int CONWAY_RAD = 1024;
Conway conway(CONWAY_RAD);

void initConway() {
  ConwayPresets::gosperGun(conway);
  conway.flipH();
  conway.translate({4, 10});
}

void initConwayFace() {
  auto leftHalf = vectorRect<bool>(grid0, 0, 0, 32, 64);
  conway = ConwayUtils::loadVec(leftHalf);
  conway.setRadius(CONWAY_RAD);
  conway.flipDiagonal();
}

// END CONWAY INFO

// BEGIN BOIDS INFO

#include <Boids.h>
Boids::Boids boids;

// END BOIDS INFO

void setup() {
  eeprom_setup();
  matrixsetup();
  disp(grid0);

  // PROJECT SETUP
  if (flashbyte == 2) {
    initVorPoints();
  } else if (flashbyte == 3) {
    initDoomFire();
  } else if (flashbyte == 4) {
    initConway();
  } else if (flashbyte == 5) {
    initConwayFace();
    delay(2000);
  } else if (flashbyte == 6) {
    for (int i = 0; i < 64; i++) boids.add({Boids::rand01() * 2 - 1, Boids::rand01() - 0.5});
    boids.TR = {1, 0.5};
    boids.BL = {-1, -0.5};
  }
}

int blinkdelay = 0;
int blinkthreshold = 0;

// ANIMATION

void anim_loop(uint16_t color) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m * chain; j++) col[i][j] = color;
  }
  delay(100);
  // Serial.println(rand() % 1024);
  disp(grid0);

  blinkdelay++;
  if (blinkdelay >= blinkthreshold) {
    blinkdelay = 0;
    blinkthreshold = (int)(runif(16, 64));
    playAnimation(animation, 20, false);
    playAnimation(animation, 20, true);
    dispstats();
    Serial.printf("BLINK THRESHOLD %ld\n", blinkthreshold);
  }
}

// LOOP

std::queue<int64_t> prior_timings;
int64_t run_sum = 0;

int WINDOW = 64;

// BEGIN MANDELBROT INFO

#include <mandelbrot.h>

std::pair<double, double> p = {-0.74453986035590838012, 0.12172377389442482241}; 
double R0 = 4;
std::vector<std::vector<int>> plot;
std::vector<std::vector<int>> values;
double max_iter = 1024;
int BANDS = 16;

void runMandelbrot() {
    period = 64;
    timestep = (timestep + 1) % (period);
    double sine = -1 * cos((M_PI * 2 * timestep) / period) + 1;
    double R = R0 / pow(2.0, sine * 10);
    double inc = R / 32.0;

    bool eic = R >= 1.0 / (1<<6); // for larger values of R where more of the large parts of the set are in view, do a check on the massive quantity of interior points to early exit.
    plot = plotCentered(p.first, p.second, R, R/2, inc, max_iter, eic);
    values = histo_values(plot, max_iter, BANDS);

    for (int x = 0; x < m && x < values.size(); x++) {
      for (int y = 0; y < n && y < values[x].size(); y++) {
        uint32_t c = 0;
        if (values[x][y] >= 0) c = 255 * ((frac)(values[x][y]) / BANDS);
        uint32_t rgb = matrixpanel->color565(c, c, c);
        matrixpanel->drawPixel(x, n - y - 1, rgb);
        matrixpanel->drawPixel(x + m, n - y - 1, rgb);
      }
    }
    cout << R << " : " << values.size() << " " << values[0].size() << endl;
}

// END MANDELBROT INFO

// BEGIN VORONOI INFO

#include <iostream>

// rounding function
int roundFunc(double d) {
  return iround(d);
}

void runVoronoi() {
  auto start = std::chrono::high_resolution_clock::now();

  timestep = (timestep + 1) % period;
  double t = (2 * M_PI * timestep) / period;

  VORONOI = fullhue((256 * 6 * timestep) / period);

  for (int i = 0; i < points.size(); i++) {
    points[i].first = points_original[i].first + amplitude[i].first * sin(freq[i].second * t + phase[i].first);
    points[i].second = points_original[i].second + amplitude[i].second * sin(freq[i].second * t + phase[i].second);
  }
  
  
  // compute
  auto dt = triangulate(points);
  auto tri = dt.first;
  auto edges = dt.second;

  vector<vorPoint> circ;
	for (auto i : tri) {
 		auto cc = circumcenter(points[i[0]], points[i[1]], points[i[2]]);
		circ.push_back(cc);
	}

  auto voronoiDiagram = voronoi(tri);
  auto vor = voronoiDiagram.first;

  // begin

  matrixpanel->fillScreenRGB888(0, 0, 0);

  // draw triangles
  if (TRIANGLE) {
    for (auto e : edges) {
      vorPoint a = points[e.first];
      vorPoint b = points[e.second];
      matrixpanel->drawLine(roundFunc(a.first), roundFunc(a.second), roundFunc(b.first), roundFunc(b.second), TRIANGLE);
    }
  }
  if (VORONOI) {
    for (auto vv : vor) {
      vorPoint a = circ[vv.first];
      vorPoint b = circ[vv.second];
      if (a == INF || b == INF) continue;
      matrixpanel->drawLine(roundFunc(a.first), roundFunc(a.second), roundFunc(b.first), roundFunc(b.second), VORONOI);
    }
  }

  // draw points
  for (auto p : points) {
    matrixpanel->drawPixel(roundFunc(p.first), roundFunc(p.second), POINT);
  }
  // draw circumcenters
  if (CIRC) {
    for (auto c : circ) matrixpanel->drawPixel(roundFunc(c.first), roundFunc(c.second), CIRC);
  }
  if (rand() % 16 == 0) dispstats();


  // BEGIN TIME PRINTOUT
	auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  int64_t count = duration.count();
	std::cout << "Time taken: " << count * 0.001 << " ms" << std::endl;
  // END TIME PRINTOUT
}

// END VORONOI INFO

// BEGIN DOOMFIRE INFO

void runDoomFire() {
  period = 64;
  timestep = (timestep + 1) % period;
  uint16_t color = fullhue((256 * 6 * timestep) / period);
  const double denom = 1.0 / doomFire.SRC;

  doomFire.update();

  matrixpanel->fillScreenRGB888(0, 0, 0);

  for (int x = 0; x < 64; x++) {
    for (int y = 0; y < 32; y++) {
      int particle = doomFire.particles[x][31 - y];
      if (!particle) continue;
      double brightness = particle * denom;
      uint16_t pxColor = scale565(color, brightness);
      matrixpanel->drawPixel(x, y, pxColor);
      matrixpanel->drawPixel(x + 64, y, pxColor);
    }
  }
}

// END DOOMFIRE INFO

// BEGIN CONWAY INFO

void runConway(int dd = 20, bool flip = false, bool rainbow = true) {
  if (dd < 0) dd = 0;

  period = 64;
  timestep = (timestep + 1) % period;
  uint16_t color = matrixpanel->color565(255, 255, 255);
  if (rainbow) color = fullhue((256 * 6 * timestep) / period);
  conway.update();

  matrixpanel->fillScreenRGB888(0, 0, 0);

  for (int x = 0; x < 64; x++) {
    for (int y = 0; y < 32; y++) {
      if (conway.contains({x, y})) {
        matrixpanel->drawPixel(x, y, color);
        matrixpanel->drawPixel(flip ? 127 - x : x + 64, y, color);
      }
    }
  }
  delay(dd);

  if (timestep == 0) std::cout << "CONWAY: " << conway.points.size() << " | " << conway.printBB() << "\n";
}

// END CONWAY INFO

// BEGIN BOIDS INFO

void runBoids(bool rainbow = false, bool velocity = true) {
  matrixpanel->fillScreenRGB888(0, 0, 0);
  period = 64;
  timestep = (timestep + 1) % period;
  uint16_t color = matrixpanel->color565(255, 255, 255);
  if (rainbow) color = fullhue((256 * 6 * timestep) / period);
  boids.update();

  for (auto b : boids.boids) {
    auto x = b.pos;
    auto v = b.vel;
    int xPos = (x.x + 1) * 32;
    int yPos = (x.y + 0.5) * 32;

    if (velocity) {
      auto pol = v.rect2polar();
      int theIndex = pol.y * (256 * 6);
      if (theIndex < 0) theIndex += 256 * 6;
      uint16_t unitcolor = fullhue(theIndex);
      color = unitcolor;
    }

    for (int offset = 0; offset <= 64; offset += 64) matrixpanel->drawPixel(xPos + offset, yPos, color);
  }

  delay(50);
}

// END BOIDS INFO

void loop() {
  bool printOut = false;
  if (flashbyte == 0) {
    anim_loop(matrixpanel->color565(255, 255, 255));
    return;
  }

  auto start = std::chrono::high_resolution_clock::now();

  if (flashbyte == 1) {
    runMandelbrot();
  } else if (flashbyte == 2) {
    runVoronoi();
  } else if (flashbyte == 3) {
    runDoomFire();
  } else if (flashbyte == 4) {
    runConway();
  } else if (flashbyte == 5) {
    runConway(50, true, false);
  } else if (flashbyte == 6) {
    runBoids();
  } else {
    randTests();
    printOut = true;
  }

  if (!printOut) return;

  

  // Begin loop

  // End loop

  std::cout << "END LOOP\n";
	auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	int64_t count = duration.count();

  prior_timings.push(count);
  run_sum += count;
  while (prior_timings.size() > WINDOW) {
    run_sum -= prior_timings.front();
    prior_timings.pop();
  }

  std::cout << "Time taken: " << count << " us" << std::endl;
	std::cout << "Time taken: " << count * 0.001 << " ms" << std::endl;
	std::cout << "Time taken: " << count * 0.000001 << " s" << std::endl;

  double avg = (double)(run_sum) / std::max(1, (int)(prior_timings.size()));
  std::cout << "Average: " << avg << " us" << std::endl;
	std::cout << "Average: " << avg * 0.001 << " ms" << std::endl;
	std::cout << "Average: " << avg * 0.000001 << " s" << std::endl;
}
