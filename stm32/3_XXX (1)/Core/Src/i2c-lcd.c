#include "i2c-lcd.h"
#define SLAVE_ADDRESS_LCD 0x27 << 1

extern I2C_HandleTypeDef hi2c1; // Thêm vào đầu tệp i2c-lcd.c

void lcd_send_cmd(char cmd) {
    char data[4];
    data[0] = (cmd & 0xF0) | 0x0C;  // EN = 1, RS = 0
    data[1] = (cmd & 0xF0) | 0x08;  // EN = 0, RS = 0
    data[2] = ((cmd << 4) & 0xF0) | 0x0C;
    data[3] = ((cmd << 4) & 0xF0) | 0x08;
    HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *)data, 4, HAL_MAX_DELAY);
}

void lcd_send_data(char data) {
    char data_arr[4];
    data_arr[0] = (data & 0xF0) | 0x0D;  // EN = 1, RS = 1
    data_arr[1] = (data & 0xF0) | 0x09;  // EN = 0, RS = 1
    data_arr[2] = ((data << 4) & 0xF0) | 0x0D;
    data_arr[3] = ((data << 4) & 0xF0) | 0x09;
    HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *)data_arr, 4, HAL_MAX_DELAY);
}

void lcd_init(void) {
    HAL_Delay(50);  // Wait for >40ms after power-on
    lcd_send_cmd(0x30);
    HAL_Delay(5);   // Wait for >4.1ms
    lcd_send_cmd(0x30);
    HAL_Delay(1);   // Wait for >100us
    lcd_send_cmd(0x30);
    HAL_Delay(10);
    lcd_send_cmd(0x20);  // 4-bit mode
    HAL_Delay(10);

    lcd_send_cmd(0x28);  // Function set: 4-bit mode, 2 lines, 5x8 dots
    lcd_send_cmd(0x08);  // Display off
    lcd_send_cmd(0x01);  // Clear display
    HAL_Delay(2);
    lcd_send_cmd(0x06);  // Entry mode set
    lcd_send_cmd(0x0C);  // Display on, cursor off
}

void lcd_send_string(char *str) {
    while (*str) {
        lcd_send_data(*str++);
    }
}

void lcd_clear(void) {
    lcd_send_cmd(0x01);  // Clear display
    HAL_Delay(2);
}

void lcd_put_cur(int row, int col) {
    uint8_t pos;
    switch (row) {
        case 0:
            pos = col | 0x80;
            break;
        case 1:
            pos = col | 0xC0;
            break;
        case 2:
            pos = col | 0x94;
            break;
        case 3:
            pos = col | 0xD4;
            break;
        default:
            pos = col | 0x80;
    }
    lcd_send_cmd(pos);
}
