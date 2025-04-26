/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"
#include "i2c-lcd.h"
#include <ctype.h>
uint32_t adcValue;
uint16_t pwmValue;
uint32_t adcValue2;
uint16_t pwmValue2;
 //  ?ịnh nghĩa LEDS bằng LED_CNT


volatile bool flagRGBReceived = false;
volatile bool flagNumberReceived = false ;
volatile bool flagCommandReceived = false;
extern DMA_HandleTypeDef hdma_tim3_up;
DMA_HandleTypeDef hdma_tim3_up;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_ALARMS 20
#define MAX_NOTE_LENGTH 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim3_ch1_trig;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t data_valid = 1; // Biến để kiểm tra tính hợp lệ của dữ liệu
GPIO_PinState button1State;
GPIO_PinState button2State;
uint8_t rx_data;
uint8_t uart2_rx_buffer[6];  // Expecting 6 bytes for RGB values
uint8_t uart2_rx_index = 0;
char rx_buffer[128];  // Buffer để nhận dữ liệu từ UART
int index1 = 0; // Chỉ số cho STATE_COMMAND
int index2 = 0; // Chỉ số cho STATE_NUMBER
#define RX_BUFFER_SIZE 128
uint8_t button1Flag = 0; // Trạng thái của Button 1 (1 hoặc 0)
uint8_t button2Flag = 0;
uint8_t rxBuffer[1];
const uint32_t adcSendInterval = 500;
//static uint32_t lastAdcSendTime = 0;
uint32_t lastPressTime1 = 0;  // Th ?i gian nhấn Button 1
uint32_t lastPressTime2 = 0;  // Th ?i gian nhấn Button 2
const uint32_t debounceDelay = 300;
char rx_buffer[RX_BUFFER_SIZE];
volatile int adcActive = 0;
char RxBuffer[50];
uint8_t RxIndex = 0;
char timeStr[9] = "";    // Initialize with default values
char dateStr[11] = ""; // Initialize with default values
uint8_t lcdInitialized = 0;
volatile int brightness1 = 0;
volatile int brightness2 = 0;
//biến để lưu trạng thái LED
bool led1State = false;
bool led2State = false;
bool button1Pressed = false; // Trạng thái nút nhấn 1 (true: pressed, false: released)
bool button2Pressed = false; // Trạng thái nút nhấn 2 (true: pressed, false: released)
typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    char note[MAX_NOTE_LENGTH];
} Alarm;

Alarm alarms[MAX_ALARMS];
uint8_t alarmCount = 0;
uint8_t currentHour = 0;
uint8_t currentMinute = 0;
uint8_t currentSecond = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
#define LED1_PIN GPIO_PIN_1
#define LED2_PIN GPIO_PIN_4
#define LED_GPIO_PORT GPIOA
#define BUFFER_SIZE 6
char messageBuffer[BUFFER_SIZE];
#define noOfLEDs 17
uint16_t pwmData[24*noOfLEDs];
void ws2812Send(void);
void parseTimeAndDate(char* buffer);
void processCommand(char* command);
void updateLedStatusOnLCD(void);
void updateBrightnessOnLCD(void);
void processAlarm(char* alarmStr);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//void setRGB(uint16_t r, uint16_t g, uint16_t b) {
//    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, r); //  ? ?
//    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, g); // Xanh lá
//    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, b); // Xanh dương
//}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
    htim3.Instance->CCR1 = 0;
}

void resetAllLED(void)
{
    for (int i = 0; i < 24 * noOfLEDs; i++)
        pwmData[i] = 0; //  ?ặt lại tất cả giá trị LED v ? 0
}

void setAllLED(void)
{
    for (int i = 0; i < 24 * noOfLEDs; i++)
        pwmData[i] = 0; //  ?ặt lại giá trị của tất cả LED
}

void setLED(int LEDposition, int Red, int Green, int Blue)
{
    // Kiểm tra xem LEDposition có hợp lệ không
    if (LEDposition < 0 || LEDposition >= noOfLEDs) return;

    //  ?ặt màu cho LED
    for (int i = 0; i < 8; i++)
    {
        pwmData[24 * LEDposition + 7 - i] = ((Green >> i) & 1) + 1;  // Màu xanh
    }
    for (int i = 0; i < 8; i++)
    {
        pwmData[24 * LEDposition + 15 - i] = ((Red >> i) & 1) + 1;    // Màu đ ?
    }
    for (int i = 0; i < 8; i++)
    {
        pwmData[24 * LEDposition + 23 - i] = ((Blue >> i) & 1) + 1;   // Màu xanh dương
    }
}

