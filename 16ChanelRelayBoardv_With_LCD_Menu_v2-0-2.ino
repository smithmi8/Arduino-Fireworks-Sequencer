/*
 Version 1.4.5 - Corrected issues with timing, and conversions back and forth from relative to absolute time.
                 Added temp variables to setup so user can exit without making or saving changes
 Version 1.4.6 - Adding fire all mode.  actually adds a .001 second between fireing each Cue.
 Version 1.4.7 - Refinement of code. Sequencing function.
                 Corrected bug saving trigger type to EEPROM memory 
 Version 2.0.0 - Added Code to support RF 4 channel Radio transmitter Channel D is reserved for remote eStop A=channel 1, B= Channel 2, C = Channel 3
                 Changed firing function to a Do While loop for instant read of estop or disarm switch. also provides a more accurate fire time based on clock time.
                 Increase fire signal voltage to just under 4 volts
 Version 2.0.1 - code cleanup, adjust menu option wording.
 Version 2.0.2 - code cleanup, FireModeSt was being set after saving Relative Step mode.
                 Reset ALL Delays back to zero / navigation to wrong page
*/
                   
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
// Create lcd object of class LiquidCrystal_I2C:
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // Change to (0x27,20,4) for 20x4 LCD.

// Rotary Encoder Inputs
#include <rotary.h>
// Rotary(Encoder Pin 1, Encoder Pin 2, Button Pin SW)
Rotary r = Rotary(2, 3, 4);

// menu system
int address = 0;                    // used for referencing EEPROM address
int armed = 0;                      // used as a place holder for the armed switch value
unsigned long curTime = 0;          // used with the sleep timer for returning to main screen and putting screen to cleep
bool backlightFlag = 1;             // test flag for seeing if backlight is turned on or off
bool btn_push;                      // used as trigger / flag for when select key is pressed
int btn_Fire = 0;                   // used as a place holder for reading the Fire button
int delayPointer = 0;               // Used to pass what delay is being modified to the edit screen
int FireMode = 0;                   // 0 = Common, 1 = Step, 2 = ALL - update or read main version at EEPROM address 214
String FireModeSt = "";             // This is a string value of fireMode
int page = 0;                       //  actual page displayed on lcd screen
int returnPage = 0;                 // This is used on some page to act as a return at end of function since every button press cycles through main loop.
int returnVpoz = 0;                 // used for setting position on returning to a menu.
bool ReturnToPageFlag = false;      // used to skip button press at beging of loop when returning from sub page
int ScreenSleep = 0;                // setting to put the screen to sleep after sleep timer
int TempScreenSleep = 0;            // Used when changing the screen sleep setting but not yet saved
int SelectKey = 0;                  // used as trigger / flag for when select key is pressed
int SelectKey_max = 1;              // used to set the limit of how many times the select key needs to be pressed in a menu ( don't know why this would change yet
int SignalIn = 0;                   // used for place holder for signal in pin value
unsigned long SleepTime;            // SleepTime is a timer to hold the sleep timer - delays[34]
unsigned long TempDelay = 0;        // used for converting a single number from float to mills or mills to float
int TimeType = 0;                   // this is the type of time between cues. relative or absolute - EEPROM address 215 0 is Relative, 1 is Absolute
int TempTimeType = 0;               // This is used when changing Time type but not saving
String title = "";                  // used for screen titles when passing to pringt functions
int Trigger = 0;                    // 0 = Signal, 1 = Manual -  update or read main version at EEPROM address 213
int TempTrigger = 0;                // This is used when changing the Trigger, but not yet saved.
String TriggerSt = "";              // This is a string value of Trigger
int Version = 2;                    // update or read main version at EEPROM address 211
float SubVersion = 0.2;             // update or read main version at EEPROM address 212
int vpoz = 0;                       // cursor position on actual page think of this as a tab stop. - used to trigger case selections
int vpoz_max = 3;                   // 1 for 16x02 3 for 20x04
int vpoz_min = 0;                   // used for setting curser minimum position on lcd screen zero based number
int vpozOld = 0;                    // used for messuring change in position
int vpozNew = 0;                    // used for messuring change in position
int numOfCommonCues;                // used to specify the number of cues used when using common cues
int SetTempDelaysToZeroFlag=0;      // used for reseting dekay times before saving
int TempFireMode = 0;               // This is used when changing the Fire Mode, but not yet saved
int TempNumOfCommonCues;            // used for setting the number of cues that should be used on common delay firing
int TempLeft;                       // used for splitting 2 digit numbers when adding to EEPROM memory
int TempRight;                      // used for splitting 2 digit numbers when adding to EEPROM memory
int AccruTiming =0;                 // used for adding the sequence time from timing to apply to cue 32 when processing  0 timing zero timing delays
int RadioChannel = 1;               // This is the RF remote channel A = 1, B = 2, C = 3.  D button is being reserved for emergancy stop. EEPROM 219
String RadioChannelSt="";           // This is a alpha character representing the remote channel
int TempRadioChannel = 1;           // This is used when changing the RF channel, but not yet saved
int eStop = 0;                      // This is the variable for reading in RF remote emergancy stop (Button D). 

// ---- Arrays -----
unsigned long tempDelays[35];       // used in changing or converting delays in delays array (defined below in code)
unsigned long tempTiming[35];       // used in changing or converting Times in timing array (defined below in code)
unsigned long delays[35];           // used for storing the common delay, reguler delays then the FireTime then sleep time;
unsigned long timing[35];           // array used for when user wants to eneter times using cronalogical times
unsigned long sequence[64][3];      // array used after collecting the delays then sorts into a requence for firing(includes cue on and off)
int displayMills[35][6];            // This array is a array used for converting mills into individual characters for display and manipulation
int timingDisplayMills[35][6];      // This array is a array used for converting mills into individual characters for display and manipulation
int cueTable[33];                   // This is used to store the value of the pin assignments during the creation of the sequencing
int remoteChannels[4];               // This stores the analog read pin numbers to be used with RadioChannel as a pointer

// Special Charaters to show Battery levels put two together in upprt right cvorner
//byte Bat1Empty[] = {B00000, B01111, B10000, B10000, B10000, B10000, B01111, B00000};
//byte Bat1Half[] = {B00000, B01111, B10011, B10011, B10011, B10011, B01111, B00000};
//byte Bat1Full[] = {B00000, B01111, B11111, B11111, B11111, B11111, B01111, B00000};
//byte Bat2Full[] = {B00000, B11111, B11111, B11111, B11111, B11111, B11111, B00000};
//byte Bat2Half[] = {B00000, B11111, B00111, B00111, B00111, B00111, B11111, B00000};
//byte Bat2Empty[] = {B00000, B11111, B00001, B00001, B00001, B00001, B11111, B00000};
//byte underline[] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111};

// setup the variables and functions
// The cue is set to the Pin Number that will trigger the cue
int ArmedSwitchPin = 10;
int SignalInPin = A3;
int FireButtonPin = 9;
int eStopPin = 14;
int RadioChannelC = 15;
int RadioChannelB = 16;
int RadioChannelA = 17;
int cue1 = 22;    
int cue2 = 23;
int cue3 = 24;
int cue4 = 25;
int cue5 = 26;
int cue6 = 27;
int cue7 = 28;
int cue8 = 29;
int cue9 = 30;
int cue10 = 31;
int cue11 = 32;
int cue12 = 33;
int cue13 = 34;
int cue14 = 35;
int cue15 = 36;
int cue16 = 37;
int cue17 = 38;
int cue18 = 39;
int cue19 = 40;
int cue20 = 41;
int cue21 = 42;
int cue22 = 43;
int cue23 = 44;
int cue24 = 45;
int cue25 = 46;
int cue26 = 47;
int cue27 = 48;
int cue28 = 49;
int cue29 = 50;
int cue30 = 51;
int cue31 = 52;
int cue32 = 53;
int NoCue = 69;
unsigned long commonDelay = delays[0];
unsigned long FireTime = delays[33];   // time the relay will fire the cue - Stored delays array [33] - update or read main version at EEPROM address  197-202

int red_light_pin = 5;
int green_light_pin = 6;
int blue_light_pin = 7;

void updateEepromWithDisplayMills() {
  address = 0;
  for (int i = 0; i < 35; i = i + 1) {
    EEPROM.update(address, displayMills[i][0]);
    address++;
    EEPROM.update(address, displayMills[i][1]);
    address++;
    EEPROM.update(address, displayMills[i][2]);
    address++;
    EEPROM.update(address, displayMills[i][3]);
    address++;
    EEPROM.update(address, displayMills[i][4]);
    address++;
    EEPROM.update(address, displayMills[i][5]);
    address++;

  //  Serial.print("displayMills ");//  Serial.print(i);//  Serial.print(" = ");
  //  Serial.print(displayMills[i][0]); //  Serial.print(displayMills[i][1]); //  Serial.print(displayMills[i][2]);
  //  Serial.print(".");//  Serial.print(displayMills[i][3]); //  Serial.print(displayMills[i][4]); //  Serial.println(displayMills[i][5]);//  Serial.println();
  }
}

void getEepromToDisplayMills() {
  address = 0;
  for (int i = 0; i < 35; i = i + 1) {
    displayMills[i][0] = EEPROM.read(address);
    address++;
    displayMills[i][1] = EEPROM.read(address);
    address++;
    displayMills[i][2] = EEPROM.read(address);
    address++;
    displayMills[i][3] = EEPROM.read(address);
    address++;
    displayMills[i][4] = EEPROM.read(address);
    address++;
    displayMills[i][5] = EEPROM.read(address);
    address++;
   //  Serial.print("displayMills ");//  Serial.print(i);//  Serial.print(" = ");
   //  Serial.print(displayMills[i][0]); //  Serial.print(displayMills[i][1]); //  Serial.print(displayMills[i][2]);
   //  Serial.print(".");//  Serial.print(displayMills[i][3]); //  Serial.print(displayMills[i][4]); //  Serial.println(displayMills[i][5]);//  Serial.println();
  }
}
void copyDelaysToTempDelays() {
  for (int i = 0; i < 35; i = i + 1) {
    tempDelays[i] = delays[i];
  //  Serial.print("tempDelays ");Serial.print(i);Serial.print(" = ");Serial.println(tempDelays[i]);
  }
}

void copyTempDelaysToDelays() {
  for (int i = 0; i < 35; i = i + 1) {
    delays[i] = tempDelays[i];
  //  Serial.print("delays ");Serial.print(i);Serial.print(" = ");Serial.println(delays[i]);
  }
}

void copyTimingToTempTiming() {
  for (int i = 0; i < 33; i = i + 1) {
    tempTiming[i] = timing[i];
  //  Serial.print("tempTiming ");Serial.print(i);Serial.print(" = ");Serial.println(tempTiming[i]);
  }
}

void SetTempDelaysToZero() {
  for (int i = 1; i < 33; i = i + 1) {
    tempDelays[i] = 0;
  //  Serial.print("tempDelays ");//  Serial.print(i);Serial.print(" = ");Serial.println(tempDelays[i]);
  }
}

void copyTempTimingToTiming() {
  for (int i = 0; i < 33; i = i + 1) {
    timing[i] = tempTiming[i];
  //  Serial.print("timing ");Serial.print(i);Serial.print(" = ");Serial.println(timing[i]);
  }
}

void copyDisplayMillsToTempDelays() {
  for (int i = 0; i < 35; i = i + 1) {
    String st;
    st = displayMills[i][0];
    st = st + displayMills[i][1];
    st = st + displayMills[i][2];
    st = st + displayMills[i][3];
    st = st + displayMills[i][4];
    st = st + displayMills[i][5];
    tempDelays[i] = st.toInt();
//  Serial.print("tempDelays ");Serial.print(i);Serial.print(" = ");Serial.println(tempDelays[i]);
  }
}

void copyTimingDisplayMillsToTempTiming() {
  for (int i = 0; i < 35; i = i + 1) {
    String st;
    st = timingDisplayMills[i][0];
    st = st + timingDisplayMills[i][1];
    st = st + timingDisplayMills[i][2];
    st = st + timingDisplayMills[i][3];
    st = st + timingDisplayMills[i][4];
    st = st + timingDisplayMills[i][5];
    tempTiming[i] = st.toInt();
//  Serial.print("tempTiming ");Serial.print(i);Serial.print(" = ");Serial.println(tempTiming[i]);
  }
}

void copyTempDelaysToDisplayMills() {
  for (int i = 0; i < 35; i = i + 1) {
    displayMills[i][5] = (tempDelays[i] / 1U) % 10;
    displayMills[i][4] = (tempDelays[i] / 10U) % 10;
    displayMills[i][3] = (tempDelays[i] / 100U) % 10;
    displayMills[i][2] = (tempDelays[i] / 1000U) % 10;
    displayMills[i][1] = (tempDelays[i] / 10000U) % 10;
    displayMills[i][0] = (tempDelays[i] / 100000U) % 10;
     //tempTiming[i] = ((float)delays[i]/1000);
  }
}

void copyTempTimingToTimingDisplayMills() {
  for (int i = 0; i < 35; i = i + 1) {
    timingDisplayMills[i][5] = (tempTiming[i] / 1U) % 10;
    timingDisplayMills[i][4] = (tempTiming[i] / 10U) % 10;
    timingDisplayMills[i][3] = (tempTiming[i] / 100U) % 10;
    timingDisplayMills[i][2] = (tempTiming[i] / 1000U) % 10;
    timingDisplayMills[i][1] = (tempTiming[i] / 10000U) % 10;
    timingDisplayMills[i][0] = (tempTiming[i] / 100000U) % 10;
    //tempTiming[i] = ((float)delays[i]/1000); 
}
}

//  GET TIMING ARRAY VALUSES TO SET DELAY VALUES
//  Timing array must first be created This is generated during setup
void GetDelayValuesFromTiming() {
  delays[1] = timing[1];
  for (int i = 2; i < 33; i = i + 1) {
    if(timing[i]!=0){
    delays[i] = (timing[i] - timing[i-1]);
    }
    else {
      delays[i] = timing[i];
    }
  }
}

