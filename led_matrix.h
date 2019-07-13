#include <Arduino.h>

byte led_matrix[13][3] = {
  {  0,    0,  0  }, // OFF
  {255,    0,  0  }, // Red
  {255,   40,  0  }, // Orange
  {180,  100,  0  }, // Yellow
  {255,  255,  0  }, // Chartreuse
  {  0,  255,  0  }, // Green
  {  0,  255,  150}, // Aquamarine
  {  0,  255,  255}, // Cyan
  {  0,  150,  255}, // Azure
  {  0,    0,  255}, // Blue
  {150,    0,  255}, // Violet
  {255,    0,  255}, // Magenta
  {255,    0,  150}  // Rose
};

byte led_matrix_size = (sizeof(led_matrix) / sizeof(led_matrix[0])) - 1;