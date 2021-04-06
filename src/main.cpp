
#include "main.h"
#include "adc.h"
#include "gpio.h"
#include "tim.h"
#include "usb_device.h"

extern "C" {
    void SystemClock_Config(void);
}

inline void pwmEnable() {
    // htim1.Instance->CCR1 = PWM_PRESC * 66 / 100;
    // htim1.Instance->CCR2 = PWM_PRESC * 33 / 100; // inverted channel, need to invert compare value too
    htim1.Instance->CCR1 = PWM_PRESC / 2;
    htim1.Instance->CCR2 = PWM_PRESC / 2;
    
}

inline void pwmDisable() {
    htim1.Instance->CCR1 = 0;
    htim1.Instance->CCR2 = PWM_PRESC + 1;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM14) {
        if(htim1.Instance->CCR1 == 0) { // toggle PWM channel 1
            htim1.Instance->CCR1 = PWM_PRESC / 2;
        }
        else {
            htim1.Instance->CCR1 = 0;
        }
    }

    if (htim->Instance == TIM15) {
        if(htim1.Instance->CCR2 == PWM_PRESC) { // toggle PWM channel 2 (inverted), off = max value
            htim1.Instance->CCR2 = PWM_PRESC / 2;
        }
        else {
            htim1.Instance->CCR2 = PWM_PRESC; // inverted channel, off = max value
        }
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    MX_GPIO_Init();
    MX_ADC_Init();
    MX_TIM1_Init();
    MX_TIM14_Init();
    MX_TIM15_Init();
    MX_USB_DEVICE_Init();

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    pwmDisable();

    HAL_TIM_Base_Start_IT(&htim14);
    HAL_TIM_Base_Start_IT(&htim15);
    TIM14->CNT = 0;
    TIM15->CNT = 0;
    TIM14->ARR = NOTE_FREQ / 2 / 440;
    TIM15->ARR = NOTE_FREQ / 2 / 880;
    __HAL_TIM_ENABLE(&htim14);
    __HAL_TIM_ENABLE(&htim15);

    
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        // pwmEnable();
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        // pwmDisable();
        HAL_Delay(100);
    }
}


/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI14 | RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
    RCC_OscInitStruct.HSI14CalibrationValue = 16;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state
     */

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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, tex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
