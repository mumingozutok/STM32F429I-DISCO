/*
 * adaptor.h
 *
 *  Created on: Nov 8, 2021
 *      Author: mg
 */

#ifndef ADAPTOR_H_
#define ADAPTOR_H_

#define ANALOG_INPUT_CH_COUNT 2
#define ADDR_FLASH_SECTOR_23     ((uint32_t)0x081E0000)
#define FLASH_MEMORY_SIZE (128*1024)

typedef struct S_Adaptation{
	TIM_HandleTypeDef* comm_htim;
	UART_HandleTypeDef* comm_huart;

} runtime_adaptor;

typedef struct S_Digital_Channel{
	uint32_t port;
	uint32_t pin;
} Digital_Channel;

typedef struct S_ADC_Info{
	ADC_HandleTypeDef* adc;
	uint8_t ready_flag;
	uint16_t data[16];
	uint8_t ch_count;
} ADC_Info;

typedef struct S_Analog_Input_Channel{
	uint16_t data;
	ADC_Info* ai; //hold the information of the adc peripheral associated to this channel
} Analog_Input_Channel;

void initiate_runtime();
void hal_xfer_watch_data(uint8_t len, uint8_t* watch_data);
void Display_Clear();
void  Display_Number(int32_t startX, int32_t startY,
											int32_t width, int32_t height,
												int32_t attr, int32_t val);
void  Display_String(int32_t startX, int32_t startY,
												int32_t width, int32_t height,
													int32_t attr, char* str, uint16_t len);
int8_t hal_get_encoder_value(uint8_t ch);
uint8_t erase_flash(uint32_t start_addr);
uint8_t write_to_flash(uint8_t* p, uint32_t start_addr, uint16_t size);
void get_flash_memory_info(uint32_t* start_addr, uint32_t* size);
void get_uniqueid(uint8_t* id, uint16_t len);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void hal_modbus_uart_tx(uint8_t* pData, uint16_t Size);
void Runtime_Adaptor_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void stop_comm_timer(TIM_HandleTypeDef* htim);
void start_comm_timer(TIM_HandleTypeDef* htim);
void init_comm_timing_service();
void init_comm_data_service();
void hal_init_tick();
uint32_t hal_get_tick();
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
uint32_t hal_read_analog_ch(uint32_t chNum);
uint8_t  hal_gpio_read_pin(uint32_t chNum);
void hal_gpio_write_pin(uint16_t chNum, uint8_t value);
void initate_analog_channels();
void initiate_output_channels();
void initiate_input_channels();

#endif /* ADAPTOR_H_ */
