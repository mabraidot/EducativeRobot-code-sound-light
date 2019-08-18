/**
Slave code block: Wait for
--------------------------
Micro: Attiny84
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <TinyWireS.h>
#include "led_matrix.h"
#include "tone_matrix.h"
#include <avr/wdt.h>

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
MODE_WHILE_START            11
MODE_WHILE_END              12
MODE_END_OF_PROGRAM         99
*/

#define MODE_SLAVE_SOUND        9
#define MODE_SLAVE_LIGHT        10
byte slave_function             = MODE_SLAVE_LIGHT;

#define LED_PIN                 3
#define GATE_PIN                0
#define RESET_PIN               1
#define ENCODER_SW_PIN          2
#define ENCODER_A_PIN           9
#define ENCODER_B_PIN           10
#define RGBLED_PIN_RED          5
#define RGBLED_PIN_GREEN        8
#define RGBLED_PIN_BLUE         7
#define BUZZER_PIN              8 // Shared with green led

volatile uint8_t i2c_regs[] =
{
    0,              // 0: Set new I2C address
    0,              // 1: Activate to any child slave
    0,              // 2: Flash the LED
    0,              // 3: Activated block
    slave_function, // 4: Slave function
    0               // 5: Slave modifying value
};


volatile byte reg_position = 0;
const byte reg_size = sizeof(i2c_regs);

// Needed for software reset
void(* resetFunc) (void) = 0;

int encoder_count = 0;
byte encoder_max = (slave_function == MODE_SLAVE_LIGHT) ? led_matrix_size : tone_matrix_size;

// Encoder Switch Debouncing (Kenneth A. Kuhn algorithm)
#define DEBOUNCE_TIME               0.3
#define DEBOUNCE_SAMPLE_FREQUENCY   10
#define DEBOUNCE_MAXIMUM            (DEBOUNCE_TIME * DEBOUNCE_SAMPLE_FREQUENCY)
boolean buttonInput = true;        /* 0 or 1 depending on the input signal */
unsigned int buttonIntegrator;      /* Will range from 0 to the specified MAXIMUM */
boolean buttonOutput = true;       /* Cleaned-up version of the input signal */
byte displayNumber = 0;

void clear_eeprom(void){
  //EEPROM.write(0x00, 0);
  //EEPROM.write(0x01, 0);
  for(int i = 0; i < EEPROM.length(); i++){
    EEPROM.write(i, 0);
  }
}


/*
Rotate function to wear out the eeprom records evenly
*/
void write_eeprom(byte cell, byte value){
  /*
  values    |0|1|x|x|0|0|0|
  records   |0|1|2|3|4|5|6|
  */
  int addr = 0;
  for(int i=0; i < EEPROM.length(); i++){
    if(EEPROM.read(i) > 0){
      addr = i;
      break;
    }
  }
  if(addr+2 >= EEPROM.length()){  // End of EEPROM, move to the begining
    if(cell == 0){
      EEPROM.write(0x02, EEPROM.read(addr+2));
      EEPROM.write(0x01, value);
    }else{ // cell = 1
      EEPROM.write(0x01, EEPROM.read(addr+1));
      EEPROM.write(0x02, value);
    }
    EEPROM.write(0x00, 1);
    EEPROM.write(addr+2, 0);
    EEPROM.write(addr+1, 0);
  }else{  // Not end of EEPROM, so move to the next  set of addresses
    if(cell == 0){
      EEPROM.write(addr+3, EEPROM.read(addr+2));
      EEPROM.write(addr+2, value);
    }else{ // cell = 1
      EEPROM.write(addr+2, EEPROM.read(addr+1));
      EEPROM.write(addr+3, value);
    }
    EEPROM.write(addr+1, 1);
  }
  EEPROM.write(addr, 0);

}


byte read_eeprom(byte cell){
  /*
  values    |0|0|1|x|x|0|0|
  records   |0|1|2|3|4|5|6|
  */
  int addr = 0;
  for(int i=0; i < EEPROM.length(); i++){
    if(EEPROM.read(i) > 0){
      addr = i;
      break;
    }
  }
  return EEPROM.read(addr+1+cell);

}


