# Smart card emulator (STM32F105C8T6)

The goal of this project was smart card emulater for POS/ATM testing and replace a FIME SmartSpy device.
This souce code added "as is" (without building scripts).

## Project is consist of
* SCE-STM32F103C8T6 - source code for STM32F103C8T6 (Blue Pill board). 
* SCE-PCSV-mmpcsc-java-dll - Windows PC/SC java adapter. It is my old software from dino era (use javax.smartcardio instead)
* SCE-host-java - adapter between STM32F103C8T6 and PC/SC reader.

## Required hardware
* PC/SC smart card reader. Any reader.
* STM32F103C8T6 board. "Blue Pill" board or something like it.
* USB-TTL converter (RS232). 
* Piece of foil fiberglass (85,6 х 53,98 х 0,76 mm) for create a smart card contact pad.

## SCE-STM32F103C8T6 
STM32F103C8T6 pins:
* Smart card emulator pins: [PA2(ADC)- VCC, PB6(TIM4_CH1) - CLK, PB8 - RST, PB7 - I/O]
* RS232: USART1 [PA9 - Tx, PA10 - Rx] 115200,8,1,N

![[photo]](./schema.png)
