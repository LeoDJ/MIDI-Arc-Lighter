
#include "main.h"

#include <errno.h>
#include <sys/unistd.h>

#include "adc.h"
#include "dma.h"
#include "gpio.h"
#include "tim.h"
#include "usb_device.h"
#include "usart.h"
#include "spi.h"
#include "fatfs.h"

#include "config.h"
#include "toneOutput.h"
// #include "tune.h"
#include "usbd_cdc_if.h"
#include "usbd_midi_if.h"
#include "midiHandler.h"
#include "flash.h"
#include "midiFile.h"
#include "analog.h"


extern "C" {
    void SystemClock_Config(void);


    // enable printf functionality on PRINTF_UART
    int _write(int file, char *data, int len) {
        if ((file != STDOUT_FILENO) && (file != STDERR_FILENO)) {
            errno = EBADF;
            return -1;
        }

        // HAL_StatusTypeDef status = HAL_UART_Transmit(&PRINTF_UART, (uint8_t *)data, len, 1000);
        // return (status == HAL_OK ? len : 0);
        // HAL_GPIO_TogglePin(LED_2_B_GPIO_Port, LED_2_B_Pin);
        uint8_t statusU = CDC_Transmit_FS((uint8_t *)data, len);
        return (statusU == USBD_OK ? len : 0);
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC_Init();
    MX_TIM1_Init();
    MX_TIM14_Init();
    MX_TIM15_Init();
    MX_TIM16_Init();
    MX_TIM17_Init();
    MX_USART2_UART_Init();
    MX_SPI1_Init();
    MX_FATFS_Init();
    

    HAL_Delay(50); // delay needed for new device to enumerate after DFU upload
    MX_USB_DEVICE_Init();

    HAL_ADCEx_Calibration_Start(&hadc);

    HAL_Delay(1000);

    printf("Hello World!\n");

    toneOutputInit();
    midiInit();         // initialize midi buffers
    midiHandlerInit();  // register midi handler callbacks
    flashInit();

    flashLs("");
    flashPrintFile("/HELLOW~1.TXT");

    static MidiFile mf;
    char *midiFiles[] = {
        "/_HE'SA~1.MID",
        "/GRAVIT~2.MID",
        "/FLIGHT~1.MID",
        "/THROUG~1.MID",
        "/PINKPA~1.MID",
        "/_TETRIS.MID",
        "/NOKIA_~1.MID",
        "/MEGALO~1.MID"
    };
    static const int midiFilesNum = sizeof(midiFiles) / sizeof(midiFiles[0]);

    analogInit();
    // bool tonePlaying = false;

    // HAL_Delay(2000);
    // mf.play();

    // printf("[MAIN] Free Heap: %d\n", estimateFreeHeap(16));

    uint32_t lastTest = 0;

    while (1) {
        midiLoop();     // parse midi messages and call handler callbacks
        midiHandlerArpLoop();
        mf.process();

        // start song on button press
        if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin)) {
            // calling this multiple times shouldn't be an issue
            if(!mf.isCurrentlyPlaying()) {
                mf.openFile(midiFiles[HAL_GetTick()/20 % midiFilesNum]);
                mf.play();
            }
        }

        if (HAL_GetTick() - lastTest > 1000) {
            lastTest = HAL_GetTick();
            printf(".\n");
        }

        // dumb test code, turn on arc at 440Hz on button press
        // if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin)) {
        //     if (!tonePlaying) {
        //         tonePlaying = true;
        //         toneOutputWrite(0, 440);
        //         toneOutputWrite(1, 523);
        //     }
        // }
        // else {
        //     tonePlaying = false;
        //     toneOutputWrite(0, 0);
        //     toneOutputWrite(1, 0);
        // }


        // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        // toneOutputWrite(0, 440);
        // HAL_Delay(200);
        // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        // CDC_Transmit_FS((uint8_t *)".", 2);
        // toneOutputWrite(0, 0);
        // HAL_Delay(100);

        // playTune();

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
    while(1) {
        // HAL_Delay(50);
        // HAL_GPIO_TogglePin(LED_1_R_GPIO_Port, LED_1_R_Pin);
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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, tex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
