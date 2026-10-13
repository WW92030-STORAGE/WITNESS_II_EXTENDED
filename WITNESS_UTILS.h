#ifndef WITNESS_UTILS
#define WITNESS_UTILS


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

// Forward declaration
void render(Grid& g, EntityColor::Color bg, EntityColor::Color line, EntityColor::Color path);



const int MODE = 2; // 0 FULL 1 TEST 2 CHALLENGE

uint64_t scale565(int x, double y) {
  double r = (x>>16) % (1<<8);
  double g = (x>>8) % (1<<8);
  double b = (x % (1<<8));

  r *= y;
  g *= y;
  b *= y;
  return matrixpanel->color565((int)r, (int)g, (int)b);
}

uint64_t inv565(int x) {
  double r = 255 - (x>>16) % (1<<8);
  double g = 255 - (x>>8) % (1<<8);
  double b = 255 - (x % (1<<8));
  return matrixpanel->color565((int)r, (int)g, (int)b);
}

// NOTE TO SELF - drawRect is (top, left, width, height) while drawLine is endpoints.

double fade_scale = 1.0 / 3.0;

void render(Grid& g, EntityColor::Color bg, EntityColor::Color line, EntityColor::Color path) {
  // CONSTANTS
  auto bg_h = hex565(bg);
  auto line_h = hex565(line);
  auto path_h = hex565(path);
  float FADE_SCALE = 0.25;

  int rad = 3;



  int height = (g.R)>>1;
  int width = (g.C)>>1;  

  cout << width << " " << height << endl;

  const int HORIZ_OFFSET = 20;

  int top = 15 - rad * height;
  int left = HORIZ_OFFSET - rad * width;

  matrixpanel->fillScreenRGB888((bg>>16) % (1<<16), (bg>>8) % (1<<8), bg % (1<<8));

for (int side = 0; side < 2; side++) {

  if (side) left = m * chain - HORIZ_OFFSET - rad * width;

  
  
  if (height > 5 || width > 5) return;
  // Draw lines
  for (int yayayaya = 0; yayayaya < 2; yayayaya++) {
    // cout << top << " " << left << endl;
    // matrixpanel->fillRect(left, top, (rad<<1) * width + 2, (rad<<1) * height + 2, hex565(line));
    for (int i = 0; i <= height; i++) {
      matrixpanel->drawLine(left, top + (rad<<1) * i, left + (rad<<1) * width, top + (rad<<1) * i, line_h);
    }

    for (int i = 0; i <= width; i++) {
      matrixpanel->drawLine(left + (rad<<1) * i, top, left + (rad<<1) * i, top + (rad<<1) * height, line_h);
    }
  }

  // Draw gaps

  for (int r = 0; r < g.R; r++) {
    for (int c = 0; c < g.C; c++) {
      int rPos = left + rad * r;
      int cPos = top + rad * (g.C - c - 1);

      PuzzleEntity* pp = g.board[r][c];

      int rectRad = rad>>1;
      int rectWidth = (rectRad<<1) + 1;
      if (!pp->isPath) {
        matrixpanel->fillRect(rPos - rectRad, cPos - rectRad, rectWidth, rectWidth, bg_h);
      }
    }
  }

  // Draw paths
  for (int r = 0; r < g.R; r++) {
    for (int c = 0; c < g.C; c++) {
      int rPos = left + rad * r;
      int cPos = top + rad * (g.C - c - 1);

      PuzzleEntity* pp = g.board[r][c];

      if (!pp->isPath) continue;
      if (pp->hasLine) {
        if (r & 1 && !(c & 1)) {
          matrixpanel->drawLine(rPos - rad, cPos, rPos + rad, cPos, path_h);
        }
        else if (c & 1 && !(r & 1)) {
          matrixpanel->drawLine(rPos, cPos - rad, rPos, cPos + rad, path_h);
        }
        else {
          matrixpanel->drawPixel(rPos, cPos, path_h);
        }
      }
    }
  }

  // Draw starts and ends
  for (int r = 0; r < g.R; r++) {
    for (int c = 0; c < g.C; c++) {
      int rPos = left + rad * r;
      int cPos = top + rad * (g.C - c - 1);

      PuzzleEntity* pp = g.board[r][c];

      auto toColor = hex565((pp->hasLine) ? path : line);
      
      Endpoint* ep = dynamic_cast<Endpoint*>(pp);
      if (ep) {
        if (ep->isStart) matrixpanel->fillRect(rPos - 1, cPos - 1, 3, 3, toColor);
        else {
          if (r == 0) {
            if (c == g.C - 1) matrixpanel->drawLine(rPos - 2, cPos - 2, rPos, cPos, toColor);
            else if (c == 0) matrixpanel->drawLine(rPos - 2, cPos + 2, rPos, cPos, toColor);
            else matrixpanel->drawLine(rPos - 2, cPos, rPos, cPos, toColor);
          }
          else if (r == g.R - 1) {
            if (c == g.C - 1) matrixpanel->drawLine(rPos + 2, cPos - 2, rPos, cPos, toColor);
            else if (c == 0) matrixpanel->drawLine(rPos + 2, cPos + 2, rPos, cPos, toColor);
            else matrixpanel->drawLine(rPos + 2, cPos, rPos, cPos, toColor);
          }
          else if (c == g.C - 1) matrixpanel->drawLine(rPos, cPos - 2, rPos, cPos, toColor);
          else if (c == 0) matrixpanel->drawLine(rPos, cPos + 2, rPos, cPos, toColor);
          
          else matrixpanel->drawRect(rPos - 2, cPos - 2, 5, 5, toColor);
        }
      }
    }
  }


  // Draw internal symbols
  for (int r = 0; r < g.R; r++) {
    for (int c = 0; c < g.C; c++) {
      int rPos = left + rad * r;
      int cPos = top + rad * (g.C - c - 1);

      int symbolRad = (rad>>1);
      int symbolWidth = (symbolRad<<1) + 1;

      PuzzleEntity* pp = g.board[r][c];

      auto rawColor = pp->color;
      auto toColor = hex565(pp->color);

      // Dots

      if (auto bg = instanceof<PathDot>(pp)) {
        matrixpanel->drawPixel(rPos, cPos, path_h);
      }

      // Blobs

      else if (auto bg = instanceof<Blob>(pp)) {
        matrixpanel->fillRect(rPos - symbolRad, cPos - symbolRad, symbolWidth, symbolWidth, toColor);
      }

      else if (auto st = instanceof<Star>(pp)) {
        const int dx[4] = {1, 0, -1, 0};
        const int dy[4] = {0, 1, 0, -1};
        matrixpanel->drawPixel(rPos, cPos, toColor);
        for (int i = 0; i < 4; i++) matrixpanel->drawPixel(rPos + dx[i], cPos + dy[i], toColor);
      }

      else if (auto trix = instanceof<Triangle>(pp)) {
        const int dx[4] = {1, 1, -1, -1};
        const int dy[4] = {1, -1, 1, -1};
        for (int i = 0; i < 4; i++) matrixpanel->drawPixel(rPos + dx[i] * (rad - 1), cPos + dy[i] * (rad - 1), scale565(rawColor, FADE_SCALE));
        int cnt = trix->count;

        if (cnt == 1) matrixpanel->drawPixel(rPos, cPos, toColor);
        else if (cnt == 2) {
          matrixpanel->drawPixel(rPos - 1, cPos, toColor);
          matrixpanel->drawPixel(rPos + 1, cPos, toColor);
        }
        else if (cnt == 3) {
          matrixpanel->drawPixel(rPos - 1, cPos + 1, toColor);
          matrixpanel->drawPixel(rPos + 1, cPos + 1, toColor);
          matrixpanel->drawPixel(rPos, cPos - 1, toColor);
        }
      }
      
      else if (auto bg = instanceof<BlockGroup>(pp)) {
        
        matrixpanel->drawRect(rPos - (rad - 1), cPos - (rad - 1), (rad<<1) - 1, (rad<<1) - 1, scale565(rawColor, FADE_SCALE));

        if (!bg->fixed) {
          matrixpanel->drawPixel(rPos - (rad - 1), cPos - (rad - 1), bg_h);
          matrixpanel->drawPixel(rPos + (rad - 1), cPos - (rad - 1), bg_h);
          matrixpanel->drawPixel(rPos - (rad - 1), cPos + (rad - 1), bg_h);
          matrixpanel->drawPixel(rPos + (rad - 1), cPos + (rad - 1), bg_h);
        }

        if (bg->value < 0) {
          matrixpanel->drawPixel(rPos - (rad - 1), cPos, bg_h);
          matrixpanel->drawPixel(rPos, cPos - (rad - 1), bg_h);
          matrixpanel->drawPixel(rPos - (rad - 1), cPos, bg_h);
          matrixpanel->drawPixel(rPos, cPos + (rad - 1), bg_h);
        }

        Utils::point BL = bg->boundingbox.first;
        Utils::point TR = bg->boundingbox.second;

        Utils::point center = {(BL.first + TR.first) / 2, (BL.second + TR.second) / 2};

        for (auto p : bg->points) {
          Utils::point normalized = {p.first - center.first, p.second - center.second};
          matrixpanel->drawPixel(rPos + normalized.first, cPos - normalized.second, toColor);
        }
      }
    }
  }

}


}

#endif