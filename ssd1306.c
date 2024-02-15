#include "stdlib.h"
#include "ssd1306.h"

#define SSD1306_REG_DATA_ADDR				0x40
#define SSD1306_REG_CMD_ADDR				0x00

#define SSD1306_SET_CONTRAST				0x81 		/*!< 0x81 + 0x00~0xFF Contrast ... reset = 0x7F */
#define SSD1306_DISPLAYALLON_RESUME 		0xA4		/*!< Resume to RAM content display */
#define SSD1306_DISPLAYALLON_IGNORE 		0xA5		/*!< Ignore RAM content display */
#define SSD1306_DISPLAY_NORMAL 				0xA6		/*!< White: 1; Black: 0 */
#define SSD1306_DISPLAY_INVERSE 			0xA7		/*!< White: 0; Black: 1 */
#define SSD1306_DISPLAY_OFF 				0xAE		/*!< Screen OFF */
#define SSD1306_DISPLAY_ON 					0xAF		/*!< Screen ON */

#define SSD1306_SET_MEMORYMODE 				0x20 		/*!< 0x20 + 0x00: horizontal; 0x01: vertical; 0x02: page */
#define SSD1306_SET_MEMORYMODE_HOR 			0x00
#define SSD1306_SET_MEMORYMODE_VER 			0x01
#define SSD1306_SET_MEMORYMODE_PAGE 		0x02

#define SSD1306_SET_COLUMN_ADDR 			0x21  		/*!< 0x21 + 0~127 + 0~127: colum start address + column end address */
#define SSD1306_SET_PAGE_ADDR 				0x22		/*!< 0x22 + 0~7 + 0~7: page start address + page end address */

#define SSD1306_SET_STARTLINE_ZERO 			0x40
#define SSD1306_SET_SEGREMAP_NORMAL  		0xA0
#define SSD1306_SET_SEGREMAP_INV 			0xA1
#define SSD1306_SET_MULTIPLEX 				0XA8
#define SSD1306_COMSCAN_INC 				0xC0
#define SSD1306_COMSCAN_DEC 				0xC8
#define SSD1306_SET_DISPLAYOFFSET 			0xD3
#define SSD1306_SET_COMPINS 				0xDA

#define SSD1306_SET_CLKDIV 					0xD5
#define SSD1306_SET_PRECHARGE 				0xD9
#define SSD1306_SET_COMDESELECT 			0xDB
#define SSD1306_NOP 						0xE3

#define SSD1306_CHARGEPUMP 					0x8D
#define SSD1306_CHARGEPUMP_ON 				0x14
#define SSD1306_CHARGEPUMP_OFF 				0x10

#define NUM_OF_BUF  						2

#define SPI_CS_ACTIVE  						0
#define SPI_CS_UNACTIVE  					1
#define SPI_TIMEOUT_MS 						100

#define I2C_TIMEOUT_MS 						100

typedef err_code_t (*write_cmd_func)(ssd1306_handle_t handle, uint8_t cmd);
typedef err_code_t (*write_data_func)(ssd1306_handle_t handle, uint8_t *data, uint16_t len);

typedef struct ssd1306 {
	uint16_t  				width;					/*!< Screen width */
	uint16_t 				height;					/*!< Screen height */
	ssd1306_comm_mode_t 	comm_mode;				/*!< Communication mode */
	uint8_t 				inverse;				/*!< Inverse mode */
	ssd1306_func_set_cs 	set_cs;					/*!< Function set CS. Used in SPI mode */
	ssd1306_func_set_dc 	set_dc;					/*!< Function set DC. Used in SPI mode */
	ssd1306_func_set_rst 	set_rst;				/*!< Function set RST. Used in SPI mode */
	ssd1306_func_spi_send 	spi_send;				/*!< Function send SPI data */
	ssd1306_func_i2c_send 	i2c_send;				/*!< Function send I2C data */
	write_cmd_func 			write_cmd; 				/*!< Function write command */
	write_data_func  		write_data;				/*!< Function write data */
	uint8_t 				*buf[NUM_OF_BUF];		/*!< Data buffer */
	uint32_t 				buf_len;				/*!< Buffer length */
	uint8_t					buf_idx;				/*!< Buffer index */
	uint16_t 				pos_x;					/*!< Position x */
	uint16_t  				pos_y;					/*!< Position y */
} ssd1306_t;

static void draw_pixel(ssd1306_handle_t handle, uint8_t x, uint8_t y, ssd1306_color_t color)
{
	if (handle->inverse) {
		if (color == SSD1306_COLOR_WHITE) {
			handle->buf[handle->buf_idx][x + (y / 8)*handle->width] &= ~ (1 << (y % 8));
		} else {
			handle->buf[handle->buf_idx][x + (y / 8)*handle->width] |= (1 << (y % 8));
		}
	} else {
		if (color == SSD1306_COLOR_WHITE) {
			handle->buf[handle->buf_idx][x + (y / 8)*handle->width] |= (1 << (y % 8));
		} else {
			handle->buf[handle->buf_idx][x + (y / 8)*handle->width] &= ~ (1 << (y % 8));
		}
	}
}

