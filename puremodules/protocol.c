#include "protocol.h"

/*
  Protocol:

  from Host to Pure:

  Byte 0 -> > is TX, < is RX, } is TX non-stop
  Byte 1 -> len from 0 to f
  for TX then there are hexa bytes.

  from Pure to Host:

  : -> followed by hexa data if any
  ! -> error followed by code as 16 bit hexa. (0xffff is error parsing, 0xfffe
  is too big)

*/

#define MAXB 16

int prepare_twi_response(nrf_drv_twi_t const *p_instance, uint8_t *buf, int n,
                         twi_tx_f txf, twi_rx_f rxf) {
  /*   uint8_t txb[MAXB]; */
  /*   uint8_t rxb[MAXB]; */
  /*   int err = 0; */
  /*   int cur = 0; */
  /*   int cur_tx = 0; */
  /*   int cur_rx = 0; */
  /*   int i; */
  /*   while (cur < length) { */
  /*     uint8_t cmd = p_data[cur++]; */
  /*     if (cmd == '>' || cmd == '}') { */
  /* 	int n = p_data[cur++] - '0'; */
  /* 	if (n < 1 | n > MAXB) { */
  /* 	  err = 0xffff; */
  /* 	  goto send_response; */
  /* 	} */
  /* 	if (cur + 2 *n > length) { */
  /* 	  err = 0xffff; */
  /* 	  goto send_response; */
  /* 	} */
  /* 	for (i=0; i < n; i++) { */
  /* 	} */
  /*     } */
  /*   } */
  /* send_reponse: */
  return 0;
}
