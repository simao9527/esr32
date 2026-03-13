
#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include <stdbool.h>

// OLED显示屏尺寸定义
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// OLED初始化函数
void OLED_Init(void);

// OLED清屏函数
void OLED_Clear(void);

// OLED刷新显示
void OLED_Refresh(void);

// OLED绘制像素点
// 参数:
//   x - 横坐标(0-127)
//   y - 纵坐标(0-63)
//   color - 颜色(1-亮, 0-灭)
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

// OLED绘制字符
// 参数:
//   x - 起始横坐标
//   y - 起始纵坐标
//   ch - 要显示的字符
//   size - 字体大小(12/16/24)
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size);

// OLED绘制字符串
// 参数:
//   x - 起始横坐标
//   y - 起始纵坐标
//   str - 要显示的字符串
//   size - 字体大小(12/16/24)
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);

// OLED绘制数字
// 参数:
//   x - 起始横坐标
//   y - 起始纵坐标
//   num - 要显示的数字
//   len - 数字长度
//   size - 字体大小(12/16/24)
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);

// OLED绘制浮点数
// 参数:
//   x - 起始横坐标
//   y - 起始纵坐标
//   num - 要显示的浮点数
//   len - 整数部分长度
//   size - 字体大小(12/16/24)
//   decimal - 小数点位数
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len, uint8_t size, uint8_t decimal);

// OLED绘制汉字
// 参数:
//   x - 起始横坐标
//   y - 起始纵坐标
//   index - 汉字索引
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t index);

// OLED绘制图像
// 参数:
//   x - 起始横坐标
//   y - 起始纵坐标
//   width - 图像宽度
//   height - 图像高度
//   bmp - 图像数据
void OLED_DrawBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp);

#endif // OLED_H
