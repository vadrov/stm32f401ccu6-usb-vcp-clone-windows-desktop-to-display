/* USER CODE BEGIN Header */
/*
 *  Copyright (C)2023, VadRov, all right reserved.
 *
 *  Проект демонстрирует работу с виртуальным com портом.
 *  Воспроизведение стримингового видео (motion jpeg)
 *  Версия для stm32f401x
 *
 *	Клиентская часть
 *  Данная программа для м/к в реальном времени непрерывно получает закодированные jpeg
 *  изображения по USB (виртуальному COM-порту клиенту) от компьютера (сервера),
 *  на стороне которого они и формируются (см. описание и код серверной программы).
 *  Полученные MCU-клиентом данные изображений декодируются из jpeg и выводятся на дисплей.
 *  Таким образом, изображение на мониторе компьютера дублируется на дисплее,
 *  подключенном к микроконтроллеру, выполняющему функции клиента, подключенного к серверу
 *  (компьютеру) посредством USB.
 *
 *  Проект "выходного дня", но может быть полезен в качестве наглядного примера организации
 *  работы с USB CDC с ипользованием двойного буфера (см. функцию CDC_Receive_FS в
 *  файле usbd_cdc_if.c).
 *
 *  Максимальная скорость "без нагрузки", которую удавалось достичь при передаче данных от компьютера
 *  к MCU с использованием библиотек от STM составила около 4,5 Мбит/с при теоретическом
 *  пределе в 12 Мбит/с.
 *
 *  Допускается свободное распространение.
 *  При любом способе распространения указание автора ОБЯЗАТЕЛЬНО.
 *  В случае внесения изменений и распространения модификаций указание первоначального автора ОБЯЗАТЕЛЬНО.
 *  Распространяется по типу "как есть", то есть использование осуществляется на свой страх и риск.
 *  Автор не предоставляет никаких гарантий.
 *
 *  https://www.youtube.com/@VadRov
 *  https://dzen.ru/vadrov
 *  https://vk.com/vadrov
 *  https://t.me/vadrov_channel
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "display.h"
#include "st7789.h"
#include "jpeg_chan.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DISPLAY_DEBUG_DATA	0 /* Вывод некоторой отладочной информации на дисплей
								 (количество кадров в секунду, количество принимаемых байт в секунду)
							  	 1 - да, иное - нет
							   */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
//Объявляем и инициализируем переменные:
//Буфер приема и его размер объявляются, соответственно, в файлах usbd_cdc_if.c и usdb_cdc_if.h
extern uint8_t buffer_rx[]; 		//Двойной буфер для приема сообщений от сервера - объявляется в файле usbd_cdc_if.c
									//Также не забываем прописать buffer_rx в функции CDC_Init_FS в файле usbd_cdc_if.c
volatile uint16_t buf_offs = 0;		//Переменная определяет активную половину буфера
volatile uint16_t len_buf = 0;		//Переменная определяем размер принятого сообщения от сервера
volatile uint8_t recieve_fl = 0; 	//Переменная определяет, что сообщение от сервера принято/не принято
//Не забудем "прописать" эти переменные в usbd_cdc_if.c
//volatile, потому что переменная изменяется, в т.ч., в прерывании.
volatile uint32_t millis = 0;		//Счетчик "системного времени", мс (инкрементируется каждую мс)
//Не забудем "прописать" millis в stm32f4xx_it.c
//Также в функции SysTick_Handler() указанного файла добавить: millis++;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  //---------------------- Разгон для STM32F401xB/C, STM32F401xD/E до 108 MHz- Как пример использования ----------------------------
#if defined(STM32F401xB) || defined(STM32F401xC) || defined(STM32F401xD) || defined(STM32F401xE)
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_3); 								//Переходя на повышенную частоту,
	while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_3) ;						//предварительно увеличиваем задержку чтения флеш-памяти.
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);						//Выбираем HSE, как источник тактирования.
	while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) ;	//Ожидаем переход на новый источник тактирования.
	LL_RCC_PLL_Disable();													//Выключаем модуль PLL.
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 432, LL_RCC_PLLP_DIV_4); //Меняем коэффициенты модуля PLL.
	LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 432, LL_RCC_PLLQ_DIV_9);
	LL_RCC_PLL_Enable();										//Включаем модуль PLL.
	while(LL_RCC_PLL_IsReady() != 1) ;							//Ожидаем готовности модуля PLL.
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);			//Выбираем PLL, как источник тактирования.
	while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) ; 	//Ожидаем переход на новый источник тактирования.
	SystemCoreClock = 108000000;											//Переопределяем частоту ядра на повышенную.
	SysTick_Config(SystemCoreClock/1000);									//В связи с изменением частоты ядра, перенастраиваем
																			//системный таймер SysTick для корректной работы
																			//счетчика "системного времени".
