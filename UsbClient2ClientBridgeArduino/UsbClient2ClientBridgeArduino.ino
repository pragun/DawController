

#include "Keyboard.h"
//#include "Mouse.h">
#include <Mouse_Alt.h>
#include "MIDIUSB.h"
#include <NeoHWSerial.h>
#define NEWLINE 10
#define RX_BUF_SIZE 8

const byte RX_Interrupt_Pin = 7;
//This is going to be our poor man's software interrupt pin number

uint8_t rxBuf1[RX_BUF_SIZE] = {};
uint8_t rxBuf2[RX_BUF_SIZE] = {};
uint8_t rxBuf_i[2] = {};
uint8_t rx_buf_j = 0;
uint8_t *rx_buf[2];
uint8_t rxing_size = 0;
uint8_t rxed_size = 0;
uint8_t* rx_buf_to_proc;
uint8_t rx_buf_k = 0;
uint8_t msg_handler_id = 0;
typedef void (* eventFP)(uint8_t*);
eventFP MsgHandlers[3] = {&sendMouse,&sendKeyboard,&sendMIDIData};


/*
 * Mouse packet byte definitions
 * packet = [a][b][c][d][e][f]
 * a =  'a', literally the character a, makes sure that sendMouse is selected
 * b =  0bxyz[b5][b4][b3][b2][b1] <x:1 momentary click>, <y:1 press> <z:1 release>, needless to say only one can be active at a time
 *      b1-5, denote which button(s), the <x,y,z> bits talk about
 * c = signed_uint8_t carrying del X
 * d = signed_uint8_t carrying del Y
 * e = signed_uint8_t carrying del wheelX
 * f = signed_uint8_t carrying del wheelY
 */

/* 
 *  Midi packet byte definition
 *  packet = [a][b][c][d][e]
 *  a = 'c', literally the character c
 *  b = Header byte, basically c(status byte) >> 4
 *  c = status byte for midi
 *  d = data 1 byte for midi
 *  e = data 2 byte for midi
 */

 /*
  * Keyboard packet byte definition
  * packet = [a] [b] [c]
  * a = 'b', literally the character b
  * b = the ascii code for the key to be pressed/released
  * c = 0 for release, 1 for press
  */


 typedef struct
 {
  uint8_t midi_selector;
  midiEventPacket_t midi_event; //This follows the arduino MIDIUSB Library specification

  // As Arduino example https://www.arduino.cc/en/Tutorial/MidiDevice says
  // First parameter is the event type (0x09 = note on, 0x08 = note off).
  // Second parameter is note-on/note-off, combined with the channel.
  // Channel can be anything between 0-15. Typically reported to the user as 1-16.
  // Third parameter is the note number (48 = middle C).
  // Fourth parameter is the velocity (64 = normal, 127 = fastest).

 } midiDataStruct;
 midiDataStruct *midiData = NULL;

 typedef struct 
 {
  uint8_t mouse_selector;
  uint8_t button;
  uint8_t delx;
  uint8_t dely;
  uint8_t del_wx;
  uint8_t del_wy;
} mouseDataStruct;
mouseDataStruct *mouseData = NULL;

typedef struct
{
  uint8_t keyboard_selector;
  char key_ascii;
  uint8_t press_release;
} keyboardDataStruct;
keyboardDataStruct *keyboardData = NULL;


void setup() {
  // Fix the dual receive buffers for data received over serial
  rx_buf[0] = &rxBuf1[0];
  rx_buf[1] = &rxBuf2[0];

  pinMode(RX_Interrupt_Pin, OUTPUT);
  digitalWrite(RX_Interrupt_Pin, 0);
  attachInterrupt(digitalPinToInterrupt(RX_Interrupt_Pin), handleCMD, RISING);
  
  NeoSerial1.begin(1000000);
  Serial.begin(9600);
  // initialize control over the keyboard:
  Keyboard.begin();
  Mouse.begin();
  NeoSerial1.attachInterrupt( handleRxChar );
  Serial.println("Setup completed MIDIUsb, MouseUSB, KeyboardUSB");
}