// SETS THE TIMING ARRAY VALUES FROM DELAYS - USED FOR SEQUENCING
void GetTimingValuesFromDelays() {
  unsigned long previ = 0;
  timing[1] = delays[1];
  previ = timing[1];
  for (int i = 2; i < 33; i = i + 1) {
    if(delays[i]==0){
      timing[i]=0;
    }else{
    timing[i] = delays[i] + previ;
    previ = timing[i];
    }
  }
}

void GetTimingValuesFromCommonDelays() {
//  Serial.print(" Number of Common Cues = ");Serial.println(numOfCommonCues);
  unsigned long previ = 0;
  timing[1] = delays[0];
  previ = timing[1];
  for (int i = 2; i < 33; i = i + 1) {
    if (i-1 < numOfCommonCues) {
      timing[i] = delays[0] + previ;
      previ = timing[i];
//  Serial.println("i-1 < numOfCommonCues");
//  Serial.print(" i = ");Serial.println(i);
//  Serial.print(" Timing = ");Serial.println(timing[i]);
    }
    else {
      timing[i] = previ;
//  Serial.print(" i = ");Serial.println(i);
//  Serial.print(" Timing = ");Serial.println(timing[i]);
    }
  }
}

void GetTimingValuesFromSequence(){
  unsigned long PreviTime = 0;
  int counter=1; // timing array for delays start at 1.  sequence4 starts at 0
  for (int i = 0; i < 64; i = i + 1) {
        if (sequence[i][0] == 0 && sequence[i][1] == cue1 && sequence[i][2] == LOW) {
        timing[i] = 0;
        counter++;
      }
    else if(sequence[i][1] == cue32 && sequence[i][0] == timing[counter-1] && sequence[i][2] == LOW){
        timing[counter] = 0;
    }
    else if(sequence[i][0] > 0 && sequence[i][2] == LOW) {
        timing[counter] = (sequence[i][0]);
        counter++;
      }
    else{
      timing[i] = 0;
    }
  }
}

void GetTimingValuesForFireAll(){
// starts first cue after the start delay time, then adds a .001 second between each cue. This is to overcome logic in the sequence sorting.
  timing[1] = delays[0];
  for (int i = 2; i < 35; i = i + 1) {
        timing[i] = timing[i-1]+1;
  }
}

void allCuesHigh() {
  digitalWrite(cue1, HIGH); digitalWrite(cue2, HIGH); digitalWrite(cue3, HIGH); digitalWrite(cue4, HIGH); digitalWrite(cue5, HIGH);
  digitalWrite(cue6, HIGH); digitalWrite(cue7, HIGH); digitalWrite(cue8, HIGH); digitalWrite(cue9, HIGH); digitalWrite(cue10, HIGH);
  digitalWrite(cue11, HIGH); digitalWrite(cue12, HIGH); digitalWrite(cue13, HIGH); digitalWrite(cue14, HIGH); digitalWrite(cue15, HIGH);
  digitalWrite(cue16, HIGH); digitalWrite(cue17, HIGH); digitalWrite(cue18, HIGH); digitalWrite(cue19, HIGH); digitalWrite(cue20, HIGH);
  digitalWrite(cue21, HIGH); digitalWrite(cue22, HIGH); digitalWrite(cue23, HIGH); digitalWrite(cue24, HIGH); digitalWrite(cue25, HIGH);
  digitalWrite(cue26, HIGH); digitalWrite(cue27, HIGH); digitalWrite(cue28, HIGH); digitalWrite(cue29, HIGH); digitalWrite(cue30, HIGH);
  digitalWrite(cue31, HIGH); digitalWrite(cue32, HIGH);
}

void allCuesLow() {
  digitalWrite(cue1, LOW); digitalWrite(cue2, LOW); digitalWrite(cue3, LOW); digitalWrite(cue4, LOW); digitalWrite(cue5, LOW);
  digitalWrite(cue6, LOW); digitalWrite(cue7, LOW); digitalWrite(cue8, LOW); digitalWrite(cue9, LOW); digitalWrite(cue10, LOW);
  digitalWrite(cue11, LOW); digitalWrite(cue12, LOW); digitalWrite(cue13, LOW); digitalWrite(cue14, LOW); digitalWrite(cue15, LOW);
  digitalWrite(cue16, LOW); digitalWrite(cue17, LOW); digitalWrite(cue18, LOW); digitalWrite(cue19, LOW); digitalWrite(cue20, LOW);
  digitalWrite(cue21, LOW); digitalWrite(cue22, LOW); digitalWrite(cue23, LOW); digitalWrite(cue24, LOW); digitalWrite(cue25, LOW);
  digitalWrite(cue26, LOW); digitalWrite(cue27, LOW); digitalWrite(cue28, LOW); digitalWrite(cue29, LOW); digitalWrite(cue30, LOW);
  digitalWrite(cue31, LOW); digitalWrite(cue32, LOW);
}



// CREATE SEQUENCE FROM TIMING
// Timing Array should be set to desire timing before calling this function
void CreateSequenceFromTiming() {
  int S=2;
  unsigned long AccruTiming = 0;
  sequence[0][0] = timing[1];                sequence[0][1] = cueTable[1]; sequence [0][2] = LOW;
  sequence[1][0] = timing[1] + delays[33];   sequence[1][1] = cueTable[1]; sequence [1][2] = HIGH;
  for (int i = 2; i < 32; i++) {
  if (timing[i] != timing[i-1] && timing[i] != 0) {if(AccruTiming<timing[i]){AccruTiming = timing[i];}
    sequence[S][0] = timing[i];                sequence[S][1] = cueTable[i]; sequence [S][2] = LOW;
    sequence[S+1][0] = timing[i] + delays[33];   sequence[S+1][1] = cueTable[i]; sequence [S+1][2] = HIGH;
  } else {
    sequence[S][0] = timing[i];   sequence[S][1] = cueTable[0]; sequence [S][2] = LOW;
    sequence[S+1][0] = timing[i];   sequence[S+1][1] = cueTable[0]; sequence [S+1][2] = LOW;
  }
  S=S+2;
  }
  if (timing[32] != timing[31] && timing[32] != 0) {if(AccruTiming<timing[32]){AccruTiming = timing[32];}
  sequence[62][0] = timing[32];              sequence[62][1] = cueTable[32]; sequence [62][2] = LOW;
  sequence[63][0] = timing[32] + delays[33]; sequence[63][1] = cueTable[32]; sequence [63][2] = HIGH;
  } else {
  sequence[62][0] = AccruTiming;              sequence[62][1] = cueTable[32]; sequence [62][2] = LOW;
  sequence[63][0] = AccruTiming + delays[33]; sequence[63][1] = cueTable[32]; sequence [63][2] = HIGH;
  }
  
 // THIS WILL SORT THE ARRAY JUST CREATED
  for (uint8_t c = 0; c < 64 - 1; c++) { // do a bubble sort c < Depth of array
    for (uint8_t d = 0; d < ( 64 - (c + 1)); d++) {        // set d < Depth of array
      if (sequence[d][0] > sequence[d + 1][0]) { // compare d location in the coloumn
        for (uint8_t e = 0; e < 3; e++) {  // swap both values & index - set e < the width of the array
          unsigned long tempval = sequence[d][e];
          sequence[d][e] = sequence[d + 1][e];
          sequence[d + 1][e] = tempval;
        }
      }
    }
  }
}

void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}


// ##### These functions are for troubleshooting #####
void printDisplayMills() {Serial.println("Display Mills group ");  
  for (int i = 0; i < 35; i++) {Serial.print("Display Mills ");Serial.print(i);Serial.print(" = ");Serial.print(displayMills[i][0]);Serial.print(displayMills[i][1]);Serial.print(displayMills[i][2]);Serial.print(".");Serial.print(displayMills[i][3]);Serial.print(displayMills[i][4]);Serial.println(displayMills[i][5]);
  }
}

void printTimingDisplayMills() {Serial.println("Timing Display Mills group ");
  for (int i = 0; i < 35; i++) {Serial.print("Timing Display Mills ");Serial.print(i);Serial.print(" = ");Serial.print(timingDisplayMills[i][0]);Serial.print(timingDisplayMills[i][1]);Serial.print(timingDisplayMills[i][2]);Serial.print(".");Serial.print(timingDisplayMills[i][3]);Serial.print(timingDisplayMills[i][4]);Serial.println(timingDisplayMills[i][5]);
  }
}

void printTempDelays() {Serial.println("TempDelay Group");
  for (int i = 0; i < 35; i++) {Serial.print("TempDelay ");Serial.print(i);Serial.print(" = ");Serial.println(tempDelays[i]);
  }
}

void printDelays() {Serial.println("Delays Group");
  for (int i = 0; i < 35; i++) {Serial.print("Delay ");Serial.print(i);Serial.print(" = ");Serial.println(delays[i]);
  }
}

void printTiming() {Serial.println("Timing Group");
  for (int i = 0; i < 35; i++) {Serial.print("Timing ");Serial.print(i);Serial.print(" = ");Serial.println(timing[i]);
  }
}

void printSequence() {Serial.println("Sequence Group");
  for (int i = 0; i < 64; i++) {Serial.print("Sequence Time = ");Serial.print(sequence[i][0]);Serial.print(", Pin Number = ");Serial.print(sequence[i][1]);Serial.print(", Pin State (low is fire) = ");Serial.println(sequence[i][2]);
  }
}

//********************************************************* SETUP **************************************************
void setup() {
  pinMode(13, INPUT_PULLUP); // Signal In pin
  pinMode(10, INPUT_PULLUP); // Armed switch
  pinMode(9, INPUT);  // Manual Fire Button
  pinMode(eStopPin, INPUT); // eStop Pin Remote button D
  pinMode(RadioChannelA, INPUT); // RadioChannelA Pin
  pinMode(RadioChannelB, INPUT); //  RadioChannelB Pin
  pinMode(RadioChannelC, INPUT); //  RadioChannelC Pin

  pinMode(cue1, OUTPUT);  digitalWrite(cue1, HIGH);  cueTable[1]=cue1;
  pinMode(cue2, OUTPUT);  digitalWrite(cue2, HIGH);  cueTable[2]=cue2;
  pinMode(cue3, OUTPUT);  digitalWrite(cue3, HIGH);  cueTable[3]=cue3;
  pinMode(cue4, OUTPUT);  digitalWrite(cue4, HIGH);  cueTable[4]=cue4;
  pinMode(cue5, OUTPUT);  digitalWrite(cue5, HIGH);  cueTable[5]=cue5;
  pinMode(cue6, OUTPUT);  digitalWrite(cue6, HIGH);  cueTable[6]=cue6;
  pinMode(cue7, OUTPUT);  digitalWrite(cue7, HIGH);  cueTable[7]=cue7;
  pinMode(cue8, OUTPUT);  digitalWrite(cue8, HIGH);  cueTable[8]=cue8;
  pinMode(cue9, OUTPUT);  digitalWrite(cue9, HIGH);  cueTable[9]=cue9;
  pinMode(cue10, OUTPUT); digitalWrite(cue10, HIGH);  cueTable[10]=cue10;
  pinMode(cue11, OUTPUT); digitalWrite(cue11, HIGH);  cueTable[11]=cue11;
  pinMode(cue12, OUTPUT); digitalWrite(cue12, HIGH);  cueTable[12]=cue12;
  pinMode(cue13, OUTPUT); digitalWrite(cue13, HIGH);  cueTable[13]=cue13;
  pinMode(cue14, OUTPUT); digitalWrite(cue14, HIGH);  cueTable[14]=cue14;
  pinMode(cue15, OUTPUT); digitalWrite(cue15, HIGH);  cueTable[15]=cue15;
  pinMode(cue16, OUTPUT); digitalWrite(cue16, HIGH);  cueTable[16]=cue16;
  pinMode(cue17, OUTPUT); digitalWrite(cue17, HIGH);  cueTable[17]=cue17;
  pinMode(cue18, OUTPUT); digitalWrite(cue18, HIGH);  cueTable[18]=cue18;
  pinMode(cue19, OUTPUT); digitalWrite(cue19, HIGH);  cueTable[19]=cue19;
  pinMode(cue20, OUTPUT); digitalWrite(cue20, HIGH);  cueTable[20]=cue20;
  pinMode(cue21, OUTPUT); digitalWrite(cue21, HIGH);  cueTable[21]=cue21;
  pinMode(cue22, OUTPUT); digitalWrite(cue22, HIGH);  cueTable[22]=cue22;
  pinMode(cue23, OUTPUT); digitalWrite(cue23, HIGH);  cueTable[23]=cue23;
  pinMode(cue24, OUTPUT); digitalWrite(cue24, HIGH);  cueTable[24]=cue24;
  pinMode(cue25, OUTPUT); digitalWrite(cue25, HIGH);  cueTable[25]=cue25;
  pinMode(cue26, OUTPUT); digitalWrite(cue26, HIGH);  cueTable[26]=cue26;
  pinMode(cue27, OUTPUT); digitalWrite(cue27, HIGH);  cueTable[27]=cue27;
  pinMode(cue28, OUTPUT); digitalWrite(cue28, HIGH);  cueTable[28]=cue28;
  pinMode(cue29, OUTPUT); digitalWrite(cue29, HIGH);  cueTable[29]=cue29;
  pinMode(cue30, OUTPUT); digitalWrite(cue30, HIGH);  cueTable[30]=cue30;
  pinMode(cue31, OUTPUT); digitalWrite(cue31, HIGH);  cueTable[31]=cue31;
  pinMode(cue32, OUTPUT); digitalWrite(cue32, HIGH);  cueTable[32]=cue32;
  cueTable[0]=NoCue;
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);

  remoteChannels[0]=0; // seros out for when no remote is selected
  remoteChannels[1]=RadioChannelA;
  remoteChannels[2]=RadioChannelB;
  remoteChannels[3]=RadioChannelC;
  
  Serial.begin(9600);// initialize serial monitor with 9600 baud

  numOfCommonCues=((EEPROM.read(211)*10)+EEPROM.read(212));
  FireMode = EEPROM.read(214);
  Trigger = EEPROM.read(213);
  TimeType = EEPROM.read(215);
  ScreenSleep = EEPROM.read(217);
  RadioChannel = EEPROM.read(218);