#endif
	//-----------------------------------------------------------------------------------------------------------------------------
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USB_DEVICE_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	//------------------- Настройка параметров и инициализация дисплея ---------------------
	LCD_BackLight_data bl_dat = { .htim_bk = TIM3,
		  	  	  	  	  	  	  .channel_htim_bk = LL_TIM_CHANNEL_CH1,
								  .blk_port = 0,
								  .blk_pin = 0,
								  .bk_percent = 75 };

	LCD_DMA_TypeDef hdma_spi1_tx = { .dma = DMA2,
		  	  	  	  	  	  	     .stream = LL_DMA_STREAM_3 };

	LCD_SPI_Connected_data spi_dat = { .spi = SPI1,
		  							   .dma_tx = hdma_spi1_tx,
 									   .reset_port = LCD_RES_GPIO_Port,
 									   .reset_pin = LCD_RES_Pin,
 									   .dc_port = LCD_DC_GPIO_Port,
 									   .dc_pin = LCD_DC_Pin,
 									   .cs_port = LCD_CS_GPIO_Port,
 									   .cs_pin = LCD_CS_Pin  };
#ifndef LCD_DYNAMIC_MEM
	LCD_Handler lcd1;
#endif
	//Для дисплея на контроллере ST7789
	LCD = LCD_DisplayAdd( LCD,
#ifndef LCD_DYNAMIC_MEM
	  	  	  	  	  	  &lcd1,
#endif
						  240,
						  240,
						  ST7789_CONTROLLER_WIDTH,
						  ST7789_CONTROLLER_HEIGHT,
						  //Задаем смещение по ширине и высоте для нестандартных или бракованных дисплеев:
						  0,		//смещение по ширине дисплейной матрицы
						  0,		//смещение по высоте дисплейной матрицы
						  PAGE_ORIENTATION_PORTRAIT,
						  ST7789_Init,
						  ST7789_SetWindow,
						  ST7789_SleepIn,
						  ST7789_SleepOut,
						  &spi_dat,
						  LCD_DATA_16BIT_BUS,
						  bl_dat );

	LCD_Handler *lcd = LCD; //указатель на первый дисплей в списке
	LCD_Init(lcd);
	LCD_Fill(lcd, 0x319bb1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  iPicture_jpg file;

#if (DISPLAY_DEBUG_DATA == 1)
  uint32_t old_time = millis, bytes = 0;
  char fps_txt[10] = "???";
  char bytes_txt[10] = "???";
  uint32_t frames_s = 0;
#endif

  char message[100];
  uint8_t res = USBD_FAIL;

  //Формируем сообщение серверу, содержащее графическое разрешение дисплея
  //Можно и вот так, через sprintf, но на выходе код больше, что не есть хорошо
  /* sprintf(message, "MCU:%dX%d.F:%d.", lcd->Width, lcd->Height, BUFFER_SIZE/2); */
  strcpy(message, "MCU:");
  utoa(lcd->Width, &message[strlen(message)], 10);
  strcat(message, "X");
  utoa(lcd->Height, &message[strlen(message)], 10);
  strcat(message, ".F:");
  utoa(BUFFER_RX_SIZE/2, &message[strlen(message)], 10);
  strcat(message, ".");

  //Отправляем это сообщение компьютеру-серверу
  //Здесь цикл бесконечный. Для реальных задач/проектов
  //разумно сделать соответствующий таймаут с обработкой ошибки
  while (res != USBD_OK) {
	  res = CDC_Transmit_FS((uint8_t*)message, strlen(message));
  }

  while (1) {
	  if (recieve_fl) {	//Приняли сообщение от сервера?

		  //Подготавливаем данные для декодера jpeg
		  file.data = &buffer_rx[buf_offs]; //Стартовая позиция файла с картинкой в буфере
		  file.size = len_buf;				//Размер файла

#if (DISPLAY_DEBUG_DATA == 1)
		  bytes += len_buf;
#endif

		  //Инициализируем:
		  buf_offs = BUFFER_RX_SIZE/2 - buf_offs;	//Переменную, определяющую активную половину двойного буфера
		  recieve_fl = 0;							//Переменную, определяющую, что сообщение от сервера принято
		  len_buf = 0;								//Размер принятого от сервера сообщения

		  //Мы готовы принять следующее сообщение (вызываем процедуру подготовки к приему)
		  //Это сообщение уже будет приниматься в новую активную часть буфера,
		  //а в это время одновременно будет декодироваться картинка из jpeg файла, находящаяся
		  //в неактивной части буфера. Этот файл мы получили в предыдущем сообщении от сервера.
		  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

		  //Декодируем файл с картинкой из неактивной части двойного буфера
		  (void)LCD_Load_JPG_chan(lcd, 0, 0, lcd->Width, lcd->Height, &file, PICTURE_IN_MEMORY);

#if (DISPLAY_DEBUG_DATA == 1) //Вывод отладочной информации (если разрешено)
		  LCD_WriteString(lcd, 8, lcd->Height - Font_8x13.height, fps_txt, &Font_8x13, COLOR_BLACK, COLOR_WHITE, LCD_SYMBOL_PRINT_FAST);
		  LCD_WriteString(lcd, 100, lcd->Height - Font_8x13.height, bytes_txt, &Font_8x13, COLOR_BLACK, COLOR_WHITE, LCD_SYMBOL_PRINT_FAST);
		  frames_s++; //Увеличиваем счетчик кадров
		  if (millis - old_time >= 1000) { 	//Прошла секунда (1000 миллисекунд)?
			  utoa(frames_s, fps_txt, 10); 	//Преобразуем в строку счетчик кадров, отображенных за секунду
			  frames_s = 0;				  	//и инициализируем его
			  old_time = millis;			//Инициализируем "старое" системное время
			  utoa(bytes, bytes_txt, 10);	//Преобразуем в строку счетчик количества байт, переданных за секунду
			  bytes = 0;					//и инициализируем его
		  }
#endif
	  }
//	  __NOP();								//Эквивалент ассемблерной мнемоники "nop", простой/задержка системы на 1 такт
	  	  	  	  	  	  	  	  	  	  	//Эту команду ярдро Cortex-M может "сбросить" с конвейера без исполнения
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 336, LL_RCC_PLLP_DIV_4);
  LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 336, LL_RCC_PLLQ_DIV_7);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_SetSystemCoreClock(84000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  LL_SPI_InitTypeDef SPI_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**SPI1 GPIO Configuration
  PA5   ------> SPI1_SCK
  PA7   ------> SPI1_MOSI
  */
  GPIO_InitStruct.Pin = LCD_SCL_Pin|LCD_SDA_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* SPI1 DMA Init */

  /* SPI1_TX Init */
  LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_3, LL_DMA_CHANNEL_3);

  LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_3, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_3, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA2, LL_DMA_STREAM_3, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_3, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_3, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_3, LL_DMA_PDATAALIGN_HALFWORD);

  LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_3, LL_DMA_MDATAALIGN_HALFWORD);

  LL_DMA_DisableFifoMode(DMA2, LL_DMA_STREAM_3);

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_16BIT;
  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH;
  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.CRCPoly = 10;
  LL_SPI_Init(SPI1, &SPI_InitStruct);
  LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  TIM_InitStruct.Prescaler = 999;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 209;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM3, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM3);
  LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH1);
  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.CompareValue = 0;
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
  LL_TIM_OC_DisableFast(TIM3, LL_TIM_CHANNEL_CH1);
  LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM3);
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**TIM3 GPIO Configuration
  PA6   ------> TIM3_CH1
  */
  GPIO_InitStruct.Pin = LCD_BLK_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(LCD_BLK_GPIO_Port, &GPIO_InitStruct);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* Init with LL driver */
  /* DMA controller clock enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  NVIC_SetPriority(DMA2_Stream3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),1, 0));
  NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

  /**/
  LL_GPIO_SetOutputPin(LED_INDIC_GPIO_Port, LED_INDIC_Pin);

  /**/
  LL_GPIO_SetOutputPin(GPIOA, LCD_DC_Pin|LCD_RES_Pin|LCD_CS_Pin);

  /**/
  GPIO_InitStruct.Pin = LED_INDIC_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LED_INDIC_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DC_Pin|LCD_RES_Pin|LCD_CS_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
