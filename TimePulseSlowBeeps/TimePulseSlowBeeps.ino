#include <Arduino.h>

// ----------------------------------------------------------------------------
// uncomment lines to your taste:
//#define FORCE_COLDSTART
// ----------------------------------------------------------------------------

// https://github.com/rydepier/UBlox-NEO-7-GPS-and-Arduino/blob/master/sketch_neo7_gps_data_extracted.ino

#define USB Serial
#define GPS Serial2

// cfg-rst message: reset receiver / clear backup data structures
// this message can reset the gps module with cold/warm/hot start
// the shown configuration is a cold start example
// notes                         |       |     |           |           |           |          | CRC
// description         header    | class | id  | length    | coldstart | resetMode | reserved | ck_a  ck_b
// decimal                       |       |     | 4         |           |           |          |
uint8_t cfg_rst_4[] = {0xB5, 0x62, 0x06,   0x04, 0x04, 0x00, 0xFF, 0xFF, 0x02,       0x00,      0x0E, 0x61};

void calcCRC(uint8_t* message);
void configureTimepulse(uint32_t freq, uint32_t freqLock, double pulseRatio, double pulseRatioLock);

void setup()
{
  USB.begin(115200);
  GPS.begin(9600);

#ifdef FORCE_COLDSTART
  delay(500); // let gps module boot
  calcCRC(&cfg_rst_4[0]); // calculate the checksum
  GPS.write(cfg_rst_4, sizeof(cfg_rst_4)); // send the coldstart message
#endif

  delay(5000);
  USB.write("Sending TimePulse configuration to the GPS module...\n");

  // Configure the gps module for 12 MHz output
  while (1) {
    USB.write("Sending config 1...\n");
    configureTimepulse(5, 1000000, 0.5, 0.5); // go out of range ;)
    delay(500);
    USB.write("Sending config 2...\n");
    configureTimepulse(5, 12000000, 0.5, 0.5);
    delay(500);
  }
}

void loop()
{
  /* if (GPS.available())
    USB.write(GPS.read()); */
}

// calculate the crc for the message
void calcChecksum(byte * checksumPayload, byte payloadSize) {
  byte CK_A = 0, CK_B = 0;
  for (int i = 0; i < payloadSize ; i++) {
    CK_A = CK_A + *checksumPayload;
    CK_B = CK_B + CK_A;
    checksumPayload++;
  }
  *checksumPayload = CK_A;
  checksumPayload++;
  *checksumPayload = CK_B;
}

// send a message to set a new frequency and locked frequency
// this does not wait for an acknowledge, nor poll the actual settings
void configureTimepulse(uint32_t freq, uint32_t freqLock, double pulseRatio, double pulseRatioLock) {
  double factor = powl(2.0, -32.0);
  pulseRatio = 1 - pulseRatio;
  uint32_t dc = (pulseRatio / factor);
  pulseRatioLock = 1 - pulseRatioLock;
  uint32_t dcLock = (pulseRatioLock / factor);

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
}