if(RadioChannel==0){RadioChannelSt="NA";} 
if(RadioChannel==1){RadioChannelSt="A";} 
if(RadioChannel==2){RadioChannelSt="B";} 
if(RadioChannel==3){RadioChannelSt="C";} 
if (FireMode == 0) {FireModeSt = "COM DELY";}
if (FireMode == 1) {FireModeSt = "STEP";}
if (FireMode == 2) {FireModeSt="FIRE ALL";}
if (Trigger == 0) {TriggerSt = "SIGNAL";}
if (Trigger == 1) {TriggerSt = "MANUAL";}
if (Trigger == 2) {TriggerSt = "REMOTE";}

getEepromToDisplayMills();            // Read in the stored delay values from EEPROM
copyDisplayMillsToTempDelays();       // store the read values that were assigned to the displayMills array to Temp Delays array
copyTempDelaysToDelays();             // Convert the tempDelays array data to delays array
copyTimingToTempTiming();
copyTempTimingToTimingDisplayMills();
CreateSequenceFromTiming();           // Create final fireing sequence based on the timing array. This will include the firing time delay and sort all cues based on final sequence timing


// Initialize LCD and turn on the backlight:
lcd.init();
lcd.backlight();
backlightFlag = 1;
Serial.begin(9600);
//lcd.begin(20, 4);
//lcd.createChar(0, Bat1Empty);
//lcd.createChar(1, Bat1Half);
//lcd.createChar(2, Bat1Full);
//lcd.createChar(3, Bat2Full);
//lcd.createChar(4, Bat2Half);
//lcd.createChar(5, Bat2Empty);
//lcd.clear();        //Clears the screen and moves to 0,0
//lcd.setCursor(4, 1); lcd.print("Press Select");
//lcd.setCursor(3, 2); lcd.print("To Get Started");


lcd.clear();
//                              01234567890123456789
lcd.setCursor(0, 0); lcd.print("---INFO SCREEN--- "); if(FireMode == 0){lcd.print(numOfCommonCues);}else{lcd.print("  ");} //battLevel();
lcd.setCursor(0, 1); lcd.print("FIRE MODE = "); lcd.setCursor(12, 1); lcd.print(FireModeSt);
lcd.setCursor(0, 2); lcd.print("TRIGGER = ");  lcd.setCursor(10, 2); lcd.print(TriggerSt);
lcd.setCursor(0, 3); lcd.print("REMOTE CHANNEL =  ");lcd.setCursor(17, 3); if (Trigger == 2){lcd.print(RadioChannelSt);}else{lcd.print("NA");}

RGB_color(0, 50, 0); // Green 0, 255, 0

//  Serial.println("Initialized");
SleepTime = millis() + delays[34];

//Serial.print("Number of Common Cues = ");Serial.println(numOfCommonCues);
//Serial.print("SleepTime =");Serial.println(SleepTime);
//printDisplayMills();
//printTimingDisplayMills();
//printTempDelays();
//printDelays();
//printTiming();
//printSequence();
//delay(20000);


// ***************************************************Show Battery level***********************************************
//void battLevel() {
//  int batLvl = 75;   // Just setting this until I can build the voltage divider and read the batt level
//  //Code should be added here before going to the case
//
//
//  if (batLvl > 80) {
//    lcd.setCursor(18, 0);
//    lcd.write(0);
//    lcd.write(3);
//  }
//  else if (batLvl > 60) {
//    lcd.setCursor(18, 0);
//    lcd.write(1);
//    lcd.write(3);
//  }
//  else if (batLvl > 40) {
//    lcd.setCursor(18, 0);
//    lcd.write(2);
//    lcd.write(3);
//  }
//  else if (batLvl > 20) {
//    lcd.setCursor(18, 0);
//    lcd.write(2);
//    lcd.write(4);
//  }
//  else {
//    lcd.setCursor(18, 0);
//    lcd.write(2);
//    lcd.write(5);
//  }

}
//********************************************************* LOOP **************************************************
void loop() {
  curTime = millis();
  //   //  Serial.print("Sleep Delay"); //  Serial.print(delays[34]);//  Serial.print(", curTime"); //  Serial.print(curTime); //  Serial.print(", SleepTime = "); //  Serial.println(SleepTime);
  if (curTime >= SleepTime) {
    if (ScreenSleep == 0 ) {
      lcd.noBacklight();
      backlightFlag = 0;
    }
//    page = 0;
//    vpoz = 0;
}
//  if (digitalRead(eStopPin) == HIGH){
//    Serial.println("button D pressed");
//  }
//  if (digitalRead(RadioChannelA) == HIGH){
//    Serial.println("button C pressed");
//  } 
//  if (digitalRead(RadioChannelB) == HIGH){
//    Serial.println("button B pressed");
//  }  
//  if (digitalRead(RadioChannelC) == HIGH){
//    Serial.println("button A pressed");
//  }


  volatile unsigned char val = r.process();

  // if the encoder has been turned, check the direction
  if (val) {
    if (val == r.counterClockwise()) {
      vpoz++;
      if (vpoz > vpoz_max) {
        vpoz = vpoz_max;
      }
      vpozNew = vpoz;
    }
    else if (val == r.clockwise()) {
      vpoz--;
      if (vpoz < vpoz_min) {
        vpoz = vpoz_min;
      }
      vpozNew = vpoz;
    }
  }
  // Check to see if the button has been pressed.
  // Passes in a debounce delay of 20 milliseconds
  if (r.buttonPressedReleased(20)) {
    btn_push = true;
//    //  Serial.println("btn push was pressed on main loop");
  }
  delay(10);

  armed = digitalRead(ArmedSwitchPin); // check the armed switch
//  Serial.print("Digital Read from 10 ="); //  Serial.println(armed);
  if (armed == 1) {
    page = 4; // LAUNCH FIREWORKS PAGE
    RGB_color(150, 45, 0); // Yellow 255,255,0
    ReturnToPageFlag = true;
    SelectKey = 0;
    btn_push = false;
    lcd.clear();
    lcd.setCursor(0, 1);
    //       01234567890123456789
    lcd.print("     FINALIZING     ");
    lcd.setCursor(0, 2);
    lcd.print("   LAUNCH SEQUENCE  ");
  }

  if (ReturnToPageFlag == true) {
    SleepTime = millis() + delays[34];
    ReturnToPageFlag = false;;
    goto ReturnToPage;
  }

  if (val == r.clockwise() || val == r.counterClockwise() || btn_push == true) {
    SleepTime = millis() + delays[34];
    if (btn_push == true) {//enter selected menu
      SelectKey++;
      delay(10);
      if (SelectKey > SelectKey_max) {
        SelectKey = SelectKey_max;
      }
    }
    // page select

ReturnToPage:
    if ( backlightFlag == 0) {
      lcd.backlight();
      backlightFlag = 1;
    }

    switch (page) {

      case 0:
        page0();
        break;
      case 1:
        page1();
        break;
      case 11:
        page11();
        break;
      case 111:
        page111();
        break;
      case 112:
        page112();
        break;
      case 114:
        page114();
        break;
      case 1111: //set time page
        page1111();
        break;
      case 1112: //set time page
        page1112();
        break;
      case 1120:
        page1120();
        break;
      case 1121:
        page1121();
        break;
      case 1122:
        page1122();
        break;
      case 1123:
        page1123();
        break;
      case 1124:
        page1124();
        break;
      case 1125:
        page1125();
        break;
      case 1126:
        page1126();
        break;
      case 1127:
        page1127();
        break;
      case 1128:
        page1128();
        break;
      case 1129:
        page1128();
        break;
      case 12:
        page12();
        break;
      case 13:
        page13();
        break;
      case 131:
        page131();
        break;
      case 1313:
        page1313();
        break;
      case 132:
        page132();
        break;
      case 133:
        page133();
        break;
      case 1311:
        page1311();
        break;
      case 1312:
        page1312();
        break;
      case 1130:
        page1130();
        break;
      case 1131:
        page1131();
        break;
      case 1132:
        page1132();
        break;
      case 1133:
        page1133();
        break;
      case 1134:
        page1134();
        break;
      case 1135:
        page1135();
        break;
      case 1136:
        page1136();
        break;
      case 1137:
        page1137();
        break;
      case 1138:
        page1138();
        break;
      case 4:
        page4();
        break;
    }
  }
  btn_push = false;
}
//
//*******************************************************Front Page 0*******************************************
void page0() {
  RGB_color(0, 50, 0); // Green 0,255,0
  vpoz_min = 0;
  vpoz_max = 0; // Set for the menu items zero base
  SelectKey_max = 1;
  lcd.clear();
  
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("---INFO SCREEN--- "); if(FireMode == 0){lcd.print(numOfCommonCues);}else{lcd.print("  ");} //battLevel();
  lcd.setCursor(0, 1); lcd.print("FIRE MODE ="); lcd.setCursor(12, 1); lcd.print(FireModeSt);
  lcd.setCursor(0, 2); lcd.print("TRIGGER = ");  lcd.setCursor(10, 2); lcd.print(TriggerSt);
  lcd.setCursor(0, 3); lcd.print("REMOTE CHANNEL =  ");lcd.setCursor(17, 3); if (Trigger == 2){lcd.print(RadioChannelSt);}else{lcd.print("NA");}

  switch (vpoz)
  {
    case 0:
      if (SelectKey == SelectKey_max) {
        SelectKey = 0;
        vpoz = 0;  page = 1;  RGB_color(0, 0, 50); // Blue 0,0,255
        lcd.clear();  page1();
      }
      break;
  }

}
//******************************************************* Page 1 *******************************************
void page1() {
  vpoz_min = 0;
  vpoz_max = 3; // Set for the menu items zero base
  SelectKey_max = 1;

  lcd.clear();

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" FIRE MODE ="); lcd.setCursor(13, 0); lcd.print(FireModeSt);
  lcd.setCursor(0, 1); lcd.print(" TRIGGER = "); lcd.setCursor(11, 1); lcd.print(TriggerSt);
  lcd.setCursor(0, 2); lcd.print(" INFO / SETUP");
  lcd.setCursor(0, 3); lcd.print(" Exit");

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*" );
      lcd.setCursor(0, 1); lcd.print(" " );
      lcd.setCursor(0, 2); lcd.print(" " );
      lcd.setCursor(0, 3); lcd.print(" " );
      if (SelectKey == SelectKey_max) {
        TempFireMode=FireMode;
        SelectKey = 0;  vpoz = 0;  page = 11; lcd.clear();  page11();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 0;  page = 12;  lcd.clear(); page12();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //  Serial.print("SelectKey from page0 case2 = ");Serial.println(SelectKey);
        TempTimeType = TimeType;
        TempScreenSleep = ScreenSleep;
        TempRadioChannel = RadioChannel;
        SelectKey = 0;  vpoz = 0; page = 13; lcd.clear();  page13();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0;  vpoz = 0; page = 0; lcd.clear();  page0();
      }
      break;
  }
}
//*************************************************page 11*****************************************
void page11() {
  //  Serial.println("I am on Page 11");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");
  vpoz_min = 0;
  vpoz_max = 3;
  SelectKey_max = 1;
  lcd.clear();

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" COMMON DELAY");
  lcd.setCursor(0, 1); lcd.print(" STEP"); if (TempTimeType == 0) {
                                              lcd.print(" RELATIVE");
                                            } if (TimeType == 1) {
                                              lcd.print(" ABSOLUTE");
                                            }
  lcd.setCursor(0, 2); lcd.print(" FIRE ALL");
  lcd.setCursor(0, 3); lcd.print(" Exit");


  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*" );
      lcd.setCursor(0, 1); lcd.print(" " );
      lcd.setCursor(0, 2); lcd.print(" " );
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        TempNumOfCommonCues = numOfCommonCues;
        TempFireMode = 0; // Common Delay
        SelectKey = 0; vpoz = 0;  page = 111; delayPointer = 0; returnPage = 11; lcd.clear();  page111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //        copyDelaysToTempDelays(); // This is for manipulation on the next screen we change anything.
        //        copyTempDelaysToDisplayMills(); // this is for display and manipulation of individual characters on next screen
        TempFireMode = 1; // Step Delay
        if (TimeType == 0) {
          SelectKey = 0; vpoz = 0;  page = 1120; lcd.clear(); returnPage = 11, page1120();
        }
        if (TimeType == 1) {
          SelectKey = 0; vpoz = 0;  page = 1130; lcd.clear(); returnPage = 11, page1130();
        }
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 0;  page = 114; lcd.clear();  page114();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear();  page0();
      }
      break;
  }
}
//************************************************* page 111 *****************************************
void page111() {
  vpoz_min = 0;
  vpoz_max = 2;
  SelectKey_max = 1;
  lcd.clear();

  //  Serial.print("CommonDelay = "); //  Serial.println(delays[0]);
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" -SET COMMON DELAY- ");
  lcd.setCursor(5, 1); lcd.print(displayMills[delayPointer][0]); lcd.print(displayMills[delayPointer][1]); lcd.print(displayMills[delayPointer][2]);
  lcd.print("."); lcd.print(displayMills[delayPointer][3]); lcd.print(displayMills[delayPointer][4]); lcd.print(displayMills[delayPointer][5]);
  lcd.setCursor(0, 2); lcd.print(" Exit");
  lcd.setCursor(0, 3); lcd.print(" Next");

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0,  1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = " -SET COMMON DELAY- ";
        TempNumOfCommonCues = numOfCommonCues;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 112; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        copyDelaysToTempDelays();
        copyTempDelaysToDisplayMills();
        TempFireMode=FireMode;
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear();  page0();
      }
      break;
    case 2:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