static void draw_line(ssd1306_handle_t handle, uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end, ssd1306_color_t color)
{
	int32_t deltaX = abs(x_end - x_start);
	int32_t deltaY = abs(y_end - y_start);
	int32_t signX = ((x_start < x_end) ? 1 : -1);
	int32_t signY = ((y_start < y_end) ? 1 : -1);
	int32_t error = deltaX - deltaY;
	int32_t error2;

	if (handle->inverse) {
		if (color == SSD1306_COLOR_WHITE) {
			handle->buf[handle->buf_idx][x_end + (y_end / 8)*handle->width] &= ~ (1 << (y_end % 8));
		} else {
			handle->buf[handle->buf_idx][x_end + (y_end / 8)*handle->width] |= (1 << (y_end % 8));
		}
	} else {
		if (color == SSD1306_COLOR_WHITE) {
			handle->buf[handle->buf_idx][x_end + (y_end / 8)*handle->width] |= (1 << (y_end % 8));
		} else {
			handle->buf[handle->buf_idx][x_end + (y_end / 8)*handle->width] &= ~ (1 << (y_end % 8));
		}
	}

	while ((x_start != x_end) || (y_start != y_end))
	{
		if (handle->inverse) {
			if (color == SSD1306_COLOR_WHITE) {
				handle->buf[handle->buf_idx][x_start + (y_start / 8)*handle->width] &= ~ (1 << (y_start % 8));
			} else {
				handle->buf[handle->buf_idx][x_start + (y_start / 8)*handle->width] |= (1 << (y_start % 8));
			}
		} else {
			if (color == SSD1306_COLOR_WHITE) {
				handle->buf[handle->buf_idx][x_start + (y_start / 8)*handle->width] |= (1 << (y_start % 8));
			} else {
				handle->buf[handle->buf_idx][x_start + (y_start / 8)*handle->width] &= ~ (1 << (y_start % 8));
			}
		}

		error2 = error * 2;
		if (error2 > -deltaY)
		{
			error -= deltaY;
			x_start += signX;
		}
		else
		{
			/*nothing to do*/
		}

		if (error2 < deltaX)
		{
			error += deltaX;
			y_start += signY;
		}
		else
		{
			/*nothing to do*/
		}
	}
}

static err_code_t ssd1306_spi_write_cmd(ssd1306_handle_t handle, uint8_t cmd)
{
	handle->set_cs(SPI_CS_ACTIVE);
	handle->set_dc(0);
	handle->spi_send(&cmd, 1, SPI_TIMEOUT_MS);
	handle->set_cs(SPI_CS_UNACTIVE);

	return ERR_CODE_SUCCESS;
}

static err_code_t ssd1306_spi_write_data(ssd1306_handle_t handle, uint8_t *data, uint16_t len)
{
	handle->set_cs(SPI_CS_ACTIVE);
	handle->set_dc(1);
	handle->spi_send(data, len, SPI_TIMEOUT_MS);
	handle->set_cs(SPI_CS_UNACTIVE);

	return ERR_CODE_SUCCESS;
}

static err_code_t ssd1306_i2c_write_cmd(ssd1306_handle_t handle, uint8_t cmd)
{
	uint8_t buf[2];

	buf[0] = SSD1306_REG_CMD_ADDR;
	buf[1] = cmd;
	handle->i2c_send(buf, 2, I2C_TIMEOUT_MS);

	return ERR_CODE_SUCCESS;
}

static err_code_t ssd1306_i2c_write_data(ssd1306_handle_t handle, uint8_t *data, uint16_t len)
{
	uint8_t buf[len + 1];

	buf[0] = SSD1306_REG_DATA_ADDR;
	memcpy(&buf[1], data, len);
	handle->i2c_send(buf, len + 1, I2C_TIMEOUT_MS);

	return ERR_CODE_SUCCESS;
}

ssd1306_handle_t ssd1306_init(void)
{
	ssd1306_handle_t handle = calloc(1, sizeof(ssd1306_t));
	if (handle == NULL)
	{
		return NULL;
	}

	return handle;
}

