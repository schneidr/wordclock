/**
  @title Wordclock
  @Version 0.3
  @Author Gerald Schneider <gerald@schneidr.de>
  @URL http://projects.schneidr.de
  @Sources
  http://sqlskills.com/blogs/paulselec/post/Arduino-cascading-shift-registers-to-drive-7-segment-displays.aspx
  http://www.docstoc.com/docs/40139593/Example-of-output
*/

// internal calculations
int mins;
int hours;
int offset;

// shift register stuff
const int g_pinCommLatch = 1;
const int g_pinData = 2;
const int g_pinClock = 3;
const int g_registers = 3;
byte g_registerArray[g_registers];

// Simple function to send serial data to one or more shift registers by iterating backwards through an array.
// Although g_registers exists, they may not all be being used, hence the input parameter.
void sendSerialData (
  byte registerCount,  // How many shift registers?
  byte *pValueArray)   // Array of bytes with LSByte in array [0]
{
  // Signal to the 595s to listen for data
  digitalWrite (g_pinCommLatch, LOW);
  
  for (byte reg = registerCount; reg > 0; reg--) {
    byte value = pValueArray [reg - 1];
    for (byte bitMask = 128; bitMask > 0; bitMask >>= 1) {
      digitalWrite(g_pinClock, LOW);
      digitalWrite(g_pinData, value & bitMask ? HIGH : LOW);
      digitalWrite(g_pinClock, HIGH);
    }
  }
  // Signal to the 595s that I'm done sending
  digitalWrite (g_pinCommLatch, HIGH);
}  // sendSerialData

// DCF77 stuff
#define DCF77 0 // analog in - DCF77 module
int DCF77value = 0; // analog value from DCF77 module
int DCF77data = 0; // 0 = low / 1 = high
int DCF77start = 0; // start high in millis
int DCF77tick = 0; // most recent in millis
int DCF77signal[60]; // array of DCF77 values (http://en.wikipedia.org/wiki/DCF77#Time_code_interpretation)
int DCF77count = 0; // count variable for array manipulation
int DCF77dw = 1; // day of week translation (e.g. 1 = Monday)

void displayTime() {
  // plausability check
  if (DCF77signal[17] != 1 && DCF77signal[18] != 1) {
    return;
  }
  if ((DCF77signal[36] * 1 + DCF77signal[37] * 2 + DCF77signal[38] * 4 + DCF77signal[39] * 8 + DCF77signal[40] * 10 + DCF77signal[41] * 20) == 0) return;
  if ((DCF77signal[45] * 1 + DCF77signal[46] * 2 + DCF77signal[47] * 4 + DCF77signal[48] * 8 + DCF77signal[49] * 10) == 0) return;
  if ((DCF77signal[50] * 1 + DCF77signal[51] * 2 + DCF77signal[52] * 4 + DCF77signal[53] * 8 + DCF77signal[54] * 10 + DCF77signal[55] * 20 + DCF77signal[56] * 40 + DCF77signal[57] * 80) == 0) return;
  // parity checks for hours and minutes
  if ((DCF77signal[29] ^ DCF77signal[30] ^ DCF77signal[31] ^ DCF77signal[32] ^ DCF77signal[33] ^ DCF77signal[34]) != DCF77signal[35]) return;
  if ((DCF77signal[21] ^ DCF77signal[22] ^ DCF77signal[23] ^ DCF77signal[24] ^ DCF77signal[25] ^ DCF77signal[26] ^ DCF77signal[27]) != DCF77signal[28]) return;
  hours = DCF77signal[29] * 1 + DCF77signal[30] * 2 + DCF77signal[31] * 4 + DCF77signal[32] * 8 + DCF77signal[33] * 10 + DCF77signal[34] * 20;
  mins = DCF77signal[21] * 1 + DCF77signal[22] * 2 + DCF77signal[23] * 4 + DCF77signal[24] * 8 + DCF77signal[25] * 10 + DCF77signal[26] * 20 + DCF77signal[27] * 40;
} // displayTime

