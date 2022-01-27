#ifndef __MCRuntime_H__
#define __MCRuntime_H__

//Core Functions
void mcCore_SM();
void Runtime_CommDataService_NewData_Received(uint32_t channel, uint8_t* buf, uint32_t len);

uint8_t register_user_callback_func(uint8_t slot, void (*p) (void*));

#define MODBUS_UART_CALLBACK_FUNCTION_SLOT 0


#endif
