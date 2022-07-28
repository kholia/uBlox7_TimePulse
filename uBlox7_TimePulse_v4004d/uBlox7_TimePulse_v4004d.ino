#define SRCREV "uBlox7_TimePulse.vu3cer.v4004d.11b"

// Instructions:
// Unseat usb to pico, hold button down, reseat, cancel the popup
// Unseat, hold button in, seat, upload using the IDE

#include <Arduino.h>

//  https://github.com/kholia/uBlox7_TimePulse.git from Dhiru VU3CER

// ----------------------------------------------------------------------------
// uncomment lines to your taste:
//#define FORCE_COLDSTART
// ----------------------------------------------------------------------------

// https://github.com/rydepier/UBlox-NEO-7-GPS-and-Arduino/blob/master/sketch_neo7_gps_data_extracted.ino

#define USB Serial
#define GPS Serial2

void calcChecksum(byte *checksumPayload, byte payloadSize);

// cfg-rst message: reset receiver / clear backup data structures
// this message can reset the gps module with cold/warm/hot start
// the shown configuration is a cold start example
// notes                         |       |     |           |           |           |          | CRC
// description         header    | class | id  | length    | coldstart | resetMode | reserved | ck_a  ck_b
// decimal                       |       |     | 4         |           |           |          |
uint8_t cfg_rst_4[] = {0xB5, 0x62, 0x06,   0x04, 0x04, 0x00, 0xFF, 0xFF, 0x02,       0x00,      0x0E, 0x61};

void calcCRC(uint8_t* message);
void configureTimepulse(uint32_t freq, uint32_t freqLock, double pulseRatio, double pulseRatioLock);

char serialMSG[256];

void setup()
{
  USB.begin(115200);
  GPS.begin(9600);

  delay(12000);
  sprintf(serialMSG, "\r\n\n\n\n\n\n\n\n\n\n\n\n\r");
  USB.write(serialMSG, strlen(serialMSG));
  sprintf(serialMSG, "<<%s>>\n\r", SRCREV);
  USB.write(serialMSG, strlen(serialMSG));
  USB.write(serialMSG, strlen(serialMSG));
  USB.write(serialMSG, strlen(serialMSG));
  USB.write(serialMSG, strlen(serialMSG));
  sprintf(serialMSG, "\r\n\n");
  USB.write(serialMSG, strlen(serialMSG));
  delay(6000);

  /*
    WSPR Frequencies are listed in the footer of each page on this web site. I noticed these on an early visit to the site, left it a couple of months before running the program, and could not find them again!! I expected a link to these frequencies, or to a "getting started" document. Eventually a web search turned up the table below. At the time, I did not record where I found it, but subsequent stumbling around reveals I cut it from Julian Moss, G4ILO's excellent article at http://www.g4ilo.com/wspr.html which is an excellent "Getting Started" document.

    Band   Dial freq (MHz) Tx freq (MHz)

    160m 1.836600     1.838000 -   1.838200
    80m 3.592600     3.594000 -   3.594200
    60m 5.287200     5.288600 -   5.288800
    40m 7.038600     7.040000 -   7.040200
    30m 10.138700   10.140100 -  10.140300
    20m 14.095600   14.097000 -  14.097200
    17m 18.104600   18.106000 -  18.106200
    15m 21.094600   21.096000 -  21.096200
    12m 24.924600   24.926000 -  24.926200
    10m 28.124600   28.126000 -  28.126200
    6m 50.293000   50.294400 -  50.294600
    2m 144.488500 144.489900 - 144.490100
  */

#if 0
  delay(500); // let gps module boot
  //sprintf(serialMSG, "  GPS.write(cfg_rst_4, sizeof(cfg_rst_4)); <<cold start>>;\r\n\n");
  //USB.write(serialMSG, strlen(serialMSG));

  //calcChecksum(&cfg_rst_4[0],sizeof(cfg_rst_4)); // calculate the checksum
  //GPS.write(cfg_rst_4, sizeof(cfg_rst_4)); // send the coldstart message
  delay(4000);
#endif

  //#define LOCKED 9000123   // 3   //  12000000  // 10000000 // wspr 10.140100
#define   LOCKED  1420007  // 3   //  12000000  // 10000000 // wspr 10.140100
#define   NOTLOCK 1444441  //

  sprintf(serialMSG, "  configureTimepulse(NOTLOCK=%d, LOCKED=%d, 0.2, 0.7);\r\n\n", NOTLOCK, LOCKED);
  USB.write(serialMSG, strlen(serialMSG));

  configureTimepulse(NOTLOCK, LOCKED, 0.2, 0.7);
}