err_code_t ssd1306_set_config(ssd1306_handle_t handle, ssd1306_cfg_t config)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	write_cmd_func write_cmd;
	write_data_func write_data;

	if (config.comm_mode == SSD1306_COMM_MODE_I2C)
	{
		write_cmd = ssd1306_i2c_write_cmd;
		write_data = ssd1306_i2c_write_data;
	}
	else
	{
		write_cmd = ssd1306_spi_write_cmd;
		write_data = ssd1306_spi_write_data;
	}

	handle->width = config.width;
	handle->height = config.height;
	handle->comm_mode = config.comm_mode;
	handle->inverse = config.inverse;
	handle->set_cs = config.set_cs;
	handle->set_dc = config.set_dc;
	handle->set_rst = config.set_rst;
	handle->spi_send = config.spi_send;
	handle->i2c_send = config.i2c_send;
	handle->write_cmd = write_cmd;
	handle->write_data = write_data;
	handle->buf_len = config.width * config.height / 8;
	handle->buf_idx = 0;
	handle->pos_x = 0;
	handle->pos_y = 0;

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_config(ssd1306_handle_t handle)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	for (uint8_t i = 0; i < NUM_OF_BUF; i++)
	{
		handle->buf[i] = calloc(handle->buf_len, sizeof(uint8_t));
	}

	handle->write_cmd(handle, SSD1306_DISPLAY_OFF);
	handle->write_cmd(handle, SSD1306_SET_MEMORYMODE);
	handle->write_cmd(handle, SSD1306_SET_MEMORYMODE_HOR);
	handle->write_cmd(handle, SSD1306_COMSCAN_DEC);
	handle->write_cmd(handle, 0x00);
	handle->write_cmd(handle, 0x10);
	handle->write_cmd(handle, SSD1306_SET_STARTLINE_ZERO);
	handle->write_cmd(handle, SSD1306_SET_SEGREMAP_INV);
	handle->write_cmd(handle, handle->inverse == 0 ? SSD1306_DISPLAY_NORMAL : SSD1306_DISPLAY_INVERSE);
	handle->write_cmd(handle, 0xFF);
	handle->write_cmd(handle, handle->width == 32 ? 0x1F : 0x3F );
	handle->write_cmd(handle, SSD1306_DISPLAYALLON_RESUME);
	handle->write_cmd(handle, SSD1306_SET_DISPLAYOFFSET);
	handle->write_cmd(handle, 0x00);
	handle->write_cmd(handle, SSD1306_SET_CLKDIV);
	handle->write_cmd(handle, 0xF0);
	handle->write_cmd(handle, SSD1306_SET_PRECHARGE);
	handle->write_cmd(handle, 0x22);
	handle->write_cmd(handle, SSD1306_SET_COMPINS);
	handle->write_cmd(handle, handle->width == 32 ? 0x02 : 0x12);
	handle->write_cmd(handle, SSD1306_SET_COMDESELECT);
	handle->write_cmd(handle, 0x20);
	handle->write_cmd(handle, SSD1306_CHARGEPUMP);
	handle->write_cmd(handle, SSD1306_CHARGEPUMP_ON);
	handle->write_cmd(handle, SSD1306_DISPLAY_ON);

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_refresh(ssd1306_handle_t handle)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	for (uint8_t i = 0; i < (handle->height / 8); i++)
	{
		handle->write_cmd(handle, 0xB0 + i);
		handle->write_cmd(handle, 0x00);
		handle->write_cmd(handle, 0x10);
		handle->write_data(handle, &handle->buf[handle->buf_idx][i * handle->width], handle->width);
	}

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_clear(ssd1306_handle_t handle)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	handle->buf_idx ^= 1;
	for (uint32_t i = 0; i < (handle->width * handle->height / 8); i++) {
		handle->buf[handle->buf_idx][i] = 0x00;
	}

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_fill(ssd1306_handle_t handle, ssd1306_color_t color)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	handle->buf_idx ^= 1;
	for (uint32_t i = 0; i < (handle->width * handle->height / 8); i++) {
		handle->buf[handle->buf_idx][i] = handle->inverse == 0 ?
		                                  ((color == SSD1306_COLOR_WHITE) ? 0xFF : 0x00) :
		                                  ((color == SSD1306_COLOR_WHITE) ? 0x00 : 0xFF);
	}

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_write_char(ssd1306_handle_t handle, font_size_t font_size, uint8_t chr)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	font_t font;
	get_font(chr, font_size, &font);

	handle->buf_idx ^= 1;
	memcpy(handle->buf[handle->buf_idx], handle->buf[handle->buf_idx ^ 1], handle->buf_len);

	uint8_t num_byte_per_row = font.data_len / font.height;

	for (uint8_t height_idx = 0; height_idx < font.height; height_idx ++) {
		for ( uint8_t byte_idx = 0; byte_idx < num_byte_per_row; byte_idx++) {
			for (uint8_t width_idx = 0; width_idx < 8; width_idx++) {
				uint8_t x = handle->pos_x + width_idx + byte_idx * 8;
				uint8_t y = handle->pos_y + height_idx;

				if (((font.data[height_idx * num_byte_per_row + byte_idx] << width_idx) & 0x80) == 0x80) {
					handle->buf[handle->buf_idx][x + (y / 8)*handle->width] |= (1 << (y % 8));
				} else {
					handle->buf[handle->buf_idx][x + (y / 8)*handle->width] &= ~ (1 << (y % 8));
				}
			}
		}
	}

	handle->pos_x += font.width + num_byte_per_row;

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_write_string(ssd1306_handle_t handle, font_size_t font_size, uint8_t *str)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	handle->buf_idx ^= 1;
	memcpy(handle->buf[handle->buf_idx], handle->buf[handle->buf_idx ^ 1], handle->buf_len);

	uint8_t pos_x = handle->pos_x;
	uint8_t pos_y = handle->pos_y;

	while (*str) {
		font_t font;
		get_font(*str, font_size, &font);

		uint8_t num_byte_per_row = font.data_len / font.height;

		for (uint8_t height_idx = 0; height_idx < font.height; height_idx ++) {
			for ( uint8_t byte_idx = 0; byte_idx < num_byte_per_row; byte_idx++) {
				for (uint8_t width_idx = 0; width_idx < 8; width_idx++) {
					uint8_t x = pos_x + width_idx + byte_idx * 8;
					uint8_t y = pos_y + height_idx;

					if (((font.data[height_idx * num_byte_per_row + byte_idx] << width_idx) & 0x80) == 0x80) {
						handle->buf[handle->buf_idx][x + (y / 8)*handle->width] |= (1 << (y % 8));
					} else {
						handle->buf[handle->buf_idx][x + (y / 8)*handle->width] &= ~ (1 << (y % 8));
					}
				}
			}
		}
		pos_x += font.width + num_byte_per_row;
		str++;
	}

	handle->pos_x = pos_x;

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_draw_pixel(ssd1306_handle_t handle, uint8_t x, uint8_t y, ssd1306_color_t color)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	handle->buf_idx ^= 1;
	memcpy(handle->buf[handle->buf_idx], handle->buf[handle->buf_idx ^ 1], handle->buf_len);

	draw_pixel(handle, x, y, color);

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_draw_line(ssd1306_handle_t handle, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, ssd1306_color_t color)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	handle->buf_idx ^= 1;
	memcpy(handle->buf[handle->buf_idx], handle->buf[handle->buf_idx ^ 1], handle->buf_len);

	draw_line(handle, x1, y1, x2, y2, color);

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_draw_rectangle(ssd1306_handle_t handle, uint8_t x_origin, uint8_t y_origin, uint8_t width, uint8_t height, ssd1306_color_t color)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	handle->buf_idx ^= 1;
	memcpy(handle->buf[handle->buf_idx], handle->buf[handle->buf_idx ^ 1], handle->buf_len);

	draw_line(handle, x_origin, y_origin, x_origin + width, y_origin, color);
	draw_line(handle, x_origin + width, y_origin, x_origin + width, y_origin + height, color);
	draw_line(handle, x_origin + width, y_origin + height, x_origin, y_origin + height, color);
	draw_line(handle, x_origin, y_origin + height, x_origin, y_origin, color);

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_draw_circle(ssd1306_handle_t handle, uint8_t x_origin, uint8_t y_origin, uint8_t radius, ssd1306_color_t color)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	handle->buf_idx ^= 1;
	memcpy(handle->buf[handle->buf_idx], handle->buf[handle->buf_idx ^ 1], handle->buf_len);

	int32_t x = -radius;
	int32_t y = 0;
	int32_t err = 2 - 2 * radius;
	int32_t e2;

	do {
		draw_pixel(handle, x_origin - x, y_origin + y, color);
		draw_pixel(handle, x_origin + x, y_origin + y, color);
		draw_pixel(handle, x_origin + x, y_origin - y, color);
		draw_pixel(handle, x_origin - x, y_origin - y, color);

		e2 = err;
		if (e2 <= y) {
			y++;
			err = err + (y * 2 + 1);
			if (-x == y && e2 <= x) {
				e2 = 0;
			}
			else {
				/*nothing to do*/
			}
		} else {
			/*nothing to do*/
		}

		if (e2 > x) {
			x++;
			err = err + (x * 2 + 1);
		} else {
			/*nothing to do*/
		}
	} while (x <= 0);

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_set_position(ssd1306_handle_t handle, uint8_t x, uint8_t y)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	handle->pos_x = x;
	handle->pos_y = y;

	return ERR_CODE_SUCCESS;
}

err_code_t ssd1306_get_position(ssd1306_handle_t handle, uint8_t *x, uint8_t *y)
{
	/* Check if handle structure is NULL */
	if (handle == NULL)
	{
		return ERR_CODE_NULL_PTR;
	}

	*x = handle->pos_x;
	*y = handle->pos_y;

	return ERR_CODE_SUCCESS;
}