// Hàm xử lý giá trị RGB
void ProcessReceivedRGBValues(char *rgbData) {
    int red, green, blue;
    sscanf(rgbData, "%d,%d,%d", &red, &green, &blue);

    // Chuyển đổi giá trị RGB cho từng LED
    for (int i = 0; i < 16; i++) {
        setLED(i, red, green, blue);
    }

    ws2812Send(); // Gửi dữ liệu màu cho LED
}

void ws2812Send(void)
{
    HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t *)pwmData, 24 * noOfLEDs);
}

uint32_t mapValue(uint32_t input)
{
	return (input * 1023)/3200;
}
uint32_t mapValue2(uint32_t input2)
{
	return (input2 * 1023)/3200;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t currentTime = HAL_GetTick();

    if (GPIO_Pin == GPIO_PIN_0 && (currentTime - lastPressTime1 > debounceDelay))
    {
        button1State = !button1State;
        led1State = button1State;  // �?ồng bộ trạng thái LED với nút nhấn
        if (button1State) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
            button1Flag = 1;
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
            button1Flag = 0;
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        }
        lastPressTime1 = currentTime;
        updateLedStatusOnLCD();
    }

    if (GPIO_Pin == GPIO_PIN_5 && (currentTime - lastPressTime2 > debounceDelay))
    {
        button2State = !button2State;
        led2State = button2State;  // �?ồng bộ trạng thái LED với nút nhấn
        if (button2State) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
            button2Flag = 1;
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
            button2Flag = 0;
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
        }
        lastPressTime2 = currentTime;
        updateLedStatusOnLCD();
    }
}

void handleCommand(char receivedChar) {

    switch (receivedChar) {
        case 'a':
            led1State = true;
            button1State = true;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
            button1Flag = 1;
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            break;
        case 'b':
            led1State = false;
            button1State = false;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
            button1Flag = 0;
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
            break;
        case 'c':
            led2State = true;
            button2State = true;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
            button2Flag = 1;
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
            break;
        case 'd':
            led2State = false;
            button2State = false;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
            button2Flag = 0;
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
            break;
        case 'e':
            adcActive = 1;
            break;
        case 'f':
            adcActive = 0;
            break;
        default:
            break;
    }
    updateLedStatusOnLCD();
}

enum UARTState {
    STATE_COMMAND,
    STATE_RGB,
    STATE_NUMBER
};

enum UARTState currentState = STATE_COMMAND;


// Function to process the received RGB values

void ProcessReceivedCommaSeparatedString(char *data) {
    // B�? qua ký tự 'B' ở đầu nếu có
    if(data[0] == 'B') {
        data++; // Di chuyển con tr�? qua ký tự đầu tiên
    }

    char *token;
    char *rest = data;

    // Tách giá trị đầu tiên
    token = strtok(rest, ",");
    if (token != NULL) {
        int newBrightness1 = atoi(token);
        if(newBrightness1 >= 0 && newBrightness1 <= 100) {
            brightness1 = newBrightness1;
            uint32_t pwm_value1 = (brightness1 * htim1.Init.Period) / 100;
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm_value1);
        }
    }

    // Tách giá trị thứ hai
    token = strtok(NULL, ",");
    if (token != NULL) {
        int newBrightness2 = atoi(token);
        if(newBrightness2 >= 0 && newBrightness2 <= 100) {
            brightness2 = newBrightness2;
            uint32_t pwm_value2 = (brightness2 * htim1.Init.Period) / 100;
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm_value2);
        }
    }

    // Cập nhật LCD
    updateBrightnessOnLCD();
}

void updateBrightnessOnLCD(void) {
    char buffer[20];

    // Xóa dòng cũ trước khi hiển thị giá trị mới
    lcd_put_cur(2, 0);
    lcd_send_string("          ");
    lcd_put_cur(3, 0);
    lcd_send_string("          ");

    // Hiển thị giá trị mới
    lcd_put_cur(2, 0);
    snprintf(buffer, sizeof(buffer), "LB1: %3d%%", brightness1);
    lcd_send_string(buffer);

    lcd_put_cur(3, 0);
    snprintf(buffer, sizeof(buffer), "LB2: %3d%%", brightness2);
    lcd_send_string(buffer);

    HAL_Delay(10); // Đảm bảo LCD có đủ thời gian cập nhật
}

