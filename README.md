# stm32f401ccu6_usb_vcp_clone_windows_desktop_to_display
 
The project demonstrates working with a virtual com port. Playing streaming video (motion jpeg). The server (computer) continuously takes screenshots of the desktop, encodes them in jpeg and transmits them to the client (microcontroller). The microcontroller decodes the image and displays it on the display.

MCU - blackpill stm401ccu6

Display - st7789 (and compatible). Supported by ili9341 - initialization string needs to be changed (set prefix ILI9341 in display add function).

Connection:
```
LCD_DC  ---> PA2
LCD_RES ---> PA3
LCD_CS  ---> PA4
LCD_SCL ---> PA5
LCD_BLK ---> PA6
LCD_SDA ---> PA7
```
