#include "protocol.h"

#include<stdio.h>
#include<string.h>

uint8_t expected_address;
uint8_t expected_data[1024];
uint8_t expected_length;
bool expected_no_stop;
int expected;
uint8_t provide_address;
uint8_t provide_data[1024];
uint8_t provide_length;

uint32_t test_tx(uint8_t address,
		 uint8_t const *p_data,
		 uint8_t length,
		 bool no_stop) {
  uint8_t i;

  expected++;
  if (address != expected_address)
    printf("FAIL, tx address want %d, got %d\n", expected_address, address);
  if (no_stop != expected_no_stop)
    printf("FAIL, tx no_stop want %d, got %d\n", expected_no_stop, no_stop);
  if (length != expected_length) {
    printf("FAIL, tx length want %d, got %d\n", expected_length, length);
    return 0;
  }
  for(i = 0; i < length; i++) {
    if (p_data[i] != expected_data[i])
      printf("FAIL, tx data[%d] want %d, got %d\n", i, expected_data[i], p_data[i]);
  }
  return 0;
}

uint32_t test_rx (uint8_t address,
			  uint8_t *p_data,
			  uint8_t length) {
  if (address != provide_address)
    printf("FAIL, rx address want %d, got %d\n", provide_address, address);
  if (length != provide_length) {
    printf("FAIL, rx length want %d, got %d\n", provide_length, length);
    return 0;
  }
  memcpy(p_data, provide_data, length);
  return 0;
}

#define STRL(X) X, strlen(X)

int main(void) {
  uint8_t ibuf[1024];
  uint8_t obuf[1024];
  int ret;
  char *ins;

  puts("Testing Send");
  ins = ">AB03010203";
  memcpy(ibuf, STRL(ins));
  expected_address = 0xab;
  expected_length = 3;
  expected_no_stop = false;
  memcpy(expected_data, STRL("\x01\x02\x03"));
  memset(obuf, 0, sizeof(obuf));
  expected = 0;
  ret = prepare_twi_response(ibuf, strlen(ins), obuf, 16, test_tx, test_rx);
  if (ret != 5)
    printf("FAIL, value returned: %d\n", ret);
  if (memcmp(obuf, STRL("!0000")) != 0)
    printf("FAIL, returned string not zero but: <%s>\n", obuf);
  if (expected != 1)
    printf("FAIL, tx called the wrong number of times: %d\n", expected);

  puts("Testing Receiving");
  ins = "<AB03";
  memcpy(ibuf, STRL(ins));
  provide_address = 0xab;
  provide_length = 3;
  memcpy(provide_data, STRL("\x01\x02\x03"));
  memset(obuf, 0, sizeof(obuf));
  expected = 0;
  ret = prepare_twi_response(ibuf, strlen(ins), obuf, 16, test_tx, test_rx);
  if (ret != 7)
    printf("FAIL, error value returned: %d\n", ret);
  if (memcmp(obuf, STRL(":010203")) != 0)
    printf("FAIL, bad returned buffer: <%s>\n", obuf);
  if (expected != 0)
    printf("FAIL, tx called the wrong number of times: %d\n", expected);

  puts("Testing Tx-Rx");
  ins = "|17010487";
  memcpy(ibuf, STRL(ins));
  expected_address = 0x17;
  expected_length = 1;
  expected_no_stop = true;
  memcpy(expected_data, STRL("\x87"));
  provide_address = 0x17;
  provide_length = 4;
  memcpy(provide_data, STRL("\x01\x02\x03\x4a"));
  memset(obuf, 0, sizeof(obuf));
  expected = 0;
  ret = prepare_twi_response(ibuf, strlen(ins), obuf, 16, test_tx, test_rx);
  if (ret != 9)
    printf("FAIL, value returned: %d\n", ret);
  if (memcmp(obuf, STRL(":0102034a")) != 0)
    printf("FAIL, returned string not zero but: <%s>\n", obuf);
  if (expected != 1)
    printf("FAIL, tx called the wrong number of times: %d\n", expected);

  return 0;
}