//printDisplayMills();
//printTimingDisplayMills();
//printTempDelays();
//printDelays();
//printTiming();
//printSequence();       
        SelectKey = 0; vpoz = 0;  page = 112; lcd.clear();  page112();
      }
      break;
  }
}
//************************************************* page 112 *****************************************
void page112() {
  vpoz_min = 0;
  vpoz_max = 2;
  SelectKey_max = 1;
  lcd.clear();

  //  Serial.print("CommonDelay = "); //  Serial.println(delays[0]);
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("-SET NUMBER OF CUES-");
  lcd.setCursor(9, 1); lcd.print(TempNumOfCommonCues);
  lcd.setCursor(0, 2); lcd.print(" Exit");
  lcd.setCursor(0, 3); lcd.print(" Save");

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        TempNumOfCommonCues = ChangeValue(TempNumOfCommonCues, 9, 1);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 1:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear();  page0();
      }
      break;
    case 2:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        numOfCommonCues = TempNumOfCommonCues;
        TempLeft = (numOfCommonCues / 10U) % 10;
        TempRight = (numOfCommonCues / 1U) % 10;
        EEPROM.update(211,TempLeft);
        EEPROM.update(212,TempRight);
//  Serial.print("TempFireMode = ");Serial.println(TempFireMode);
        FireMode=TempFireMode;
//  Serial.print("FireMode = ");Serial.println(FireMode);
        EEPROM.update(214, FireMode);
        FireModeSt = "COM DELY";
        updateEepromWithDisplayMills();
        copyDisplayMillsToTempDelays();   // store the read values that were assigned to the displayMills array to Temp Delays array
        copyTempDelaysToDelays();         // Convert the tempDelays array data to delays array
        GetTimingValuesFromCommonDelays();  // create a timing based on the delays array data
        CreateSequenceFromTiming();       // Create final fireing sequence based on the timing array. This will include the firing time delay and sort all cues based on final sequence timing
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear();  page0();
      }
      break;
  }
}

//************************************************* page 114 *****************************************
void page114() {
  vpoz_min = 0;
  vpoz_max = 1;
  SelectKey_max = 1;

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("---- FIRE ALL ----  ");
  lcd.setCursor(0, 1); lcd.print("This will fire all  ");
  lcd.setCursor(0, 2); lcd.print("cues at same time   ");
  lcd.setCursor(0, 3); lcd.print(" Exit           Save");
  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 3); lcd.print("*Exit           Save");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear(); page0();
      }
      break;
    case 1:
      lcd.setCursor(0, 3); lcd.print(" Exit          *Save");
      if (SelectKey == SelectKey_max) {
        FireMode = 2; // ALL
        EEPROM.update(214,FireMode); //Fire sequnce for this will be calculated when the system is armed. forcing update to timing delays tables would mess up the conversion between relative and absolute times.
        FireModeSt = "FIRE ALL";
        // I DO NOT SET THE FIRE ALL TIMING AND SEQUENCING HERE BECUASE IT WOULD MESS UP THE CONVERSION FROM TIMING RELATIVE TO ABSOLUTE AND ABSOLUTE TO RELATIVE
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear(); page0();
      }
      break;
     }
}

