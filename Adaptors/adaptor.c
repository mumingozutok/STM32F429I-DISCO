/*
 * adaptor.c
 *
 *  Created on: Nov 8, 2021
 *      Author: mg
 */

#include "stm32f4xx_hal.h"
#include "adaptor.h"

//images
#include "image_ok.h"
#include "image_error.h"
#include "image_warning1.h"
#include "image_techsafe_logo_color.h"
#include "bmp_parser.h"

#include "stm32f429i_discovery_lcd.h"

extern TIM_HandleTypeDef htim7; //communication timer
extern TIM_HandleTypeDef htim1; //encoder
extern UART_HandleTypeDef huart1;
extern ADC_HandleTypeDef hadc1;

static ADC_Info adc_info = {.adc = &hadc1, .ready_flag = 1, .ch_count = 2};
static Digital_Channel inputChannel[1];
static Digital_Channel outputChannel[4];
static Analog_Input_Channel analog_input_channel[ANALOG_INPUT_CH_COUNT];

runtime_adaptor ra = {.comm_htim = &htim7,
		.comm_huart = &huart1};
runtime_adaptor* get_runtime_adaptor(){return &ra;}

uint8_t uart_rx_data;
uint32_t counter_hal_dma = 0, counter_dma_callback = 0;


void initiate_input_channels(){
	inputChannel[0].port = GPIOA;
	inputChannel[0].pin = GPIO_PIN_0;
}

void initiate_output_channels(){
	outputChannel[0].port = GPIOG;
	outputChannel[0].pin = GPIO_PIN_13;

	outputChannel[1].port = GPIOG;
	outputChannel[1].pin = GPIO_PIN_14;

	/*
	outputChannel[2].port = GPIOB;
	outputChannel[2].pin = GPIO_PIN_13;

	outputChannel[3].port = GPIOC;
	outputChannel[3].pin = GPIO_PIN_5;
	 */
}

void initate_analog_channels(){
	analog_input_channel[0].ai = &adc_info;
	analog_input_channel[0].data = 0;

	analog_input_channel[1].ai = &adc_info;
	analog_input_channel[1].data = 0;

}

//Leds are connected to: PF0-PF2-PF13
//Please write down GPIO output function in your hardware
void hal_gpio_write_pin(uint16_t chNum, uint8_t value){
	HAL_GPIO_WritePin(outputChannel[chNum].port, outputChannel[chNum].pin, value);
}

uint8_t  hal_gpio_read_pin(uint32_t chNum){
	//return values >= 2, depicts error
	return HAL_GPIO_ReadPin(inputChannel[chNum].port, inputChannel[chNum].pin);
}

//------------------ADC readings with DMA---------------------------------------
uint32_t hal_read_analog_ch(uint32_t chNum){
	Analog_Input_Channel* ch = &analog_input_channel[chNum];
	ADC_Info* ai = ch->ai;
	if(ai->ready_flag == 1){ //check for the conv finish
		//Start DMA controlled ADC single convertion
		//this adc is working on two channels IN3 and IN10
		//so there will be two channel value after one conversion
		ai->ready_flag = 0; //disable the flag to notify there is an ongoing process
		HAL_ADC_Start_DMA(ai->adc, (uint32_t*) &ai->data, ai->ch_count);
		counter_hal_dma++;
	}

	return analog_input_channel[chNum].data;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	Analog_Input_Channel* ch;
	ADC_Info* ai;
	for(uint8_t i=0;i<ANALOG_INPUT_CH_COUNT;i++){
		ch = &analog_input_channel[i];
		ai = ch->ai;
		if(hadc == ai->adc){
			ch->data = ai->data[i];
			ai->ready_flag = 1;
			counter_dma_callback++;
		}
	}
}
//-------------------------------------------------------------------------------

//Please write down "get system tick" function in your hardware
uint32_t hal_get_tick(){
	return HAL_GetTick();
}

void hal_init_tick(){
	HAL_InitTick(0);
}

//Communication Channel Adaptation

//Please write down functions for your communication channel
//And put this function right after your initialisations
void init_comm_data_service(){
	HAL_UART_Receive_IT(ra.comm_huart, &uart_rx_data, 1);
}

//Please write down functions for communication timing services
//And put this function right after your initialisations
void init_comm_timing_service(){
	HAL_TIM_Base_Start_IT(ra.comm_htim);
	stop_comm_timer(ra.comm_htim);
}

void start_comm_timer(TIM_HandleTypeDef* htim){
	htim->Instance->CR1 &= ~0x01; //Stop Timer
	htim->Instance->CNT = 0; //Reset Counter
	htim->Instance->CR1 |= 0x01; //Start Timer
}

