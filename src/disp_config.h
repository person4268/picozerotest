#pragma once
#define DISP_I2C (&i2c0_inst)
#define DISP_I2C_SDA_PIN 12
#define DISP_I2C_SCL_PIN 13

#define DISP_I2C_ADDR 0x3C
#define DISP_I2C_FREQ 400000

#define DISP_WIDTH 128
#define DISP_HEIGHT 64

#define DISP_PAGE_HEIGHT         _u(8)
#define DISP_NUM_PAGES           (DISP_HEIGHT / DISP_PAGE_HEIGHT)
#define DISP_BUF_LEN             (DISP_NUM_PAGES * DISP_WIDTH)

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
};

inline void calc_render_area_buflen(struct render_area *area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

#define DISP_USE_SSD1306 1

#if DISP_USE_SSD1306
 #define disp_init ssd1306_init
 #define disp_render_buf ssd1306_render_buf
 #define disp_blit_data ssd1306_blit_data
 #define disp_set_pixel ssd1306_set_pixel
 #define disp_set_all_white ssd1306_set_all_white
#else
  #define disp_init sh1106_init
  #define disp_render_buf sh1106_render_buf
  #define disp_blit_data sh1106_blit_data
  #define disp_set_pixel sh1106_set_pixel
  #define disp_set_all_white sh1106_set_all_white
#endif