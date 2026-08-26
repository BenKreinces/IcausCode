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
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "daq_packet.h"
#include "tim1_freq_capture.h"
#include "ad7689.h"
#include "ad7124_ll.h"
#include "mcp23s17.h"
#include "mcp23s17_stm32_hal.h"
#include "daq_udp.h"
#include "vesc_uart.h"
#ifdef USE_LWIP
#include "lwip.h"
#endif
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

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi4;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
DMA_HandleTypeDef hdma_tim1_ch1;

#define AD7124_CH0_AIN2_AIN3_ENABLED      0x8043U
#define AD7124_CONFIG0_REFIN1             0x0860U
#define AD7124_ADC_CTRL_CONT_READ_BIT     (1U << 11)
#define AD7124_ADC_CTRL_DATA_STATUS_BIT   (1U << 10)
#define AD7124_ADC_CTRL_MODE_CONTINUOUS   (0x0U << 2)
#define AD7124_CMD_READ_DATA              0x42U

static ad7124_ll_ctx_t ad7124_ctx;
volatile uint8_t g_ad7124_drdy_flag;
volatile int32_t ad7124_raw24;

static AD7689_Handle_t hadc_pt;
static mcp23s17_t sol_dev;
static mcp23s17_stm32_user_t sol_user;

static DataPacket_t dataBuffer[DAQ_BUFFER_SIZE];
static volatile int bufferIndex;

static uint32_t t_connected;
#define TIMEOUT_MS 2000000U

static bool dualSolenoidState[2] = { true, true };
static int current_throttle;

static vesc_values_t g_vesc_values;

static void AD7124_SetupFilter0_Sinc4_Fast(void);
static void AD7124_Start_Continuous_Read(void);
static void AD7124_ReadSample_IfReady(void);
static void handleInstruction(uint8_t instruction, const uint8_t *payload, uint16_t len);
static void handleSolenoid(uint8_t code);
static void MX_DMA_Init(void);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI4_Init(void);
static void MX_USART1_UART_Init(void);
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable CPU cache only when using LwIP/ETH */
#ifdef USE_LWIP
  SCB_EnableICache();
  SCB_EnableDCache();
#endif

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI4_Init();
  MX_USART1_UART_Init();
#ifdef USE_LWIP
  MX_LWIP_Init();
