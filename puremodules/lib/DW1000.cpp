/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file DW1000.cpp
 * Arduino driver library (source file) for the Decawave DW1000 UWB transceiver IC.
 */

/*
*   All arduino dependencies and code changes for use with nrf52
*
*/

#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_drv_twi.h"
#include "ble_nus.h"
#include "bsp.h"
#include "i2c_driver.h"
#include "ble_driver.h"
#include "math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "DW1000Time.h"
#include "DW1000.h"
#include "main.h"
#include "nrf_drv_spi.h"
#include "nrf_delay.h"

//TODO: figure out what these vars are used for
void *__gxx_personality_v0;
void *__cxa_end_cleanup;

#define DEBUG_PRINTF(...) printf( __VA_ARGS__); 
//#define DEBUG_PRINTF(...)

DW1000Class DW1000;

// pins
unsigned int DW1000Class::_ss;
unsigned int DW1000Class::_rst;
unsigned int DW1000Class::_irq;
// IRQ callbacks
void (*DW1000Class::_handleSent)(void) = 0;
void (*DW1000Class::_handleError)(void) = 0;
void (*DW1000Class::_handleReceived)(void) = 0;
void (*DW1000Class::_handleReceiveFailed)(void) = 0;
void (*DW1000Class::_handleReceiveTimeout)(void) = 0;
void (*DW1000Class::_handleReceiveTimestampAvailable)(void) = 0;
// registers
uint8_t DW1000Class::_syscfg[LEN_SYS_CFG];
uint8_t DW1000Class::_sysctrl[LEN_SYS_CTRL];
uint8_t DW1000Class::_sysstatus[LEN_SYS_STATUS];
uint8_t DW1000Class::_txfctrl[LEN_TX_FCTRL];
uint8_t DW1000Class::_sysmask[LEN_SYS_MASK];
uint8_t DW1000Class::_chanctrl[LEN_CHAN_CTRL];
uint8_t DW1000Class::_networkAndAddress[LEN_PANADR];
// driver internal state
uint8_t DW1000Class::_extendedFrameLength = FRAME_LENGTH_NORMAL;
uint8_t DW1000Class::_pacSize = PAC_SIZE_8;
uint8_t DW1000Class::_pulseFrequency = TX_PULSE_FREQ_16MHZ;
uint8_t DW1000Class::_dataRate = TRX_RATE_6800KBPS;
uint8_t DW1000Class::_preambleLength = TX_PREAMBLE_LEN_128;
uint8_t DW1000Class::_preambleCode = PREAMBLE_CODE_16MHZ_4;
uint8_t DW1000Class::_channel = CHANNEL_5;
DW1000Time DW1000Class::_antennaDelay;
bool DW1000Class::_smartPower = false;
bool DW1000Class::_frameCheck = true;
bool DW1000Class::_permanentReceive = false;
int DW1000Class::_deviceMode = IDLE_MODE;
// modes of operation
const uint8_t DW1000Class::MODE_LONGDATA_RANGE_LOWPOWER[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_2048};
const uint8_t DW1000Class::MODE_SHORTDATA_FAST_LOWPOWER[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_128};
const uint8_t DW1000Class::MODE_LONGDATA_FAST_LOWPOWER[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_1024};
const uint8_t DW1000Class::MODE_SHORTDATA_FAST_ACCURACY[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_128};
const uint8_t DW1000Class::MODE_LONGDATA_FAST_ACCURACY[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_1024};
const uint8_t DW1000Class::MODE_LONGDATA_RANGE_ACCURACY[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_2048};
// SPI settings
//const SPISettings DW1000Class::_fastSPI = SPISettings(16000000L, MSBFIRST, SPI_MODE0);
//const SPISettings DW1000Class::_slowSPI = SPISettings(2000000L, MSBFIRST, SPI_MODE0);
//const SPISettings* DW1000Class::_currentSPI = &_fastSPI;

/* ###########################################################################
 * #### Init and end #######################################################
 * ######################################################################### */

//void DW1000Class::end(spi) {
	//
//}
/*
void DW1000Class::select(int ss) {
	reselect(ss);
	// try locking clock at PLL speed (should be done already, 
	// but just to be sure)
	enableClock(AUTO_CLOCK);
	delay(5);
	// reset chip (either soft or hard)
	if(_rst > 0) {
		//TODO: Code for reset
	}
	reset();
	// default network and node id
	writeValueToBytes(_networkAndAddress, 0xFF, LEN_PANADR);
	writeNetworkIdAndDeviceAddress();
	// default system configuration
	memset(_syscfg, 0, LEN_SYS_CFG);
	setDoubleBuffering(false);
	setInterruptPolarity(true);
	writeSystemConfigurationRegister();
	// default interrupt mask, i.e. no interrupts
	clearInterrupts();
	writeSystemEventMaskRegister();
	// load LDE micro-code
	enableClock(XTI_CLOCK);
	delay(5);
	manageLDE();
	delay(5);
	enableClock(AUTO_CLOCK);
	delay(5);
}

void DW1000Class::reselect(int ss) {
	_ss = ss;
	pinMode(_ss, OUTPUT);
	digitalWrite(_ss, HIGH);
}*/
/*
void DW1000Class::begin(int irq) {
	//begin(irq, -1);
}

void DW1000Class::begin(int irq, int rst) {
	// generous initial init/wake-up-idle delay
	nrf_delay_ms(5);
	// start SPI
 	SPI.begin(); 
	SPI.usingInterrupt(irq);
	// pin and basic member setup
	_rst = rst;
	_irq = irq;
	_deviceMode = IDLE_MODE;
	// attach interrupt
	attachInterrupt(_irq, DW1000Class::handleInterrupt, RISING);
}*/