static void handleCMD(){
  //Serial.println("Command handler..");
  rx_buf_k = (rx_buf_j + 1) % 2;
  rx_buf_to_proc = rx_buf[rx_buf_k];
  //Serial.println((char*) rx_buf_to_proc);
  msg_handler_id = rx_buf_to_proc[0]-97;
  //Serial.println((char) msg_handler_id,DEC);
  
  if ((msg_handler_id >= 0)&&(msg_handler_id <=3)){
    MsgHandlers[msg_handler_id](rx_buf_to_proc);
  }
  digitalWrite(RX_Interrupt_Pin, 0);
}

void sendMouse(uint8_t* rx_buf){
  //Serial.println("SendMouse");
  mouseData = (mouseDataStruct *) rx_buf;
  //Serial.println(mouseData->mouse_selector);
  //Serial.println(mouseData->button);
  //Serial.println(mouseData->delx);
  //Serial.println(mouseData->dely);
  //Serial.println(mouseData->del_wx);
  //Serial.println(mouseData->del_wy);

  switch(mouseData->button & 0b11100000){
    case 0b00000000:
      break;
      
    case 0b10000000:
      Mouse.buttons(mouseData->button & 0b00011111);
      return;
    
      
    case 0b01000000:
      Mouse.press(mouseData->button & 0b00011111);  
      return;
      
    case 0b00100000:
      Mouse.release(mouseData->button & 0b00011111);  
      return;
  }

  Mouse.move(mouseData->delx,mouseData->dely,mouseData->del_wx);
}

void sendKeyboard(uint8_t* rx_buf){
  Serial.println("SendKeyboard");
  Serial.println(keyboardData->key_ascii);
  Serial.println(keyboardData->press_release);
  keyboardData = (keyboardDataStruct *) rx_buf;
  if (keyboardData->press_release == 0){
    Keyboard.release(keyboardData->key_ascii);
  }else{
    Keyboard.press(keyboardData->key_ascii);
  }
  //Mouse.click(MOUSE_RIGHT);
}

void sendMIDIData(uint8_t* rx_buf){
  //Serial.println("SendMIDIData");
  midiData = (midiDataStruct *) rx_buf;
  //Serial.println(midiData->midi_event.header);
  //Serial.println(midiData->midi_event.byte1);
  //Serial.println(midiData->midi_event.byte2);
  //Serial.println(midiData->midi_event.byte3);
  MidiUSB.sendMIDI(midiData->midi_event);
  MidiUSB.flush();
}

static void handleRxChar( uint8_t c )
{
    //Serial.print("I received: ");
    //Serial.println(c, DEC);
    if(c == NEWLINE){
       rx_buf[rx_buf_j][rxing_size] = 0;
       //Serial.print("NewLine Received -- Swapping. Received: ");
       //Serial.print((char*) rx_buf[rx_buf_j]);
       rx_buf_j = (rx_buf_j+1) % 2;
       rxed_size = rxing_size;
       rxing_size = 0;
       //Serial.print("RX BUF J");
       //Serial.print(rx_buf_j,DEC);
       //Serial.print("RXED Size ");
       //Serial.print(rxed_size,DEC);
       digitalWrite(RX_Interrupt_Pin, 1);
    }else{
      rx_buf[rx_buf_j][rxing_size] = c;
      rxing_size ++;
      rxing_size = rxing_size % RX_BUF_SIZE;
      //Serial.print((char*) rx_buf[rx_buf_j]); 
    }
}


void loop() {
   /* if (NeoSerial1.available() > 0) {
                // read the incoming byte:
                incomingByte = NeoSerial1.read();

                // say what you got:
                Serial.print("I received: ");
                Serial.println(incomingByte, DEC);
  } */
  // check for incoming serial data:
  /* if (Serial.available() > 0) {
    // read incoming serial data:
    char inChar = Serial.read();
    // Type the next ASCII value from what you received:
    Keyboard.write(inChar + 1);
  }
  Serial.write("testing..\n");
  Serial1.write("From the other side...\n"); */
  delay(1);
}
