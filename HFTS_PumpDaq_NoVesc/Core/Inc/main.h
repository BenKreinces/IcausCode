/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void MX_I2C1_Init(void);
void MX_SPI5_Init(void);
void MX_UART8_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define I2C1_BUS_HZ 100000
#define TMP117_ADDRESS 0b1001000x
#define SPI4_CS_SOL1_12_Pin GPIO_PIN_3
#define SPI4_CS_SOL1_12_GPIO_Port GPIOE
#define SPI4_CS_SOL13_24_Pin GPIO_PIN_4
#define SPI4_CS_SOL13_24_GPIO_Port GPIOE
#define SPI5_CS_TC_Pin GPIO_PIN_6
#define SPI5_CS_TC_GPIO_Port GPIOF
#define SPI1_CS_PT1_8_Pin GPIO_PIN_12
#define SPI1_CS_PT1_8_GPIO_Port GPIOB
#define SPI2_DRDY_Pin GPIO_PIN_8
#define SPI2_DRDY_GPIO_Port GPIOD
#define ONBOARD_LED_Pin GPIO_PIN_0
#define ONBOARD_LED_GPIO_Port GPIOI
#define SPI1_CS_PT9_16_Pin GPIO_PIN_0
#define SPI1_CS_PT9_16_GPIO_Port GPIOD
#define SPI1_CS_PT17_24_Pin GPIO_PIN_1
#define SPI1_CS_PT17_24_GPIO_Port GPIOD
#define SPI2_CS_LC_Pin GPIO_PIN_4
#define SPI2_CS_LC_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