#endif
  /* USER CODE BEGIN 2 */

  t_connected = 0;
  bufferIndex = 0;
  current_throttle = 0;

  AD7689_DWT_Init();
  hadc_pt.hspi = &hspi1;
  hadc_pt.CNV_GPIO = SPI1_CS_PT1_8_GPIO_Port;
  hadc_pt.CNV_Pin = SPI1_CS_PT1_8_Pin;
  hadc_pt.vref_volts = 4.096f;
  hadc_pt.tconv_ns = 5000;
  hadc_pt.next_channel = 0;
  (void)AD7689_Init(&hadc_pt);

  start_tim1();
  start_ch1();

  ad7124_ctx.hspi = &hspi2;
  ad7124_ctx.cs_port = SPI2_CS_LC_GPIO_Port;
  ad7124_ctx.cs_pin = SPI2_CS_LC_Pin;
  (void)ad7124_reset(&ad7124_ctx);
  {
    uint8_t v[2] = { (uint8_t)(AD7124_CONFIG0_REFIN1 >> 8), (uint8_t)(AD7124_CONFIG0_REFIN1 & 0xFF) };
    (void)ad7124_write_reg(&ad7124_ctx, AD7124_REG_CONFIG_0, v, 2);
  }
  AD7124_SetupFilter0_Sinc4_Fast();
  {
    uint8_t ch0_val[2] = { (uint8_t)(AD7124_CH0_AIN2_AIN3_ENABLED >> 8), (uint8_t)(AD7124_CH0_AIN2_AIN3_ENABLED & 0xFF) };
    (void)ad7124_write_reg(&ad7124_ctx, AD7124_REG_CHANNEL_0, ch0_val, 2);
  }
  AD7124_Start_Continuous_Read();
  g_ad7124_drdy_flag = 0;
  ad7124_raw24 = 0;

  {
    GPIO_InitTypeDef g = {0};
    g.Pin = SPI2_DRDY_Pin;
    g.Mode = GPIO_MODE_IT_FALLING;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SPI2_DRDY_GPIO_Port, &g);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  }

  {
    mcp23s17_hal_t hal;
    mcp23s17_stm32_hal_init(&hspi4, SPI4_CS_SOL1_12_GPIO_Port, SPI4_CS_SOL1_12_Pin, 10, &sol_user, &hal);
    (void)mcp23s17_init(&sol_dev, &hal);
  }

  DAQ_UDP_SetRecvCallback(handleInstruction);
  DAQ_UDP_Init();

  g_vesc_values.tempMosfet      = 0.0f;
  g_vesc_values.avgMotorCurrent = 0.0f;
  g_vesc_values.avgInputCurrent = 0.0f;
  g_vesc_values.rpm             = 0.0f;
  g_vesc_values.inpVoltage      = 0.0f;
  g_vesc_values.ampHours        = 0.0f;
  g_vesc_values.tachometerAbs   = 0.0f;
  VESC_UART_Init(&huart1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_tick_ms = HAL_GetTick();
  uint32_t last_esc_poll_ms = last_tick_ms;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    DAQ_UDP_Process();

    uint32_t now_ms = HAL_GetTick();
    if ((now_ms - last_tick_ms) >= 1U) {
      last_tick_ms = now_ms;
      if (t_connected < TIMEOUT_MS) {
        t_connected++;
      }

      uint16_t pt0_code = 0U;
      uint16_t pt3_code = 0U;
      (void)AD7689_ReadChannelRaw(&hadc_pt, 0U, &pt0_code);
      (void)AD7689_ReadChannelRaw(&hadc_pt, 3U, &pt3_code);
      hadc_pt.raw[0] = pt0_code;
      hadc_pt.raw[3] = pt3_code;

      AD7124_ReadSample_IfReady();

      float flow_hz = get_freq_hz(TIM1_FREQ_CAPTURE_CH_1);
      /*
      if ((now_ms - last_esc_poll_ms) >= 100U) {
        last_esc_poll_ms = now_ms;
        VESC_UART_Poll(now_ms, current_throttle, &g_vesc_values);
      }
      */

      DataPacket_t *packet = &dataBuffer[bufferIndex];
      packet->timestamp = now_ms;

      packet->pt[0] = (double)pt0_code;
      packet->pt[1] = (double)pt3_code;
      packet->pt[2] = 0.0;
      packet->pt[3] = 0.0;
      packet->pt[4] = 0.0;

      packet->flowmeter[0] = (double)flow_hz;
      packet->flowmeter[1] = 0.0;

      packet->loadcell[0] = (double)ad7124_raw24;
      packet->loadcell[1] = 0.0;

      packet->tempMosfet      = g_vesc_values.tempMosfet;
      packet->avgMotorCurrent = g_vesc_values.avgMotorCurrent;
      packet->avgInputCurrent = g_vesc_values.avgInputCurrent;
      packet->rpm             = g_vesc_values.rpm;
      packet->inpVoltage      = g_vesc_values.inpVoltage;
      packet->ampHours        = g_vesc_values.ampHours;
      packet->tachometerAbs   = g_vesc_values.tachometerAbs;

      packet->solenoidState = 0;
      for (int i = 0; i < 6; i++) {
        bool level = false;
        if (mcp23s17_read_solenoid(&sol_dev, (uint8_t)(i + 1), &level) && level) {
          packet->solenoidState |= (uint8_t)(1u << i);
        }
      }
      packet->dualSolenoidState = 0;
      if (!dualSolenoidState[0]) packet->dualSolenoidState |= 1u;
      if (!dualSolenoidState[1]) packet->dualSolenoidState |= 2u;

      if (++bufferIndex >= DAQ_BUFFER_SIZE) {
        if (t_connected < TIMEOUT_MS) {
          DAQ_UDP_SendPacket((const uint8_t *)dataBuffer, sizeof(dataBuffer));
        }
        bufferIndex = 0;
      }
    }
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI4|RCC_PERIPHCLK_SPI5;
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 16;
  PeriphClkInitStruct.PLL2.PLL2P = 4;
  PeriphClkInitStruct.PLL2.PLL2Q = 4;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
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

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration: AD7689, 16-bit, mode 0. */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration: AD7124, 8-bit, mode 3. */
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x0;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration: MCP23S17, 8-bit, mode 0. */
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 0x0;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 399;
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
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 2;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, SPI4_CS_SOL1_12_Pin|SPI4_CS_SOL13_24_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI5_CS_TC_GPIO_Port, SPI5_CS_TC_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_PT1_8_GPIO_Port, SPI1_CS_PT1_8_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ONBOARD_LED_GPIO_Port, ONBOARD_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, SPI1_CS_PT9_16_Pin|SPI1_CS_PT17_24_Pin|SPI2_CS_LC_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : SPI4_CS_SOL1_12_Pin SPI4_CS_SOL13_24_Pin */
  GPIO_InitStruct.Pin = SPI4_CS_SOL1_12_Pin|SPI4_CS_SOL13_24_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI5_CS_TC_Pin */
  GPIO_InitStruct.Pin = SPI5_CS_TC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI5_CS_TC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_PT1_8_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_PT1_8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_PT1_8_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_DRDY_Pin */
  GPIO_InitStruct.Pin = SPI2_DRDY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SPI2_DRDY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ONBOARD_LED_Pin */
  GPIO_InitStruct.Pin = ONBOARD_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ONBOARD_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_CS_PT9_16_Pin SPI1_CS_PT17_24_Pin SPI2_CS_LC_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_PT9_16_Pin|SPI1_CS_PT17_24_Pin|SPI2_CS_LC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static void MX_DMA_Init(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();
}

static void AD7124_SetupFilter0_Sinc4_Fast(void)
{
  const uint8_t filter_code = 4U;
  const uint16_t fs = 2U;
  uint32_t reg = ((uint32_t)(filter_code & 0x7U) << 21) | ((uint32_t)(fs & 0x7FFU) << 0);
  uint8_t buf[3] = { (uint8_t)(reg >> 16), (uint8_t)(reg >> 8), (uint8_t)(reg >> 0) };
  (void)ad7124_write_reg(&ad7124_ctx, AD7124_REG_FILTER_0, buf, 3);
}

static void AD7124_Start_Continuous_Read(void)
{
  uint16_t ctrl = AD7124_ADC_CTRL_CONT_READ_BIT |
                  AD7124_ADC_CTRL_DATA_STATUS_BIT |
                  AD7124_ADC_CTRL_MODE_CONTINUOUS;
  uint8_t v[2] = { (uint8_t)(ctrl >> 8), (uint8_t)(ctrl & 0xFF) };
  (void)ad7124_write_reg(&ad7124_ctx, AD7124_REG_ADC_CONTROL, v, 2);
  ad7124_ll_cs_assert(&ad7124_ctx);
  uint8_t cmd = AD7124_CMD_READ_DATA;
  (void)HAL_SPI_Transmit(ad7124_ctx.hspi, &cmd, 1, 10);
}

static void AD7124_ReadSample_IfReady(void)
{
  if (!g_ad7124_drdy_flag) return;
  g_ad7124_drdy_flag = 0;
  uint8_t rx4[4];
  if (HAL_SPI_Receive(ad7124_ctx.hspi, rx4, 4, 2) != HAL_OK) return;
  uint32_t data24 = (uint32_t)((rx4[0] << 16) | (rx4[1] << 8) | rx4[2]);
  ad7124_raw24 = (int32_t)data24 - (int32_t)0x800000;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == SPI2_DRDY_Pin) {
    g_ad7124_drdy_flag = 1;
  }
}

