#define DISP_I2C (&i2c0_inst)
#define DISP_I2C_SDA_PIN 14
#define DISP_I2C_SCL_PIN 15

#define DISP_I2C_ADDR 0x3C
#define DISP_I2C_FREQ 400000

#define DISP_WIDTH 128
#define DISP_HEIGHT 64

#define DISP_PAGE_HEIGHT         _u(8)
#define DISP_NUM_PAGES           (DISP_HEIGHT / DISP_PAGE_HEIGHT)
#define DISP_BUF_LEN             (DISP_NUM_PAGES * DISP_WIDTH)