//********************************************Page 1111 *****************************************
void page1111() {
  // Set TempDelay and title before calling this void function
  vpoz_min = 0;
  vpoz_max = 7;
  SelectKey_max = 1;
  lcd.setCursor(0, 0); lcd.print(title);
  lcd.setCursor(0, 3); lcd.print(" Exit           Save");

  // place the delay characters into the DisplayMills array
  lcd.setCursor(7, 2); lcd.print(displayMills[delayPointer][0]); lcd.print(displayMills[delayPointer][1]); lcd.print(displayMills[delayPointer][2]);
  lcd.print("."); lcd.print(displayMills[delayPointer][3]); lcd.print(displayMills[delayPointer][4]); lcd.print(displayMills[delayPointer][5]);

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(7, 2); lcd.cursor();
      //  Serial.println("page 1111 case 0");
      if (SelectKey == 1) {
        ChangeDigit(0, 7, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 1:
      lcd.setCursor(8, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(1, 8, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 2:
      lcd.setCursor(9, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(2, 9, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 3:
      lcd.setCursor(11, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(3, 11, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 4:
      lcd.setCursor(12, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(4, 12, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 5:
      lcd.setCursor(13, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(5, 13, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 6: // Exit
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
//  Serial.print("tempDelays[delayPointer]=");Serial.println(tempDelays[delayPointer]);
//  Serial.print("TempDelay=");Serial.println(TempDelay);  
//        tempDelays[delayPointer]=TempDelay;
        SelectKey = 0; vpoz = 0;  page = returnPage ; lcd.noCursor(); lcd.clear(); ReturnToPageFlag = true;
      }
      break;
    case 7: //Save
      lcd.setCursor(15, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        copyDisplayMillsToTempDelays();

        SelectKey = 0; vpoz = returnVpoz;  page = returnPage; lcd.noCursor(); lcd.clear(); ReturnToPageFlag = true;
      }
      break;
  }
}

// ****************************************** ChangeValue **********************************
int ChangeValue(int Variable, int Col, int Row) {
  delay(100); //This is to let encoder feedback settle down
  unsigned long startMillis;  //some global variables available anywhere in the program
  unsigned long currentMillis;
  const unsigned long period = 2000;  //the value is a number of milliseconds
  int counter = 0; btn_push = false; String st = ""; SelectKey = 0;

  //lcd.noCursor() ;
  // //  Serial.print("btn_push == "); //  Serial.println(btn_push);
  counter = Variable;
  while (btn_push == false) {
    //  Serial.println("I am in the loop");
    startMillis = millis();  //initial start time
    do {
      unsigned char val = r.process();
      // if the encoder has been turned, check the direction
      if (val == DIR_CW) {
        // //  Serial.print("val = "); //  Serial.println(val);
        delay(100);
        counter = (counter + 1);
        // //  Serial.print("counter = "); //  Serial.println(counter);
        if (counter > 32) {
          counter = 0;
        }
      }
      else if (val == DIR_CCW) {
        // //  Serial.print("val = "); //  Serial.println(val);
        delay(100);
        counter = (counter - 1);
        // //  Serial.print("counter = "); //  Serial.println(counter);
        if (counter < 0) {
          counter = 32;
        }
      }
      if (r.buttonPressedReleased(1)) {
        btn_push = true;
        delay(100);
      }
      delay(5);
      currentMillis = millis();
    } while (currentMillis - startMillis >= period); // do while condition
    lcd.setCursor(Col, Row);
    if(counter <= 9){
      lcd.print(" "); lcd.print(counter); lcd.setCursor(Col, Row);
    }else{
      lcd.print(counter); lcd.setCursor(Col, Row);
    }
  }
  //lcd.cursor();
  btn_push = false;
  return counter;
}
// ****************************************** Change Digit UP / Down **********************************
void ChangeDigit(int DMPos, int Col, int Row) {
  //  Serial.println("I am on page Up / Down ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.print("Decimal Position is = "); //  Serial.println(DMPos);
  //  Serial.print("Col = "); //  Serial.println(Col);
  //  Serial.print("Row = "); //  Serial.println(Row);
  //  Serial.print("delayPointer = "); //  Serial.print(delayPointer);
  delay(100); //This is to let encoder feedback settle down
  unsigned long startMillis;  //some global variables available anywhere in the program
  unsigned long currentMillis;
  const unsigned long period = 2000;  //the value is a number of milliseconds
  int counter = 0; btn_push = false; String st = ""; SelectKey = 0;
//  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
//  Serial.print("btn_push = "); //  Serial.println(btn_push);

  //lcd.noCursor() ;
//  Serial.print("btn_push == "); //  Serial.println(btn_push);
  if(TempDelay==0 || TempDelay == 33 || TempDelay ==34){
    counter = displayMills[delayPointer][DMPos];
  }
  if(TimeType==0){
    counter = displayMills[delayPointer][DMPos];
  }
  if(TimeType==1){
    counter = timingDisplayMills[delayPointer][DMPos];
  }

  while (btn_push == false) {
    //  Serial.println("I am in the loop");
    startMillis = millis();  //initial start time
    do {
      unsigned char val = r.process();
      // if the encoder has been turned, check the direction
      if (val == DIR_CW) {
//        //  Serial.print(val);
        delay(100);
        counter = (counter + 1);
//        //  Serial.print("counter = "); //  Serial.println(counter);
        if (counter > 9) {
          counter = 9;
        }
      }
      else if (val == DIR_CCW) {
//        //  Serial.print(val);
        delay(100);
        counter = (counter - 1);
//        //  Serial.print("counter = "); //  Serial.println(counter);
        if (counter < 0) {
          counter = 0;
        }
      }
      if (r.buttonPressedReleased(1)) {
        btn_push = true;
        delay(100);

        if(TempDelay==0 || TempDelay == 33 || TempDelay ==34){
          displayMills[delayPointer][DMPos] = counter;
        }
        if(TimeType==0){
          displayMills[delayPointer][DMPos] = counter;
        }
        if(TimeType==1){
          timingDisplayMills[delayPointer][DMPos] = counter;
        }

//        //  Serial.print("displayMills[delayPointer][DMPos] = "); //  Serial.println(displayMills[delayPointer][DMPos]);
//        //  Serial.print("delayPointer = "); //  Serial.print(delayPointer);
//        //  Serial.print("displayMills[delayPointer] = "); //  Serial.print(displayMills[delayPointer][0]); //  Serial.print(displayMills[delayPointer][1]); //  Serial.print(displayMills[delayPointer][2]);
//        //  Serial.print(displayMills[delayPointer][3]); //  Serial.print(displayMills[delayPointer][4]); //  Serial.println(displayMills[delayPointer][5]);
      } 
      delay(5);
      currentMillis = millis();
    } while (currentMillis - startMillis >= period); // do while condition
    lcd.print(counter); lcd.setCursor(Col, Row);
  }
  //lcd.cursor();
  btn_push = false;
  if(TimeType==0){
    copyDisplayMillsToTempDelays();
  }
  if(TimeType==1){
    copyTimingDisplayMillsToTempTiming();
  }
  //st = displayMills[delayPointer][0];  st = st + displayMills[delayPointer][1]; st = st + displayMills[delayPointer][2];
  //st = st + displayMills[delayPointer][3]; st = st + displayMills[delayPointer][4]; st = st + displayMills[delayPointer][5];
  //TempDelay=st.toInt();

  return;
}
//************************************************* page 1120 *****************************************
void page1120() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Delay 1 ="); lcd.setCursor(11, 0); lcd.print(displayMills[1][0]); lcd.print(displayMills[1][1]); lcd.print(displayMills[1][2]);
  lcd.print("."); lcd.print(displayMills[1][3]); lcd.print(displayMills[1][4]); lcd.print(displayMills[1][5]);
  lcd.setCursor(0, 1); lcd.print(" Delay 2 ="); lcd.setCursor(11, 1); lcd.print(displayMills[2][0]); lcd.print(displayMills[2][1]); lcd.print(displayMills[2][2]);
  lcd.print("."); lcd.print(displayMills[2][3]); lcd.print(displayMills[2][4]); lcd.print(displayMills[2][5]);
  lcd.setCursor(0, 2); lcd.print(" Delay 3 ="); lcd.setCursor(11, 2); lcd.print(displayMills[3][0]); lcd.print(displayMills[3][1]); lcd.print(displayMills[3][2]);
  lcd.print("."); lcd.print(displayMills[3][3]); lcd.print(displayMills[3][4]); lcd.print(displayMills[3][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 4 ="); lcd.setCursor(11, 3); lcd.print(displayMills[4][0]); lcd.print(displayMills[4][1]); lcd.print(displayMills[4][2]);
  lcd.print("."); lcd.print(displayMills[4][3]); lcd.print(displayMills[4][4]); lcd.print(displayMills[4][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET DELAY 1 -  ";
        delayPointer = 1;
        TempDelay = tempDelays[delayPointer];
//  Serial.print("tempDelays[delayPointer]=");Serial.println(tempDelays[delayPointer]);
//  Serial.print("TempDelay=");Serial.println(TempDelay);
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1120; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 2 -  ";
        delayPointer = 2;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1120; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 3 -  ";
        delayPointer = 3;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1120; returnVpoz = 2; lcd.clear(); page1111();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 4 -  ";
        delayPointer = 4;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1120; returnVpoz = 3; lcd.clear(); page1111();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1121; lcd.clear();  page1121();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1128; lcd.clear();  page1128();
      break;
  }
}
//************************************************* page 1121 *****************************************
void page1121() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1121 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Delay 5 ="); lcd.setCursor(11, 0); lcd.print(displayMills[5][0]); lcd.print(displayMills[5][1]); lcd.print(displayMills[5][2]);
  lcd.print("."); lcd.print(displayMills[5][3]); lcd.print(displayMills[5][4]); lcd.print(displayMills[5][5]);
  lcd.setCursor(0, 1); lcd.print(" Delay 6 ="); lcd.setCursor(11, 1); lcd.print(displayMills[6][0]); lcd.print(displayMills[6][1]); lcd.print(displayMills[6][2]);
  lcd.print("."); lcd.print(displayMills[6][3]); lcd.print(displayMills[6][4]); lcd.print(displayMills[6][5]);
  lcd.setCursor(0, 2); lcd.print(" Delay 7 ="); lcd.setCursor(11, 2); lcd.print(displayMills[7][0]); lcd.print(displayMills[7][1]); lcd.print(displayMills[7][2]);
  lcd.print("."); lcd.print(displayMills[7][3]); lcd.print(displayMills[7][4]); lcd.print(displayMills[7][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 8 ="); lcd.setCursor(11, 3); lcd.print(displayMills[8][0]); lcd.print(displayMills[8][1]); lcd.print(displayMills[8][2]);
  lcd.print("."); lcd.print(displayMills[8][3]); lcd.print(displayMills[8][4]); lcd.print(displayMills[8][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET DELAY 5 -  ";
        delayPointer = 5;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1121; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 6 -  ";
        delayPointer = 6;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1121; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 7 -  ";
        delayPointer = 7;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1121; returnVpoz = 2; lcd.clear(); page1111();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 8 -  ";
        delayPointer = 8;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1121; returnVpoz = 3; lcd.clear(); page1111();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1122; lcd.clear();  page1122();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1120; lcd.clear();  page1120();
      break;
  }
}
//************************************************* page 1122 *****************************************
void page1122() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1122 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Delay 9 ="); lcd.setCursor(11, 0); lcd.print(displayMills[9][0]); lcd.print(displayMills[9][1]); lcd.print(displayMills[9][2]);
  lcd.print("."); lcd.print(displayMills[9][3]); lcd.print(displayMills[9][4]); lcd.print(displayMills[9][5]);
  lcd.setCursor(0, 1); lcd.print(" Delay 10 ="); lcd.setCursor(12, 1); lcd.print(displayMills[10][0]); lcd.print(displayMills[10][1]); lcd.print(displayMills[10][2]);
  lcd.print("."); lcd.print(displayMills[10][3]); lcd.print(displayMills[10][4]); lcd.print(displayMills[10][5]);
  lcd.setCursor(0, 2); lcd.print(" Delay 11 ="); lcd.setCursor(12, 2); lcd.print(displayMills[11][0]); lcd.print(displayMills[11][1]); lcd.print(displayMills[11][2]);
  lcd.print("."); lcd.print(displayMills[11][3]); lcd.print(displayMills[11][4]); lcd.print(displayMills[11][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 12 ="); lcd.setCursor(12, 3); lcd.print(displayMills[12][0]); lcd.print(displayMills[12][1]); lcd.print(displayMills[12][2]);
  lcd.print("."); lcd.print(displayMills[12][3]); lcd.print(displayMills[12][4]); lcd.print(displayMills[12][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET DELAY 9 -  ";
        delayPointer = 9;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1122; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 10 -  ";
        delayPointer = 10;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1122; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 11 -  ";
        delayPointer = 11;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1122; returnVpoz = 2; lcd.clear(); page1111();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 12 -  ";
        delayPointer = 12;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1122; returnVpoz = 3; lcd.clear(); page1111();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1123; lcd.clear();  page1123();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1121; lcd.clear();  page1121();
      break;
  }
}
//************************************************* page 1123 *****************************************
void page1123() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 112 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Delay 13 ="); lcd.setCursor(12, 0); lcd.print(displayMills[13][0]); lcd.print(displayMills[13][1]); lcd.print(displayMills[13][2]);
  lcd.print("."); lcd.print(displayMills[13][3]); lcd.print(displayMills[13][4]); lcd.print(displayMills[13][5]);
  lcd.setCursor(0, 1); lcd.print(" Delay 14 ="); lcd.setCursor(12, 1); lcd.print(displayMills[14][0]); lcd.print(displayMills[14][1]); lcd.print(displayMills[14][2]);
  lcd.print("."); lcd.print(displayMills[14][3]); lcd.print(displayMills[14][4]); lcd.print(displayMills[14][5]);
  lcd.setCursor(0, 2); lcd.print(" Delay 15 ="); lcd.setCursor(12, 2); lcd.print(displayMills[15][0]); lcd.print(displayMills[15][1]); lcd.print(displayMills[15][2]);
  lcd.print("."); lcd.print(displayMills[15][3]); lcd.print(displayMills[15][4]); lcd.print(displayMills[15][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 16 ="); lcd.setCursor(12, 3); lcd.print(displayMills[16][0]); lcd.print(displayMills[16][1]); lcd.print(displayMills[16][2]);
  lcd.print("."); lcd.print(displayMills[16][3]); lcd.print(displayMills[16][4]); lcd.print(displayMills[16][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET DELAY 13 -  ";
        delayPointer = 13;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1123; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 14 -  ";
        delayPointer = 14;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1123; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 15 -  ";
        delayPointer = 15;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1123; returnVpoz = 2; lcd.clear(); page1111();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 16 -  ";
        delayPointer = 16;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1123; returnVpoz = 3; lcd.clear(); page1111();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1124; lcd.clear();  page1124();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1122; lcd.clear();  page1122();
      break;
  }
}
//************************************************* page 1124 *****************************************
void page1124() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1124 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Delay 17 ="); lcd.setCursor(12, 0); lcd.print(displayMills[17][0]); lcd.print(displayMills[17][1]); lcd.print(displayMills[17][2]);
  lcd.print("."); lcd.print(displayMills[17][3]); lcd.print(displayMills[17][4]); lcd.print(displayMills[17][5]);
  lcd.setCursor(0, 1); lcd.print(" Delay 18 ="); lcd.setCursor(12, 1); lcd.print(displayMills[18][0]); lcd.print(displayMills[18][1]); lcd.print(displayMills[18][2]);
  lcd.print("."); lcd.print(displayMills[18][3]); lcd.print(displayMills[18][4]); lcd.print(displayMills[18][5]);
  lcd.setCursor(0, 2); lcd.print(" Delay 19 ="); lcd.setCursor(12, 2); lcd.print(displayMills[19][0]); lcd.print(displayMills[19][1]); lcd.print(displayMills[19][2]);
  lcd.print("."); lcd.print(displayMills[19][3]); lcd.print(displayMills[19][4]); lcd.print(displayMills[19][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 20 ="); lcd.setCursor(12, 3); lcd.print(displayMills[20][0]); lcd.print(displayMills[20][1]); lcd.print(displayMills[20][2]);
  lcd.print("."); lcd.print(displayMills[20][3]); lcd.print(displayMills[20][4]); lcd.print(displayMills[20][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET DELAY 17 -  ";
        delayPointer = 17;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1124; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 18 -  ";
        delayPointer = 18;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1124; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 19 -  ";
        delayPointer = 19;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1124; returnVpoz = 2; lcd.clear(); page1111();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 20 -  ";
        delayPointer = 20;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1124; returnVpoz = 3; lcd.clear(); page1111();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1125; lcd.clear();  page1125();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1123; lcd.clear();  page1123();
      break;
  }
}
//************************************************* page 1125 *****************************************
void page1125() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1125 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Delay 21 ="); lcd.setCursor(12, 0); lcd.print(displayMills[21][0]); lcd.print(displayMills[21][1]); lcd.print(displayMills[21][2]);
  lcd.print("."); lcd.print(displayMills[21][3]); lcd.print(displayMills[21][4]); lcd.print(displayMills[21][5]);
  lcd.setCursor(0, 1); lcd.print(" Delay 22 ="); lcd.setCursor(12, 1); lcd.print(displayMills[22][0]); lcd.print(displayMills[22][1]); lcd.print(displayMills[22][2]);
  lcd.print("."); lcd.print(displayMills[22][3]); lcd.print(displayMills[22][4]); lcd.print(displayMills[22][5]);
  lcd.setCursor(0, 2); lcd.print(" Delay 23 ="); lcd.setCursor(12, 2); lcd.print(displayMills[23][0]); lcd.print(displayMills[23][1]); lcd.print(displayMills[23][2]);
  lcd.print("."); lcd.print(displayMills[23][3]); lcd.print(displayMills[23][4]); lcd.print(displayMills[23][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 24 ="); lcd.setCursor(12, 3); lcd.print(displayMills[24][0]); lcd.print(displayMills[24][1]); lcd.print(displayMills[24][2]);
  lcd.print("."); lcd.print(displayMills[24][3]); lcd.print(displayMills[24][4]); lcd.print(displayMills[24][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET DELAY 21 -  ";
        delayPointer = 21;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1125; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 22 -  ";
        delayPointer = 22;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1125; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 23 -  ";
        delayPointer = 23;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1125; returnVpoz = 2; lcd.clear(); page1111();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 24 -  ";
        delayPointer = 24;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1125; returnVpoz = 3; lcd.clear(); page1111();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1126; lcd.clear();  page1126();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1124; lcd.clear();  page1124();
      break;
  }
}
//************************************************* page 1126 *****************************************
void page1126() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1126 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Delay 25 ="); lcd.setCursor(12, 0); lcd.print(displayMills[25][0]); lcd.print(displayMills[25][1]); lcd.print(displayMills[25][2]);
  lcd.print("."); lcd.print(displayMills[25][3]); lcd.print(displayMills[25][4]); lcd.print(displayMills[25][5]);
  lcd.setCursor(0, 1); lcd.print(" Delay 26 ="); lcd.setCursor(12, 1); lcd.print(displayMills[26][0]); lcd.print(displayMills[26][1]); lcd.print(displayMills[26][2]);
  lcd.print("."); lcd.print(displayMills[26][3]); lcd.print(displayMills[26][4]); lcd.print(displayMills[26][5]);
  lcd.setCursor(0, 2); lcd.print(" Delay 27 ="); lcd.setCursor(12, 2); lcd.print(displayMills[27][0]); lcd.print(displayMills[27][1]); lcd.print(displayMills[27][2]);
  lcd.print("."); lcd.print(displayMills[27][3]); lcd.print(displayMills[27][4]); lcd.print(displayMills[27][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 28 ="); lcd.setCursor(12, 3); lcd.print(displayMills[28][0]); lcd.print(displayMills[28][1]); lcd.print(displayMills[28][2]);
  lcd.print("."); lcd.print(displayMills[28][3]); lcd.print(displayMills[28][4]); lcd.print(displayMills[28][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET DELAY 25 -  ";
        delayPointer = 25;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1126; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 26 -  ";
        delayPointer = 26;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1126; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 27 -  ";
        delayPointer = 27;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1126; returnVpoz = 2; lcd.clear(); page1111();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 28 -  ";
        delayPointer = 28;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1126; returnVpoz = 3; lcd.clear(); page1111();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1127; lcd.clear();  page1127();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1125; lcd.clear();  page1125();
      break;
  }
}
//************************************************* page 1127 *****************************************
void page1127() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1127 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Delay 29 ="); lcd.setCursor(12, 0); lcd.print(displayMills[29][0]); lcd.print(displayMills[29][1]); lcd.print(displayMills[29][2]);
  lcd.print("."); lcd.print(displayMills[29][3]); lcd.print(displayMills[29][4]); lcd.print(displayMills[29][5]);
  lcd.setCursor(0, 1); lcd.print(" Delay 30 ="); lcd.setCursor(12, 1); lcd.print(displayMills[30][0]); lcd.print(displayMills[30][1]); lcd.print(displayMills[30][2]);
  lcd.print("."); lcd.print(displayMills[30][3]); lcd.print(displayMills[30][4]); lcd.print(displayMills[30][5]);
  lcd.setCursor(0, 2); lcd.print(" Delay 31 ="); lcd.setCursor(12, 2); lcd.print(displayMills[31][0]); lcd.print(displayMills[31][1]); lcd.print(displayMills[31][2]);
  lcd.print("."); lcd.print(displayMills[31][3]); lcd.print(displayMills[31][4]); lcd.print(displayMills[31][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 32 ="); lcd.setCursor(12, 3); lcd.print(displayMills[32][0]); lcd.print(displayMills[32][1]); lcd.print(displayMills[32][2]);
  lcd.print("."); lcd.print(displayMills[32][3]); lcd.print(displayMills[32][4]); lcd.print(displayMills[32][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET DELAY 29 -  ";
        delayPointer = 29;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1127; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 30 -  ";
        delayPointer = 30;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1127; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 31 -  ";
        delayPointer = 31;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1127; returnVpoz = 2; lcd.clear(); page1111();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET DELAY 32 -  ";
        delayPointer = 32;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1111; returnPage = 1127; returnVpoz = 3; lcd.clear(); page1111();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1128; lcd.clear();  page1128();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1126; lcd.clear();  page1126();
      break;
  }
}
//************************************************* page 1128 *****************************************
void page1128() {
  vpoz_min = -1;
  vpoz_max = 2;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1125 ");
  //  Serial.print("page = "); //  Serial.println(page);
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("  - Save Options -  ");
  lcd.setCursor(0, 1); lcd.print("                    ");
  lcd.setCursor(0, 2); lcd.print(" SAVE and Exit      ");
  lcd.setCursor(0, 3); lcd.print(" EXIT Without Saving");

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
//  Serial.print("TempFireMode = ");Serial.print(TempFireMode);
        FireMode=TempFireMode;
//  Serial.print("FireMode = ");Serial.print(FireMode);
        EEPROM.update(214, FireMode);
        updateEepromWithDisplayMills();
        copyDisplayMillsToTempDelays();       // store the read values that were assigned to the displayMills array to Temp Delays array
        copyTempDelaysToDelays();             // Convert the tempDelays array data to delays array
        GetTimingValuesFromDelays();        // create a timing based on the delays array data
        FireModeSt = "STEP";
        CreateSequenceFromTiming();       // Create final fireing sequence based on the timing array. This will include the firing time delay and sort all cues based on final sequence timing
//printDisplayMills();
//printTimingDisplayMills();
//printTempDelays();
//printDelays();
//printTiming();
//printSequence();        
        SelectKey = 0; vpoz = 0; page = 0; lcd.clear(); page0();
      }
      break;
    case 1:
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        copyDelaysToTempDelays();
        copyTempDelaysToDisplayMills();
        TempFireMode=FireMode;
        SelectKey = 0; vpoz = 0; page = 11; lcd.clear(); page11();
      }
      break;
    case 2:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1120; lcd.clear();  page1120();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1127; lcd.clear();  page1127();
      break;
  }
}
//*************************************************page 12*****************************************
//Set Trigger Type

void page12() {
  vpoz_min = 0;
  vpoz_max = 3;
  SelectKey_max = 1;
  lcd.clear();
  lcd.home();
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("--- TRIGGER TYPE ---");
  lcd.setCursor(0, 1); lcd.print(" SIGNAL");
  lcd.setCursor(0, 2); lcd.print(" MANUAL PUSH BUTTON");
  lcd.setCursor(0, 3); lcd.print(" REMOTE");

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 1); lcd.print("*" );
      lcd.setCursor(0, 2); lcd.print(" " );
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        Trigger = 0; // Signal
        EEPROM.update(213, Trigger);
        TriggerSt = "SIGNAL";
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear();  page0();
      }
      break;
    case 1:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        Trigger = 1; // Push Button
        EEPROM.update(213, Trigger);
        TriggerSt = "MANUAL";
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear();  page0();
      }
      break;
    case 2:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        Trigger = 2; // Remote Control
        EEPROM.update(213, Trigger);
        TriggerSt = "REMOTE";
        SelectKey = 0; vpoz = 0;  page = 0; lcd.clear();  page0();
      }
      break;
  }
}
//*************************************************page 13*****************************************
void page13() {

  vpoz_min = 0;
  vpoz_max = 4;
  SelectKey_max = 1;
    lcd.clear();
  lcd.home();
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("--- INFO / SETUP ---");
  lcd.setCursor(0, 1); lcd.print(" TIME = "); 
                        if (TempTimeType == 0) {
                          lcd.print("RELATIVE");
                        } 
                        if (TempTimeType == 1) {
                          lcd.print("ABSOLUTE");
                        }
  lcd.setCursor(0, 2); lcd.print(" SLEEP DELAY = "); lcd.setCursor(15, 2); lcd.print(displayMills[34][0]); lcd.print(displayMills[34][1]); lcd.print(displayMills[34][2]);
                       // lcd.print("."); lcd.print(displayMills[34][3]); lcd.print(displayMills[34][4]); lcd.print(displayMills[34][5]);
  lcd.setCursor(0, 3);
                        if (TempScreenSleep == 0 ) {
                          lcd.print(" SCREEN OFF AT SLEEP");
                        }
                        if (TempScreenSleep == 1 ) {
                          lcd.print(" SCREEN ON AT SLEEP");
                        }

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 0;  page = 131; page131();
      }
      break;
    case 1:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //       01234567890123456789
        title = "-- SET SLEEP TIME --";
        delayPointer = 34;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 13; returnVpoz = 1; lcd.clear(); page1111();
      }
      break;
    case 2:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 0;  page = 132; returnPage = 13; returnVpoz = 2; lcd.clear();  page132();
      }
      break;
    case 3:
      SelectKey = 0; vpoz = 0; page = 1311; lcd.clear(); page1311();
      break;
  }
}

//*************************************************page 1311 *****************************************
void page1311() {

  vpoz_min = -1;
  vpoz_max = 3;
  SelectKey_max = 1;
  lcd.clear();
  lcd.home();
  //                              01234567890123456789
    lcd.setCursor(0, 0); lcd.print(" FIRE TIME = "); lcd.setCursor(13, 0); lcd.print(displayMills[33][0]); lcd.print(displayMills[33][1]); lcd.print(displayMills[33][2]);
  lcd.print("."); lcd.print(displayMills[33][3]); lcd.print(displayMills[33][4]); lcd.print(displayMills[33][5]);
  lcd.setCursor(0, 1); lcd.print(" RESET ALL DELAYS");
  lcd.setCursor(0, 2); lcd.print(" SET REMOTE CHANNEL");
  lcd.setCursor(0, 3); lcd.print(" VERSION:  "); lcd.print(Version); lcd.print("."); lcd.print(SubVersion,1);

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*" );
      lcd.setCursor(0, 1); lcd.print(" " );
      lcd.setCursor(0, 2); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //       01234567890123456789
        title = "-- SET FIRE TIME --";
        delayPointer = 33;
        TempDelay = tempDelays[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1111; returnPage = 1311; returnVpoz = 0; lcd.clear(); page1111();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 0; page = 133; returnVpoz = 2; lcd.clear(); page133();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      if (SelectKey == SelectKey_max) {
       SelectKey = 0; vpoz = 0; page = 1313; lcd.clear(); page1313();
      }
      break;
    case 3:
      SelectKey = 0; vpoz = 0; page = 1312; lcd.clear(); page1312();
      break;
    case -1:
      SelectKey = 0; vpoz = 2; page = 13; lcd.clear(); page13();
      break;
  }
}

//*************************************************page 1312 *****************************************
void page1312() {

  vpoz_min = -1;
  vpoz_max = 1;
  SelectKey_max = 1;
  lcd.clear();
  lcd.home();
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" SETUP SAVE OPTIONS ");
  lcd.setCursor(0, 1); lcd.print("                    ");
  lcd.setCursor(0, 2); lcd.print("                    ");
  lcd.setCursor(0, 3); lcd.print(" Save           Exit");

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 3); lcd.print("*");
      lcd.setCursor(15, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        EEPROM.update(215, TempTimeType);
        TimeType=TempTimeType;
        EEPROM.update(217, TempScreenSleep);
        ScreenSleep=TempScreenSleep;
        EEPROM.update(218,TempRadioChannel);
        RadioChannel=TempRadioChannel;
        if(RadioChannel==0){RadioChannelSt="NA";} 
        if(RadioChannel==1){RadioChannelSt="A";} 
        if(RadioChannel==2){RadioChannelSt="B";} 
        if(RadioChannel==3){RadioChannelSt="C";} 
        if(SetTempDelaysToZeroFlag==1){
          SetTempDelaysToZero();
          copyTempDelaysToDisplayMills();
          SetTempDelaysToZeroFlag==0;
        }
        updateEepromWithDisplayMills();
        copyDisplayMillsToTempDelays();   // store the read values that were assigned to the displayMills array to Temp Delays array
        copyTempDelaysToDelays();         // Convert the tempDelays array data to delays array
        if(FireMode==0){
          FireModeSt = "COM DELY";
          GetTimingValuesFromCommonDelays();  // create a timing based on the delays array data
        }
        if(FireMode==1){
          FireModeSt = "STEP";
          GetTimingValuesFromDelays();        // create a timing based on the delays array data
        }
        if(FireMode==2){
          FireModeSt="FIRE ALL";
          GetTimingValuesForFireAll();        // create a timing based on firing all using a .001 second delay between cues. Fires first cue after Common Delay time.
        }
        copyTimingToTempTiming();
        copyTempTimingToTimingDisplayMills();
        CreateSequenceFromTiming();       // Create final fireing sequence based on the timing array. This will include the firing time delay and sort all cues based on final sequence timing
        SelectKey = 0; vpoz = 0; page = 0; lcd.clear(); page0();
      }
      break;
    case 1:
      lcd.setCursor(0, 3); lcd.print(" ");
      lcd.setCursor(15, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        TimeType=TempTimeType;
        TempRadioChannel=RadioChannel;
        ScreenSleep=TempScreenSleep;
        copyDelaysToTempDelays();
        copyTempDelaysToDisplayMills();
        SelectKey = 0; vpoz = 0; page = 0; lcd.clear(); page0();
      }
      break;
    case -1:
      SelectKey = 0; vpoz = 2; page = 1311; lcd.clear(); page1311();
      break;
  }
}

//*************************************************page 1313 *****************************************
void page1313() {

  vpoz_min = 0;
  vpoz_max = 2;
  SelectKey_max = 1;
  lcd.clear();
  lcd.home();
  //                                01234567890123456789
    lcd.setCursor(0, 0); lcd.print("-SELECT RF CHANNEL-"); 
    lcd.setCursor(0, 1); lcd.print(" BUTTON A");
    lcd.setCursor(0, 2); lcd.print(" BUTTON B");
    lcd.setCursor(0, 3); lcd.print(" BUTTON C");

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 1); lcd.print("*" );
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        TempRadioChannel = 1;
        SelectKey = 0; vpoz = 2; page = 1311; lcd.clear(); page1311();
      }
      break;
    case 1:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        TempRadioChannel = 2;
        SelectKey = 0; vpoz = 2; page = 1311; lcd.clear(); page1311();
      }
      break;
    case 2:
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        TempRadioChannel = 3;
        SelectKey = 0; vpoz = 2; page = 1311; lcd.clear(); page1311();
      }
      break;
  }
}

//************************************************* page 131 *****************************************
void page131() {

  vpoz_min = 0;
  vpoz_max = 1;
  SelectKey_max = 1;
  lcd.clear();
  lcd.home();
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("---- TIME SETUP ----");
  lcd.setCursor(0, 2); lcd.print(" RELATIVE TIME");
  lcd.setCursor(0, 3); lcd.print(" ABSOLUTE TIME");

  switch (vpoz)
  {
    case 0:
      //                              01234567890123456789
      lcd.setCursor(0, 1); lcd.print("TIME BETWEEN CUES");
      lcd.setCursor(0, 2); lcd.print("*" );
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        TempTimeType = 0;
        SelectKey = 0; vpoz = 0;  page = 13; page13();
      }
      break;
    case 1:
      //                              01234567890123456789
      lcd.setCursor(0, 1); lcd.print("TIMELINE BASED CUES");
      lcd.setCursor(0, 2); lcd.print(" " );
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        TempTimeType = 1;
        SelectKey = 0; vpoz = 0;  page = 13; page13();
      }
      break;
  }
}

//************************************************* page 132 *****************************************
void page132() {

  vpoz_min = 0;
  vpoz_max = 1;
  SelectKey_max = 1;
  lcd.clear();
  lcd.home();
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("SCREEN OFF AT SLEEP");
  lcd.setCursor(0, 2); lcd.print(" YES");
  lcd.setCursor(0, 3); lcd.print(" NO ");

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 2); lcd.print("*" );
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        TempScreenSleep = 0; // screen blanks out on sleep timer
        SelectKey = 0; vpoz = returnVpoz;  page = 13; page13();
      }
      break;
    case 1:
      lcd.setCursor(0, 2); lcd.print(" " );
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        TempScreenSleep = 1; // screen blanks out on sleep timer
        SelectKey = 0; vpoz = returnVpoz;  page = 13; page13();
      }
      break;
  }
}
//************************************************* page 133 *****************************************
void page133() {
  vpoz_min = 0;
  vpoz_max = 1;
  SelectKey_max = 1;
  lcd.clear();
  lcd.home();
  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("  RESET ALL DELAYS  ");
  lcd.setCursor(0, 1); lcd.print("       TO ZERO      ");
  lcd.setCursor(0, 2); lcd.print(" YES");
  lcd.setCursor(0, 3); lcd.print(" NO ");

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(0, 2); lcd.print("*" );
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        SetTempDelaysToZeroFlag=1;
        SelectKey = 0; vpoz = 1;  page = 1311 ; lcd.noCursor(); lcd.clear(); page1311();
      }
      break;
    case 1:
      lcd.setCursor(0, 2); lcd.print(" " );
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 1;  page = 1311 ; lcd.noCursor(); lcd.clear(); page1311();
      }
      break;
  }
}
//############################################# LAUNCH THE FIREWORKS #############################################
//**************************************FIRE WHEN IT SEES A SIGNAL USING COMMON FIRE TIME*************************
void page4() {
  lcd.clear();
  lcd.home();
  int counter = 0;
  int LargestVal=0;
  String Message = "";

  if (FireMode == 0) {
    GetTimingValuesFromCommonDelays();
    //         01234567890123456789
    Message = " USING COMMON DELAY ";
  }
  if (FireMode == 1) {
    GetTimingValuesFromDelays();
    //         01234567890123456789
    Message = "  USING STEP DELAY  ";
  }
   if (FireMode==2){
    GetTimingValuesForFireAll();
    //         01234567890123456789
    Message = "FIRE ALL AFTER DELAY";
   }

  CreateSequenceFromTiming();
//**************************** SIGNAL FIRE *****************************
  if (Trigger == 0) {
    //                              01234567890123456789
    lcd.setCursor(0, 0); lcd.print("--- READY TO FIRE---");
    lcd.setCursor(0, 1); lcd.print(Message);
    lcd.setCursor(0, 2); lcd.print(" WAITING FOR SIGNAL ");
    lcd.setCursor(0, 3); lcd.print("-- DISARM TO STOP --");

    do { // -------------- WAITING FOR A SIGNAL -------------
      curTime = millis();
      if (curTime >= SleepTime) {
        if (ScreenSleep == 0 ) {
          lcd.noBacklight();
          backlightFlag = 0;
        }
        page = 0;
        vpoz = 0;
      }

      SignalIn = analogRead(SignalInPin); // check the sensors
      armed = digitalRead(ArmedSwitchPin); // see if we are still armed
      eStop = digitalRead(eStopPin); // check for remote eStop signal
      if (armed == 0||eStop==HIGH) {
        lcd.backlight();
        backlightFlag = 1;
        page = 0;
        page0();
        allCuesHigh(); // Turns all relays off
        curTime = millis();
        return;
      }
//                // THIS IS FOR TESTING TO SEE HOW STRONG THE VOLTAGE SIGNAL COMES IN.
//                // SET THE while (SignalIn < ### && armed == 1) //VALUE TO 1023 TO FULL READINGS AND PREVENT TRIGGERING
//                
//                  if(SignalIn>LargestVal){LargestVal=SignalIn;}
//                  counter++;
//                
//                  if(counter==2000){
//                  counter=0;
//                  Serial.println(LargestVal);          // debug value
//                  LargestVal=0;
//                  }
  
    } while (SignalIn < 800 && armed == 1); //818 SHOULD BE A 4.0 VOLT SIGNAL
//  Serial.println(SignalIn);
  launchTheFireworks();
  }
//**************************** MANUAL FIRE *****************************
  if (Trigger == 1) {
    //                              01234567890123456789
    lcd.setCursor(0, 0); lcd.print("--- READY TO FIRE---");
    lcd.setCursor(0, 1); lcd.print(Message);
    lcd.setCursor(0, 2); lcd.print("PRESS BUTTON TO FIRE");
    lcd.setCursor(0, 3); lcd.print("-- DISARM TO STOP --");
    
    do { // -------------- WAITING FOR A SIGNAL -------------
      curTime = millis();
      if (curTime >= SleepTime) {
        lcd.noBacklight();
        backlightFlag = 0;
        vpoz = 0;
      }

      btn_Fire = digitalRead(FireButtonPin); // check the sensors
      armed = digitalRead(ArmedSwitchPin); // see if we are still armed
      eStop = digitalRead(eStopPin); // check for remote eStop signal
      if (armed == 0||eStop==HIGH) {
        lcd.backlight();
        backlightFlag = 1;
        page = 0;
        page0();
        allCuesHigh(); // Turns all relays off
        curTime = millis();
        return;
      }
    } while (btn_Fire < 1 && armed == 1);
    launchTheFireworks();
  }
//**************************** REMOTE FIRE *****************************
 if (Trigger == 2) {
    //                              01234567890123456789
    lcd.setCursor(0, 0); lcd.print("--- READY TO FIRE---");
    lcd.setCursor(0, 1); lcd.print(Message);
    lcd.setCursor(0, 2); lcd.print("PRESS REMOTE BUTTON ");
    lcd.setCursor(0, 3); lcd.print("       TO FIRE      ");
    lcd.setCursor(5, 3); lcd.print(RadioChannelSt);

    do { // -------------- WAITING FOR A SIGNAL -------------
      curTime = millis();
      if (curTime >= SleepTime) {
        lcd.noBacklight();
        backlightFlag = 0;
        vpoz = 0;
      }

      SignalIn=digitalRead(remoteChannels[RadioChannel]); // check the sensors
      armed = digitalRead(ArmedSwitchPin); // see if we are still armed
      eStop = digitalRead(eStopPin); // check for remote eStop signal
      if (armed == 0||eStop==HIGH) {
        lcd.backlight();
        backlightFlag = 1;
        page = 0;
        page0();
        allCuesHigh(); // Turns all relays off
        curTime = millis();
        return;
      }
    } while (SignalIn < 1 && armed == 1);
    launchTheFireworks();
  }
}