void initializeLCD(void) {
  HAL_Delay(100);
  lcd_init();
  HAL_Delay(100);

  lcd_clear();
  HAL_Delay(50);

  // Hiển thị nhãn ban đầu
  lcd_put_cur(0, 0);
  lcd_send_string("Time: "); // 6 ký tự cho "Time: "
  lcd_put_cur(1, 0);
  lcd_send_string("Date: "); // 6 ký tự cho "Date: "

  // Hiển thị trạng thái LED ban đầu
  lcd_put_cur(0, 11); // Bắt đầu từ cột thứ 11 (sau "Time: ")
  lcd_send_string("LED1: OFF");
  lcd_put_cur(1, 11);
  lcd_send_string("LED2: OFF");

  // Thêm dòng cho độ sáng
  lcd_put_cur(2, 0);
  lcd_send_string("LB1: 0%  ");
  lcd_put_cur(3, 0);
  lcd_send_string("LB2: 0%  ");

  lcdInitialized = 1;
}

// Function to update LCD without clearing
void updateLCD(void) {
    if (!lcdInitialized) {
        initializeLCD();
        return;
    }

    // Update only time portion
    lcd_put_cur(0, 0);
    lcd_send_string(timeStr);

    // Update only date portion
    lcd_put_cur(1, 0);
    lcd_send_string(dateStr);
}

void parseTimeAndDate(char* buffer) {
    // Check if received string has minimum required length
    if(strlen(buffer) >= 19) {
        // Extract time (HH:MM:SS)
        strncpy(timeStr, buffer, 8);
        timeStr[8] = '\0';

        // Extract date (DD/MM/YYYY)
        strncpy(dateStr, buffer + 9, 10);
        dateStr[10] = '\0';

        // Update LCD without clearing
        updateLCD();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == USART2) {
        char receivedChar = rx_data;

        // Process time/date data
        if((receivedChar >= '0' && receivedChar <= '9') ||
           receivedChar == ':' || receivedChar == '/' ||
           receivedChar == ' ' || receivedChar == '\n') {

            if(RxIndex < sizeof(RxBuffer) - 1) {
                RxBuffer[RxIndex++] = receivedChar;
            }

            if(receivedChar == '\n' || RxIndex >= sizeof(RxBuffer) - 1) {
                RxBuffer[RxIndex] = '\0';
                parseTimeAndDate(RxBuffer);
                RxIndex = 0;
            }
        }
        // Process commands
        else if(receivedChar == 'L' || receivedChar == 'a' || receivedChar == 'b' ||
                receivedChar == 'c' || receivedChar == 'd' || receivedChar == 'e' ||
                receivedChar == 'f' || receivedChar == 'S') {

            if(RxIndex < sizeof(RxBuffer) - 1) {
                RxBuffer[RxIndex++] = receivedChar;
            }

            if(receivedChar == '\n' || RxIndex >= sizeof(RxBuffer) - 1) {
                RxBuffer[RxIndex] = '\0';
                if(RxBuffer[0] == 'S') {
                    processAlarm(RxBuffer);
                } else {
                    processCommand(RxBuffer);
                }
                RxIndex = 0;
            }
        }
        // Process brightness data
        else if(isdigit(receivedChar) || receivedChar == ',' || receivedChar == 'B'|| receivedChar == '\n') {
            if(RxIndex < sizeof(RxBuffer) - 1) {
                RxBuffer[RxIndex++] = receivedChar;
            }

            if(receivedChar == '\n' || RxIndex >= sizeof(RxBuffer) - 1) {
                RxBuffer[RxIndex] = '\0';
                ProcessReceivedCommaSeparatedString(RxBuffer);
                RxIndex = 0;
            }
        }

        HAL_UART_Receive_IT(&huart2, (uint8_t *)&rx_data, 1);
    }
}

void processCommand(char* command) {
    if (command [0] == 'L') {
        int ledIndex;
        char ledStateStr[4]; // To store "ON" or "OFF"
        sscanf(command + 2, "%d %s", &ledIndex, ledStateStr);

        bool ledState = (strcmp(ledStateStr, "ON") == 0);

        if (ledIndex == 1) {
            led1State = ledState;
            if (ledState) {
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            } else {
                HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
            }
        } else if (ledIndex == 2) {
            led2State = ledState;
            if (ledState) {
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
            } else {
                HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
            }
        }

        updateLedStatusOnLCD();
    } else {
        // Handle other commands ('a', 'b', 'c', 'd', 'e', 'f')
        handleCommand(command[0]);
    }
}