void loop()
{
}

// calculate the crc for the message
// the whole message is given (with header and crc bytes)
void calcChecksum(byte *checksumPayload, byte payloadSize) {
  char serialMSG[40];
  byte CK_A = 0, CK_B = 0;
  for (int i = 0; i < payloadSize ; i++) {
    CK_A = CK_A + *checksumPayload;
    CK_B = CK_B + CK_A;
    checksumPayload++;
  }
  *checksumPayload = CK_A;
  checksumPayload++;
  *checksumPayload = CK_B;

  sprintf(serialMSG, "\r\n  Checksum()  CK_A = 0x%02X  ", CK_A);
  USB.write(serialMSG, strlen(serialMSG));
  sprintf(serialMSG, "  CK_B = 0x%02X,\r\n\n", CK_B);
  USB.write(serialMSG, strlen(serialMSG));
}

// send a message to set a new frequency and locked frequency
// this does not wait for an acknowledge, nor poll the actual settings
void configureTimepulse(uint32_t freq, uint32_t freqLock, double pulseRatio, double pulseRatioLock) {
  double factor = powl(2.0, -32.0);
  pulseRatio = 1 - pulseRatio;
  uint32_t dc = (pulseRatio / factor);
  pulseRatioLock = 1 - pulseRatioLock;
  uint32_t dcLock = (pulseRatioLock / factor);

  char serialMSG[40];
  uint8_t message[40];
  message[0] =  0xB5; // header
  message[1] =  0x62; // header
  message[2] =  0x06; // class
  message[3] =  0x31; // id
  message[4] =  0x20; // length
  message[5] =  0x00; // length
  message[6] =  0x00; // time pulse selection
  message[7] =  0x01; // version
  message[8] =  0x00; // reserved
  message[9] =  0x00; // reserved
  message[10] = 0x32; // antenna cable delay (here fixed)
  message[11] = 0x00; // antenna cable delay
  message[12] = 0x00; // rf group delay (here fixed)
  message[13] = 0x00; // rf group delay
  message[14] = (freq >>  0) & 0xFF; // frequency
  message[15] = (freq >>  8) & 0xFF; // frequency
  message[16] = (freq >> 16) & 0xFF; // frequency
  message[17] = (freq >> 24) & 0xFF; // frequency
  message[18] = (freqLock >>  0) & 0xFF; // frequency on lock
  message[19] = (freqLock >>  8) & 0xFF; // frequency on lock
  message[20] = (freqLock >> 16) & 0xFF; // frequency on lock
  message[21] = (freqLock >> 24) & 0xFF; // frequency on lock
  message[22] = (dc >>  0) & 0xFF; // dutycycle
  message[23] = (dc >>  8) & 0xFF; // dutycycle
  message[24] = (dc >> 16) & 0xFF; // dutycycle
  message[25] = (dc >> 24) & 0xFF; // dutycycle
  message[26] = (dcLock >>  0) & 0xFF; // dutycycle on lock
  message[27] = (dcLock >>  8) & 0xFF; // dutycycle on lock
  message[28] = (dcLock >> 16) & 0xFF; // dutycycle on lock
  message[29] = (dcLock >> 24) & 0xFF; // dutycycle on lock
  message[30] = 0x00; // user configured delay
  message[31] = 0x00; // user configured delay
  message[32] = 0x00; // user configured delay
  message[33] = 0x00; // user configured delay
  message[34] = 0xEF; // flags
  message[35] = 0x00; // flags
  message[36] = 0x00; // flags
  message[37] = 0x00; // flags
  message[38] = 0x00; // crc will be included after calculation
  message[39] = 0x00; // crc will be included after calculation

  calcChecksum(&message[2], sizeof(message) - 4);
  GPS.write(message, sizeof(message));

  for (unsigned int i = 0; i < sizeof(message); i++) {
    sprintf(serialMSG, "0x%02X,", message[i]);
    USB.write(serialMSG, strlen(serialMSG));

    if (0 == (i + 1) % 10) {
      sprintf(serialMSG, "\r\n");
      USB.write(serialMSG, strlen(serialMSG));
    }
  }

  delay(6000);
  sprintf(serialMSG, "\n\n<<CONFIG SENT>>\n\r");
  USB.write(serialMSG, strlen(serialMSG));
  delay(6000);
  sprintf(serialMSG, "\n<<done>>\n\r");
  USB.write(serialMSG, strlen(serialMSG));
  delay(6000);
}