//****************************************** LAUNCH THE FIREWORKS ****************************************
void launchTheFireworks() {
  unsigned long StartTime = millis();
  unsigned long CurTime = millis();
  unsigned long NextTime = 0;
  int CurPin = 0;
  int CurState = LOW;

    //  Serial.println("  LAUNCHING THE FIREWORKS ");
  RGB_color(50, 0, 0); // Red 255,0,0
  for ( int f = 0; f < 64; f = f + 1) {
    NextTime = sequence[f][0] + StartTime;
    CurPin = sequence[f][1];
    CurState = sequence[f][2];
    
    do{
    armed = digitalRead(ArmedSwitchPin); // see if we are still armed
    eStop = digitalRead(eStopPin); // see if we are still armed
    if (armed == 0||eStop==HIGH) {
        lcd.backlight();
        backlightFlag = 1;
        page = 0;
        page0();
        allCuesHigh(); // Turns all relays off
        curTime = millis();
        //break;
        return;    
    }
    CurTime = millis();
    }while(CurTime<NextTime);
    digitalWrite(CurPin, CurState);
    
  }
  return;
}

////****************************************** LAUNCH THE FIREWORKS ****************************************
//void launchTheFireworks() {
//  unsigned long CurTime = 0;
//  int CurPin = 0;
//  int CurState = LOW;
//  unsigned long PrevTime = 0;
//  //  Serial.println("  LAUNCHING THE FIREWORKS ");
//  RGB_color(50, 0, 0); // Red 255,0,0
//  for ( int f = 0; f < 64; f = f + 1) {
//    TempDelay = sequence[f][0] - PrevTime;
//    CurTime = sequence[f][0];
//    PrevTime = CurTime;
//    CurPin = sequence[f][1];
//    CurState = sequence[f][2];
//    //  Serial.print("Loop Number = ");
//    //  Serial.print(f);
//    //  Serial.print(", TempDelay =");
//    //  Serial.print(TempDelay);
//    //  Serial.print(", CurTime =");
//    //  Serial.print(CurTime);
//    //  Serial.print(", CurPin =");
//    //  Serial.print(CurPin);
//    //  Serial.print(", CurState =");
//    //  Serial.println(CurState);
//    delay(TempDelay);
//    armed =  digitalRead(ArmedSwitchPin); // see if we are still armed
//    if (armed == 0) {
//      //           //  Serial.println("It looks like I was disarmed");
//      //           delay(100);
//      lcd.backlight();
//      backlightFlag = 1;
//      page = 0;
//      page0();
//      allCuesHigh(); // Turns all relays off
//      curTime = millis();
//      break;
//      return;
//    }
//    digitalWrite(CurPin, CurState);
//    //          //  Serial.println("i JUST FIRED OFF A CUE");
//
//  }
//  return;
//}

