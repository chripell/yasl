#include "protocol.h"

/*
  Protocol:

  Byte is 2 character in hexa.
  Word is 4 characters in hexa.

  Write I2C:
  > [address byte] [N byte] [data byte... up to N]
   ! [error code word, 0 is OK 0xffXX is cannot parse]

  Read I2C:
  < [address byte] [N byte]
   : [value byte ... up to N]
   ! [error code word 0xffXX is cannot parse]

  Write the Read I2C:
  | [address byte] [N byte] [M byte to read] [data byte...]
   : [value byte ... up to M]
   ! [error code word 0xffXX is cannot parse]

*/

#define MAXB 16

static void to_hex(uint8_t *b, uint32_t v, int n) {
  int i;

  for(i = n-1; i >= 0; i--) {
    uint8_t c = v & 0xf;
    if (c < 10)
      b[i] = '0' + c;
    else
      b[i] = 'a' + c - 10;
    v >>= 4;
  }
}

static uint32_t from_hex(uint8_t *b, int n) {
  int i;
  uint32_t r = 0;

  for(i = 0; i < n; i++) {
    uint32_t c = 0;
    r <<= 4;
    if (b[i] >= '0' && b[i] <= '9')
      c = b[i] - '0';
    else if (b[i] >= 'a' && b[i] <= 'f')
      c = b[i] - 'a' + 10;
    else if (b[i] >= 'A' && b[i] <= 'F')
      c = b[i] - 'A' + 10;
    r += c;
  }
  return r;
}

static int send_error(uint32_t err, uint8_t *obuf, int on) {
  if (err > 0xffff)
    err = 0xfffe;
  if (on < 1)
    return 0;
  obuf[0] = '!';
  if (on < 5)
    return 1;
  to_hex(&obuf[1], err, 4);
  return 5;
}

int prepare_twi_response(uint8_t *ibuf, uint32_t in,
			 uint8_t *obuf, uint32_t on,
                         twi_tx_f txf, twi_rx_f rxf) {
  uint8_t b[MAXB];
  uint32_t addr, n, m, r;
  uint32_t i;

  switch (ibuf[0]) {
  case '>':
    if (in < 5)
      return send_error(0xffff, obuf, on);
    addr = from_hex(&ibuf[1], 2);
    n = from_hex(&ibuf[3], 2);
    if (n > MAXB)
      return send_error(0xfffd, obuf, on);
    if (n * 2 + 5 != in)
      return send_error(0xfffc, obuf, on);
    for (i = 0; i < n; i++)
      b[i] = from_hex(&ibuf[5 + i * 2], 2);
    return send_error(txf(addr, b, n, 0),
		      obuf, on);
  case '<':
    if (in != 5)
      return send_error(0xffff, obuf, on);
    addr = from_hex(&ibuf[1], 2);
    n = from_hex(&ibuf[3], 2);
    if (n > MAXB)
      return send_error(0xfffd, obuf, on);
    if (on < 1 + 2 * n)
      return send_error(0xfffc, obuf, on);
    r = rxf(addr, b, n);
    if (r != 0)
      return send_error(r, obuf, on);
    obuf[0] = ':';
    for (i = 0; i < n; i++)
      to_hex(&obuf[1 + i * 2], b[i], 2);
    return 1 + 2 * n;
  case '|':
    if (in < 7)
      return send_error(0xffff, obuf, on);
    addr = from_hex(&ibuf[1], 2);
    n = from_hex(&ibuf[3], 2);
    m = from_hex(&ibuf[5], 2);
    if (n > MAXB)
      return send_error(0xfffd, obuf, on);
    if (m > MAXB)
      return send_error(0xfffc, obuf, on);
    if (on < 1 + 2 * m)
      return send_error(0xfffb, obuf, on);
    if (n * 2 + 7 != in)
      return send_error(0xfffa, obuf, on);
    for (i = 0; i < n; i++)
      b[i] = from_hex(&ibuf[7 + i * 2], 2);
    r = txf(addr, b, n, 1);
    if (r != 0)
      return send_error(r, obuf, on);
    r = rxf(addr, b, m);
    if (r != 0)
      return send_error(r, obuf, on);
    obuf[0] = ':';
    for (i = 0; i < m; i++)
      to_hex(&obuf[1 + i * 2], b[i], 2);
    return 1 + 2 * m;
  default:
    return send_error(0xff00, obuf, on);
  }
  return send_error(0xff01, obuf, on);
}