void DW1000Class::manageLDE() {
	// transfer any ldo tune values
	uint8_t  ldoTune[LEN_OTP_RDAT];
	readBytesOTP(spi, 0x04, ldoTune); // TODO #define
	if(ldoTune[0] != 0) {
		// TODO tuning available, copy over to RAM: use OTP_LDO bit
	}
	// tell the chip to load the LDE microcode
	// TODO remove clock-related code (PMSC_CTRL) as handled separately
	uint8_t  pmscctrl0[LEN_PMSC_CTRL0];
	uint8_t  otpctrl[LEN_OTP_CTRL];
	memset(pmscctrl0, 0, LEN_PMSC_CTRL0);
	memset(otpctrl, 0, LEN_OTP_CTRL);
	readBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	readBytes(spi, OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	pmscctrl0[0] = 0x01;
	pmscctrl0[1] = 0x03;
	otpctrl[0] = 0x00;
	otpctrl[1] = 0x80;
	writeBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	writeBytes(spi, OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	nrf_delay_ms(5);
	pmscctrl0[0] = 0x00;
	pmscctrl0[1] = 0x02;
	writeBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
}

void DW1000Class::enableClock(uint8_t  clock) {
	uint8_t  pmscctrl0[LEN_PMSC_CTRL0];
	memset(pmscctrl0, 0, LEN_PMSC_CTRL0);
	readBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	if(clock == AUTO_CLOCK) {
		//_currentSPI = &_fastSPI;
		pmscctrl0[0] = AUTO_CLOCK;
		pmscctrl0[1] &= 0xFE;
	} else if(clock == XTI_CLOCK) {
		//_currentSPI = &_slowSPI;
		pmscctrl0[0] &= 0xFC;
		pmscctrl0[0] |= XTI_CLOCK;
	} else if(clock == PLL_CLOCK) {
		//_currentSPI = &_fastSPI;
		pmscctrl0[0] &= 0xFC;
		pmscctrl0[0] |= PLL_CLOCK;
	} else {
		// TODO deliver proper warning
	}
	writeBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, 1);
    writeBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
}
/*
void DW1000Class::reset() {
	if(_rst < 0) {
		softReset();
	} else {
		digitalWrite(_rst, LOW);
		nrf_delay_ms(10);
		digitalWrite(_rst, HIGH);
		nrf_delay_ms(10);
		// force into idle mode (although it should be already after reset)
		idle();
	}
}*/

void DW1000Class::softReset() {
	uint8_t  pmscctrl0[LEN_PMSC_CTRL0];
	readBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	pmscctrl0[0] = 0x01;
	writeBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	pmscctrl0[3] = 0x00;
	writeBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	nrf_delay_ms(10);
	pmscctrl0[0] = 0x00;
	pmscctrl0[3] = 0xF0;
	writeBytes(spi, PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	// force into idle mode
	idle();
}

void DW1000Class::enableMode(const uint8_t  mode[]) {
	setDataRate(mode[0]);
	setPulseFrequency(mode[1]);
	setPreambleLength(mode[2]);
	// TODO add channel and code to mode tuples 
	// TODO add channel and code settings with checks (see Table 58)
	setChannel(CHANNEL_5);
	if(mode[1] == TX_PULSE_FREQ_16MHZ) {
		setPreambleCode(PREAMBLE_CODE_16MHZ_4);
	} else {
		setPreambleCode(PREAMBLE_CODE_64MHZ_10);
	}
}

void DW1000Class::tune() {
	// these registers are going to be tuned/configured
	uint8_t  agctune1[LEN_AGC_TUNE1];
	uint8_t  agctune2[LEN_AGC_TUNE2];
	uint8_t  agctune3[LEN_AGC_TUNE3];
	uint8_t  drxtune0b[LEN_DRX_TUNE0b];
	uint8_t  drxtune1a[LEN_DRX_TUNE1a];
	uint8_t  drxtune1b[LEN_DRX_TUNE1b];
	uint8_t  drxtune2[LEN_DRX_TUNE2];
	uint8_t  drxtune4H[LEN_DRX_TUNE4H];
	uint8_t  ldecfg1[LEN_LDE_CFG1];
	uint8_t  ldecfg2[LEN_LDE_CFG2];
	uint8_t  lderepc[LEN_LDE_REPC];
	uint8_t  txpower[LEN_TX_POWER];
	uint8_t  rfrxctrlh[LEN_RF_RXCTRLH];
	uint8_t  rftxctrl[LEN_RF_TXCTRL];
	uint8_t  tcpgdelay[LEN_TC_PGDELAY];
	uint8_t  fspllcfg[LEN_FS_PLLCFG];
	uint8_t  fsplltune[LEN_FS_PLLTUNE];
	//uint8_t  fsxtalt[LEN_FS_XTALT];
	// AGC_TUNE1
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		writeValueToBytes(agctune1, 0x8870, LEN_AGC_TUNE1);
	} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
		writeValueToBytes(agctune1, 0x889B, LEN_AGC_TUNE1);
	} else {
		// TODO proper error/warning handling
	}
	// AGC_TUNE2
	writeValueToBytes(agctune2, 0x2502A907L, LEN_AGC_TUNE2);
	// AGC_TUNE3
	writeValueToBytes(agctune3, 0x0035, LEN_AGC_TUNE3);
	// DRX_TUNE0b (already optimized according to Table 20 of user manual)
	if(_dataRate == TRX_RATE_110KBPS) {
		writeValueToBytes(drxtune0b, 0x0016, LEN_DRX_TUNE0b);
	} else if(_dataRate == TRX_RATE_850KBPS) {
		writeValueToBytes(drxtune0b, 0x0006, LEN_DRX_TUNE0b);
	} else if(_dataRate == TRX_RATE_6800KBPS) {
		writeValueToBytes(drxtune0b, 0x0001, LEN_DRX_TUNE0b);
	} else {
		// TODO proper error/warning handling
	}
	// DRX_TUNE1a
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		writeValueToBytes(drxtune1a, 0x0087, LEN_DRX_TUNE1a);
	} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
		writeValueToBytes(drxtune1a, 0x008D, LEN_DRX_TUNE1a);
	} else {
		// TODO proper error/warning handling
	}
	// DRX_TUNE1b
	if(_preambleLength ==  TX_PREAMBLE_LEN_1536 || _preambleLength ==  TX_PREAMBLE_LEN_2048 ||  
			_preambleLength ==  TX_PREAMBLE_LEN_4096) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(drxtune1b, 0x0064, LEN_DRX_TUNE1b);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_preambleLength != TX_PREAMBLE_LEN_64) {
		if(_dataRate == TRX_RATE_850KBPS || _dataRate == TRX_RATE_6800KBPS) {
			writeValueToBytes(drxtune1b, 0x0020, LEN_DRX_TUNE1b);
		} else {
			// TODO proper error/warning handling
		}
	} else {
		if(_dataRate == TRX_RATE_6800KBPS) {
			writeValueToBytes(drxtune1b, 0x0010, LEN_DRX_TUNE1b);
		} else {
			// TODO proper error/warning handling
		}
	}
	// DRX_TUNE2
	if(_pacSize == PAC_SIZE_8) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(drxtune2, 0x311A002DL, LEN_DRX_TUNE2);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(drxtune2, 0x313B006BL, LEN_DRX_TUNE2);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_pacSize == PAC_SIZE_16) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(drxtune2, 0x331A0052L, LEN_DRX_TUNE2);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(drxtune2, 0x333B00BEL, LEN_DRX_TUNE2);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_pacSize == PAC_SIZE_32) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(drxtune2, 0x351A009AL, LEN_DRX_TUNE2);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(drxtune2, 0x353B015EL, LEN_DRX_TUNE2);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_pacSize == PAC_SIZE_64) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(drxtune2, 0x371A011DL, LEN_DRX_TUNE2);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(drxtune2, 0x373B0296L, LEN_DRX_TUNE2);
		} else {
			// TODO proper error/warning handling
		}
	} else {
		// TODO proper error/warning handling
	}
	// DRX_TUNE4H
	if(_preambleLength == TX_PREAMBLE_LEN_64) {
		writeValueToBytes(drxtune4H, 0x0010, LEN_DRX_TUNE4H);
	} else {
		writeValueToBytes(drxtune4H, 0x0028, LEN_DRX_TUNE4H);
	}
	// RF_RXCTRLH
	if(_channel != CHANNEL_4 && _channel != CHANNEL_7) {
		writeValueToBytes(rfrxctrlh, 0xD8, LEN_RF_RXCTRLH);
	} else {
		writeValueToBytes(rfrxctrlh, 0xBC, LEN_RF_RXCTRLH);
	}
	// RX_TXCTRL
	if(_channel == CHANNEL_1) {
		writeValueToBytes(rftxctrl, 0x00005C40L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_2) {
		writeValueToBytes(rftxctrl, 0x00045CA0L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_3) {
		writeValueToBytes(rftxctrl, 0x00086CC0L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_4) {
		writeValueToBytes(rftxctrl, 0x00045C80L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_5) {
		writeValueToBytes(rftxctrl, 0x001E3FE0L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_7) {
		writeValueToBytes(rftxctrl, 0x001E7DE0L, LEN_RF_TXCTRL);
	} else {
		// TODO proper error/warning handling
	}
	// TC_PGDELAY
	if(_channel == CHANNEL_1) {
		writeValueToBytes(tcpgdelay, 0xC9, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_2) {
		writeValueToBytes(tcpgdelay, 0xC2, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_3) {
		writeValueToBytes(tcpgdelay, 0xC5, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_4) {
		writeValueToBytes(tcpgdelay, 0x95, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_5) {
		writeValueToBytes(tcpgdelay, 0xC0, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_7) {
		writeValueToBytes(tcpgdelay, 0x93, LEN_TC_PGDELAY);
	} else {
		// TODO proper error/warning handling
	}
	// FS_PLLCFG and FS_PLLTUNE
	if(_channel == CHANNEL_1) {
		writeValueToBytes(fspllcfg, 0x09000407L, LEN_FS_PLLCFG);
		writeValueToBytes(fsplltune, 0x1E, LEN_FS_PLLTUNE);
	} else if(_channel == CHANNEL_2 || _channel == CHANNEL_4) {
		writeValueToBytes(fspllcfg, 0x08400508L, LEN_FS_PLLCFG);
		writeValueToBytes(fsplltune, 0x26, LEN_FS_PLLTUNE);
	} else if(_channel == CHANNEL_3) {
		writeValueToBytes(fspllcfg, 0x08401009L, LEN_FS_PLLCFG);
		writeValueToBytes(fsplltune, 0x5E, LEN_FS_PLLTUNE);
	} else if(_channel == CHANNEL_5 || _channel == CHANNEL_7) {
		writeValueToBytes(fspllcfg, 0x0800041DL, LEN_FS_PLLCFG);
		writeValueToBytes(fsplltune, 0xA6, LEN_FS_PLLTUNE);
	} else {
		// TODO proper error/warning handling
	}
	// LDE_CFG1
	writeValueToBytes(ldecfg1, 0xD, LEN_LDE_CFG1);
	// LDE_CFG2
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		writeValueToBytes(ldecfg2, 0x1607, LEN_LDE_CFG2);
	} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
		writeValueToBytes(ldecfg2, 0x0607, LEN_LDE_CFG2);
	} else {
		// TODO proper error/warning handling
	}
	// LDE_REPC
	if(_preambleCode == PREAMBLE_CODE_16MHZ_1 || _preambleCode == PREAMBLE_CODE_16MHZ_2) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x5998 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x5998, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_3 || _preambleCode == PREAMBLE_CODE_16MHZ_8) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x51EA >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x51EA, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_4) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x428E >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x428E, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_5) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x451E >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x451E, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_6) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x2E14 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x2E14, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_7) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x8000 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x8000, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_9) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x28F4 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x28F4, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_10 || _preambleCode == PREAMBLE_CODE_64MHZ_17) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x3332 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x3332, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_11) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x3AE0 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x3AE0, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_12) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x3D70 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x3D70, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_18 || _preambleCode == PREAMBLE_CODE_64MHZ_19) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x35C2 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x35C2, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_20) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x47AE >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x47AE, LEN_LDE_REPC);
		}
	} else {
		// TODO proper error/warning handling
	}
	// TX_POWER (enabled smart transmit power control)
	if(_channel == CHANNEL_1 || _channel == CHANNEL_2) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x15355575L, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x75757575L, LEN_TX_POWER);
			}
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x07274767L, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x67676767L, LEN_TX_POWER);
			}
		} else {
			// TODO proper error/warning handling
		}
	} else if(_channel == CHANNEL_3) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x0F2F4F6FL, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x6F6F6F6FL, LEN_TX_POWER);
			}
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x2B4B6B8BL, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x8B8B8B8BL, LEN_TX_POWER);
			}
		} else {
			// TODO proper error/warning handling
		}
	} else if(_channel == CHANNEL_4) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x1F1F3F5FL, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x5F5F5F5FL, LEN_TX_POWER);
			}
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x3A5A7A9AL, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x9A9A9A9AL, LEN_TX_POWER);
			}
		} else {
			// TODO proper error/warning handling
		}
	} else if(_channel == CHANNEL_5) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x0E082848L, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x48484848L, LEN_TX_POWER);
			}
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x25456585L, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x85858585L, LEN_TX_POWER);
			}
		} else {
			// TODO proper error/warning handling
		}
	} else if(_channel == CHANNEL_7) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x32527292L, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0x92929292L, LEN_TX_POWER);
			}
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			if(_smartPower) {
				writeValueToBytes(txpower, 0x5171B1D1L, LEN_TX_POWER);
			} else {
				writeValueToBytes(txpower, 0xD1D1D1D1L, LEN_TX_POWER);
			}
		} else {
			// TODO proper error/warning handling
		}
	} else {
		// TODO proper error/warning handling
	}
	// mid range XTAL trim (TODO here we assume no calibration data available in OTP)
	//writeValueToBytes(fsxtalt, 0x60, LEN_FS_XTALT);
	// write configuration back to chip
	writeBytes(spi, AGC_TUNE, AGC_TUNE1_SUB, agctune1, LEN_AGC_TUNE1);
	writeBytes(spi, AGC_TUNE, AGC_TUNE2_SUB, agctune2, LEN_AGC_TUNE2);
	writeBytes(spi, AGC_TUNE, AGC_TUNE3_SUB, agctune3, LEN_AGC_TUNE3);
	writeBytes(spi, DRX_TUNE, DRX_TUNE0b_SUB, drxtune0b, LEN_DRX_TUNE0b);
	writeBytes(spi, DRX_TUNE, DRX_TUNE1a_SUB, drxtune1a, LEN_DRX_TUNE1a);
	writeBytes(spi, DRX_TUNE, DRX_TUNE1b_SUB, drxtune1b, LEN_DRX_TUNE1b);
	writeBytes(spi, DRX_TUNE, DRX_TUNE2_SUB, drxtune2, LEN_DRX_TUNE2);
	writeBytes(spi, DRX_TUNE, DRX_TUNE4H_SUB, drxtune4H, LEN_DRX_TUNE4H);
	writeBytes(spi, LDE_IF, LDE_CFG1_SUB, ldecfg1, LEN_LDE_CFG1);
	writeBytes(spi, LDE_IF, LDE_CFG2_SUB, ldecfg2, LEN_LDE_CFG2);
	writeBytes(spi, LDE_IF, LDE_REPC_SUB, lderepc, LEN_LDE_REPC);
	writeBytes(spi, TX_POWER, NO_SUB, txpower, LEN_TX_POWER);
	writeBytes(spi, RF_CONF, RF_RXCTRLH_SUB, rfrxctrlh, LEN_RF_RXCTRLH);
	writeBytes(spi, RF_CONF, RF_TXCTRL_SUB, rftxctrl, LEN_RF_TXCTRL);
	writeBytes(spi, TX_CAL, TC_PGDELAY_SUB, tcpgdelay, LEN_TC_PGDELAY);
	writeBytes(spi, FS_CTRL, FS_PLLTUNE_SUB, fsplltune, LEN_FS_PLLTUNE);
	writeBytes(spi, FS_CTRL, FS_PLLCFG_SUB, fspllcfg, LEN_FS_PLLCFG);
	//writeBytes(spi, FS_CTRL, FS_XTALT_SUB, fsxtalt, LEN_FS_XTALT);
}