//************************************************* page 1130 *****************************************
void page1130() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Time 1 ="); lcd.setCursor(11, 0); lcd.print(timingDisplayMills[1][0]); lcd.print(timingDisplayMills[1][1]); lcd.print(timingDisplayMills[1][2]);
  lcd.print("."); lcd.print(timingDisplayMills[1][3]); lcd.print(timingDisplayMills[1][4]); lcd.print(timingDisplayMills[1][5]);
  lcd.setCursor(0, 1); lcd.print(" Time 2 ="); lcd.setCursor(11, 1); lcd.print(timingDisplayMills[2][0]); lcd.print(timingDisplayMills[2][1]); lcd.print(timingDisplayMills[2][2]);
  lcd.print("."); lcd.print(timingDisplayMills[2][3]); lcd.print(timingDisplayMills[2][4]); lcd.print(timingDisplayMills[2][5]);
  lcd.setCursor(0, 2); lcd.print(" Time 3 ="); lcd.setCursor(11, 2); lcd.print(timingDisplayMills[3][0]); lcd.print(timingDisplayMills[3][1]); lcd.print(timingDisplayMills[3][2]);
  lcd.print("."); lcd.print(timingDisplayMills[3][3]); lcd.print(timingDisplayMills[3][4]); lcd.print(timingDisplayMills[3][5]);
  lcd.setCursor(0, 3); lcd.print(" Time 4 ="); lcd.setCursor(11, 3); lcd.print(timingDisplayMills[4][0]); lcd.print(timingDisplayMills[4][1]); lcd.print(timingDisplayMills[4][2]);
  lcd.print("."); lcd.print(timingDisplayMills[4][3]); lcd.print(timingDisplayMills[4][4]); lcd.print(timingDisplayMills[4][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET TIME 1 -  ";
        delayPointer = 1;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1130; returnVpoz = 0; lcd.clear(); page1112 ();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 2 -  ";
        delayPointer = 2;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1130; returnVpoz = 1; lcd.clear(); page1112 ();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 3 -  ";
        delayPointer = 3;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1130; returnVpoz = 2; lcd.clear(); page1112 ();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 4 -  ";
        delayPointer = 4;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1130; returnVpoz = 3; lcd.clear(); page1112 ();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1131; lcd.clear();  page1131();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1138; lcd.clear();  page1138();
      break;
  }
}
//************************************************* page 1131 *****************************************
void page1131() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1131 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Time 5 ="); lcd.setCursor(11, 0); lcd.print(timingDisplayMills[5][0]); lcd.print(timingDisplayMills[5][1]); lcd.print(timingDisplayMills[5][2]);
  lcd.print("."); lcd.print(timingDisplayMills[5][3]); lcd.print(timingDisplayMills[5][4]); lcd.print(timingDisplayMills[5][5]);
  lcd.setCursor(0, 1); lcd.print(" Time 6 ="); lcd.setCursor(11, 1); lcd.print(timingDisplayMills[6][0]); lcd.print(timingDisplayMills[6][1]); lcd.print(timingDisplayMills[6][2]);
  lcd.print("."); lcd.print(timingDisplayMills[6][3]); lcd.print(timingDisplayMills[6][4]); lcd.print(timingDisplayMills[6][5]);
  lcd.setCursor(0, 2); lcd.print(" Time 7 ="); lcd.setCursor(11, 2); lcd.print(timingDisplayMills[7][0]); lcd.print(timingDisplayMills[7][1]); lcd.print(timingDisplayMills[7][2]);
  lcd.print("."); lcd.print(timingDisplayMills[7][3]); lcd.print(timingDisplayMills[7][4]); lcd.print(timingDisplayMills[7][5]);
  lcd.setCursor(0, 3); lcd.print(" Time 8 ="); lcd.setCursor(11, 3); lcd.print(timingDisplayMills[8][0]); lcd.print(timingDisplayMills[8][1]); lcd.print(timingDisplayMills[8][2]);
  lcd.print("."); lcd.print(timingDisplayMills[8][3]); lcd.print(timingDisplayMills[8][4]); lcd.print(timingDisplayMills[8][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET TIME 5 -  ";
        delayPointer = 5;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1131; returnVpoz = 0; lcd.clear(); page1112 ();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 6 -  ";
        delayPointer = 6;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1131; returnVpoz = 1; lcd.clear(); page1112 ();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 7 -  ";
        delayPointer = 7;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1131; returnVpoz = 2; lcd.clear(); page1112 ();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 8 -  ";
        delayPointer = 8;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1131; returnVpoz = 3; lcd.clear(); page1112 ();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1132; lcd.clear();  page1132();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1130; lcd.clear();  page1130();
      break;
  }
}
//************************************************* page 1132 *****************************************
void page1132() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1132 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Time 9 ="); lcd.setCursor(11, 0); lcd.print(timingDisplayMills[9][0]); lcd.print(timingDisplayMills[9][1]); lcd.print(timingDisplayMills[9][2]);
  lcd.print("."); lcd.print(timingDisplayMills[9][3]); lcd.print(timingDisplayMills[9][4]); lcd.print(timingDisplayMills[9][5]);
  lcd.setCursor(0, 1); lcd.print(" Time 10 ="); lcd.setCursor(12, 1); lcd.print(timingDisplayMills[10][0]); lcd.print(timingDisplayMills[10][1]); lcd.print(timingDisplayMills[10][2]);
  lcd.print("."); lcd.print(timingDisplayMills[10][3]); lcd.print(timingDisplayMills[10][4]); lcd.print(timingDisplayMills[10][5]);
  lcd.setCursor(0, 2); lcd.print(" Time 11 ="); lcd.setCursor(12, 2); lcd.print(timingDisplayMills[11][0]); lcd.print(timingDisplayMills[11][1]); lcd.print(timingDisplayMills[11][2]);
  lcd.print("."); lcd.print(timingDisplayMills[11][3]); lcd.print(timingDisplayMills[11][4]); lcd.print(timingDisplayMills[11][5]);
  lcd.setCursor(0, 3); lcd.print(" Time 12 ="); lcd.setCursor(12, 3); lcd.print(timingDisplayMills[12][0]); lcd.print(timingDisplayMills[12][1]); lcd.print(timingDisplayMills[12][2]);
  lcd.print("."); lcd.print(timingDisplayMills[12][3]); lcd.print(timingDisplayMills[12][4]); lcd.print(timingDisplayMills[12][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET TIME 9 -  ";
        delayPointer = 9;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1132; returnVpoz = 0; lcd.clear(); page1112 ();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 10 -  ";
        delayPointer = 10;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1132; returnVpoz = 1; lcd.clear(); page1112 ();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 11 -  ";
        delayPointer = 11;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1132; returnVpoz = 2; lcd.clear(); page1112 ();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 12 -  ";
        delayPointer = 12;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1132; returnVpoz = 3; lcd.clear(); page1112 ();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1133; lcd.clear();  page1133();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1131; lcd.clear();  page1131();
      break;
  }
}
//************************************************* page 1133 *****************************************
void page1133() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1133");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Time 13 ="); lcd.setCursor(12, 0); lcd.print(timingDisplayMills[13][0]); lcd.print(timingDisplayMills[13][1]); lcd.print(timingDisplayMills[13][2]);
  lcd.print("."); lcd.print(timingDisplayMills[13][3]); lcd.print(timingDisplayMills[13][4]); lcd.print(timingDisplayMills[13][5]);
  lcd.setCursor(0, 1); lcd.print(" Time 14 ="); lcd.setCursor(12, 1); lcd.print(timingDisplayMills[14][0]); lcd.print(timingDisplayMills[14][1]); lcd.print(timingDisplayMills[14][2]);
  lcd.print("."); lcd.print(timingDisplayMills[14][3]); lcd.print(timingDisplayMills[14][4]); lcd.print(timingDisplayMills[14][5]);
  lcd.setCursor(0, 2); lcd.print(" Time 15 ="); lcd.setCursor(12, 2); lcd.print(timingDisplayMills[15][0]); lcd.print(timingDisplayMills[15][1]); lcd.print(timingDisplayMills[15][2]);
  lcd.print("."); lcd.print(timingDisplayMills[15][3]); lcd.print(timingDisplayMills[15][4]); lcd.print(timingDisplayMills[15][5]);
  lcd.setCursor(0, 3); lcd.print(" Time 16 ="); lcd.setCursor(12, 3); lcd.print(timingDisplayMills[16][0]); lcd.print(timingDisplayMills[16][1]); lcd.print(timingDisplayMills[16][2]);
  lcd.print("."); lcd.print(timingDisplayMills[16][3]); lcd.print(timingDisplayMills[16][4]); lcd.print(timingDisplayMills[16][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET TIME 13 -  ";
        delayPointer = 13;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1133; returnVpoz = 0; lcd.clear(); page1112 ();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 14 -  ";
        delayPointer = 14;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1133; returnVpoz = 1; lcd.clear(); page1112 ();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 15 -  ";
        delayPointer = 15;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1133; returnVpoz = 2; lcd.clear(); page1112 ();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 16 -  ";
        delayPointer = 16;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1133; returnVpoz = 3; lcd.clear(); page1112 ();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1134; lcd.clear();  page1134();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1132; lcd.clear();  page1132();
      break;
  }
}
//************************************************* page 1134 *****************************************
void page1134() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1134 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Time 17 ="); lcd.setCursor(12, 0); lcd.print(timingDisplayMills[17][0]); lcd.print(timingDisplayMills[17][1]); lcd.print(timingDisplayMills[17][2]);
  lcd.print("."); lcd.print(timingDisplayMills[17][3]); lcd.print(timingDisplayMills[17][4]); lcd.print(timingDisplayMills[17][5]);
  lcd.setCursor(0, 1); lcd.print(" Time 18 ="); lcd.setCursor(12, 1); lcd.print(timingDisplayMills[18][0]); lcd.print(timingDisplayMills[18][1]); lcd.print(timingDisplayMills[18][2]);
  lcd.print("."); lcd.print(timingDisplayMills[18][3]); lcd.print(timingDisplayMills[18][4]); lcd.print(timingDisplayMills[18][5]);
  lcd.setCursor(0, 2); lcd.print(" Time 19 ="); lcd.setCursor(12, 2); lcd.print(timingDisplayMills[19][0]); lcd.print(timingDisplayMills[19][1]); lcd.print(timingDisplayMills[19][2]);
  lcd.print("."); lcd.print(timingDisplayMills[18][3]); lcd.print(timingDisplayMills[19][4]); lcd.print(timingDisplayMills[19][5]);
  lcd.setCursor(0, 3); lcd.print(" Time 20 ="); lcd.setCursor(12, 3); lcd.print(timingDisplayMills[20][0]); lcd.print(timingDisplayMills[20][1]); lcd.print(timingDisplayMills[20][2]);
  lcd.print("."); lcd.print(timingDisplayMills[18][3]); lcd.print(timingDisplayMills[20][4]); lcd.print(timingDisplayMills[20][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET TIME 17 -  ";
        delayPointer = 17;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1134; returnVpoz = 0; lcd.clear(); page1112 ();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 18 -  ";
        delayPointer = 18;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1134; returnVpoz = 1; lcd.clear(); page1112 ();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 19 -  ";
        delayPointer = 19;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1134; returnVpoz = 2; lcd.clear(); page1112 ();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 20 -  ";
        delayPointer = 20;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1134; returnVpoz = 3; lcd.clear(); page1112 ();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1135; lcd.clear();  page1135();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1133; lcd.clear();  page1133();
      break;
  }
}
//************************************************* page 1135 *****************************************
void page1135() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1135 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Time 21 ="); lcd.setCursor(12, 0); lcd.print(timingDisplayMills[21][0]); lcd.print(timingDisplayMills[21][1]); lcd.print(timingDisplayMills[21][2]);
  lcd.print("."); lcd.print(timingDisplayMills[21][3]); lcd.print(timingDisplayMills[21][4]); lcd.print(timingDisplayMills[21][5]);
  lcd.setCursor(0, 1); lcd.print(" Time 22 ="); lcd.setCursor(12, 1); lcd.print(timingDisplayMills[22][0]); lcd.print(timingDisplayMills[22][1]); lcd.print(timingDisplayMills[22][2]);
  lcd.print("."); lcd.print(timingDisplayMills[22][3]); lcd.print(timingDisplayMills[22][4]); lcd.print(timingDisplayMills[22][5]);
  lcd.setCursor(0, 2); lcd.print(" Time 23 ="); lcd.setCursor(12, 2); lcd.print(timingDisplayMills[23][0]); lcd.print(timingDisplayMills[23][1]); lcd.print(timingDisplayMills[23][2]);
  lcd.print("."); lcd.print(timingDisplayMills[23][3]); lcd.print(timingDisplayMills[23][4]); lcd.print(timingDisplayMills[23][5]);
  lcd.setCursor(0, 3); lcd.print(" Delay 24 ="); lcd.setCursor(12, 3); lcd.print(timingDisplayMills[24][0]); lcd.print(timingDisplayMills[24][1]); lcd.print(timingDisplayMills[24][2]);
  lcd.print("."); lcd.print(timingDisplayMills[24][3]); lcd.print(timingDisplayMills[24][4]); lcd.print(timingDisplayMills[24][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET TIME 21 -  ";
        delayPointer = 21;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1135; returnVpoz = 0; lcd.clear(); page1112 ();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 22 -  ";
        delayPointer = 22;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1135; returnVpoz = 1; lcd.clear(); page1112 ();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 23 -  ";
        delayPointer = 23;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1135; returnVpoz = 2; lcd.clear(); page1112 ();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 24 -  ";
        delayPointer = 24;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1135; returnVpoz = 3; lcd.clear(); page1112 ();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1136; lcd.clear();  page1136();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1134; lcd.clear();  page1134();
      break;
  }
}
//************************************************* page 1136 *****************************************
void page1136() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1136 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Time 25 ="); lcd.setCursor(12, 0); lcd.print(timingDisplayMills[25][0]); lcd.print(timingDisplayMills[25][1]); lcd.print(timingDisplayMills[25][2]);
  lcd.print("."); lcd.print(timingDisplayMills[25][3]); lcd.print(timingDisplayMills[25][4]); lcd.print(timingDisplayMills[25][5]);
  lcd.setCursor(0, 1); lcd.print(" Time 26 ="); lcd.setCursor(12, 1); lcd.print(timingDisplayMills[26][0]); lcd.print(timingDisplayMills[26][1]); lcd.print(timingDisplayMills[26][2]);
  lcd.print("."); lcd.print(timingDisplayMills[26][3]); lcd.print(timingDisplayMills[26][4]); lcd.print(timingDisplayMills[26][5]);
  lcd.setCursor(0, 2); lcd.print(" Time 27 ="); lcd.setCursor(12, 2); lcd.print(timingDisplayMills[27][0]); lcd.print(timingDisplayMills[27][1]); lcd.print(timingDisplayMills[27][2]);
  lcd.print("."); lcd.print(timingDisplayMills[27][3]); lcd.print(timingDisplayMills[27][4]); lcd.print(timingDisplayMills[27][5]);
  lcd.setCursor(0, 3); lcd.print(" Time 28 ="); lcd.setCursor(12, 3); lcd.print(timingDisplayMills[28][0]); lcd.print(timingDisplayMills[28][1]); lcd.print(timingDisplayMills[28][2]);
  lcd.print("."); lcd.print(timingDisplayMills[28][3]); lcd.print(timingDisplayMills[28][4]); lcd.print(timingDisplayMills[28][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET TIME 25 -  ";
        delayPointer = 25;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1136; returnVpoz = 0; lcd.clear(); page1112 ();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 26 -  ";
        delayPointer = 26;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1136; returnVpoz = 1; lcd.clear(); page1112 ();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 27 -  ";
        delayPointer = 27;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1136; returnVpoz = 2; lcd.clear(); page1112 ();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 28 -  ";
        delayPointer = 28;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1136; returnVpoz = 3; lcd.clear(); page1112 ();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1137; lcd.clear();  page1137();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1135; lcd.clear();  page1135();
      break;
  }
}
//************************************************* page 1137 *****************************************
void page1137() {
  vpoz_min = -1;
  vpoz_max = 4;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1127 ");
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print(" Time 29 ="); lcd.setCursor(12, 0); lcd.print(timingDisplayMills[29][0]); lcd.print(timingDisplayMills[29][1]); lcd.print(timingDisplayMills[29][2]);
  lcd.print("."); lcd.print(timingDisplayMills[29][3]); lcd.print(timingDisplayMills[29][4]); lcd.print(timingDisplayMills[29][5]);
  lcd.setCursor(0, 1); lcd.print(" Time 30 ="); lcd.setCursor(12, 1); lcd.print(timingDisplayMills[30][0]); lcd.print(timingDisplayMills[30][1]); lcd.print(timingDisplayMills[30][2]);
  lcd.print("."); lcd.print(timingDisplayMills[30][3]); lcd.print(timingDisplayMills[30][4]); lcd.print(timingDisplayMills[30][5]);
  lcd.setCursor(0, 2); lcd.print(" Time 31 ="); lcd.setCursor(12, 2); lcd.print(timingDisplayMills[31][0]); lcd.print(timingDisplayMills[31][1]); lcd.print(timingDisplayMills[31][2]);
  lcd.print("."); lcd.print(timingDisplayMills[31][3]); lcd.print(timingDisplayMills[31][4]); lcd.print(timingDisplayMills[31][5]);
  lcd.setCursor(0, 3); lcd.print(" Time 32 ="); lcd.setCursor(12, 3); lcd.print(timingDisplayMills[32][0]); lcd.print(timingDisplayMills[32][1]); lcd.print(timingDisplayMills[32][2]);
  lcd.print("."); lcd.print(timingDisplayMills[32][3]); lcd.print(timingDisplayMills[32][4]); lcd.print(timingDisplayMills[32][5]);

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("*");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        //            01234567890123456789
        title = "   - SET TIME 29 -  ";
        delayPointer = 29;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1137; returnVpoz = 0; lcd.clear(); page1112 ();
      }
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("*");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 30 -  ";
        delayPointer = 30;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0; page = 1112 ; returnPage = 1137; returnVpoz = 1; lcd.clear(); page1112 ();
      }
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 31 -  ";
        delayPointer = 31;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1137; returnVpoz = 2; lcd.clear(); page1112 ();
      }
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print(" ");
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        title = "   - SET TIME 32 -  ";
        delayPointer = 32;
        TempDelay = tempTiming[delayPointer];
        SelectKey = 0; vpoz = 0;  page = 1112 ; returnPage = 1137; returnVpoz = 3; lcd.clear(); page1112 ();
      }
      break;
    case 4:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1138; lcd.clear();  page1138();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1136; lcd.clear();  page1136();
      break;
  }
}

