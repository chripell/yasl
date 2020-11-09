#include "RFduinoBLE.h"
#include "Wire.h"
#include "variant.h"

extern "C" {
  void setup(void);
  void loop(void);

#include "protocol.h"
}

#define CONNECTED (1<<0)

static volatile uint8_t status = 0;
#define MAX_BUFFER 32
static uint8_t buf[MAX_BUFFER+1];

void setup() {
#ifdef ADEBUG
  Serial.begin(9600);
#endif
  Wire.begin();
  RFduinoBLE.advertisementData = "I2CServer";
  RFduinoBLE.advertisementInterval = 500;
  RFduinoBLE.begin();
}

void RFduinoBLE_onConnect()
{
  status |= CONNECTED;
#ifdef ADEBUG
  Serial.println("Connected CB");
#endif
}

void RFduinoBLE_onDisconnect()
{
  status &= ~CONNECTED;
#ifdef ADEBUG
  Serial.println("Disconnected CB");
#endif
}

static uint32_t i2c_read(uint8_t addr,
			uint8_t *p_data,
			uint8_t length) {
  int i;

  if (Wire.requestFrom(addr, length) != length)
    return 0x150;
  for (i = 0; i < length; i++) {
    int val = Wire.read();

    if (val == -1)
      return 0x151;
    p_data[i] = val & 0xff;
  }
  return 0;
}

static uint32_t i2c_write(uint8_t addr,
			  uint8_t const *p_data,
			  uint8_t length,
			  bool no_stop) {
  uint32_t ret = 0;

  Wire.beginTransmission(addr);
  if (Wire.write(p_data, length) != length)
    ret = 0x50;
  ret = Wire.endTransmission(!no_stop);
  if (ret != 0)
    ret += 0x200;
  return ret;
}

void RFduinoBLE_onReceive(char *data, int len)
{
  int n;

  if (len > MAX_BUFFER) {
#ifdef ADEBUG
    Serial.println("len too big");
#endif
    return;
  }
#ifdef ADEBUG
  Serial.println("RXTX");
  Serial.write((const uint8_t *) data, len);
  Serial.println();
#endif
  n = prepare_twi_response((uint8_t *) data, len,
			   buf, MAX_BUFFER,
			   i2c_write,
			   i2c_read);
#ifdef ADEBUG
  Serial.write(buf, n);
  Serial.println();
#endif
  RFduinoBLE.send((char *) buf, n);
}

void loop() {
#ifdef ADEBUG
  if (status & CONNECTED) {
    Serial.println("Connected");
  }
#endif
  RFduino_ULPDelay(SECONDS(1));
#ifdef ADEBUG
  Serial.println("Loop");
#endif
}
