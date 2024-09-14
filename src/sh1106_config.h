#pragma once


#define SH1106_WIDTH 132
#define SH1106_HEIGHT 64

// One weird display I have needs have the left and right sides swapped.
// I don't know if that's an issue with my particular display or if it's all SH1106es,
// or if I messed something up.
#define SH1106_MIDDLE_FLIP 1
// idk in case your display is upside down
#define SH1106_VERTICAL_FLIP 1 

// commands (see datasheet)
// mostly taken from the ssd1306 datasheet with the completely wrong ones changed
#define SH1106_SET_MEM_MODE        _u(0x20)
#define SH1106_SET_COL_ADDR_LOW    _u(0x00)
#define SH1106_SET_COL_ADDR_HIGH   _u(0x10)
#define SH1106_SET_PAGE_ADDR       _u(0xB0)
#define SH1106_SET_HORIZ_SCROLL    _u(0x26)
#define SH1106_SET_SCROLL          _u(0x2E)

#define SH1106_SET_DISP_START_LINE _u(0x40)

#define SH1106_SET_CONTRAST        _u(0x81)
#define SH1106_SET_CHARGE_PUMP     _u(0x8D)

#define SH1106_SET_SEG_REMAP       _u(0xA0)
#define SH1106_SET_ENTIRE_ON       _u(0xA4)
#define SH1106_SET_ALL_ON          _u(0xA5)
#define SH1106_SET_NORM_DISP       _u(0xA6)
#define SH1106_SET_INV_DISP        _u(0xA7)
#define SH1106_SET_MUX_RATIO       _u(0xA8)
#define SH1106_SET_DISP            _u(0xAE)
#define SH1106_SET_COM_OUT_DIR     _u(0xC0)
#define SH1106_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SH1106_SET_DISP_OFFSET     _u(0xD3)
#define SH1106_SET_DISP_CLK_DIV    _u(0xD5)
#define SH1106_SET_PRECHARGE       _u(0xD9)
#define SH1106_SET_COM_PIN_CFG     _u(0xDA)
#define SH1106_SET_VCOM_DESEL      _u(0xDB)

#define SH1106_PAGE_HEIGHT         _u(8)
#define SH1106_NUM_PAGES           (SH1106_HEIGHT / SH1106_PAGE_HEIGHT)
#define SH1106_BUF_LEN             (SH1106_NUM_PAGES * SH1106_WIDTH)

#define SH1106_RMW_MODE           _u(0xE0)
#define SH1106_END_RMW            _u(0xEE)
