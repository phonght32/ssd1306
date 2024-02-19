// MIT License

// Copyright (c) 2024 phonght32

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __SSD1306_H__
#define __SSD1306_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "err_code.h"
#include "fonts.h"

typedef err_code_t (*ssd1306_func_set_cs)(uint8_t level);
typedef err_code_t (*ssd1306_func_set_dc)(uint8_t level);
typedef err_code_t (*ssd1306_func_set_rst)(uint8_t level);
typedef err_code_t (*ssd1306_func_spi_send)(uint8_t *buf_send, uint16_t len);
typedef err_code_t (*ssd1306_func_i2c_send)(uint8_t *buf_send, uint16_t len);

/**
 * @brief   Handle structure.
 */
typedef struct ssd1306 *ssd1306_handle_t;

/**
 * @brief   Color.
 */
typedef enum {
	SSD1306_COLOR_BLACK = 0,
	SSD1306_COLOR_WHITE,
	SSD1306_COLOR_MAX
} ssd1306_color_t;

/**
 * @brief   Comminication mode.
 */
typedef enum {
	SSD1306_COMM_MODE_I2C = 0,
	SSD1306_COMM_MODE_SPI,
	SSD1306_COMM_MODE_MAX
} ssd1306_comm_mode_t;

/**
 * @brief   Configuration structure.
 */
typedef struct {
	uint16_t  				width;			/*!< Screen width */
	uint16_t 				height;			/*!< Screen height */
	ssd1306_comm_mode_t 	comm_mode;		/*!< Communication mode */
	uint8_t 				inverse;		/*!< Inverse mode */
	ssd1306_func_set_cs 	set_cs;			/*!< Function set CS. Used in SPI mode */
	ssd1306_func_set_dc 	set_dc;			/*!< Function set DC. Used in SPI mode */
	ssd1306_func_set_rst 	set_rst;		/*!< Function set RST. Used in SPI mode */
	ssd1306_func_spi_send 	spi_send;		/*!< Function send SPI data */
	ssd1306_func_i2c_send 	i2c_send;		/*!< Function send I2C data */
} ssd1306_cfg_t;

/*
 * @brief   Initialize SSD1306 with default parameters.
 *
 * @note    This function must be called first.
 *
 * @param   None.
 *
 * @return
 *      - Handle structure: Success.
 *      - Others:           Fail.
 */
ssd1306_handle_t ssd1306_init(void);

/*
 * @brief   Set configuration parameters.
 *
 * @param 	handle Handle structure.
 * @param   config Configuration structure.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_set_config(ssd1306_handle_t handle, ssd1306_cfg_t config);

/*
 * @brief   Configure SSD1306 to run.
 *
 * @param 	handle Handle structure.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_config(ssd1306_handle_t handle);

/*
 * @brief   Refresh screen.
 *
 * @param   handle Handle structure.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_refresh(ssd1306_handle_t handle);

/*
 * @brief   Clear screen.
 *
 * @param   handle Handle structure.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_clear(ssd1306_handle_t handle);

/*
 * @brief   Fill screen.
 *
 * @param   handle Handle structure.
 * @param 	color Color.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_fill(ssd1306_handle_t handle, ssd1306_color_t color);

/*
 * @brief   Write character.
 *
 * @param   handle Handle structure.
 * @param   font_size Font size.
 * @param   chr Character.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_write_char(ssd1306_handle_t handle, font_size_t font_size, uint8_t chr);

/*
 * @brief   Write string.
 *
 * @param   handle Handle structure.
 * @param   font_size Font size.
 * @param   str Pointer references to the data.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_write_string(ssd1306_handle_t handle, font_size_t font_size, uint8_t *str);

/*
 * @brief   Draw pixel.
 *
 * @param   handle Handle structure.
 * @param   x Horizontal position.
 * @param   y Vertical position.
 * @param   color Color.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_draw_pixel(ssd1306_handle_t handle, uint8_t x, uint8_t y, ssd1306_color_t color);

/*
 * @brief   Draw line.
 *
 * @param   handle Handle structure.
 * @param   x1 The first horizontal position.
 * @param   y1 The first vertical postion.
 * @param   x2 The second horizontal position.
 * @param   y2 The second vertical position.
 * @param   color Color.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_draw_line(ssd1306_handle_t handle, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, ssd1306_color_t color);

/*
 * @brief   Draw rectangle.
 *
 * @param   handle Handle structure.
 * @param   x_origin Origin horizontal position.
 * @param   y_origin Origin vertical position.
 * @param   width Width in pixel.
 * @param   height Height in pixel.
 * @param   color Color.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_draw_rectangle(ssd1306_handle_t handle, uint8_t x_origin, uint8_t y_origin, uint8_t width, uint8_t height, ssd1306_color_t color);

/*
 * @brief   Draw circle.
 *
 * @param   handle Handle structure.
 * @param   x_origin Origin horizontal position.
 * @param   y_origin Origin vertical position.
 * @param   radius Radius in pixel.
 * @param   color Color.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_draw_circle(ssd1306_handle_t handle, uint8_t x_origin, uint8_t y_origin, uint8_t radius, ssd1306_color_t color);

/*
 * @brief   Draw bitmap.
 *
 * @param   handle Handle structure.
 * @param   x_origin Origin horizontal position.
 * @param   y_origin Origin vertical position.
 * @param   width Width in pixel.
 * @param   height Height in pixel.
 * @param 	bitmap Bitmap.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_draw_bitmap(ssd1306_handle_t handle, uint8_t x_origin, uint8_t y_origin, uint8_t width, uint8_t height, uint8_t *bitmap);

/*
 * @brief   Set current position.
 *
 * @param   handle Handle structure.
 * @param   x Horizontal position.
 * @param   y Vertical position.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_set_position(ssd1306_handle_t handle, uint8_t x, uint8_t y);

/*
 * @brief   Get current position.
 *
 * @param   handle Handle structure.
 * @param   x Pointer references to the horizontal position.
 * @param   y Pointer references to the vertical position.
 *
 * @return
 *      - ERR_CODE_SUCCESS: Success.
 *      - Others:           Fail.
 */
err_code_t ssd1306_get_position(ssd1306_handle_t handle, uint8_t *x, uint8_t *y);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H__ */