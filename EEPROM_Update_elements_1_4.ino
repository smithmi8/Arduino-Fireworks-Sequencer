/***
   EEPROM Update method

   Stores values read from analog input 0 into the EEPROM.
   These values will stay in the EEPROM when the board is
   turned off and may be retrieved later by another sketch.

   If a value has not changed in the EEPROM, it is not overwritten
   which would reduce the life span of the EEPROM unnecessarily.

   Released using MIT licence.
 ***/

#include <EEPROM.h>

// ---- Arrays -----
unsigned long tempDelays[35];        // used in changing or converting delays in delays array (defined below in code)
int displayMills[35][6];             // This array is a array used for converting mills into individual characters for display and manipulation

/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
int address = 0;

// setup the variables and functions
// ****** BOX 1 *****
//unsigned long delay0 = 1000; // Common delay
//unsigned long delay1 = 0000;    
//unsigned long delay2 = 2100;
//unsigned long delay3 = 2734;
//unsigned long delay4 = 1199;
//unsigned long delay5 = 2267;
//unsigned long delay6 = 2800;
//unsigned long delay7 = 6700;
//unsigned long delay8 = 2633;
//unsigned long delay9 = 4033;
//unsigned long delay10 = 2301;
//unsigned long delay11 = 266;
//unsigned long delay12 = 3834;
//unsigned long delay13 = 6399;
//unsigned long delay14 = 2167;
//unsigned long delay15 = 10166;
//unsigned long delay16 = 4634;
//unsigned long delay17 = 2534;
//unsigned long delay18 = 23133;
//unsigned long delay19 = 64100;
//unsigned long delay20 = 2300;
//unsigned long delay21 = 266;
//unsigned long delay22 = 3800;
//unsigned long delay23 = 53867;
//unsigned long delay24 = 2133;
//unsigned long delay25 = 267;
//unsigned long delay26 = 3800;
//unsigned long delay27 = 0;
//unsigned long delay28 = 0;
//unsigned long delay29 = 0;
//unsigned long delay30 = 0;
//unsigned long delay31 = 0;
//unsigned long delay32 = 0;

// ****** BOX 2 *****
//unsigned long delay0 = 1000; // Common delay
//unsigned long delay1 = 0000;    
//unsigned long delay2 = 2100;
//unsigned long delay3 = 2734;
//unsigned long delay4 = 1199;
//unsigned long delay5 = 2267;
//unsigned long delay6 = 2800;
//unsigned long delay7 = 1133;
//unsigned long delay8 = 2767;
//unsigned long delay9 = 9699;
//unsigned long delay10 = 1767;
//unsigned long delay11 = 834;
//unsigned long delay12 = 3567;
//unsigned long delay13 = 6933;
//unsigned long delay14 = 7866;
//unsigned long delay15 = 7000;
//unsigned long delay16 = 4101;
//unsigned long delay17 = 23133;
//unsigned long delay18 = 64300;
//unsigned long delay19 = 1767;
//unsigned long delay20 = 833;
//unsigned long delay21 = 3566;
//unsigned long delay22 = 53900;
//unsigned long delay23 = 1767;
//unsigned long delay24 = 834;
//unsigned long delay25 = 3566;
//unsigned long delay26 = 0;
//unsigned long delay27 = 0;
//unsigned long delay28 = 0;
//unsigned long delay29 = 0;
//unsigned long delay30 = 0;
//unsigned long delay31 = 0;
//unsigned long delay32 = 0;

// ****** BOX 3 *****
unsigned long delay0 = 1000; // Common delay
unsigned long delay1 = 0000;    
unsigned long delay2 = 2100;
unsigned long delay3 = 2734;
unsigned long delay4 = 366;
unsigned long delay5 = 833;
unsigned long delay6 = 2267;
unsigned long delay7 = 2800;
unsigned long delay8 = 400;
unsigned long delay9 = 733;
unsigned long delay10 = 12734;
unsigned long delay11 = 1200;
unsigned long delay12 = 1466;
unsigned long delay13 = 633;
unsigned long delay14 = 2433;
unsigned long delay15 = 168;
unsigned long delay16 = 7333;
unsigned long delay17 = 5200;
unsigned long delay18 = 13367;
unsigned long delay19 = 21666;
unsigned long delay20 = 1467;
unsigned long delay21 = 64566;
unsigned long delay22 = 1200;
unsigned long delay23 = 1467;
unsigned long delay24 = 634;
unsigned long delay25 = 2433;
unsigned long delay26 = 166;
unsigned long delay27 = 54167;
unsigned long delay28 = 1201;
unsigned long delay29 = 1466;
unsigned long delay30 = 634;
unsigned long delay31 = 2432;
unsigned long delay32 = 167;

