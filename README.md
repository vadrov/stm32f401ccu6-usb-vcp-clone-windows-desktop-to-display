Copyright (C)2023, VadRov, all right reserved / www.youtube.com/@VadRov / www.dzen.ru/vadrov
# Cloning a Windows desktop image on a display connected to a microcontroller. Virtual com port. JPEG encoding and decoding.
The project demonstrates working with a virtual com port. Play streaming video (motion jpeg). The server (computer) constantly takes screenshots of the desktop, encodes them in jpeg and transfers them to the client (microcontroller) via USB (virtual COM port). The microcontroller decodes the image and shows it on the display.\
MCU - blackpill stm32f401ccu6\
Display - st7789 (and compatible). Supported by ili9341 - initialization string needs to be changed (set prefix ILI9341 in display add function)\
Connection:
```
LCD_DC  ---> PA2
LCD_RES ---> PA3
LCD_CS  ---> PA4
LCD_SCL ---> PA5
LCD_BLK ---> PA6
LCD_SDA ---> PA7
```
1. Connect the microcontroller board to the USB connector of the computer.
2. Launch the terminal program from the folder of the same name.
3. Select the com port to which the microcontroller is connected.

If everything went well, then a copy of the Windows screen should be displayed on the display connected to the microcontroller:
![1704679158070](https://github.com/vadrov/stm32f401ccu6-usb-vcp-clone-windows-desktop-to-display/assets/111627147/105a61d5-7477-465b-ad95-42c0ddf2ef16)

Author: **VadRov**\
Contacts: [Youtube](https://www.youtube.com/@VadRov) [Дзен](https://dzen.ru/vadrov) [VK](https://vk.com/vadrov) [Telegram](https://t.me/vadrov_channel)\
Donate: [donate.yoomoney](https://yoomoney.ru/to/4100117522443917)