void setup()
{
  pinMode (g_pinCommLatch, OUTPUT);
  pinMode (g_pinClock, OUTPUT);
  pinMode (g_pinData, OUTPUT);

  mins = 0;
  hours = 0;

  g_registerArray[0] = 254;
  g_registerArray[1] = 254;
  g_registerArray[2] = 254;
  sendSerialData (g_registers, g_registerArray);
  delay(3000);
  
  g_registerArray[0] = 0;
  g_registerArray[1] = 0;
  g_registerArray[2] = 0;
  sendSerialData (g_registers, g_registerArray);

} // setup

void loop()
{
  offset = 0;
  
  DCF77value = analogRead(DCF77);
  if (DCF77value >= 200) {
    if (DCF77data == 0) {
      DCF77start = millis();
      if (DCF77start - DCF77tick > 1200) {
        displayTime();
        for (DCF77count = 0; DCF77count < 60; DCF77count = DCF77count + 1) {
          DCF77signal[DCF77count] = 0;
        }
        DCF77count = 0;
      }
      else {
        if (DCF77start - DCF77tick > 850) {
          DCF77signal[DCF77count] = 0;
        }
        else {
          if (DCF77start - DCF77tick < 850) {
            if (DCF77start - DCF77tick > 650) {
              DCF77signal[DCF77count] = 1;
            }
          }
        }
        if (DCF77start - DCF77tick > 650) {
          DCF77count = DCF77count + 1;
        }
      }
    }
    DCF77data = 1;
    DCF77tick = millis();
  }
  else {
    DCF77data = 0;
  }

  g_registerArray[0] = 2;
  g_registerArray[1] = 0;
  g_registerArray[2] = 0;
  
  switch (mins) {
    case 58:
    case 59:
      offset = 1;
    case 0:
    case 1:
    case 2:
      // X Uhr
      if (hours + offset != 1 && hours + offset != 13) {
        g_registerArray[2] += 128;
      }
      break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      //5 nach X
      g_registerArray[0] += 4;
      g_registerArray[0] += 128;
      break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
      // 10 nach X
      g_registerArray[0] += 8;
      g_registerArray[0] += 128;
      break;
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
      // Viertel nach X
      g_registerArray[0] += 16;
      g_registerArray[0] += 128;
      break;
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
      // 20 nach X
      g_registerArray[0] += 32;
      g_registerArray[0] += 128;
      break;
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
      // 5 vor halb X+1
      g_registerArray[0] += 4;
      g_registerArray[1] += 8;
//      g_registerArray[0] += 64;
      g_registerArray[1] += 2;
      offset = 1;
      break;
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
      // halb X+1
      g_registerArray[1] += 2;
      offset = 1;
      break;
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
      // 5 nach halb X+1
      g_registerArray[0] += 4;
      g_registerArray[0] += 128;
      g_registerArray[1] += 2;
      offset = 1;
      break;
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
      // 20 vor X+1
      g_registerArray[0] += 32;
      g_registerArray[1] += 8;
      offset = 1;
      break;
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
      // viertel vor X+1
      g_registerArray[0] += 16;
      g_registerArray[1] += 8;
      offset = 1;
      break;
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
      // 10 vor X+1
      g_registerArray[0] += 8;
      g_registerArray[1] += 8;
      offset = 1;
      break;
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
      // 5 vor X+1
      g_registerArray[0] += 4;
      g_registerArray[1] += 8;
      offset = 1;
      break;
  }
  
  switch (hours + offset) {
    case 0:
    case 12:
    case 24:
      g_registerArray[2] += 64;
      break;
    case 1:
    case 13:
      g_registerArray[1] += 4;
      break;
    case 2:
    case 14:
      g_registerArray[0] += 64;
      break;
    case 3:
    case 15:
      g_registerArray[1] += 16;
      break;
    case 4:
    case 16:
      g_registerArray[1] += 32;
      break;
    case 5:
    case 17:
      g_registerArray[1] += 64;
      break;
    case 6:
    case 18:
      g_registerArray[1] += 128;
      break;
    case 7:
    case 19:
      g_registerArray[2] += 2;
      break;
    case 8:
    case 20:
      g_registerArray[2] += 4;
      break;
    case 9:
    case 21:
      g_registerArray[2] += 8;
      break;
    case 10:
    case 22:
      g_registerArray[2] += 16;
      break;
    case 11:
    case 23:
      g_registerArray[2] += 32;
      break;
  }
  sendSerialData (g_registers, g_registerArray);
} // loop