void stop_comm_timer(TIM_HandleTypeDef* htim){
	htim->Instance->CR1 &= ~0x01; //Stop Timer
	htim->Instance->CNT = 0; //Reset Counter
}

//Callbacks
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	HAL_UART_Receive_IT(huart, &uart_rx_data, 1);
	start_comm_timer(ra.comm_htim);

	//When new data received copy this data to the runtime buffers
	Runtime_CommDataService_NewData_Received(0, &uart_rx_data, 1);
}

void Runtime_Adaptor_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
	if(htim == ra.comm_htim){
		stop_comm_timer(ra.comm_htim);

		//External trigger makes runtime to process data
		//This trigging is needed for Modbus (3.5 Char)
		Runtime_CommDataService_Process_DataBuffer(0);
	}
}

//Modbus UART Transmit Functions
void hal_modbus_uart_tx(uint8_t* pData, uint16_t Size){
	HAL_UART_Transmit_IT(ra.comm_huart, pData, Size);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
}

//-----------------------UNIQUE ID-----------------------------------------
void get_uniqueid(uint8_t* id, uint16_t len){
	uint32_t* buf = id;
	buf[0] 	= (uint32_t) READ_REG(*((uint32_t *)UID_BASE));
	buf[1] = (uint32_t) READ_REG(*((uint32_t *)(UID_BASE + 0x04U)));
	buf[2] = (uint32_t) READ_REG(*((uint32_t *)(UID_BASE + 0x14U)));
}

//---------------------Flash functions---------------------------------------
void get_flash_memory_info(uint32_t* start_addr, uint32_t* size){
	*start_addr = ADDR_FLASH_SECTOR_23;
	*size = FLASH_MEMORY_SIZE;
}

uint8_t write_to_flash(uint8_t* p, uint32_t start_addr, uint16_t size)
{
	uint8_t ret = 0;
	uint16_t i;
	uint32_t data;

	HAL_FLASH_Unlock();

	for (i = 0; i < size; i+=4) {
		data = *(uint32_t*)(p+i);
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, start_addr + i, data) == HAL_OK) ret = 1;
		else {
			ret = 0;
			break;
		}
	}

	HAL_FLASH_Lock();
}

