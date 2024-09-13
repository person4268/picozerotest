#pragma once
#define sh1106_I2C (&i2c0_inst)
#define sh1106_I2C_SDA_PIN 4
#define sh1106_I2C_SCL_PIN 5

#define sh1106_I2C_ADDR 0x3C
#define sh1106_I2C_FREQ 400000

#define sh1106_WIDTH 132
#define sh1106_ACTUAL_WIDTH 128
#define sh1106_HEIGHT 64

// commands (see datasheet)
#define sh1106_SET_MEM_MODE        _u(0x20)
#define sh1106_SET_COL_ADDR_LOW    _u(0x00)
#define sh1106_SET_COL_ADDR_HIGH   _u(0x10)
#define sh1106_SET_PAGE_ADDR       _u(0xB0)
#define sh1106_SET_HORIZ_SCROLL    _u(0x26)
#define sh1106_SET_SCROLL          _u(0x2E)

#define sh1106_SET_DISP_START_LINE _u(0x40)

#define sh1106_SET_CONTRAST        _u(0x81)
#define sh1106_SET_CHARGE_PUMP     _u(0x8D)

#define sh1106_SET_SEG_REMAP       _u(0xA0)
#define sh1106_SET_ENTIRE_ON       _u(0xA4)
#define sh1106_SET_ALL_ON          _u(0xA5)
#define sh1106_SET_NORM_DISP       _u(0xA6)
#define sh1106_SET_INV_DISP        _u(0xA7)
#define sh1106_SET_MUX_RATIO       _u(0xA8)
#define sh1106_SET_DISP            _u(0xAE)
#define sh1106_SET_COM_OUT_DIR     _u(0xC0)
#define sh1106_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define sh1106_SET_DISP_OFFSET     _u(0xD3)
#define sh1106_SET_DISP_CLK_DIV    _u(0xD5)
#define sh1106_SET_PRECHARGE       _u(0xD9)
#define sh1106_SET_COM_PIN_CFG     _u(0xDA)
#define sh1106_SET_VCOM_DESEL      _u(0xDB)

#define sh1106_PAGE_HEIGHT         _u(8)
#define sh1106_NUM_PAGES           (sh1106_HEIGHT / sh1106_PAGE_HEIGHT)
#define sh1106_BUF_LEN             (sh1106_NUM_PAGES * sh1106_WIDTH)
#define sh1106_ACTUAL_BUF_LEN      (sh1106_NUM_PAGES * sh1106_ACTUAL_WIDTH)

#define sh1106_RMW_MODE           _u(0xE0)
#define sh1106_END_RMW            _u(0xEE)
