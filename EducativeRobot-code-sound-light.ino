/**
Slave code block: Wait for
--------------------------
Micro: Attiny84
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <TinyWireS.h>

// The default buffer size
#ifndef TWI_RX_BUFFER_SIZE
  #define TWI_RX_BUFFER_SIZE ( 16 )
#endif

byte i2c_slave_address  = 0x08;
/*
Slave function modes

MODE_FUNCTION               1
MODE_MODIFIER_LOOP          2
MODE_SLAVE_FORWARD_ARROW    3
MODE_SLAVE_BACKWARD_ARROW   4
MODE_SLAVE_LEFT_ARROW       5
MODE_SLAVE_RIGHT_ARROW      6
MODE_SLAVE_WAIT_LIGHT       7
MODE_SLAVE_WAIT_SOUND       8
MODE_SLAVE_SOUND            9
MODE_SLAVE_LIGHT            10
*/
byte slave_function     = 9;

#define LED_PIN                 3
#define GATE_PIN                5
#define RESET_PIN               7
#define SHIFT_DATA_PIN          0
#define SHIFT_LATCH_PIN         1
#define SHIFT_CLOCK_PIN         2
#define ENCODER_SW_ACTION_PIN   7
#define ENCODER_SW_PIN          8
#define ENCODER_A_PIN           9
#define ENCODER_B_PIN           10