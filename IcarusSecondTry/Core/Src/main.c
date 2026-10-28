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
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#if defined(HAL_SPI_MODULE_ENABLED)
#include "ad7124.h"
#include "ad7124_stm32_hal.h"
#include "load_cell_adc_profile.h"
#endif
#if defined(HAL_ETH_MODULE_ENABLED)
#include "ethernet_link_monitor.h"
#include "ksz8081.h"
#include "ksz8081_stm32_hal.h"
#endif
#include "network_runtime.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BLINK_PERIOD_MS 500U
#define CONSOLE_HEARTBEAT_PERIOD_MS 1000U
#define CONSOLE_TX_TIMEOUT_MS 20U
#define CONSOLE_LOCK_TIMEOUT_MS 25U
#define AD7124_SPI_TIMEOUT_MS 20U
#define AD7124_SAMPLE_PERIOD_MS 10U
#define AD7124_RETRY_PERIOD_MS 1000U
#define PHY_DIAGNOSTIC_PERIOD_MS 1000U
/* CubeMX LwIP ethernetif owns ETH/MDIO after the LwIP transition. */
#define ICARUS_LWIP_ETHERNETIF_OWNS_ETH 1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart8;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for HeartbeatTask */
osThreadId_t HeartbeatTaskHandle;
const osThreadAttr_t HeartbeatTask_attributes = {
  .name = "HeartbeatTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */

static const uint8_t console_startup_message[] =
    "\r\nIcarusSecondTry: UART8 diagnostics online\r\n";
static const uint8_t console_gpio_safe_message[] =
    "IcarusSecondTry: GPIO safe outputs asserted\r\n";
static const uint8_t console_heartbeat_message[] =
    "IcarusSecondTry: heartbeat\r\n";

/* Created before application threads. It serializes the temporary shared
 * UART diagnostic path with a finite wait. A later DiagnosticsTask can become
 * the sole UART owner without changing producer diagnostics. */
static osMutexId_t console_mutex;
static const osMutexAttr_t console_mutex_attributes = {
  .name = "Console",
};

/* This initial UDP slice has no configured actuator peripheral. It still owns
 * raw-API UDP, validates all ingress, serves echo, and publishes zeroed
 * telemetry. A later ActuatorTask supplies the only target-application
 * callback; until then no network input can energize an output. */
static icarus_network_runtime_t network_runtime;

#if defined(HAL_SPI_MODULE_ENABLED)
static ad7124_t load_cell_adc;
static ad7124_stm32_hal_t load_cell_transport;
static ad7124_diagnostics_t load_cell_diagnostics;
/* Debugger-visible only until telemetry owns an immutable sample snapshot. */
static ad7124_sample_t load_cell_latest_sample;
static ad7124_result_t load_cell_last_result;
static uint32_t load_cell_sample_sequence;

static const uint8_t console_ad7124_ready_message[] =
    "IcarusSecondTry: AD7124 acquisition configured\r\n";
static const uint8_t console_ad7124_failure_message[] =
    "IcarusSecondTry: AD7124 acquisition fault\r\n";

static osThreadId_t load_cell_acquisition_task_handle;
static const osThreadAttr_t load_cell_acquisition_task_attributes = {
  .name = "LoadCellAcq",
  .stack_size = 512U * 4U,
  .priority = (osPriority_t)osPriorityBelowNormal,
};
#endif

#if defined(HAL_ETH_MODULE_ENABLED) && !defined(ICARUS_LWIP_ETHERNETIF_OWNS_ETH)
/* The NetworkTask is the sole post-startup owner of ETH MDIO operations. */
static ksz8081_t ethernet_phy;
static ksz8081_stm32_hal_t ethernet_phy_transport;
static ksz8081_status_t ethernet_phy_status;
static ksz8081_result_t ethernet_phy_last_result;
/* Debugger-visible state and counters; only NetworkTask writes this object. */
static ethernet_link_monitor_t ethernet_link_monitor;

static const uint8_t console_ethernet_phy_found_message[] =
    "IcarusSecondTry: KSZ8081 discovered\r\n";
static const uint8_t console_ethernet_link_down_message[] =
    "IcarusSecondTry: Ethernet link down\r\n";
static const uint8_t console_ethernet_negotiating_message[] =
    "IcarusSecondTry: Ethernet negotiating\r\n";
static const uint8_t console_ethernet_link_up_message[] =
    "IcarusSecondTry: Ethernet link up\r\n";
static const uint8_t console_ethernet_mdio_fault_message[] =
    "IcarusSecondTry: Ethernet MDIO/PHY fault\r\n";

static osThreadId_t network_task_handle;
static const osThreadAttr_t network_task_attributes = {
  .name = "NetworkTask",
  .stack_size = 768U * 4U,
  .priority = (osPriority_t)osPriorityBelowNormal,
};
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART8_Init(void);
static void MX_SPI2_Init(void);
void StartDefaultTask(void *argument);
void StartHeartbeatTask(void *argument);

/* USER CODE BEGIN PFP */

static HAL_StatusTypeDef Console_Write(const uint8_t *data, uint16_t length);
static void Board_AssertSafeOutputs(void);
#if defined(HAL_SPI_MODULE_ENABLED)
static void StartLoadCellAcquisitionTask(void *argument);
#endif
#if defined(HAL_ETH_MODULE_ENABLED) && !defined(ICARUS_LWIP_ETHERNETIF_OWNS_ETH)
static void StartNetworkTask(void *argument);
static void Ethernet_ReportStateTransition(ethernet_link_state_t state);
#endif

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * This is intentionally a small polling console for bring-up.  Every write
 * has a finite timeout so a fault cannot stall the firmware indefinitely.
 * Do not redirect printf() here: its buffering and error handling would hide
 * the exact UART operation being tested.  DMA/interrupt-driven logging comes
 * later, once the basic electrical UART path is proven.
 */
static HAL_StatusTypeDef Console_Write(const uint8_t *data, uint16_t length)
{
  HAL_StatusTypeDef result;
  bool lock_acquired = false;

  if ((data == NULL) || (length == 0U))
  {
    return HAL_ERROR;
  }

  if (osKernelGetState() == osKernelRunning)
  {
    if ((console_mutex == NULL) ||
        (osMutexAcquire(console_mutex, CONSOLE_LOCK_TIMEOUT_MS) != osOK))
    {
      return HAL_TIMEOUT;
    }
    lock_acquired = true;
  }

  result = HAL_UART_Transmit(&huart8, data, length, CONSOLE_TX_TIMEOUT_MS);
  if (lock_acquired)
  {
    (void)osMutexRelease(console_mutex);
  }
  return result;
}

/*
 * Keep every device chip select inactive until that device's driver takes
 * ownership. CubeMX also establishes these levels during GPIO setup, but this
 * explicit call makes the board safety policy visible in handwritten code and
 * safely reasserts it before the scheduler starts.
 *
 * This function deliberately does not configure GPIO modes or enable any
 * peripheral. Those hardware selections remain in the .ioc file.
 */
static void Board_AssertSafeOutputs(void)
{
  HAL_GPIO_WritePin(GPIOE,
                    SPI4_CS_SOL1_12_Pin | SPI4_CS_SOL13_24_Pin,
                    GPIO_PIN_SET);
  HAL_GPIO_WritePin(SPI5_CS_TC_GPIO_Port, SPI5_CS_TC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SPI1_CS_PT1_8_GPIO_Port, SPI1_CS_PT1_8_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOD,
                    SPI1_CS_PT9_16_Pin | SPI1_CS_PT17_24_Pin | SPI2_CS_LC_Pin,
                    GPIO_PIN_SET);
  HAL_GPIO_WritePin(ONBOARD_LED_GPIO_Port, ONBOARD_LED_Pin, GPIO_PIN_RESET);
}

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
  Board_AssertSafeOutputs();
  MX_UART8_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */

  if (Console_Write(console_startup_message,
                    sizeof(console_startup_message) - 1U) != HAL_OK)
  {
    Error_Handler();
  }

  if (Console_Write(console_gpio_safe_message,
                    sizeof(console_gpio_safe_message) - 1U) != HAL_OK)
  {
    Error_Handler();
  }

#if defined(HAL_SPI_MODULE_ENABLED)
  /* hspi2 is generated and initialized by CubeMX when SPI2 is enabled in the
   * .ioc file. Binding is separate from the probe so this setup performs no
   * AD7124 transaction before the acquisition task owns SPI2. */
  if (AD7124_Stm32HalBind(&load_cell_adc,
                          &load_cell_transport,
                          &hspi2,
                          SPI2_CS_LC_GPIO_Port,
                          SPI2_CS_LC_Pin,
                          AD7124_SPI_TIMEOUT_MS) != AD7124_RESULT_OK)
  {
    Error_Handler();
  }
#endif

#if defined(HAL_ETH_MODULE_ENABLED) && !defined(ICARUS_LWIP_ETHERNETIF_OWNS_ETH)
  if (KSZ8081_Stm32HalBind(&ethernet_phy, &ethernet_phy_transport, &heth) !=
      KSZ8081_RESULT_OK)
  {
    Error_Handler();
  }
  EthernetLinkMonitor_Init(&ethernet_link_monitor);
#endif

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  console_mutex = osMutexNew(&console_mutex_attributes);
  if (console_mutex == NULL)
  {
    Error_Handler();
  }
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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of HeartbeatTask */
  HeartbeatTaskHandle = osThreadNew(StartHeartbeatTask, NULL, &HeartbeatTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
#if defined(HAL_SPI_MODULE_ENABLED)
  load_cell_acquisition_task_handle = osThreadNew(StartLoadCellAcquisitionTask,
                                                   NULL,
                                                   &load_cell_acquisition_task_attributes);
  if (load_cell_acquisition_task_handle == NULL)
  {
    Error_Handler();
  }
#endif
#if defined(HAL_ETH_MODULE_ENABLED) && !defined(ICARUS_LWIP_ETHERNETIF_OWNS_ETH)
  network_task_handle = osThreadNew(StartNetworkTask, NULL,
                                    &network_task_attributes);
  if (network_task_handle == NULL)
  {
    Error_Handler();
  }
#endif
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

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
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
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
  * @brief UART8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART8_Init(void)
{

  /* USER CODE BEGIN UART8_Init 0 */

  /* USER CODE END UART8_Init 0 */

  /* USER CODE BEGIN UART8_Init 1 */

  /* USER CODE END UART8_Init 1 */
  huart8.Instance = UART8;
  huart8.Init.BaudRate = 115200;
  huart8.Init.WordLength = UART_WORDLENGTH_8B;
  huart8.Init.StopBits = UART_STOPBITS_1;
  huart8.Init.Parity = UART_PARITY_NONE;
  huart8.Init.Mode = UART_MODE_TX_RX;
  huart8.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart8.Init.OverSampling = UART_OVERSAMPLING_16;
  huart8.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart8.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart8.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart8, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart8, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart8) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART8_Init 2 */

  /* USER CODE END UART8_Init 2 */

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
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
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

  /*Configure GPIO pins : PB3 PB4 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

#if defined(HAL_SPI_MODULE_ENABLED)
/*
 * This task is the sole current owner of SPI2. It configures one provisional
 * load-cell channel, verifies every written register, then publishes only raw
 * ADC codes for later telemetry/calibration work. A failed transaction never
 * becomes a plausible sample; configuration retries after a bounded delay.
 */
static void StartLoadCellAcquisitionTask(void *argument)
{
  ad7124_result_t previous_result = AD7124_RESULT_ARGUMENT;

  (void)argument;

  for (;;)
  {
    load_cell_last_result = AD7124_ConfigureSingleChannel(
        &load_cell_adc, &load_cell_adc_profile);
    if (load_cell_last_result == AD7124_RESULT_OK)
    {
      load_cell_last_result = AD7124_ReadDiagnostics(&load_cell_adc,
                                                      &load_cell_diagnostics);
    }

    if (load_cell_last_result != previous_result)
    {
      const uint8_t *message = (load_cell_last_result == AD7124_RESULT_OK)
          ? console_ad7124_ready_message
          : console_ad7124_failure_message;
      const uint16_t message_length = (load_cell_last_result == AD7124_RESULT_OK)
          ? (uint16_t)(sizeof(console_ad7124_ready_message) - 1U)
          : (uint16_t)(sizeof(console_ad7124_failure_message) - 1U);

      (void)Console_Write(message, message_length);
      previous_result = load_cell_last_result;
    }

    if (load_cell_last_result != AD7124_RESULT_OK)
    {
      osDelay(AD7124_RETRY_PERIOD_MS);
      continue;
    }

    for (;;)
    {
      load_cell_last_result = AD7124_ReadSample(&load_cell_adc,
                                                 &load_cell_latest_sample);
      if (load_cell_last_result == AD7124_RESULT_OK)
      {
        load_cell_sample_sequence++;
      }
      else if (load_cell_last_result != AD7124_RESULT_NOT_READY)
      {
        if (load_cell_last_result != previous_result)
        {
          (void)Console_Write(console_ad7124_failure_message,
                              (uint16_t)(sizeof(console_ad7124_failure_message) - 1U));
          previous_result = load_cell_last_result;
        }
        break;
      }
      osDelay(AD7124_SAMPLE_PERIOD_MS);
    }
  }
}
#endif

#if defined(HAL_ETH_MODULE_ENABLED) && !defined(ICARUS_LWIP_ETHERNETIF_OWNS_ETH)
/*
 * No LwIP or packet DMA is started in this milestone. This task only performs
 * bounded, read-only MDIO discovery and link-state snapshots for the KSZ8081.
 * The monitor's counters are debugger-visible; state transitions are also sent
 * through the bounded UART diagnostics path for bench testing.
 */
static void StartNetworkTask(void *argument)
{
  bool phy_discovered = false;

  (void)argument;
  for (;;)
  {
    const bool is_scan = !phy_discovered;
    bool state_changed;

    if (!phy_discovered)
    {
      ethernet_phy_last_result = KSZ8081_Scan(&ethernet_phy,
                                              &ethernet_phy_status);
      phy_discovered = (ethernet_phy_last_result == KSZ8081_RESULT_OK);
    }
    else
    {
      ethernet_phy_last_result = KSZ8081_ReadStatus(
          &ethernet_phy, ethernet_phy_status.phy_address, &ethernet_phy_status);
      if (ethernet_phy_last_result != KSZ8081_RESULT_OK)
      {
        phy_discovered = false;
      }
    }

    state_changed = EthernetLinkMonitor_Observe(&ethernet_link_monitor,
                                                 is_scan,
                                                 ethernet_phy_last_result,
                                                 (ethernet_phy_last_result ==
                                                  KSZ8081_RESULT_OK)
                                                     ? &ethernet_phy_status
                                                     : NULL,
                                                 HAL_GetTick());
    if (is_scan && (ethernet_phy_last_result == KSZ8081_RESULT_OK))
    {
      (void)Console_Write(console_ethernet_phy_found_message,
                          (uint16_t)(sizeof(console_ethernet_phy_found_message) - 1U));
    }
    if (state_changed)
    {
      Ethernet_ReportStateTransition(ethernet_link_monitor.state);
    }

    osDelay(PHY_DIAGNOSTIC_PERIOD_MS);
  }
}

static void Ethernet_ReportStateTransition(ethernet_link_state_t state)
{
  const uint8_t *message = NULL;
  uint16_t message_length = 0U;

  switch (state)
  {
    case ETHERNET_LINK_STATE_DOWN:
      message = console_ethernet_link_down_message;
      message_length = (uint16_t)(sizeof(console_ethernet_link_down_message) - 1U);
      break;
    case ETHERNET_LINK_STATE_NEGOTIATING:
      message = console_ethernet_negotiating_message;
      message_length = (uint16_t)(sizeof(console_ethernet_negotiating_message) - 1U);
      break;
    case ETHERNET_LINK_STATE_UP:
      message = console_ethernet_link_up_message;
      message_length = (uint16_t)(sizeof(console_ethernet_link_up_message) - 1U);
      break;
    case ETHERNET_LINK_STATE_FAULT:
      message = console_ethernet_mdio_fault_message;
      message_length = (uint16_t)(sizeof(console_ethernet_mdio_fault_message) - 1U);
      break;
    case ETHERNET_LINK_STATE_INITIALIZING:
    default:
      return;
  }

  (void)Console_Write(message, message_length);
}
#endif

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN 5 */
  /* This is the sole application owner of raw-API UDP. LwIP core calls are
   * marshalled by udp_service through tcpip_callback(); receive callbacks only
   * enqueue bounded datagrams. No packet reaches an actuator in this slice. */
  if (!IcarusNetworkRuntime_Init(&network_runtime, NULL, NULL))
  {
    Error_Handler();
  }
  for(;;)
  {
    IcarusNetworkRuntime_Process(&network_runtime, HAL_GetTick());
    osDelay(5U);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartHeartbeatTask */
/**
* @brief Function implementing the HeartbeatTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartHeartbeatTask */
void StartHeartbeatTask(void *argument)
{
  /* USER CODE BEGIN StartHeartbeatTask */
  uint32_t elapsed_ms = 0U;

  for(;;)
  {
    HAL_GPIO_TogglePin(ONBOARD_LED_GPIO_Port, ONBOARD_LED_Pin);

    elapsed_ms += BLINK_PERIOD_MS;
    if (elapsed_ms >= CONSOLE_HEARTBEAT_PERIOD_MS)
    {
      if (Console_Write(console_heartbeat_message,
                        sizeof(console_heartbeat_message) - 1U) != HAL_OK)
      {
        Error_Handler();
      }

      elapsed_ms = 0U;
    }

    /* Tasks must yield with the RTOS delay API. HAL_Delay() depends on the
     * HAL time base and must not be used as a task scheduling mechanism. */
    osDelay(BLINK_PERIOD_MS);
  }
  /* USER CODE END StartHeartbeatTask */
}

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
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

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
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