uint8_t erase_flash(uint32_t start_addr)
{
	uint8_t ret = 0;

	uint32_t SectorError = 0;
	FLASH_EraseInitTypeDef EraseInitStruct;


	/* Unlock the Flash to enable the flash control register access *************/
	HAL_FLASH_Unlock();

	/* Erase the user Flash area
	(area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS         ;
	EraseInitStruct.NbSectors = 1;
	EraseInitStruct.Sector = FLASH_SECTOR_23;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
	{
		ret = 1;
	}

	else ret = 0;

	return ret;
}

//---------------------Encoder Function---------------------------------------------------
int8_t hal_get_encoder_value(uint8_t ch)
{
	if(ch==0){
		return (htim1.Instance->CNT>>2);
	}
	else{
		return 0;
	}
}


//---------------------Display FUnctions---------------------------------------------------

//Display Functions
void  Display_String(int32_t startX, int32_t startY,
		int32_t width, int32_t height,
		int32_t attr, char* str, uint16_t len){
	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_DARKGRAY); //can gather from attr
	BSP_LCD_DisplayStringAt(startX, startY, str, LEFT_MODE);
}

void  Display_Number(int32_t startX, int32_t startY,
		int32_t width, int32_t height,
		int32_t attr, int32_t val){


	char str[16];
	itoa(val, str, 10);

	BSP_LCD_SetFont(&Font16);

	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_FillRect(startX, startY, width, height);

	BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	BSP_LCD_DisplayStringAt(startX, startY, str, LEFT_MODE);
}

void Display_Image(int32_t startX, int32_t startY,
		int32_t width, int32_t height,
		int32_t attr, int32_t val){
	//define the image table enties
	uint16_t* image_table[] = {
			image_techsafe_logo,
			image_ok,
			image_warning,
			image_error
	};

	if(val >= sizeof(image_table)) return;

	//First clear the screen part
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_FillRect(startX, startY, width, height);

	//Then draw the bmp to the given coordinates
	GUI_Disbitmap(startX, startY, width, height, image_table[val]);
}

//-------------------------------Analog Bar-------------------
uint8_t analogbar_draw_first_call = 1;

void Display_Clear_Area(uint16_t x, uint16_t y, uint16_t w, uint16_t h){
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(x, y, w, h);
}

void Display_AnalogBar_PercText(uint16_t x, uint16_t y, uint8_t val_perc){
	BSP_LCD_SetFont(&Font16); //width:11, height:16

	//Display_Clear_Area(x, y, 44, 16); //%100
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_FillRect(x, y, 44, 16);

	Display_String(x, y, 11, 16, 0, "%", 1);
	Display_Number(x+13, y, 33, 16, 0, val_perc);
}

void Display_AnalogBar_BoundingRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h){
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK); //can gather from attr
	BSP_LCD_DrawRect(x, y, w, h);
}

void Display_AnalogBar(int32_t startX, int32_t startY,
		int32_t width, int32_t height,
		int32_t attr, int32_t val){
	static uint8_t old_val_perc=0;
	static uint8_t old_val_perc_div10 = 0;

	//val_max, val_min should come from the upper layers, 0->val_min, 4096->val_max
	int16_t val_max = 100, val_min=0;

	//boundary control
	if(val > val_max) val = val_max;
	if(val < val_min) val = val_min;
	if(width < 25) width = 25;
	if(height < 25) height = 25;

	//uint16_t text_y = (height - 16) / 2 + startY;

	//calculate val to cell count
	uint8_t val_perc = (uint8_t)((float)((float)val/(float)(val_max-val_min) * 100.0)); //0-100

	if(analogbar_draw_first_call == 1){
		analogbar_draw_first_call = 0;
		Display_Clear_Area(startX, startY, width, height);
		Display_AnalogBar_BoundingRect(startX, startY, width, height);
		//Display_AnalogBar_PercText(startX+width+5, text_y, 0);
	}

	else{

		//if change in percentage redraw text area
		if(val_perc != old_val_perc){
			old_val_perc = val_perc;
			//Display_AnalogBar_PercText(startX+width+5, text_y, val_perc);
		}


		//if change in the graph then draw again
		if(val_perc / 10 != old_val_perc_div10){
			old_val_perc_div10 = val_perc/10;

			Display_Clear_Area(startX, startY, width, height);
			Display_AnalogBar_BoundingRect(startX, startY, width, height);

			width = width - 10; //5 left-right margin
			height = height -10; //5 top bottom margin
			startX += 5;
			startY += 5;

			uint16_t cell_width = width / 10;
			uint16_t draw_cell_width = cell_width - cell_width / 4;

			uint8_t bar_count = 0;
			for(uint8_t i = 0;i<val_perc;i+=10){
				if(bar_count < 3){
					BSP_LCD_SetTextColor(LCD_COLOR_RED); //can gather from attr
				}

				else if(bar_count < 6){
					BSP_LCD_SetTextColor(LCD_COLOR_YELLOW); //can gather from attr
				}

				else{
					BSP_LCD_SetTextColor(LCD_COLOR_GREEN); //can gather from attr
				}

				BSP_LCD_FillRect(startX+cell_width*bar_count++, startY, draw_cell_width, height);
			}
		}
	}
}

#define LCD_FRAME_BUFFER_LAYER0                  (LCD_FRAME_BUFFER+0x130000)
#define LCD_FRAME_BUFFER_LAYER1                  LCD_FRAME_BUFFER
#define CONVERTED_FRAME_BUFFER                   (LCD_FRAME_BUFFER+0x260000))

void init_lcd_display(){
  BSP_LCD_Init();

   /* Layer2 Init */
   BSP_LCD_LayerDefaultInit(1, LCD_FRAME_BUFFER_LAYER1);
   /* Set Foreground Layer */
   BSP_LCD_SelectLayer(1);
   /* Clear the LCD */
   BSP_LCD_Clear(LCD_COLOR_WHITE);
   BSP_LCD_SetColorKeying(1, LCD_COLOR_WHITE);
   BSP_LCD_SetLayerVisible(1, DISABLE);

   /* Layer1 Init */
   BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER_LAYER0);

   /* Set Foreground Layer */
   BSP_LCD_SelectLayer(0);

   /* Enable The LCD */
   BSP_LCD_DisplayOn();

   /* Clear the LCD */
   BSP_LCD_Clear(LCD_COLOR_WHITE);


   //BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
   //BSP_LCD_DisplayStringAt(5, 5, "hello", LEFT_MODE);
}

void Display_Clear()
{
	analogbar_draw_first_call = 1;
	BSP_LCD_Clear(LCD_COLOR_WHITE);
}

//Watch function

//interrupt management
void hal_disable_interrupts(){
	__disable_irq();
}

void hal_enable_interrupts(){
	__enable_irq();
}

void initiate_runtime()
{
	  init_comm_data_service();
	  init_comm_timing_service();
	  initiate_input_channels();
	  initiate_output_channels();
	  initate_analog_channels();
	  init_lcd_display();
	  //init_fatfs();
}