//************************************************* page 1138 *****************************************
void page1138() {
  vpoz_min = -1;
  vpoz_max = 2;
  SelectKey_max = 1;

  //  Serial.println("I am on page 1138 ");
  //  Serial.print("page = "); //  Serial.println(page);
  //  Serial.print("vpoz = "); //  Serial.println(vpoz);
  //  Serial.print("SelectKey = "); //  Serial.println(SelectKey);
  //  Serial.print("btn_push = "); //  Serial.println(btn_push);
  //  Serial.println("");
  //  Serial.println("");

  //                              01234567890123456789
  lcd.setCursor(0, 0); lcd.print("  - Save Options -  ");
  lcd.setCursor(0, 1); lcd.print("                    ");
  lcd.setCursor(0, 2); lcd.print(" SAVE and Exit      ");
  lcd.setCursor(0, 3); lcd.print(" EXIT Without Saving");

  switch (vpoz) {
    case 0:
      lcd.setCursor(0, 2); lcd.print("*");
      lcd.setCursor(0, 3); lcd.print(" ");
      if (SelectKey == SelectKey_max) {
  Serial.print("TempFireMode = ");Serial.println(TempFireMode);
        FireMode=TempFireMode;
  Serial.print("FireMode = ");Serial.println(FireMode);
        EEPROM.update(214, FireMode);
        FireModeSt = "STEP";
        
        copyTimingDisplayMillsToTempTiming();
//      printTimingDisplayMills();
        copyTempTimingToTiming();       // We are moving the temp timing all the way to the Sequence so it can bubble sort the results Then we will move to the delays
//      printTiming();
        CreateSequenceFromTiming();
//      printSequence();
        GetTimingValuesFromSequence();  // The creation of the sequence sorts the timing bytime stap. this will rearange the times for when they are not created cronalogically and switch back to Relative time.
//      printTiming();
        GetDelayValuesFromTiming();
//      printDelays();
        copyDelaysToTempDelays();
//      printTempDelays();
        copyTempDelaysToDisplayMills();
//      printDisplayMills();
        updateEepromWithDisplayMills();
        
      //printSequence(); // Create final fireing sequence based on the timing array. This will include the firing time delay and sort all cues based on final sequence timing
      //delay(20000); 

        SelectKey = 0; vpoz = 0; page = 0; lcd.clear(); page0();
      }
      break;
    case 1:
      lcd.setCursor(0, 2); lcd.print(" ");
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
      copyTimingToTempTiming();
      copyTempTimingToTimingDisplayMills();
        TempFireMode=FireMode;
        SelectKey = 0; vpoz = 0; page = 0; lcd.clear(); page0();
      }
      break;
    case 2:
      lcd.clear();
      SelectKey = 0; vpoz = 0;  page = 1130; lcd.clear();  page1130();
      break;
    case -1:
      lcd.clear();
      SelectKey = 0; vpoz = 3;  page = 1137; lcd.clear();  page1137();
      break;
  }
}
//************************************************* 1112 *****************************************
void page1112() {
  // Set TempDelay and title before calling this void function
  vpoz_min = -1;
  vpoz_max = 7;
  SelectKey_max = 1;
  lcd.setCursor(0, 0); lcd.print(title);
  lcd.setCursor(0, 3); lcd.print(" Exit           Save");

  // place the delay characters into the timingDisplayMills array
  lcd.setCursor(7, 2); lcd.print(timingDisplayMills[delayPointer][0]); lcd.print(timingDisplayMills[delayPointer][1]); lcd.print(timingDisplayMills[delayPointer][2]);
  lcd.print("."); lcd.print(timingDisplayMills[delayPointer][3]); lcd.print(timingDisplayMills[delayPointer][4]); lcd.print(timingDisplayMills[delayPointer][5]);

  switch (vpoz)
  {
    case 0:
      lcd.setCursor(7, 2); lcd.cursor();
      //  Serial.println("page 1112 case 0");
      if (SelectKey == 1) {
        ChangeDigit(0, 7, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 1:
      lcd.setCursor(8, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(1, 8, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 2:
      lcd.setCursor(9, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(2, 9, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 3:
      lcd.setCursor(11, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(3, 11, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 4:
      lcd.setCursor(12, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(4, 12, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 5:
      lcd.setCursor(13, 2); lcd.cursor();
      if (SelectKey == 1) {
        ChangeDigit(5, 13, 2);
        SelectKey == 0;
        btn_push = false;
      }
      break;
    case 6: // Exit
      lcd.setCursor(0, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        SelectKey = 0; vpoz = 0;  page = returnPage ; lcd.noCursor(); lcd.clear();
      }
      break;
    case 7: //Save
      lcd.setCursor(15, 3); lcd.print("*");
      if (SelectKey == SelectKey_max) {
        //tempDelays[delayPointer] = TempDelay;
        //copyTempDelaysTotimingDisplayMills();
        copyTimingDisplayMillsToTempTiming();
                //  Serial.print("tempDelays 0 = "); //  Serial.println(tempDelays[0]);
                //  Serial.print("delays 0 = "); //  Serial.println(delays[0]);
                //  Serial.print("returnPage = "); //  Serial.println(returnPage);

        SelectKey = 0; vpoz = returnVpoz;  page = returnPage; lcd.noCursor(); lcd.clear(); ReturnToPageFlag = true;
      }
      break;
  }
}
