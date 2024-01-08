################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Display/display.c \
../Display/display_offsets.c \
../Display/fonts.c \
../Display/ili9341.c \
../Display/st7789.c 

S_UPPER_SRCS += \
../Display/display_asm.S 

OBJS += \
./Display/display.o \
./Display/display_asm.o \
./Display/display_offsets.o \
./Display/fonts.o \
./Display/ili9341.o \
./Display/st7789.o 

S_UPPER_DEPS += \
./Display/display_asm.d 

C_DEPS += \
./Display/display.d \
./Display/display_offsets.d \
./Display/fonts.d \
./Display/ili9341.d \
./Display/st7789.d 


# Each subdirectory must supply rules for building sources it contributes
Display/%.o Display/%.su Display/%.cyclo: ../Display/%.c Display/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/Vadim/STM32Cube/Repository/stm32f401ccu6_usb_virtual_com_port/Display" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f401ccu6_usb_virtual_com_port/JPEG" -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Display/%.o: ../Display/%.S Display/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -c -I"C:/Users/Vadim/STM32Cube/Repository/stm32f401ccu6_usb_virtual_com_port/Display" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f401ccu6_usb_virtual_com_port/JPEG" -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-Display

clean-Display:
	-$(RM) ./Display/display.cyclo ./Display/display.d ./Display/display.o ./Display/display.su ./Display/display_asm.d ./Display/display_asm.o ./Display/display_offsets.cyclo ./Display/display_offsets.d ./Display/display_offsets.o ./Display/display_offsets.su ./Display/fonts.cyclo ./Display/fonts.d ./Display/fonts.o ./Display/fonts.su ./Display/ili9341.cyclo ./Display/ili9341.d ./Display/ili9341.o ./Display/ili9341.su ./Display/st7789.cyclo ./Display/st7789.d ./Display/st7789.o ./Display/st7789.su

.PHONY: clean-Display