// Slave address is stored at EEPROM address 0x00
byte getAddress() {
  //byte i2c_new_address = EEPROM.read(0x00);
  byte i2c_new_address = read_eeprom(0);
  if (i2c_new_address == 0x00) { 
    i2c_new_address = i2c_slave_address; 
  }else if(i2c_new_address != i2c_slave_address){
    // Write back the original placeholder address, in case of an unplug
    //EEPROM.write(0x00, i2c_slave_address);
    write_eeprom(0, i2c_slave_address);
    activate_child();
  }

  return i2c_new_address;
}

// Gets called when the ATtiny receives an i2c command
// First byte is the starting reg address, next is the value
void receiveEvent(uint8_t howMany)
{
  if (howMany < 1)
  {
      // Sanity-check
      reg_position = 0;
      return;
  }
  if (howMany > TWI_RX_BUFFER_SIZE)
  {
      // Also insane number
      reg_position = 0;
      return;
  }

  reg_position = TinyWireS.receive();
  if(reg_position == 255){
    clear_eeprom();
  }
  howMany--;
  if (!howMany)
  {
      // This write was only to set the buffer for next read
      reg_position = 0;
      return;
  }
  while(howMany--)
  {
      i2c_regs[reg_position] = TinyWireS.receive();
      // reset position
      reg_position = 0;
      /*reg_position++;
      if (reg_position >= reg_size)
      {
          reg_position = 0;
      }*/
  }
}


// Gets called when the ATtiny receives an i2c request
void requestEvent()
{
  if(i2c_regs[3]){
    TinyWireS.send(i2c_regs[reg_position]);
    // Increment the reg position on each read, and loop back to zero
    reg_position++;
    if (reg_position >= reg_size)
    {
      reg_position = 0;
    }
  }
}


void setup() {

  wdt_disable();
  if(slave_function == MODE_SLAVE_SOUND){
	  wdt_enable(WDTO_4S); // Watchdog 4 s
  }else{
    wdt_enable(WDTO_60MS); // Watchdog 60 ms
  }

  pinMode(RESET_PIN, INPUT);            // Soft RESET
  pinMode(LED_PIN, OUTPUT);             // Status LED
  pinMode(GATE_PIN, OUTPUT);            // Status GATE for child slave

  TinyWireS.begin(getAddress());        // Join i2c network
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
  
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
  pinMode(ENCODER_A_PIN, INPUT);
  pinMode(ENCODER_B_PIN, INPUT);

  pinMode(RGBLED_PIN_RED, OUTPUT);
  pinMode(RGBLED_PIN_GREEN, OUTPUT);
  pinMode(RGBLED_PIN_BLUE, OUTPUT);

  encoder_count = displayNumber = i2c_regs[5] = read_eeprom(1);
  if(slave_function == MODE_SLAVE_LIGHT){
    play(i2c_regs[5], true);
  }
  
}


void loop() {
  
  ////////////////////////////////////
  // This needs to be here
  TinyWireS_stop_check();
  ////////////////////////////////////

  set_new_address();
  led();
  activate_child();

  if (readEncoder()){
    play(encoder_count, true);
  }
  if (readEncoderButton()){
    clickEncoder();
  }
  if(slave_function == MODE_SLAVE_LIGHT){
    set_play_number();
  }
  readReset();

  wdt_reset();
}


void led()
{
  if(i2c_regs[3])
  {
    // Blink
    if(i2c_regs[2] == 2){
      static byte led_on = 1;
      static int blink_interval = 500;
      static unsigned long blink_timeout = millis() + blink_interval;
      
      if(blink_timeout < millis()){
        led_on = !led_on;
        blink_timeout = millis() + blink_interval;
      }
      digitalWrite(LED_PIN, led_on);
    }else if(i2c_regs[2] == 1){
      digitalWrite(LED_PIN, 1);
    }else{
      digitalWrite(LED_PIN, 0);
    }
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
    i2c_regs[2] = 0;
  }

}