static void handleInstruction(uint8_t instruction, const uint8_t *payload, uint16_t len)
{
  switch (instruction) {
    case 0x0A: {
      t_connected = 0;
      uint8_t ack = 0x0B;
      DAQ_UDP_SendPacket(&ack, 1);
      break;
    }
    case 0xAA: {
      if (t_connected < TIMEOUT_MS && payload && len >= 1U) {
        handleSolenoid(payload[0]);
        uint8_t ack = 0xAC;
        DAQ_UDP_SendPacket(&ack, 1);
      }
      break;
    }
    case 0xBA: {
      if (t_connected < TIMEOUT_MS && payload && len >= 2U) {
        uint16_t throttle = (uint16_t)((payload[0] << 8) | payload[1]);
        current_throttle = (int)(throttle * 3);
        uint8_t ack = 0xBB;
        DAQ_UDP_SendPacket(&ack, 1);
      }
      break;
    }
    default:
      break;
  }
}

static void handleSolenoid(uint8_t code)
{
  if (code >= 0x01 && code <= 0x08) {
    bool level = false;
    if (mcp23s17_read_solenoid(&sol_dev, code, &level)) {
      (void)mcp23s17_write_solenoid(&sol_dev, code, !level);
    }
  } else if (code == 0x11) {
    dualSolenoidState[0] = !dualSolenoidState[0];
    (void)mcp23s17_write_solenoid(&sol_dev, 9,  dualSolenoidState[0]);
    (void)mcp23s17_write_solenoid(&sol_dev, 10, !dualSolenoidState[0]);
  } else if (code == 0x12) {
    dualSolenoidState[1] = !dualSolenoidState[1];
    (void)mcp23s17_write_solenoid(&sol_dev, 11,  dualSolenoidState[1]);
    (void)mcp23s17_write_solenoid(&sol_dev, 12, !dualSolenoidState[1]);
  }
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Region 0: default no-access */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

#ifdef USE_LWIP
  /* Region 1: ETH DMA descriptors + LwIP in D2 SRAM (0x30000000): non-cacheable. */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

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
#ifdef USE_FULL_ASSERT
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