void processAlarm(char* alarmStr) {
    if(alarmCount < MAX_ALARMS) {
        int hour, minute, second;
        char note[MAX_NOTE_LENGTH];
        if(sscanf(alarmStr, "S%d:%d:%d:%s", &hour, &minute, &second, note) == 4) {
            alarms[alarmCount].hour = hour;
            alarms[alarmCount].minute = minute;
            alarms[alarmCount].second = second;
            strncpy(alarms[alarmCount].note, note, MAX_NOTE_LENGTH - 1);
            alarms[alarmCount].note[MAX_NOTE_LENGTH - 1] = '\0';
            alarmCount++;
        }
    }
}

void checkAlarms(void) {
    for(int i = 0; i < alarmCount; i++) {
        if(alarms[i].hour == currentHour &&
           alarms[i].minute == currentMinute &&
           alarms[i].second == currentSecond) {
            processCommand(alarms[i].note);

            // Xóa báo thức đã thực hiện
            for(int j = i; j < alarmCount - 1; j++) {
                alarms[j] = alarms[j + 1];
            }
            alarmCount--;
            i--; // Kiểm tra lại vị trí hiện tại
        }
    }
}

void updateLedStatusOnLCD() {
    // Cập nhật trạng thái LED
    lcd_put_cur(0, 11);
    lcd_send_string(led1State ? "LED1: ON " : "LED1: OFF");
    lcd_put_cur(1, 11);
    lcd_send_string(led2State ? "LED2: ON " : "LED2: OFF");

    // Cập nhật trạng thái nút nhấn
    lcd_put_cur(2, 11);
    lcd_send_string(button1State ? "BTN1: ON " : "BTN1: OFF");
    lcd_put_cur(3, 11);
    lcd_send_string(button2State ? "BTN2: ON " : "BTN2: OFF");
}

void receiveDataFromSTM32() {
    if (HAL_UART_Receive(&huart2, (uint8_t *)rx_buffer, RX_BUFFER_SIZE, HAL_MAX_DELAY) == HAL_OK) {
        rx_buffer[RX_BUFFER_SIZE - 1] = '\0'; //  ?ảm bảo chuỗi kết thúc bằng null

        // G ?i hàm để xử lý chuỗi có dấu phẩy
        ProcessReceivedCommaSeparatedString(rx_buffer);
    }
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  // Initialize LCD
  lcd_init();
  // Initialize LCD with delay
     HAL_Delay(200);  // Wait for system to stabilize
     initializeLCD();

//  HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)led_data, LED_COUNT * 24);
  HAL_UART_Receive_IT(&huart2, (uint8_t *)&rx_buffer[index1], 1);
//  SendNeoPixelColor(255, 0, 255);
//  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
//  HAL_ADC_Start_IT(&hadc1);

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

  // Khởi tạo trạng thái ban đầu
     led1State = false;
     led2State = false;
     button1State = false;
     button2State = false;
     button1Flag = 0;
     button2Flag = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  checkAlarms();
	  if (adcActive) {
	      //  ? ?c giá trị từ PA6 (ADC1 Channel 6)
	      ADC_ChannelConfTypeDef sConfig = {0};
	      sConfig.Channel = ADC_CHANNEL_6;  // PA6 là Channel 6
	      sConfig.Rank = 1;
	      HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	      HAL_ADC_Start(&hadc1);
	      HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	      uint32_t adcValue = HAL_ADC_GetValue(&hadc1);  // Giá trị cho TIM1 CH1
	      HAL_ADC_Stop(&hadc1);

	      //  ? ?c giá trị từ PA7 (ADC1 Channel 7)
	      sConfig.Channel = ADC_CHANNEL_7;  // PA7 là Channel 7
	      HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	      HAL_ADC_Start(&hadc1);
	      HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	      uint32_t adcValue2 = HAL_ADC_GetValue(&hadc1); // Giá trị cho TIM1 CH2
	      HAL_ADC_Stop(&hadc1);

	      //  ?nh xạ giá trị ADC sang PWM
	      uint32_t pwmValue = mapValue(adcValue);
	      uint32_t pwmValue2 =mapValue2(adcValue2);

	      //  ?ặt giá trị PWM cho TIM1 Channel 1 và Channel 2
	      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwmValue);
	      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwmValue2);
	  }
	  HAL_Delay(100);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 64-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
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
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 30-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 99;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 50;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);

        /* Kích hoạt và cấu hình ngắt cho dòng EXTI1 */
        HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
}
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