/* ###########################################################################
 * #### Interrupt handling ###################################################
 * ######################################################################### */

void DW1000Class::handleInterrupt() {
	// read current status and handle via callbacks
	readSystemEventStatusRegister();
	if(isClockProblem() /* TODO and others */ && _handleError != 0) {
		(*_handleError)();
	}
	if(isTransmitDone() &&_handleSent != 0) {
		(*_handleSent)();
		clearTransmitStatus();
	}
	if(isReceiveTimestampAvailable() && _handleReceiveTimestampAvailable != 0) {
		(*_handleReceiveTimestampAvailable)();
		clearReceiveTimestampAvailableStatus();
	}
	if(isReceiveFailed() && _handleReceiveFailed != 0) {
		(*_handleReceiveFailed)();
		clearReceiveStatus();
		if(_permanentReceive) {
			newReceive();
			startReceive();
		}
	} else if(isReceiveTimeout() && _handleReceiveTimeout != 0) {
		(*_handleReceiveTimeout)();
		clearReceiveStatus();
		if(_permanentReceive) {
			newReceive();
			startReceive();
		}
	} else if(isReceiveDone() && _handleReceived != 0) {
		(*_handleReceived)();
		clearReceiveStatus();
		if(_permanentReceive) {
			newReceive();
			startReceive();
		}
	}
	// clear all status that is left
	clearAllStatus();
}