// ****** BOX 4 *****
//unsigned long delay0 = 1000; // Common delay
//unsigned long delay1 = 0000;    
//unsigned long delay2 = 2100;
//unsigned long delay3 = 3100;
//unsigned long delay4 = 833;
//unsigned long delay5 = 2267;
//unsigned long delay6 = 3200;
//unsigned long delay7 = 733;
//unsigned long delay8 = 2767;
//unsigned long delay9 = 10266;
//unsigned long delay10 = 634;
//unsigned long delay11 = 2033;
//unsigned long delay12 = 2934;
//unsigned long delay13 = 7666;
//unsigned long delay14 = 7133;
//unsigned long delay15 = 7767;
//unsigned long delay16 = 3334;
//unsigned long delay17 = 23133;
//unsigned long delay18 = 64867;
//unsigned long delay19 = 633;
//unsigned long delay20 = 2033;
//unsigned long delay21 = 2933;
//unsigned long delay22 = 54434;
//unsigned long delay23 = 600;
//unsigned long delay24 = 2100;
//unsigned long delay25 = 2933;
//unsigned long delay26 = 0;
//unsigned long delay27 = 0;
//unsigned long delay28 = 0;
//unsigned long delay29 = 0;
//unsigned long delay30 = 0;
//unsigned long delay31 = 0;
//unsigned long delay32 = 0;

// ****** BOX 5 *****
//unsigned long delay0 = 1000; // Common delay
//unsigned long delay1 = 0000;    
//unsigned long delay2 = 2100;
//unsigned long delay3 = 3100;
//unsigned long delay4 = 833;
//unsigned long delay5 = 2267;
//unsigned long delay6 = 3200;
//unsigned long delay7 = 6300;
//unsigned long delay8 = 2633;
//unsigned long delay9 = 5100;
//unsigned long delay10 = 2667;
//unsigned long delay11 = 2667;
//unsigned long delay12 = 8000;
//unsigned long delay13 = 566;
//unsigned long delay14 = 11600;
//unsigned long delay15 = 3934;
//unsigned long delay16 = 1800;
//unsigned long delay17 = 23133;
//unsigned long delay18 = 65133;
//unsigned long delay19 = 2666;
//unsigned long delay20 = 2667;
//unsigned long delay21 = 54734;
//unsigned long delay22 = 2667;
//unsigned long delay23 = 2666;
//unsigned long delay24 = 0;
//unsigned long delay25 = 0;
//unsigned long delay26 = 0;
//unsigned long delay27 = 0;
//unsigned long delay28 = 0;
//unsigned long delay29 = 0;
//unsigned long delay30 = 0;
//unsigned long delay31 = 0;
//unsigned long delay32 = 0;

int FireMode = 1;                    // 0 = Common, 1 = Step, 2 = ALL - update or read main version at EEPROM address 214
int Trigger = 0;                     // 0 = Signal, 1 = Manual -  update or read main version at EEPROM address 213
unsigned long FireTime = 1000;       // time the relay will fire the cue - Stored delays array [33] - update or read main version at EEPROM address  197-202
unsigned long SleepTime = 30000;     // SleepTime is a timer to hold the sleep timer - delays[34]
int TimeType = 0;                    // this is the type of time between cues. relative or absolute - EEPROM address 215
int NumOfCommonCuesLeft = 3;         // update or read main version at EEPROM address 211
int NumOfCommonCuesRight = 2;        // update or read main version at EEPROM address 212
int ScreenSleep= 0;                  // 0 Turns screen off when it goes to sleep, 1 keeps screen on.
unsigned long delays[35] = {delay0, delay1, delay2, delay3, delay4, delay5, delay6, delay7, delay8, delay9, delay10, delay11, delay12, delay13, delay14, delay15, delay16, delay17, delay18, delay19, delay20, delay21, delay22, delay23, delay24, delay25, delay26, delay27, delay28, delay29, delay30, delay31, delay32, FireTime, SleepTime};

void copyDelaysToTempDelays(){
  for(int i = 0; i < 35; i = i +1) {
    tempDelays[i] = delays[i];
    Serial.print("delays ");Serial.print(i);Serial.print(" = ");Serial.println(delays[i]);
  }
}

void copyTempDelaysToDelays(){
  for(int i = 0; i < 35; i = i +1) {
    delays[i] = tempDelays[i];
  }
}

