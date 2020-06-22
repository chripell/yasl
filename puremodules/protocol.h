#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_ 1

#ifndef _PROTOCOL_TESTING
#include "nrf_drv_twi.h"
#else
#define nrf_drv_twi_t int
#define ret_code_t int
#define bool int
#include <stdint.h>
#endif

typedef ret_code_t (*twi_tx_f) (nrf_drv_twi_t const * p_instance,
				uint8_t               address,
				uint8_t const *       p_data,
				uint8_t               length,
				bool                  no_stop);

typedef ret_code_t (*twi_rx_f) (nrf_drv_twi_t const * p_instance,
				uint8_t               address,
				uint8_t *             p_data,
				uint8_t               length);

int prepare_twi_response(nrf_drv_twi_t const * p_instance,
			 uint8_t * buf,
			 int n,
			 twi_tx_f txf,
			 twi_rx_f rxf);

#endif /* _PROTOCOL_H_ */
