#include "veml6075.h"

#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_drv_twi.h"
#include "bsp.h"

#include "i2c_driver.h"
#include "ble_driver.h"
#include "ble_nus.h"


//Different read function than the others
  uint16_t veml6075_read_2bytes(nrf_drv_twi_t twi_master,uint8_t addr, uint8_t subAddress){
    ret_code_t ret;
    uint16_t full_data;
    uint8_t return_buffer[2];
    return_buffer[0] = 0;
    return_buffer[1] = 0;
    uint8_t buffer[1];
    buffer[0] = subAddress;


    //NRF_LOG_RAW_INFO("Inside read 2 bytes \r\n");
    ret = nrf_drv_twi_tx(&twi_master, addr, buffer, 1, true);
    if (NRF_SUCCESS != ret){
        NRF_LOG_WARNING("Communication error when asking to read\r\n");
        return (uint8_t)ret;
    }
    ret = nrf_drv_twi_rx(&twi_master, addr, return_buffer, 2);
    if (NRF_SUCCESS != ret){
        NRF_LOG_WARNING("Communication error when reading first byte\r\n");
        return (uint8_t)ret;
    }
    full_data = (return_buffer[1]<<8) | return_buffer[0];
    return full_data;

}


 void veml6075_begin(nrf_drv_twi_t twi_master){
    uint8_t UV_CONF_WORD = 0x00;

    UV_CONF_WORD  = (Veml6075_settings.UV_IT   <<4)  & 0x70;        
    UV_CONF_WORD |= (Veml6075_settings.HD      <<3)  & 0x08;
    UV_CONF_WORD |= (Veml6075_settings.UV_TRIG <<2)  & 0x04;
    UV_CONF_WORD |= (Veml6075_settings.UV_AF   <<1)  & 0x02;
    UV_CONF_WORD |= (Veml6075_settings.Veml6075_SD)  & 0x01;
    //NRF_LOG_RAW_INFO("UVA about to write the conf \r\n");

    write_2bytes(twi_master,Veml6075_DEVICE_ADDRESS,Veml6075_UV_CONF,UV_CONF_WORD,0x00);

} 

void veml6075_powerdown(nrf_drv_twi_t twi_master){
    //Powerdown with SD bit = 1
    write_2bytes(twi_master,Veml6075_DEVICE_ADDRESS,Veml6075_UV_CONF,0x01,0x00);
}

 void veml6075_setup(){
    Veml6075_settings.UV_IT       = 0;
    Veml6075_settings.HD          = 0;
    Veml6075_settings.UV_TRIG     = 0;
    Veml6075_settings.UV_AF       = 0;
    Veml6075_settings.Veml6075_SD = 0; 
}

 uint16_t veml6075_init(nrf_drv_twi_t twi_master){

    veml6075_setup();
    veml6075_begin(twi_master);
    uint16_t who_am_i = veml6075_whoami(twi_master);
    
    return who_am_i;

}



bool veml6075_pass(nrf_drv_twi_t twi_master){
        
    uint16_t who_am_i = veml6075_whoami(twi_master);
    
    if(who_am_i==0x0026){
        NRF_LOG_RAW_INFO("Veml6075: Pass %x == 0x0026\r\n", who_am_i);
        //printf("Veml6075: Pass %x ==0x0026 \r\n", who_am_i);

        return true;
    }
    else{
        NRF_LOG_RAW_INFO("Veml6075: Fail %x != 0x0026 \r\n", who_am_i);
        //printf("Veml6075: Fail %x != 0x0026 \r\n", who_am_i);
        return false;
    }
}



 uint16_t veml6075_whoami(nrf_drv_twi_t twi_master){
    uint16_t who_am_i = veml6075_read_2bytes(twi_master,Veml6075_DEVICE_ADDRESS,Veml6075_ID);
    return who_am_i;
}

 uint16_t veml6075_readUVA(nrf_drv_twi_t twi_master){
    uint16_t UVA_data = veml6075_read_2bytes(twi_master,Veml6075_DEVICE_ADDRESS,Veml6075_UVA_DATA);
    return UVA_data;
}

 uint16_t veml6075_readUVB(nrf_drv_twi_t twi_master){
    uint16_t UVB_data = veml6075_read_2bytes(twi_master,Veml6075_DEVICE_ADDRESS,Veml6075_UVB_DATA);
    return UVB_data;
}

 uint16_t run_veml6075(nrf_drv_twi_t twi_master){
    uint16_t who_am_i = veml6075_whoami(twi_master);
    NRF_LOG_RAW_INFO("UVA Sensor ID: %.4x.\r\n", who_am_i);

    uint16_t UVA_data = veml6075_readUVA(twi_master);
    NRF_LOG_RAW_INFO("UVA Sensor UVA: %.4x.\r\n", UVA_data);

    uint16_t UVB_data = veml6075_readUVB(twi_master);
    NRF_LOG_RAW_INFO("UVA Sensor UVB: %.4x.\r\n", UVB_data);
    NRF_LOG_RAW_INFO("\r\n");

    return who_am_i;
}


uint8_t run_veml6075_ble(nrf_drv_twi_t twi_master,ble_nus_t m_nus){
    uint8_t length = 32;
    uint8_t *ble_string[length];
    int n;

    uint16_t who_am_i = veml6075_whoami(twi_master);
    n = snprintf((char *)ble_string, length, "veml6075id:%x\r\n",who_am_i);
    send_ble_data(m_nus,(uint8_t *)ble_string,n);

    uint16_t UVA_data = veml6075_readUVA(twi_master);
    n = snprintf((char *)ble_string, length, "veml6075a:%d\r\n",UVA_data);
    send_ble_data(m_nus,(uint8_t *)ble_string,n);

    uint16_t UVB_data = veml6075_readUVB(twi_master);
    n = snprintf((char *)ble_string, length,"veml6075b:%d\r\n",UVB_data);
    send_ble_data(m_nus,(uint8_t *)ble_string,n);

    return who_am_i;

}
