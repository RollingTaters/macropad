#include <Keypad.h>
#include "Keyboard.h"

/* Programatically keys are in a weird order, This abstraction for the following is in effect
 * [0,1,2,3]
 * [4,5,6,7]
 * [8,9,A,B]
 * [C,D,E,F]
 */ 

//Layer location is controlled here.
#define L1 0xD
#define L2 0xE

//Arbitrary macro value. In a blank space for arduino HID keyboard support
#define MRO 247

/*
 * This board has 3 layers, layer 0 is default with no modifier. Layer 0 has priority over Layer 1 has priority over layer 2.
 * The zeroed out spaces are where the layer buttons are. This can be changed, however the layer location constants above are how to change that.
 * Macros are placed with the Macro starting constant and an offset. MRO index 0 will be MRO+0.
 */
const uint16_t theMap[3][4][4]{
  {
  {KEY_0,KEY_1,KEY_2,MRO+3},
  {KEY_4,KEY_5,KEY_6,MRO+2},
  {KEY_8,KEY_9,KEY_A,MRO+1},
  {KEY_C,0x000,0x000,MRO+0}
  },
  {
  {KEY_F,KEY_U,KEY_C,KEY_K},
  {KEY_4,KEY_5,KEY_6,KEY_7},
  {KEY_8,KEY_9,KEY_A,KEY_B},
  {KEY_C,0x000,0x000,KEY_F}
  },
  {
  {KEY_S,KEY_H,KEY_I,KEY_T},
  {KEY_4,KEY_5,KEY_6,KEY_7},
  {KEY_8,KEY_9,KEY_A,KEY_B},
  {KEY_C,0x000,0x000,MRO+4}
  }
};

/* Macros are created here. They are arbitarily set at 6 length. 
 * If you modify the constant and change the array, any length will work with no other edits
 * BUG: The macro code presses all keys at once then releases them. This will not allow pushing the same key twice with the macro. This was done to keep support for shortcuts (i.e. ctrl+shift+esc)
 */
#define MRO_LEN 6
#define MRO_QUAN 8
const uint16_t macros[MRO_QUAN][MRO_LEN]{
  {KEY_LEFT_SHIFT,KEY_Q,0x00,0x00,0x00,0x00},
  {KEY_LEFT_SHIFT,KEY_P,0x00,0x00,0x00,0x00},
  {KEY_LEFT_SHIFT,KEY_G,0x00,0x00,0x00,0x00},
  {KEY_LEFT_CTRL,KEY_LEFT_SHIFT,KEY_ESC,0x00,0x00,0x00},
  {KEY_LEFT_CTRL,KEY_W,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00}
};

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

//Pins for which your keypad matrix will be created
byte rowPins[ROWS] = {7, 8, 9, 10}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3, 2}; //connect to the column pinouts of the keypad

//end of abstraction

/*
 * This map is how the matrix is defined via the hardware. This will need to be changed if your matrix differs from how mine was setup. 
 * The teensy multikey keypad example will help you figure out how yours is laid out
 */
const uint16_t keyMap[COLS][ROWS]{
  {0,4,8,12},
  {1,5,9,13},
  {2,6,10,14},
  {3,7,11,15}
};

/**
 * IF CREATING A 4x4 Macropad, no other edits need to be made below.
 */

uint8_t layer;
bool layerEn1;
bool layerEn2;


//define the symbols on the buttons of the keypads, I dont want this here.. we dont use it... but thats how keypad was written
char garbage[COLS][ROWS] = {
  {'0','1','2','3'},
  {'4','5','6','7'},
  {'8','9','A','B'},
  {'C','D','E','F'}
};


//initialize an instance of class NewKeypad
Keypad kpd = Keypad( makeKeymap(garbage), colPins, rowPins, COLS, ROWS); 


unsigned long loopCount;
unsigned long startTime;
String msg;


void setup() {
    layer = 0;
    layerEn1 = false;
    layerEn2 = false;
    Serial.begin(9600);
    loopCount = 0;
    startTime = millis();
    msg = "";
}

uint16_t keyMapper(int i){
  int j = i/COLS;
  int k = i%COLS;
  int val = keyMap[j][k];
  j = val/COLS;
  k = val%COLS;
  return theMap[layer][j][k];
}

uint16_t locMapper(int i){
  int j = i/COLS;
  int k = i%COLS;
  return(keyMap[j][k]);
}

void loop() {
    loopCount++;
    if ( (millis()-startTime)>5000 ) {
        Serial.print("Average loops per second = ");
        Serial.println(loopCount/5);
        startTime = millis();
        loopCount = 0;
    }

    // Fills kpd.key[ ] array with up-to 10 active keys.
    // Returns true if there are ANY active keys.
    if (kpd.getKeys())
    {
        for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
        {
            if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
            {
                Serial.println(kpd.key[i].kcode);
                uint16_t keypress = keyMapper(kpd.key[i].kcode);
                switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                  case PRESSED:
                    if(kpd.key[i].kcode == locMapper(L1) && !layerEn2){ //Layer 1 has priority over layer 2. If layer 1 and 2 are activated at same time, Layer will be ignored even when layer 1 is released.
                      layer = 1;
                      layerEn1 = true;
                    }else if(kpd.key[i].kcode == locMapper(L2) && !layerEn1){
                      layer = 2;
                      layerEn2 = true;
                    }else{
                      if(keypress >= MRO && keypress < MRO+MRO_QUAN){
                        for(int i = 0; i < MRO_LEN; i++){
                          Keyboard.press(macros[keypress-MRO][i]); 
                        }
                      }else{
                        Keyboard.press(keypress);
                      }
                    }
                    break;
                  case HOLD:
                    break;
                  case RELEASED:
                    if(kpd.key[i].kcode == locMapper(L1) && !layerEn2){
                      layer = 0;
                      layerEn1 = false;
                    }else if(kpd.key[i].kcode == locMapper(L2) && !layerEn1){
                      layer = 0;
                      layerEn2 = false;
                    }else{
                      if(keypress >= MRO && keypress < MRO+MRO_QUAN){
                        for(int i = 0; i < MRO_LEN; i++){
                          Keyboard.release(macros[keypress-MRO][i]);  
                        }
                      }else{
                        Keyboard.release(keypress);
                      }
                    }
                    break;
                  case IDLE:
                    break;
                }
            }
        }
    }
} 