void activate_child()
{
  if(i2c_regs[3] && i2c_regs[1])
  {
    digitalWrite(GATE_PIN, HIGH);
  }
  else
  {
    digitalWrite(GATE_PIN, LOW);
    i2c_regs[1] = 0;
  }
}


void set_new_address()
{
  if(i2c_regs[3] && i2c_regs[0])
  {
    //write EEPROM and reset
    //EEPROM.write(0x00, i2c_regs[0]);
    write_eeprom(0, i2c_regs[0]);
    i2c_regs[0] = 0;
    //EEPROM.write(0x01, displayNumber);
    write_eeprom(1, displayNumber);
    
    resetFunc();  
  }else{
    i2c_regs[0] = 0;
  }
}

boolean readEncoder(){

  boolean newEncode = false;
  static boolean last_aState = true;
  
  boolean aState = digitalRead(ENCODER_A_PIN);
  boolean bState = digitalRead(ENCODER_B_PIN);

  if (aState && aState != last_aState){
    if (aState != bState){
      encoder_count--;
      if(encoder_count < 0){
        encoder_count = encoder_max;
      }
    }else{
      encoder_count++;
      if(encoder_count > encoder_max){
        encoder_count = 0;
      }
    }
    newEncode = true;
  }

  last_aState = aState;

  return newEncode;
}


boolean readEncoderButton(){
  
  boolean newPushButton = false;

  /* Step 1: Update the integrator based on the input signal.  Note that the
  integrator follows the input, decreasing or increasing towards the limits as
  determined by the input state (0 or 1). */
  buttonInput = digitalRead(ENCODER_SW_PIN);
  if (buttonInput == false){
    if (buttonIntegrator > 0){
      buttonIntegrator--;
    }
  }else if (buttonIntegrator < DEBOUNCE_MAXIMUM){
    buttonIntegrator++;
  }
  /* Step 2: Update the output state based on the integrator.  Note that the
  output will only change states if the integrator has reached a limit, either
  0 or DEBOUNCE_MAXIMUM. */
  if (buttonIntegrator == 0){
    buttonOutput = false;
  }else if (buttonIntegrator >= DEBOUNCE_MAXIMUM){
    if(buttonInput != buttonOutput){
      newPushButton = true;
    }
    buttonOutput = true;
    buttonIntegrator = DEBOUNCE_MAXIMUM;  /* defensive code if integrator got corrupted */
  }
  //digitalWrite(ENCODER_SW_ACTION_PIN, output);

  return newPushButton;
  
}


void clickEncoder(){
  // Code to execut on encoder click
  //digitalWrite(ENCODER_SW_ACTION_PIN, !digitalRead(ENCODER_SW_ACTION_PIN));

}


void readReset(){
  if(!digitalRead(RESET_PIN)){
    if(i2c_regs[3]){      // If it is soft resetting for the first time, reset it for real
      write_eeprom(1, displayNumber);
      resetFunc();
    }
    i2c_regs[1] = 0;                    // disable slave
    i2c_regs[3] = 0;   // Set itself as an inactive block
  } else {                              // reset pin is less than 1000/1024 * 5 vcc
    i2c_regs[3] = 1;   // Set itself as an active block
  }
}

// Plays a different feature as encoder rotates
void play(int count, boolean update_reg){
  
  i2c_regs[5] = count;
  if(update_reg){
    displayNumber = i2c_regs[5];
  }
  

  // If the block is Set Light, changes the RGB colour
  if(slave_function == MODE_SLAVE_LIGHT){

    byte redBrightness    = 255 - led_matrix[count][0];
    byte greenBrightness  = 255 - led_matrix[count][1];
    byte blueBrightness   = 255 - led_matrix[count][2];

    analogWrite(RGBLED_PIN_RED,   redBrightness);
    analogWrite(RGBLED_PIN_GREEN, greenBrightness);
    analogWrite(RGBLED_PIN_BLUE,  blueBrightness);

  }
  

  // If the block is Set Sound, changes the buzzer tone
  if(slave_function == MODE_SLAVE_SOUND){
    tone_player(count, 1);
  }

}


void set_play_number(void){

  play(i2c_regs[5], false);
  
}