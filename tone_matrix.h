#include <Arduino.h>

byte tone_matrix[5][6] = {
  {   0,     0,     0,    0,     0,     0},
  { 500,   500,   600, 1100,  1000,  1200},
  {1250,   200,   300,  350,   500,   600},
  {1500,   600,   1700, 1700,   200,   1300},
  { 700,  1000,  1300,  700,  1000,  1300}
};

byte tone_matrix_size = (sizeof(tone_matrix) / sizeof(tone_matrix[0])) - 1;