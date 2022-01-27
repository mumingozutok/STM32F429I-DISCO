################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Multimedia/bmp_parser.c 

OBJS += \
./Multimedia/bmp_parser.o 

C_DEPS += \
./Multimedia/bmp_parser.d 


# Each subdirectory must supply rules for building sources it contributes
Multimedia/%.o: ../Multimedia/%.c Multimedia/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../Core/Inc -I../Adaptors -I../Runtime_Lib -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/work/techsafe/MCRuntime/stm32-works/code/boards/STM32F429I-DISCO/Drivers/BSP/STM32F429I-Discovery" -I"D:/work/techsafe/MCRuntime/stm32-works/code/boards/STM32F429I-DISCO/Drivers/BSP/Components" -I"D:/work/techsafe/MCRuntime/stm32-works/code/boards/STM32F429I-DISCO/Utilities/Fonts" -I"D:/work/techsafe/MCRuntime/stm32-works/code/boards/STM32F429I-DISCO/Multimedia" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Multimedia

clean-Multimedia:
	-$(RM) ./Multimedia/bmp_parser.d ./Multimedia/bmp_parser.o

.PHONY: clean-Multimedia

