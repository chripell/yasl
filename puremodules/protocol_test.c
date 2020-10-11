#include "protocol.h"

#include<stdio.h>
#include<string.h>

uint8_t expected_address;
uint8_t expected_data[1024];
uint8_t expected_length;
bool expected_no_stop;
int expected;
uint32_t expected_return;
uint8_t provide_address;
uint8_t provide_data[1024];
uint8_t provide_length;
uint32_t provide_return;

uint32_t test_tx(uint8_t address,
		 uint8_t const *p_data,
		 uint8_t length,
		 bool no_stop) {
  uint8_t i;

  expected++;
  if (expected_return != 0)
    return expected_return;
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
  if (provide_return != 0)
    return provide_return;
  if (address != provide_address)
    printf("FAIL, rx address want %d, got %d\n", provide_address, address);
  if (length != provide_length) {
    printf("FAIL, rx length want %d, got %d\n", provide_length, length);
    return provide_return;
  }
  memcpy(p_data, provide_data, length);
  return 0;
}

#define STRL(X) X, strlen(X)

struct test_case_s {
  char *desc;
  char *ins;
  uint8_t expected_address;
  uint8_t expected_length;
  bool expected_no_stop;
  char *expected_data;
  int want_expected;
  uint32_t expected_return;
  uint8_t provide_address;
  uint8_t provide_length;
  char *provide_data;
  uint32_t provide_return;
  int want_ret;
  char *outs;
};

void test_case(struct test_case_s *t) {
  uint8_t ibuf[1024];
  uint8_t obuf[1024];
  int ret = -1;

  puts(t->desc);
  if (t->ins != NULL)
    memcpy(ibuf, STRL(t->ins));
  expected_address = t->expected_address;
  expected_length = t->expected_length;
  expected_no_stop = t->expected_no_stop;
  if (t->expected_data != NULL)
    memcpy(expected_data, STRL(t->expected_data));
  memset(obuf, 0, sizeof(obuf));
  expected = 0;
  expected_return = t->expected_return;
  provide_address = t->provide_address;
  provide_length = t->provide_length;
  if (t->provide_data != NULL)
    memcpy(provide_data, STRL(t->provide_data));
  provide_return = t->provide_return;
  if (t->ins != NULL)
    ret = prepare_twi_response(ibuf, strlen(t->ins), obuf, sizeof(obuf), test_tx, test_rx);
  if (ret != t->want_ret)
    printf("FAIL, value returned: %d\n", ret);
  if (t->outs != NULL) {
    if (memcmp(obuf, STRL(t->outs)) != 0)
      printf("FAIL, returned string not <%s> but: <%s>\n", t->outs, obuf);
  }
  if (expected != t->want_expected)
    printf("FAIL, tx called the wrong number of times: %d\n", expected);
}

int main(void) {
  unsigned long i;
  struct test_case_s tc[] = {
    {
      .desc = "Testing Send",
      .ins = ">AB03010203",
      .expected_address = 0xab,
      .expected_length = 3,
      .expected_no_stop = false,
      .expected_data = "\x01\x02\x03",
      .expected_return = 0,
      .provide_return = 1,
      .want_expected = 1,
      .want_ret = 5,
      .outs = "!0000",
    },
    {
      .desc = "Testing Send Error",
      .ins = ">AB03010203",
      .expected_return = 0xab,
      .provide_return = 1,
      .want_expected = 1,
      .want_ret = 5,
      .outs = "!00ab",
    },
    {
      .desc = "Testing Parse Error",
      .ins = "ummm",
      .expected_return = 1,
      .provide_return = 1,
      .want_expected = 0,
      .want_ret = 5,
      .outs = "!ff00",
    },
    {
      .desc = "Testing Parse Error Too long",
      .ins = ">ABFF",
      .expected_return = 1,
      .provide_return = 1,
      .want_expected = 0,
      .want_ret = 5,
      .outs = "!fffd",
    },
    {
      .desc = "Testing Parse Error Too long 1",
      .ins = ">AB01",
      .expected_return = 1,
      .provide_return = 1,
      .want_expected = 0,
      .want_ret = 5,
      .outs = "!fffc",
    },
    {
      .desc = "Testing Receiving",
      .ins = "<AB03",
      .expected_return = 1,
      .provide_address = 0xab,
      .provide_length = 3,
      .provide_data = "\x01\x02\x03",
      .provide_return = 0,
      .want_expected = 0,
      .want_ret = 7,
      .outs = ":010203",
    },
    {
      .desc = "Testing Receiving Error",
      .ins = "<AB03",
      .expected_return = 1,
      .provide_return = 11,
      .want_expected = 0,
      .want_ret = 5,
      .outs = "!000b",
    },
    {
      .desc = "Testing Tx-Rx",
      .ins = "|17010487",
      .expected_address = 0x17,
      .expected_length = 1,
      .expected_no_stop = true,
      .expected_data = "\x87",
      .expected_return = 0,
      .provide_address = 0x17,
      .provide_length = 4,
      .provide_data = "\x01\x02\x03\x4a",
      .provide_return = 0,
      .want_expected = 1,
      .want_ret = 9,
      .outs = ":0102034a",
    },
    {
      .desc = "Testing Tx-Rx Error",
      .ins = "|17010487",
      .expected_return = 1,
      .provide_address = 0x17,
      .provide_length = 4,
      .provide_data = "\x01\x02\x03\x4a",
      .provide_return = 0,
      .want_expected = 1,
      .want_ret = 5,
      .outs = "!0001",
    },
    {
      .desc = "Testing Tx-Rx Error 1",
      .ins = "|17010487",
      .expected_address = 0x17,
      .expected_length = 1,
      .expected_no_stop = true,
      .expected_data = "\x87",
      .expected_return = 0,
      .provide_return = 255,
      .want_expected = 1,
      .want_ret = 5,
      .outs = "!00ff",
    },
    {
      .desc = "Testing Tx-Rx Long",
      .ins = "|1710100102030405060708090a0b0c0d0e0f00",
      .expected_address = 0x17,
      .expected_length = 16,
      .expected_no_stop = true,
      .expected_data = "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00",
      .expected_return = 0,
      .provide_address = 0x17,
      .provide_length = 16,
      .provide_data = "\x10\x20\x30\x40\x50\x60\x70\x80\x90\xa0\xb0\xc0\xd0\xe0\xf0\x00",
      .provide_return = 0,
      .want_expected = 1,
      .want_ret = 33,
      .outs = ":102030405060708090a0b0c0d0e0f000",
    },
  };

  for(i = 0; i < sizeof(tc)/sizeof(struct test_case_s); i++) {
    test_case(&tc[i]);
  }
  return 0;
}
