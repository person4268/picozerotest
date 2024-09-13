#pragma once
#include <cstdint>
// void sh1106_send_cmd(uint8_t cmd);
// void sh1106_send_cmd_list(uint8_t *buf, int num);
// void sh1106_send_buf(uint8_t buf[], int buflen);
// void sh1106_12864_to_13264_buf(uint8_t *in , uint8_t *out);

/* USER FACING API: ASSUMES REGULAR 128x64 BUFFERS */
void sh1106_init();
void sh1106_render_buf(uint8_t *buf);
void sh1106_blit_data(uint8_t* buf, struct render_area* source_area, uint8_t* data, int dest_col, int dest_page);
void sh1106_set_pixel(uint8_t *buf, int x, int y, bool on);
void sh1106_set_all_white(bool on);

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    // area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
};