/* ###########################################################################
 * #### Pretty printed device information ####################################
 * ######################################################################### */

void DW1000Class::getPrintableDeviceIdentifier(char msgBuffer[]) {
	uint8_t  data[LEN_DEV_ID];
	readBytes(spi, DEV_ID, NO_SUB, data, LEN_DEV_ID);
	sprintf(msgBuffer, "DECA - model: %d, version: %d, revision: %d", 
		data[1], (data[0] >> 4) & 0x0F, data[0] & 0x0F);
}

void DW1000Class::getPrintableExtendedUniqueIdentifier(char msgBuffer[]) {
	uint8_t  data[LEN_EUI];
	readBytes(spi, EUI, NO_SUB, data, LEN_EUI);
	sprintf(msgBuffer, "EUI: %d:%d:%d:%d:%d, OUI: %d:%d:%d",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
}

void DW1000Class::getPrintableNetworkIdAndShortAddress(char msgBuffer[]) {
	uint8_t  data[LEN_PANADR];
	readBytes(spi, PANADR, NO_SUB, data, LEN_PANADR);
	sprintf(msgBuffer, "PAN: %u, Short Address: %u",
		(unsigned int)((data[3] << 8) | data[2]), (unsigned int)((data[1] << 8) | data[0]));
}

void DW1000Class::getPrintableDeviceMode(char msgBuffer[]) {
	unsigned short prf;
	unsigned int plen;
	unsigned int dr;
	unsigned short ch;
	unsigned short pcode;
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		prf = 16;
	} else {
		prf = 64;
	}
	if(_preambleLength == TX_PREAMBLE_LEN_64) {
		plen = 64;
	} else if(_preambleLength == TX_PREAMBLE_LEN_128) {
		plen = 128;
	} else if(_preambleLength == TX_PREAMBLE_LEN_256) {
		plen = 256;
	} else if(_preambleLength == TX_PREAMBLE_LEN_512) {
		plen = 512;
	} else if(_preambleLength == TX_PREAMBLE_LEN_1024) {
		plen = 1024;
	} else if(_preambleLength == TX_PREAMBLE_LEN_1536) {
		plen = 1536;
	} else if(_preambleLength == TX_PREAMBLE_LEN_2048) {
		plen = 2048;
	} else {
		plen = 4096;
	}
	if(_dataRate == TRX_RATE_110KBPS) {
		dr = 110;
	} else if(_dataRate == TRX_RATE_850KBPS) {
		dr = 850;
	} else {
		dr = 6800;
	}
	ch = (short)_channel;
	pcode = (short)_preambleCode;
	sprintf(msgBuffer, "Data rate: %u kb/s, PRF: %u MHz, Preamble: %u symbols (code #%u), Channel: #%u", dr, prf, plen, pcode, ch);
}

/* ###########################################################################
 * #### DW1000 register read/write ###########################################
 * ######################################################################### */

