################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../JPEG/jpeg_chan.c \
../JPEG/tjpgd.c 

OBJS += \
./JPEG/jpeg_chan.o \
./JPEG/tjpgd.o 

C_DEPS += \
./JPEG/jpeg_chan.d \
./JPEG/tjpgd.d 


# Each subdirectory must supply rules for building sources it contributes
JPEG/%.o JPEG/%.su JPEG/%.cyclo: ../JPEG/%.c JPEG/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/Vadim/STM32Cube/Repository/stm32f401ccu6_usb_virtual_com_port/Display" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f401ccu6_usb_virtual_com_port/JPEG" -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-JPEG

clean-JPEG:
	-$(RM) ./JPEG/jpeg_chan.cyclo ./JPEG/jpeg_chan.d ./JPEG/jpeg_chan.o ./JPEG/jpeg_chan.su ./JPEG/tjpgd.cyclo ./JPEG/tjpgd.d ./JPEG/tjpgd.o ./JPEG/tjpgd.su

.PHONY: clean-JPEG

