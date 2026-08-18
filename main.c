/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_host.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"
#include "stmpe811.h"
#include "drowsiness.h"



/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

SPI_HandleTypeDef hspi5;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;

SDRAM_HandleTypeDef hsdram1;

osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */
typedef enum
{
    SCREEN_MENU = 0,
    SCREEN_DASHBOARD
} ScreenState;

ScreenState screen = SCREEN_MENU;

TS_StateTypeDef TS_State;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI5_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_LTDC_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */
void DrawMenu(void);
void DrawDashboard(char state);
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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C3_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_LTDC_Init();
  /* USER CODE BEGIN 2 */
  BSP_LCD_Init();

  BSP_LCD_LayerDefaultInit(1, LCD_FRAME_BUFFER);
  BSP_LCD_SelectLayer(1);
  BSP_LCD_SetTransparency(1, 255);

  BSP_LCD_Clear(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

  /* TOUCH */
  BSP_TS_Init(240, 320);

  /* DROWSINESS */
  Drowsiness_Init();



  /* FIRST SCREEN */
  DrawMenu();
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 4096);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 120;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  */
static void MX_CRC_Init(void)
{
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DMA2D Initialization Function
  */
static void MX_DMA2D_Init(void)
{
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C3 Initialization Function
  */
static void MX_I2C3_Init(void)
{
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LTDC Initialization Function
  */
static void MX_LTDC_Init(void)
{
  LTDC_LayerCfgTypeDef pLayerCfg = {0};
  LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 7;
  hltdc.Init.VerticalSync = 3;
  hltdc.Init.AccumulatedHBP = 14;
  hltdc.Init.AccumulatedVBP = 5;
  hltdc.Init.AccumulatedActiveW = 654;
  hltdc.Init.AccumulatedActiveH = 485;
  hltdc.Init.TotalWidth = 660;
  hltdc.Init.TotalHeigh = 487;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 0;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 0;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg.Alpha = 0;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0;
  pLayerCfg.ImageWidth = 0;
  pLayerCfg.ImageHeight = 0;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg1.WindowX0 = 0;
  pLayerCfg1.WindowX1 = 0;
  pLayerCfg1.WindowY0 = 0;
  pLayerCfg1.WindowY1 = 0;
  pLayerCfg1.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg1.Alpha = 0;
  pLayerCfg1.Alpha0 = 0;
  pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg1.FBStartAdress = 0;
  pLayerCfg1.ImageWidth = 0;
  pLayerCfg1.ImageHeight = 0;
  pLayerCfg1.Backcolor.Blue = 0;
  pLayerCfg1.Backcolor.Green = 0;
  pLayerCfg1.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI5 Initialization Function
  */
static void MX_SPI5_Init(void)
{
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  */
static void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* FMC initialization function */
static void MX_FMC_Init(void)
{
  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  hsdram1.Instance = FMC_SDRAM_DEVICE;
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 3;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, NCS_MEMS_SPI_Pin|CSX_Pin|OTG_FS_PSO_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(ACP_RST_GPIO_Port, ACP_RST_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, RDX_Pin|WRX_DCX_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOG, LD3_Pin|LD4_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = NCS_MEMS_SPI_Pin|CSX_Pin|OTG_FS_PSO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = B1_Pin|MEMS_INT2_Pin|TP_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = ACP_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACP_RST_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = OTG_FS_OC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OC_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TE_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = RDX_Pin|WRX_DCX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD3_Pin|LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}


/* USER CODE BEGIN 4 */

/* ==========================================================
 *  DrawMenu  -  Professional Splash Screen
 *  LCD: 240 x 320  |  STM32F429I Discovery
 * ========================================================== */
void DrawMenu(void)
{
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    /* Top accent bar */
    BSP_LCD_SetTextColor(0xFF001A33);
    BSP_LCD_FillRect(0, 0, 240, 5);

    /* Bottom accent bar */
    BSP_LCD_FillRect(0, 315, 240, 5);

    /* Outer border */
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_DrawRect(8, 8, 224, 304);
    BSP_LCD_DrawRect(10, 10, 220, 300);

    /* Inner top line */
    BSP_LCD_FillRect(12, 12, 216, 2);

    /* Title */
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_DisplayStringAt(0, 45, (uint8_t *)"SMART DASH", CENTER_MODE);

    /* Subtitle */
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(0, 78, (uint8_t *)"Driver Monitoring System", CENTER_MODE);

    /* Divider */
    BSP_LCD_SetTextColor(0xFF003F7F);
    BSP_LCD_FillRect(30, 98, 180, 1);

    /* Car silhouette - roof */
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_DrawHLine(80, 145, 80);
    /* windshield left */
    BSP_LCD_DrawLine(80, 145, 65, 165);
    /* windshield right */
    BSP_LCD_DrawLine(160, 145, 175, 165);
    /* body bottom */
    BSP_LCD_DrawHLine(55, 165, 130);
    /* left wheel arch */
    BSP_LCD_DrawLine(55, 165, 50, 175);
    BSP_LCD_DrawHLine(50, 175, 28);
    BSP_LCD_DrawLine(78, 175, 83, 165);
    /* right wheel arch */
    BSP_LCD_DrawLine(185, 165, 190, 175);
    BSP_LCD_DrawHLine(132, 175, 28);
    BSP_LCD_DrawLine(157, 175, 162, 165);
    /* wheels */
    BSP_LCD_DrawCircle(64, 178, 7);
    BSP_LCD_DrawCircle(176, 178, 7);

    /* Status dot + text */
    BSP_LCD_SetTextColor(0xFF00FF88);
    BSP_LCD_FillCircle(120, 215, 5);
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(0, 226, (uint8_t *)"System Ready", CENTER_MODE);

    /* START button */
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_FillRect(55, 255, 130, 38);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(0xFF00BFFF);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 266, (uint8_t *)"TAP TO START", CENTER_MODE);

    /* Version */
    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(0xFF444444);
    BSP_LCD_DisplayStringAt(0, 308, (uint8_t *)"v1.0  STM32F429I", CENTER_MODE);
}

/* ==========================================================
 *  DrawDashboard  -  Professional HUD Screen
 *  state: 'W' = Normal  |  'D' = Drowsy  |  'A' = Alert
 * ========================================================== */
void DrawDashboard(char state)
{
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    /* --------------------------------------------------
     * TOP HEADER BAR  (y: 0 - 34)
     * -------------------------------------------------- */
    BSP_LCD_SetTextColor(0xFF001A33);
    BSP_LCD_FillRect(0, 0, 240, 34);

    /* Left cyan accent stripe */
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_FillRect(0, 0, 4, 34);

    /* Title text */
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(0xFF001A33);
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_DisplayStringAt(10, 9, (uint8_t *)"SMART DASH", LEFT_MODE);

    /* LIVE green dot */
    BSP_LCD_SetTextColor(0xFF00FF88);
    BSP_LCD_FillCircle(208, 13, 5);

    /* LIVE label */
    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(0xFF00FF88);
    BSP_LCD_DisplayStringAt(192, 22, (uint8_t *)"LIVE", LEFT_MODE);

    /* Header bottom border line */
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_DrawHLine(0, 34, 240);

    /* --------------------------------------------------
     * STATE PANEL  (y: 42 - 148)
     * -------------------------------------------------- */
    uint32_t stateColor;
    uint8_t *stateLabel;
    uint8_t *stateIcon;
    uint8_t *stateDesc;

    if (state == 'W')
    {
        stateColor = 0xFF00C864;
        stateLabel = (uint8_t *)"NORMAL";
        stateIcon  = (uint8_t *)"[  OK  ]";
        stateDesc  = (uint8_t *)"Driver alert & focused";
    }
    else if (state == 'D')
    {
        stateColor = 0xFFFF8C00;
        stateLabel = (uint8_t *)"Warning";
        stateIcon  = (uint8_t *)"[ WARN ]";
        stateDesc  = (uint8_t *)"Fatigue detected!";
    }
    else
    {
        stateColor = 0xFFFF2020;
        stateLabel = (uint8_t *)"! ALERT !";
        stateIcon  = (uint8_t *)"[DANGER]";
        stateDesc  = (uint8_t *)"Pull over immediately!";
    }

    /* Colored outer panel */
    BSP_LCD_SetTextColor(stateColor);
    BSP_LCD_FillRect(12, 42, 216, 100);

    /* Dark inner inset */
    BSP_LCD_SetTextColor(0xFF0A0A14);
    BSP_LCD_FillRect(15, 45, 210, 94);

    /* Icon */
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(0xFF0A0A14);
    BSP_LCD_SetTextColor(stateColor);
    BSP_LCD_DisplayStringAt(0, 54, stateIcon, CENTER_MODE);

    /* Big state label */
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(0, 76, stateLabel, CENTER_MODE);

    /* Description */
    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(0, 110, stateDesc, CENTER_MODE);

    /* Thin colored bottom edge of panel */
    BSP_LCD_SetTextColor(stateColor);
    BSP_LCD_FillRect(12, 139, 216, 3);

    /* --------------------------------------------------
     * METRICS SECTION  (y: 152 - 224)
     * -------------------------------------------------- */
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(0xFF555555);
    BSP_LCD_DisplayStringAt(12, 150, (uint8_t *)"DRIVER METRICS", LEFT_MODE);
    BSP_LCD_DrawHLine(12, 159, 216);

    /* --- LEFT BOX: Eye Closure --- */
    BSP_LCD_SetTextColor(0xFF111122);
    BSP_LCD_FillRect(12, 163, 102, 55);
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_DrawRect(12, 163, 102, 55);

    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetBackColor(0xFF111122);
    BSP_LCD_SetTextColor(0xFF888888);
    BSP_LCD_DisplayStringAt(16, 167, (uint8_t *)"EYE CLOSURE", LEFT_MODE);

    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    if (state == 'W')
        BSP_LCD_DisplayStringAt(16, 181, (uint8_t *)"Low", LEFT_MODE);
    else if (state == 'D')
        BSP_LCD_DisplayStringAt(16, 181, (uint8_t *)"Med", LEFT_MODE);
    else
        BSP_LCD_DisplayStringAt(16, 181, (uint8_t *)"High", LEFT_MODE);

    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(0xFF555555);
    BSP_LCD_DisplayStringAt(16, 207, (uint8_t *)"Blink rate", LEFT_MODE);

    /* --- RIGHT BOX: Fatigue Level --- */
    BSP_LCD_SetTextColor(0xFF111122);
    BSP_LCD_FillRect(126, 163, 102, 55);
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_DrawRect(126, 163, 102, 55);

    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetBackColor(0xFF111122);
    BSP_LCD_SetTextColor(0xFF888888);
    BSP_LCD_DisplayStringAt(130, 167, (uint8_t *)"FATIGUE LVL", LEFT_MODE);

    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(stateColor);
    if (state == 'W')
        BSP_LCD_DisplayStringAt(130, 181, (uint8_t *)"1 / 5", LEFT_MODE);
    else if (state == 'D')
        BSP_LCD_DisplayStringAt(130, 181, (uint8_t *)"3 / 5", LEFT_MODE);
    else
        BSP_LCD_DisplayStringAt(130, 181, (uint8_t *)"5 / 5", LEFT_MODE);

    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(0xFF555555);
    BSP_LCD_DisplayStringAt(130, 207, (uint8_t *)"Risk score", LEFT_MODE);

    /* --------------------------------------------------
     * ALERT BANNER  (y: 228 - 258)  - Drowsy / Alert only
     * -------------------------------------------------- */
    if (state == 'D' || state == 'A')
    {
        BSP_LCD_SetTextColor(stateColor);
        BSP_LCD_FillRect(12, 228, 216, 26);
        BSP_LCD_SetFont(&Font12);
        BSP_LCD_SetBackColor(stateColor);
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        if (state == 'A')
            BSP_LCD_DisplayStringAt(0, 234, (uint8_t *)"!! PULL OVER NOW !!", CENTER_MODE);
        else
            BSP_LCD_DisplayStringAt(0, 234, (uint8_t *)"Take a break soon", CENTER_MODE);
    }

    /* --------------------------------------------------
     * BOTTOM BAR  (y: 270 - 320)
     * -------------------------------------------------- */
    BSP_LCD_SetTextColor(0xFF001A33);
    BSP_LCD_FillRect(0, 270, 240, 50);

    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_DrawHLine(0, 270, 240);

    /* BACK button */
    BSP_LCD_SetTextColor(0xFF00BFFF);
    BSP_LCD_FillRect(10, 279, 78, 28);
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetBackColor(0xFF00BFFF);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(15, 287, (uint8_t *)"< BACK", LEFT_MODE);

    /* Right side: session label */
    BSP_LCD_SetBackColor(0xFF001A33);
    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(0xFF555555);
    BSP_LCD_DisplayStringAt(108, 280, (uint8_t *)"SESSION", LEFT_MODE);
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(108, 292, (uint8_t *)"00:00:00", LEFT_MODE);
}

/* USER CODE END 4 */

/**
  * @brief  Function implementing the defaultTask thread.
  */
void StartDefaultTask(void const * argument)
{
  MX_USB_HOST_Init();
  ScreenState prev_screen = -1;
  char prev_state = 0;
  uint32_t send_timer = 0;

  for(;;)
  {
      BSP_TS_GetState(&TS_State);

      if(TS_State.TouchDetected)
      {
          if(screen == SCREEN_MENU)
              screen = SCREEN_DASHBOARD;
          else
              screen = SCREEN_MENU;
          osDelay(300);
      }

      Drowsiness_Task();
      char state = GetDriverState();

      /* ===== SIRF 1 SECOND MEIN BHEJO ===== */
      send_timer++;
      if(send_timer >= 50)  // 50 * 20ms = 1000ms (1 second)
      {
          Send_State_To_ESP32(state);
          send_timer = 0;
      }

      if(screen != prev_screen)
      {
          if(screen == SCREEN_MENU)
              DrawMenu();
          else
              DrawDashboard(state);

          prev_screen = screen;
          prev_state = state;
      }
      else if(screen == SCREEN_DASHBOARD && state != prev_state)
      {
          DrawDashboard(state);
          prev_state = state;
      }

      osDelay(20);
  }
}
/**
  * @brief  Period elapsed callback in non blocking mode
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