void DW1000Class::readSystemConfigurationRegister() {
	readBytes(spi, SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000Class::writeSystemConfigurationRegister() {
	writeBytes(spi, SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000Class::readSystemEventStatusRegister() {
	readBytes(spi, SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000Class::readNetworkIdAndDeviceAddress() {
	readBytes(spi, PANADR, NO_SUB, _networkAndAddress, LEN_PANADR);
}

void DW1000Class::writeNetworkIdAndDeviceAddress() {
	writeBytes(spi, PANADR, NO_SUB, _networkAndAddress, LEN_PANADR);
}

void DW1000Class::readSystemEventMaskRegister() {
	readBytes(spi, SYS_MASK, NO_SUB, _sysmask, LEN_SYS_MASK);
}

void DW1000Class::writeSystemEventMaskRegister() {
	writeBytes(spi, SYS_MASK, NO_SUB, _sysmask, LEN_SYS_MASK);
}

void DW1000Class::readChannelControlRegister() {
	readBytes(spi, CHAN_CTRL, NO_SUB, _chanctrl, LEN_CHAN_CTRL);
}

void DW1000Class::writeChannelControlRegister() {
	writeBytes(spi, CHAN_CTRL, NO_SUB, _chanctrl, LEN_CHAN_CTRL);
}

void DW1000Class::readTransmitFrameControlRegister() {
	readBytes(spi, TX_FCTRL, NO_SUB, _txfctrl, LEN_TX_FCTRL);
}

void DW1000Class::writeTransmitFrameControlRegister() {
	writeBytes(spi, TX_FCTRL, NO_SUB, _txfctrl, LEN_TX_FCTRL);
}

/* ###########################################################################
 * #### DW1000 operation functions ###########################################
 * ######################################################################### */


void DW1000Class::setInterruptPolarity(bool val) {
	setBit(_syscfg, LEN_SYS_CFG, HIRQ_POL_BIT, val);
}

void DW1000Class::setReceiverAutoReenable(bool val) {
	setBit(_syscfg, LEN_SYS_CFG, RXAUTR_BIT, val);
}

void DW1000Class::interruptOnSent(bool val) {
	setBit(_sysmask, LEN_SYS_MASK, TXFRS_BIT, val);
}

void DW1000Class::interruptOnReceived(bool val) {
	setBit(_sysmask, LEN_SYS_MASK, RXDFR_BIT, val);
	setBit(_sysmask, LEN_SYS_MASK, RXFCG_BIT, val);
}

void DW1000Class::setNetworkId(unsigned int val) {
	_networkAndAddress[2] = (uint8_t )(val & 0xFF);
	_networkAndAddress[3] = (uint8_t )((val >> 8) & 0xFF);
}

void DW1000Class::setDeviceAddress(unsigned int val) {
	_networkAndAddress[0] = (uint8_t )(val & 0xFF);
	_networkAndAddress[1] = (uint8_t )((val >> 8) & 0xFF);
}

void DW1000Class::setDoubleBuffering(bool val) {
	setBit(_syscfg, LEN_SYS_CFG, DIS_DRXB_BIT, !val);
}

void DW1000Class::setFrameFilter(bool val) {
	setBit(_syscfg, LEN_SYS_CFG, FFEN_BIT, val);
}



void DW1000Class::interruptOnReceiveFailed(bool val) {
	setBit(_sysmask, LEN_SYS_STATUS, LDEERR_BIT, val);
	setBit(_sysmask, LEN_SYS_STATUS, RXFCE_BIT, val);
	setBit(_sysmask, LEN_SYS_STATUS, RXPHE_BIT, val);
	setBit(_sysmask, LEN_SYS_STATUS, RXRFSL_BIT, val);
}

void DW1000Class::interruptOnReceiveTimeout(bool val) {
	setBit(_sysmask, LEN_SYS_MASK, RXRFTO_BIT, val);
}

void DW1000Class::interruptOnReceiveTimestampAvailable(bool val) {
	setBit(_sysmask, LEN_SYS_MASK, LDEDONE_BIT, val);
}

void DW1000Class::interruptOnAutomaticAcknowledgeTrigger(bool val) {
	setBit(_sysmask, LEN_SYS_MASK, AAT_BIT, val);
}

void DW1000Class::clearInterrupts() {
	memset(_sysmask, 0, LEN_SYS_MASK);
}

void DW1000Class::idle() {
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	setBit(_sysctrl, LEN_SYS_CTRL, TRXOFF_BIT, true);
	_deviceMode = IDLE_MODE;
	//TODO: figure out this line
	writeBytes(spi, SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
	nrf_delay_ms(5);
}

void DW1000Class::newReceive() {
	idle();
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	clearReceiveStatus();
	_deviceMode = RX_MODE;
}

void DW1000Class::startReceive() {
	setBit(_sysctrl, LEN_SYS_CTRL, SFCST_BIT, !_frameCheck);
	setBit(_sysctrl, LEN_SYS_CTRL, RXENAB_BIT, true);
	writeBytes(spi, SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
}

void DW1000Class::newTransmit() {
	idle();
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	clearTransmitStatus();
	_deviceMode = TX_MODE;
}

void DW1000Class::startTransmit() {
	writeTransmitFrameControlRegister();
	setBit(_sysctrl, LEN_SYS_CTRL, SFCST_BIT, !_frameCheck);
	setBit(_sysctrl, LEN_SYS_CTRL, TXSTRT_BIT, true);
	writeBytes(spi, SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
	if(_permanentReceive) {
		memset(_sysctrl, 0, LEN_SYS_CTRL);
		_deviceMode = RX_MODE;
		startReceive();
	} else {
		_deviceMode = IDLE_MODE;
	}
}

void DW1000Class::newConfiguration() {
	idle();
	readNetworkIdAndDeviceAddress();
	readSystemConfigurationRegister();
	readChannelControlRegister();
	readTransmitFrameControlRegister();
	readSystemEventMaskRegister();
}

void DW1000Class::commitConfiguration() {
	// write all configurations back to device
	writeNetworkIdAndDeviceAddress(); //TODO:
	writeSystemConfigurationRegister();
	writeChannelControlRegister();
	writeTransmitFrameControlRegister();
	writeSystemEventMaskRegister();
	// tune according to configuration
	tune();
	// TODO clean up code + antenna delay/calibration API
	// TODO setter + check not larger two bytes integer
	uint8_t  antennaDelayBytes[LEN_STAMP];
	writeValueToBytes(antennaDelayBytes, 16384, LEN_STAMP);
	_antennaDelay.setTimestamp(antennaDelayBytes);
	writeBytes(spi, TX_ANTD, NO_SUB, antennaDelayBytes, LEN_TX_ANTD);
    writeBytes(spi, LDE_IF, LDE_RXANTD_SUB, antennaDelayBytes, LEN_LDE_RXANTD); 
}

void DW1000Class::waitForResponse(bool val) {
	setBit(_sysctrl, LEN_SYS_CTRL, WAIT4RESP_BIT, val);
}

void DW1000Class::suppressFrameCheck(bool val) {
	_frameCheck = !val;
}

void DW1000Class::useSmartPower(bool smartPower) {
	_smartPower = smartPower;
	setBit(_syscfg, LEN_SYS_CFG, DIS_STXP_BIT, !smartPower);
}

DW1000Time DW1000Class::setDelay(const DW1000Time& delay) {
	if(_deviceMode == TX_MODE) {
		setBit(_sysctrl, LEN_SYS_CTRL, TXDLYS_BIT, true);
	} else if(_deviceMode == RX_MODE) {
		setBit(_sysctrl, LEN_SYS_CTRL, RXDLYS_BIT, true);
	} else {
		// in idle, ignore
		return DW1000Time();
	}
	uint8_t  delayBytes[5];
	DW1000Time futureTime;
	getSystemTimestamp(futureTime);
	futureTime += delay;
	futureTime.getTimestamp(delayBytes);
	delayBytes[0] = 0;
	delayBytes[1] &= 0xFE;
	writeBytes(spi, DX_TIME, NO_SUB, delayBytes, LEN_DX_TIME);
	// adjust expected time with configured antenna delay
	futureTime.setTimestamp(delayBytes);
	futureTime += _antennaDelay;
	return futureTime;
}

void DW1000Class::setDataRate(uint8_t  rate) {
	rate &= 0x03;
	_txfctrl[1] &= 0x83;
	_txfctrl[1] |= (uint8_t )((rate << 5) & 0xFF);
	// special 110kbps flag
	if(rate == TRX_RATE_110KBPS) {
		setBit(_syscfg, LEN_SYS_CFG, RXM110K_BIT, true);
	} else {
		setBit(_syscfg, LEN_SYS_CFG, RXM110K_BIT, false);
	}
	// SFD mode and type (non-configurable, as in Table )
	if(rate == TRX_RATE_6800KBPS) {
		setBit(_chanctrl, LEN_CHAN_CTRL, DWSFD_BIT, false);
		setBit(_chanctrl, LEN_CHAN_CTRL, TNSSFD_BIT, false);
		setBit(_chanctrl, LEN_CHAN_CTRL, RNSSFD_BIT, false);
	} else {
		setBit(_chanctrl, LEN_CHAN_CTRL, DWSFD_BIT, true);
		setBit(_chanctrl, LEN_CHAN_CTRL, TNSSFD_BIT, true);
		setBit(_chanctrl, LEN_CHAN_CTRL, RNSSFD_BIT, true);	
		
	}
	uint8_t  sfdLength;
	if(rate == TRX_RATE_6800KBPS) {
		sfdLength = 0x08;
	} else if(rate == TRX_RATE_850KBPS) {
		sfdLength = 0x10;
	} else {
		sfdLength = 0x40;
	}
	writeBytes(spi, USR_SFD, SFD_LENGTH_SUB, &sfdLength, LEN_SFD_LENGTH);
	_dataRate = rate;
}

void DW1000Class::setPulseFrequency(uint8_t  freq) {
	freq &= 0x03;
	_txfctrl[2] &= 0xFC;
	_txfctrl[2] |= (uint8_t )(freq & 0xFF);
	_chanctrl[2] &= 0xF3;
	_chanctrl[2] |= (uint8_t )((freq << 2) & 0xFF);
	_pulseFrequency = freq;
}

void DW1000Class::setPreambleLength(uint8_t  prealen) {
	prealen &= 0x0F;
	_txfctrl[2] &= 0xC3;
	_txfctrl[2] |= (uint8_t )((prealen << 2) & 0xFF);
	if(prealen == TX_PREAMBLE_LEN_64 || prealen == TX_PREAMBLE_LEN_128) {
		_pacSize = PAC_SIZE_8;
	} else if(prealen == TX_PREAMBLE_LEN_256 || prealen == TX_PREAMBLE_LEN_512) {
		_pacSize = PAC_SIZE_16;
	} else if(prealen == TX_PREAMBLE_LEN_1024) {
		_pacSize = PAC_SIZE_32;
	} else {
		_pacSize = PAC_SIZE_64;
	}
	_preambleLength = prealen;
}

void DW1000Class::useExtendedFrameLength(bool val) {
	_extendedFrameLength = (val ? FRAME_LENGTH_EXTENDED : FRAME_LENGTH_NORMAL);
	_syscfg[2] &= 0xFC;
	_syscfg[2] |= _extendedFrameLength;
}

void DW1000Class::receivePermanently(bool val) {
	_permanentReceive = val;
	if(val) {
		// in case permanent, also reenable receiver once failed
		setReceiverAutoReenable(true);
		writeSystemConfigurationRegister();
	}
}

void DW1000Class::setChannel(uint8_t  channel) {
	channel &= 0xF;
	_chanctrl[0] = ((channel | (channel << 4)) & 0xFF);
	_channel = channel;
}

void DW1000Class::setPreambleCode(uint8_t  preacode) {
	preacode &= 0x1F;
	_chanctrl[2] &= 0x3F;
	_chanctrl[2] |= ((preacode << 6) & 0xFF);
	_chanctrl[3] = 0x00;
	_chanctrl[3] = ((((preacode >> 2) & 0x07) | (preacode << 3)) & 0xFF);
	_preambleCode = preacode;
}

void DW1000Class::setDefaults() {
	if(_deviceMode == TX_MODE) {

	} else if(_deviceMode == RX_MODE) {

	} else if(_deviceMode == IDLE_MODE) {
		useExtendedFrameLength(false);
		useSmartPower(false);
		suppressFrameCheck(false);
		setFrameFilter(false);
		interruptOnSent(true);
		interruptOnReceived(true);
		interruptOnReceiveFailed(true);
		interruptOnReceiveTimestampAvailable(false);
		interruptOnAutomaticAcknowledgeTrigger(true);
		setReceiverAutoReenable(true);
		// default mode when powering up the chip
		// still explicitly selected for later tuning
		enableMode(MODE_LONGDATA_RANGE_LOWPOWER);
	}
}

void DW1000Class::setData(uint8_t  data[], uint16_t n) {
	if(_frameCheck) {
		n+=2; // two bytes CRC-16
	}
	if(n > LEN_EXT_UWB_FRAMES) {
		return; // TODO proper error handling: frame/buffer size
	}
	if(n > LEN_UWB_FRAMES && !_extendedFrameLength) {
		return; // TODO proper error handling: frame/buffer size
	}
	// transmit data and length
	writeBytes(spi, TX_BUFFER, NO_SUB, data, n);
	_txfctrl[0] = (uint8_t )(n & 0xFF); // 1 byte (regular length + 1 bit)
	_txfctrl[1] &= 0xE0;
	_txfctrl[1] |= (uint8_t )((n >> 8) & 0x03);	// 2 added bits if extended length
}

/*
//TODO
void DW1000Class::setData(const std::string& data) {
	unsigned int n = data.length()+1;
	uint8_t* dataBytes = (uint8_t*)malloc(n);
	data.getBytes(dataBytes, n);
	setData(dataBytes, n);
	free(dataBytes);
}*/

unsigned int DW1000Class::getDataLength() {
	unsigned int len = 0;
	if(_deviceMode == TX_MODE) {
		// 10 bits of TX frame control register
		len = ((((unsigned int)_txfctrl[1] << 8) | (unsigned int)_txfctrl[0]) & 0x03FF);
	} else if(_deviceMode == RX_MODE) {
		// 10 bits of RX frame control register
		uint8_t  rxFrameInfo[LEN_RX_FINFO];
		readBytes(spi, RX_FINFO, NO_SUB, rxFrameInfo, LEN_RX_FINFO);
		len = ((((unsigned int)rxFrameInfo[1] << 8) | (unsigned int)rxFrameInfo[0]) & 0x03FF);
	}
	if(_frameCheck && len > 2) {
		return len-2;
	}
	return len;
}

void DW1000Class::getData(uint8_t data[], uint16_t n) {
	if(n <= 0) {
		return;
	}
	readBytes(spi, RX_BUFFER, NO_SUB, data, n);
}

/*
//  TODO
void DW1000Class::getData(std::string& data) {
	unsigned int i;
	unsigned int n = getDataLength(); // number of uint8_ts w/o the two FCS ones
	if(n <= 0) {
		return;
	}
	uint8_t* dataBytes = (uint8_t*)malloc(n);
	getData(dataBytes, n);
	// clear string
	//data.remove(0);
	data.erase(data.begin(),data.end());
	data = "";
	// append to string
	for(i = 0; i < n; i++) {
		data += (char)dataBytes[i];
	}
	free(dataBytes);
}*/

void DW1000Class::getTransmitTimestamp(DW1000Time& time) {
	uint8_t txTimeBytes[LEN_TX_STAMP];
	readBytes(spi, TX_TIME, TX_STAMP_SUB, txTimeBytes, LEN_TX_STAMP);
	time.setTimestamp(txTimeBytes);
}

void DW1000Class::getReceiveTimestamp(DW1000Time& time) {
	uint8_t rxTimeBytes[LEN_RX_STAMP];
	readBytes(spi, RX_TIME, RX_STAMP_SUB, rxTimeBytes, LEN_RX_STAMP);
	time.setTimestamp(rxTimeBytes);
}

void DW1000Class::getSystemTimestamp(DW1000Time& time) {
	uint8_t sysTimeBytes[LEN_SYS_TIME];
	readBytes(spi, SYS_TIME, NO_SUB, sysTimeBytes, LEN_SYS_TIME);
	time.setTimestamp(sysTimeBytes);
}

void DW1000Class::getTransmitTimestamp(uint8_t data[]) {
	readBytes(spi, TX_TIME, TX_STAMP_SUB, data, LEN_TX_STAMP);
}

void DW1000Class::getReceiveTimestamp(uint8_t data[]) {
	readBytes(spi, RX_TIME, RX_STAMP_SUB, data, LEN_RX_STAMP);
}

void DW1000Class::getSystemTimestamp(uint8_t data[]) {
	readBytes(spi, SYS_TIME, NO_SUB, data, LEN_SYS_TIME);
}

bool DW1000Class::isTransmitDone() {
	return getBit(_sysstatus, LEN_SYS_STATUS, TXFRS_BIT);
}

bool DW1000Class::isReceiveTimestampAvailable() {
	return getBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT);
}

bool DW1000Class::isReceiveDone() {
	if(_frameCheck) {
		return getBit(_sysstatus, LEN_SYS_STATUS, RXFCG_BIT);
	}
	return getBit(_sysstatus, LEN_SYS_STATUS, RXDFR_BIT);
}

bool DW1000Class::isReceiveFailed() {
	bool ldeErr, rxCRCErr, rxHeaderErr, rxDecodeErr;
	ldeErr = getBit(_sysstatus, LEN_SYS_STATUS, LDEERR_BIT);
	rxCRCErr = getBit(_sysstatus, LEN_SYS_STATUS, RXFCE_BIT);
	rxHeaderErr = getBit(_sysstatus, LEN_SYS_STATUS, RXPHE_BIT);
	rxDecodeErr = getBit(_sysstatus, LEN_SYS_STATUS, RXRFSL_BIT);
	if(ldeErr || rxCRCErr || rxHeaderErr || rxDecodeErr) {
		return true; 
	}
	return false;
}

bool DW1000Class::isReceiveTimeout() {
	return getBit(_sysstatus, LEN_SYS_STATUS, RXRFTO_BIT);
}

bool DW1000Class::isClockProblem() {
	bool clkllErr, rfllErr;
	clkllErr = getBit(_sysstatus, LEN_SYS_STATUS, CLKPLL_LL_BIT);
	rfllErr = getBit(_sysstatus, LEN_SYS_STATUS, RFPLL_LL_BIT);
	if(clkllErr || rfllErr) {
		return true;
	}
	return false;
}

void DW1000Class::clearAllStatus() {
	memset(_sysstatus, 0, LEN_SYS_STATUS);
	writeBytes(spi, SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000Class::clearReceiveTimestampAvailableStatus() {
	setBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT, true);
	writeBytes(spi, SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000Class::clearReceiveStatus() {
	// clear latched RX bits (i.e. write 1 to clear)
	setBit(_sysstatus, LEN_SYS_STATUS, RXDFR_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, LDEERR_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXPHE_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXFCE_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXFCG_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXRFSL_BIT, true);
	writeBytes(spi, SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000Class::clearTransmitStatus() {
	// clear latched TX bits
	setBit(_sysstatus, LEN_SYS_STATUS, TXFRB_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, TXPRS_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, TXPHS_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, TXFRS_BIT, true);
	writeBytes(spi, SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

float DW1000Class::getReceiveQuality() {
	uint8_t noiseBytes[LEN_STD_NOISE];
	uint8_t fpAmpl2Bytes[LEN_FP_AMPL2];
	unsigned int noise, f2;
	readBytes(spi, RX_FQUAL, STD_NOISE_SUB, noiseBytes, LEN_STD_NOISE);
	readBytes(spi, RX_FQUAL, FP_AMPL2_SUB, fpAmpl2Bytes, LEN_FP_AMPL2);
	noise = (unsigned int)noiseBytes[0] | ((unsigned int)noiseBytes[1] << 8);
	f2 = (unsigned int)fpAmpl2Bytes[0] | ((unsigned int)fpAmpl2Bytes[1] << 8);
	return (float)f2 / noise;
}

float DW1000Class::getFirstPathPower() {
	uint8_t fpAmpl1Bytes[LEN_FP_AMPL1];
	uint8_t fpAmpl2Bytes[LEN_FP_AMPL2];
	uint8_t fpAmpl3Bytes[LEN_FP_AMPL3];
	uint8_t rxFrameInfo[LEN_RX_FINFO];
	unsigned int f1, f2, f3, N;
	float A, corrFac;
	readBytes(spi, RX_TIME, FP_AMPL1_SUB, fpAmpl1Bytes, LEN_FP_AMPL1);
	readBytes(spi, RX_FQUAL, FP_AMPL2_SUB, fpAmpl2Bytes, LEN_FP_AMPL2);
	readBytes(spi, RX_FQUAL, FP_AMPL3_SUB, fpAmpl3Bytes, LEN_FP_AMPL3);
	readBytes(spi, RX_FINFO, NO_SUB, rxFrameInfo, LEN_RX_FINFO);
	f1 = (unsigned int)fpAmpl1Bytes[0] | ((unsigned int)fpAmpl1Bytes[1] << 8);
	f2 = (unsigned int)fpAmpl2Bytes[0] | ((unsigned int)fpAmpl2Bytes[1] << 8);
	f3 = (unsigned int)fpAmpl3Bytes[0] | ((unsigned int)fpAmpl3Bytes[1] << 8);
	N = (((unsigned int)rxFrameInfo[2] >> 4) & 0xFF) | ((unsigned int)rxFrameInfo[3] << 4);
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		A = 115.72;
		corrFac = 2.3334;
	} else {
		A = 121.74;
		corrFac = 1.1667;
	}
	float estFpPwr = 10.0 * log10(((float)f1 * (float)f1 + (float)f2 * (float)f2 + (float)f3 * (float)f3) / ((float)N * (float)N)) - A;
	if(estFpPwr <= -88) {
		return estFpPwr;
	} else {
		// approximation of Fig. 22 in user manual for dbm correction
		estFpPwr += (estFpPwr + 88) * corrFac;
	}
	return estFpPwr;
}

float DW1000Class::getReceivePower() {
	uint8_t cirPwrBytes[LEN_CIR_PWR];
	uint8_t rxFrameInfo[LEN_RX_FINFO];
	unsigned long twoPower17 = 131072;
	unsigned int C, N;
	float A, corrFac;
	readBytes(spi, RX_FQUAL, CIR_PWR_SUB, cirPwrBytes, LEN_CIR_PWR);
	readBytes(spi, RX_FINFO, NO_SUB, rxFrameInfo, LEN_RX_FINFO);
	C = (unsigned int)cirPwrBytes[0] | ((unsigned int)cirPwrBytes[1] << 8);
	N = (((unsigned int)rxFrameInfo[2] >> 4) & 0xFF) | ((unsigned int)rxFrameInfo[3] << 4);
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		A = 115.72;
		corrFac = 2.3334;
	} else {
		A = 121.74;
		corrFac = 1.1667;
	}
	float estRxPwr = 10.0 * log10(((float)C * (float)twoPower17) / ((float)N * (float)N)) - A;
	if(estRxPwr <= -88) {
		return estRxPwr;
	} else {
		// approximation of Fig. 22 in user manual for dbm correction
		estRxPwr += (estRxPwr + 88) * corrFac;
	}
	return estRxPwr;
}



void DW1000Class::writeValueToBytes(uint8_t data[], long val, unsigned int n) {
	uint32_t i;	
	for(i = 0; i < n; i++) {
		data[i] = ((val >> (i * 8)) & 0xFF);
	}
}


/* ###########################################################################
 * #### Helper functions #####################################################
 * ######################################################################### */

/*
 * Set the value of a bit in an array of bytes that are considered
 * consecutive and stored from MSB to LSB.
 * @param data
 * 		The number as byte array.
 * @param n
 * 		The number of bytes in the array.
 * @param bit
 * 		The position of the bit to be set.
 * @param val
 *		The bool value to be set to the given bit position.
 */
void DW1000Class::setBit(uint8_t data[], unsigned int n, unsigned int bit, bool val) {
	uint32_t idx;
	int shift;

	idx = bit / 8;
	if(idx >= n) {
		return; // TODO proper error handling: out of bounds
	}
	//uint8_t* targetByte = &data[idx];
	//uint8_t target = data[idx];
	shift = bit % 8;
	//uint8_t mask;

	if(val) {
		data[idx] |= 0x01 << shift;
	} else {
		data[idx] &= ~(0x01 << shift);
	}
}


/*
 * Check the value of a bit in an array of bytes that are considered
 * consecutive and stored from MSB to LSB.
 * @param data
 * 		The number as byte array.
 * @param n
 * 		The number of bytes in the array.
 * @param bit
 * 		The position of the bit to be checked.
 */
bool DW1000Class::getBit(uint8_t data[], unsigned int n, unsigned int bit) {
	uint32_t idx;
	int shift;

	idx = bit / 8;
	if(idx >= n) {
		return false; // TODO proper error handling: out of bounds
	}
	uint8_t targetByte = data[idx];
	shift = bit % 8;
	uint8_t returnbit = 0;

	returnbit = (targetByte >> shift) & 0x01;
	
	return returnbit;
}


/*
 * Read bytes from the DW1000. Number of bytes depend on register length.
 * @param cmd 
 * 		The register address (see Chapter 7 in the DW1000 user manual).
 * @param data 
 *		The data array to be read into.
 * @param n
 *		The number of bytes expected to be received.
 */
void DW1000Class::readBytes(nrf_drv_spi_t m_spi, uint8_t cmd, uint16_t offset, uint8_t data[], unsigned int n) {
	uint8_t header[3] = {0,0,0};
	int headerLen = 1;
	//int i;
	spi_xfer_done = false;
	if(offset == NO_SUB) {
		header[0] = READ | cmd;
	} else {
		header[0] = READ_SUB | cmd;
		if(offset < 128) {
			header[1] = (uint8_t)offset;
			headerLen++;
		} else {
			header[1] = READ | (uint8_t)offset;
			header[2] = (uint8_t)(offset >> 7);
			headerLen+=2;
		}
	}

	APP_ERROR_CHECK(nrf_drv_spi_transfer(&m_spi, header, headerLen, data, n));

	//while (!spi_xfer_done)
	//{
	//	__WFE();
	//}

	nrf_delay_ms(1);
}

// always 4 bytes
void DW1000Class::readBytesOTP(nrf_drv_spi_t m_spi,uint16_t address, uint8_t data[]) {
	uint8_t addressBytes[LEN_OTP_ADDR];
	uint8_t otpctrl[LEN_OTP_CTRL];
	readBytes(m_spi, OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	// bytes of address
	addressBytes[0] = (address & 0xFF);
	addressBytes[1] = ((address >> 8) & 0xFF);
    // set address
    writeBytes(m_spi, OTP_IF, OTP_ADDR_SUB, addressBytes, LEN_OTP_ADDR);
	otpctrl[0] = 0x03;
	writeBytes(m_spi, OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	otpctrl[0] = 0x01;
	writeBytes(m_spi, OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	// read value
	readBytes(m_spi, OTP_IF, OTP_RDAT_SUB, data, LEN_OTP_RDAT);
	otpctrl[0] = 0x00;
	writeBytes(m_spi, OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
}

/*
 * Write bytes to the DW1000. Single bytes can be written to registers via sub-addressing.
 * @param cmd 
 * 		The register address (see Chapter 7 in the DW1000 user manual).
 * @param offset
 *		The offset to select register sub-parts for writing, or 0x00 to disable 
 * 		sub-adressing.
 * @param data 
 *		The data array to be written.
 * @param n
 *		The number of bytes to be written (take care not to go out of bounds of 
 * 		the register).
 */
void DW1000Class::writeBytes(nrf_drv_spi_t m_spi, uint8_t cmd, uint16_t offset, uint8_t data[], unsigned int n) {
	uint8_t header[1] = {0};
	uint32_t headerLen = 1;
	uint32_t i;
	// TODO proper error handling: address out of bounds
	if(offset == NO_SUB) {
		header[0] = WRITE | cmd;
	} else {
		header[0] = WRITE_SUB | cmd;
		if(offset < 128) {
			header[1] = (uint8_t)offset;
			headerLen++;
		} else {
			header[1] = WRITE | (uint8_t)offset;
			header[2] = (uint8_t)(offset >> 7);
			headerLen+=2;
		}
	}
	spi_xfer_done = false;
	uint32_t sendLen = n+headerLen;
	uint8_t send_data[sendLen];
	for(i = 0; i < n; i++) {
		if(i<headerLen){
			send_data[i] = header[i];
		}
		else{
			uint32_t dataindex = i - headerLen;
			send_data[i] = data[dataindex];

		}
	}
	uint8_t buffer[2];
	APP_ERROR_CHECK(nrf_drv_spi_transfer(&m_spi, send_data, sendLen, buffer, 2));

	//while (!spi_xfer_done)
	//{
	//	__WFE();
	//}
}



void DW1000Class::getPrettyBytes(nrf_drv_spi_t m_spi,uint8_t data[], char msgBuffer[], unsigned int n) {
	unsigned int i, j, b;
	b = sprintf(msgBuffer, "Data, bytes: %d\nB: 7 6 5 4 3 2 1 0\n", n);
	for(i = 0; i < n; i++) {
		uint8_t curByte = data[i];
		snprintf(&msgBuffer[b++], 2, "%d", (i + 1));
		msgBuffer[b++] = (char)((i + 1) & 0xFF); msgBuffer[b++] = ':'; msgBuffer[b++] = ' ';
		for(j = 0; j < 8; j++) {
			msgBuffer[b++] = ((curByte >> (7 - j)) & 0x01) ? '1' : '0';
			if(j < 7) {
				msgBuffer[b++] = ' '; 
			} else if(i < n-1) {
				msgBuffer[b++] = '\n';
			} else {
				msgBuffer[b++] = '\0';
			}
		}
		
	}
	msgBuffer[b++] = '\0';
}

void DW1000Class::getPrettyBytes(nrf_drv_spi_t m_spi, uint8_t cmd, uint16_t offset, char msgBuffer[], unsigned int n) {
	unsigned int i, j, b;
	uint8_t* readBuf = (uint8_t*)malloc(n);
	readBytes(m_spi, cmd, offset, readBuf, n);
	b = sprintf(msgBuffer, "Reg: 0x%02x, bytes: %d\nB: 7 6 5 4 3 2 1 0\n", cmd, n);
	for(i = 0; i < n; i++) {
		uint8_t curByte = readBuf[i];
		snprintf(&msgBuffer[b++], 2, "%d", (i + 1));
		msgBuffer[b++] = (char)((i + 1) & 0xFF); msgBuffer[b++] = ':'; msgBuffer[b++] = ' ';
		for(j = 0; j < 8; j++) {
			msgBuffer[b++] = ((curByte >> (7 - j)) & 0x01) ? '1' : '0';
			if(j < 7) {
				msgBuffer[b++] = ' '; 
			} else if(i < n-1) {
				msgBuffer[b++] = '\n';
			} else {
				msgBuffer[b++] = '\0';
			}
		}
		
	}
	msgBuffer[b++] = '\0';
	free(readBuf);
}


void DW1000Class::printDeviceData(){
	char msg[1024];
	DW1000.getPrintableDeviceIdentifier(msg);
	DW1000.getPrintableExtendedUniqueIdentifier(msg);
	//DEBUG_PRINTF("DW1000 Unique ID: %c \r\n", msg);
	DW1000.getPrintableNetworkIdAndShortAddress(msg);
	//DEBUG_PRINTF("DW1000 Network ID & Device address: %c \r\n", msg);
	DW1000.getPrintableDeviceMode(msg);
	//DEBUG_PRINTF("DW1000 Device Mode: %c \r\n", msg);
	nrf_delay_ms(10000);
}