void copyDisplayMillsToTempDelays(){
  for(int i = 0; i < 35; i = i +1) {
    String st; 
    st = displayMills[i][0];  
    st = st + displayMills[i][1]; 
    st = st + displayMills[i][2]; 
    st = st + displayMills[i][3]; 
    st = st + displayMills[i][4]; 
    st = st + displayMills[i][5];
    tempDelays[i] = st.toInt();
Serial.print ("tempDelays "); Serial.print(i); Serial.print (" = ");Serial.println(tempDelays[i]);
  }
}

void copyTempDelaysToDisplayMills(){
  for(int i = 0; i < 35; i = i +1) {
    Serial.print("tempDelays");Serial.print(i);Serial.println(tempDelays[i]);
    displayMills[i][5] = (tempDelays[i] / 1U) % 10;
    displayMills[i][4] = (tempDelays[i] / 10U) % 10;
    displayMills[i][3] = (tempDelays[i] / 100U) % 10;
    displayMills[i][2] = (tempDelays[i] / 1000U) % 10;
    displayMills[i][1] = (tempDelays[i] / 10000U) % 10;
    displayMills[i][0] = (tempDelays[i] / 100000U) % 10;
    Serial.print("displayMills = ");
    Serial.print(displayMills[i][0]); Serial.print(displayMills[i][1]); Serial.print(displayMills[i][2]); 
    Serial.print(".");Serial.print(displayMills[i][3]); Serial.print(displayMills[i][4]); Serial.println(displayMills[i][5]);Serial.println();
  }
}
void updateEepromWithDisplayTimes(){
   address=0;
   for(int i = 0; i < 35; i = i +1) {
    EEPROM.update(address,displayMills[i][0]);
    address++;
    EEPROM.update(address,displayMills[i][1]);
    address++;
    EEPROM.update(address,displayMills[i][2]);
    address++;
    EEPROM.update(address,displayMills[i][3]);
    address++;
    EEPROM.update(address,displayMills[i][4]);
    address++;
    EEPROM.update(address,displayMills[i][5]);
    address++;
  }
}

void setup() {
// use this first to clear all EEPROM memory. 
//  for (int i = 0 ; i < EEPROM.length() ; i++) {
//    EEPROM.write(i, 0);
//  }
  
    Serial.begin(9600);
  /** EMpty setup **/
  copyDelaysToTempDelays();
  copyTempDelaysToDisplayMills();
  updateEepromWithDisplayTimes();
  
  EEPROM.update(211,NumOfCommonCuesLeft);
  Serial.print("NumOfCommonCuesLeft = ");Serial.println(EEPROM.read(211));
  EEPROM.update(212,NumOfCommonCuesRight);
  Serial.print("NumOfCommonCuesRight = ");Serial.println(EEPROM.read(212));
  EEPROM.update(213,Trigger);
  Serial.print("Trigger = ");Serial.println(EEPROM.read(213));
  EEPROM.update(214,FireMode);
  Serial.print("FireMode = ");Serial.println(EEPROM.read(214));
  EEPROM.update(215,TimeType);
  Serial.print("TimeType = ");Serial.println(EEPROM.read(215));
  EEPROM.update(217,ScreenSleep);
  Serial.println("Initialized");
  delay(10000);
}



void loop() {
  /***
//    need to divide by 4 because analog inputs range from
//    0 to 1023 and each byte of the EEPROM can only hold a
//    value from 0 to 255.
//  ***/
//  int val = analogRead(0) / 4;
//
//  /***
//    Update the particular EEPROM cell.
//    these values will remain there when the board is
//    turned off.
//  ***/
//  EEPROM.update(address, val);
//
//  /***
//    The function EEPROM.update(address, val) is equivalent to the following:
//
//    if( EEPROM.read(address) != val ){
//      EEPROM.write(address, val);
//    }
//  ***/
//
//
//  /***
//    Advance to the next address, when at the end restart at the beginning.
//
//    Larger AVR processors have larger EEPROM sizes, E.g:
//    - Arduno Duemilanove: 512b EEPROM storage.
//    - Arduino Uno:        1kb EEPROM storage.
//    - Arduino Mega:       4kb EEPROM storage.
//
//    Rather than hard-coding the length, you should use the pre-provided length function.
//    This will make your code portable to all AVR processors.
//  ***/
//  address = address + 1;
//  if (address == EEPROM.length()) {
//    address = 0;
//  }
//
//  /***
//    As the EEPROM sizes are powers of two, wrapping (preventing overflow) of an
//    EEPROM address is also doable by a bitwise and of the length - 1.
//
//    ++address &= EEPROM.length() - 1;
//  ***/
//
//  delay(100);
}
