#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_ 1

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t (*twi_tx_f) (uint8_t address,
			      uint8_t const *p_data,
			      uint8_t length,
			      bool no_stop);

typedef uint32_t (*twi_rx_f) (uint8_t address,
			      uint8_t *p_data,
			      uint8_t length);

int prepare_twi_response(uint8_t * ibuf,
			 uint32_t in,
			 uint8_t * obuf,
			 uint32_t on,
			 twi_tx_f txf,
			 twi_rx_f rxf);

#endif /* _PROTOCOL_H_ */
