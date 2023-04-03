#include "H7_Video.h"

#include "dsi.h"
#include "st7701.h"
#include "SDRAM.h"
#include "video_modes.h"
#include "anx7625.h"

#if __has_include ("lvgl.h")
#include "lvgl.h"

void lvgl_displayFlushing(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p);
#endif

H7_Video::H7_Video(int width, int heigth, DisplayShieldModel shield) :
  ArduinoGraphics(width, heigth) {
    _shield = shield;
}

H7_Video::~H7_Video() {
}

int H7_Video::begin() {
    int ret = 0;

    ret = begin(true);

    return ret;
}

int H7_Video::begin(bool landscape) {
  if (!ArduinoGraphics::begin()) {
    return 0;
  }

  textFont(Font_5x7);
  
  #if defined(ARDUINO_PORTENTA_H7_M7)
    if (_shield == NONE_SHIELD) {
        struct edid recognized_edid;
        int err_code = 0;

        //Initialization of ANX7625
        err_code = anx7625_init(0);
        if(err_code < 0) {
          return err_code;
        }

        //Checking HDMI plug event
        anx7625_wait_hpd_event(0);

        //Read EDID
        anx7625_dp_get_edid(0, &recognized_edid);

        //DSI Configuration
        anx7625_dp_start(0, &recognized_edid, EDID_MODE_720x480_60Hz);

        //Configure SDRAM 
        SDRAM.begin(getFramebufferEnd());
    } else if (_shield == GIGA_DISPLAY_SHIELD) {
        #define EDID_MODE_SELECTED  EDID_MODE_480x800_60Hz
        struct edid _edid;
        struct display_timing dt;

        //DSI Configuration
        dt.pixelclock   = envie_known_modes[EDID_MODE_SELECTED].pixel_clock;
        dt.hactive      = envie_known_modes[EDID_MODE_SELECTED].hactive;
        dt.hsync_len    = envie_known_modes[EDID_MODE_SELECTED].hsync_len;
        dt.hback_porch  = envie_known_modes[EDID_MODE_SELECTED].hback_porch;
        dt.hfront_porch = envie_known_modes[EDID_MODE_SELECTED].hfront_porch;
        dt.vactive      = envie_known_modes[EDID_MODE_SELECTED].vactive;
        dt.vsync_len    = envie_known_modes[EDID_MODE_SELECTED].vsync_len;
        dt.vback_porch  = envie_known_modes[EDID_MODE_SELECTED].vback_porch;
        dt.vfront_porch = envie_known_modes[EDID_MODE_SELECTED].vfront_porch;
        dt.hpol         = envie_known_modes[EDID_MODE_SELECTED].hpol;
        dt.vpol         = envie_known_modes[EDID_MODE_SELECTED].vpol;
        stm32_dsi_config(0, &_edid, &dt);

        //Configure SDRAM 
        SDRAM.begin();

        //Init LCD Controller
        st7701_init();
    }
  #elif defined(ARDUINO_GIGA)
    #define EDID_MODE_SELECTED  EDID_MODE_480x800_60Hz
    struct edid _edid;
    struct display_timing dt;

    //DSI Configuration
    dt.pixelclock   = envie_known_modes[EDID_MODE_SELECTED].pixel_clock;
    dt.hactive      = envie_known_modes[EDID_MODE_SELECTED].hactive;
    dt.hsync_len    = envie_known_modes[EDID_MODE_SELECTED].hsync_len;
    dt.hback_porch  = envie_known_modes[EDID_MODE_SELECTED].hback_porch;
    dt.hfront_porch = envie_known_modes[EDID_MODE_SELECTED].hfront_porch;
    dt.vactive      = envie_known_modes[EDID_MODE_SELECTED].vactive;
    dt.vsync_len    = envie_known_modes[EDID_MODE_SELECTED].vsync_len;
    dt.vback_porch  = envie_known_modes[EDID_MODE_SELECTED].vback_porch;
    dt.vfront_porch = envie_known_modes[EDID_MODE_SELECTED].vfront_porch;
    dt.hpol         = envie_known_modes[EDID_MODE_SELECTED].hpol;
    dt.vpol         = envie_known_modes[EDID_MODE_SELECTED].vpol;
    stm32_dsi_config(0, &_edid, &dt);

    //Configure SDRAM 
    SDRAM.begin();

    //Init LCD Controller
    st7701_init();
  #else 
    #error Board not compatible with this library
  #endif

  _landscape = landscape;

  stm32_LCD_Clear(0); 

  #if __has_include("lvgl.h")
    getNextFrameBuffer();

    /* Initiliaze LVGL library */
    lv_init();

    /* Create a draw buffer */
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t * buf1;                                           
    buf1 = (lv_color_t*)malloc((width() * height() / 10) * sizeof(lv_color_t)); /* Declare a buffer for 1/10 screen size */
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, width() * height() / 10);      /* Initialize the display buffer. */

    /* Initialize display features for LVGL library */
    static lv_disp_drv_t disp_drv;          /* Descriptor of a display driver */
    lv_disp_drv_init(&disp_drv);            /* Basic initialization */
    disp_drv.flush_cb = lvgl_displayFlushing;  /* Set your driver function */
    disp_drv.draw_buf = &draw_buf;          /* Assign the buffer to the display */
    disp_drv.hor_res = width();       /* Set the horizontal resolution of the display */
    disp_drv.ver_res = height();      /* Set the vertical resolution of the display */
    disp_drv.rotated = (_landscape) ? LV_DISP_ROT_270 : LV_DISP_ROT_NONE;
    disp_drv.sw_rotate = 1;
    lv_disp_drv_register(&disp_drv);        /* Finally register the driver */
  #endif

  return 1;
}

void H7_Video::end() {
  ArduinoGraphics::end();
}

void H7_Video::beginDraw() {
  ArduinoGraphics::beginDraw();

  #if __has_include("lvgl.h")
    getNextFrameBuffer();
  #endif

  stm32_LCD_Clear(0); 
}

void H7_Video::endDraw() {
  ArduinoGraphics::endDraw();

  getNextFrameBuffer();
}

void H7_Video::clear(){
  uint32_t bg = ArduinoGraphics::background();
  stm32_LCD_Clear(bg);
}

void H7_Video::set(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    uint32_t x_rot, y_rot;

    if (_landscape) {
      x_rot = ((width()-1) - y);
      y_rot = x;
    } else {
      x_rot = x;
      y_rot = y;
    }

    uint32_t color =  (uint32_t)((uint32_t)(r << 16) | (uint32_t)(g << 8) | (uint32_t)(b << 0));
    stm32_LCD_FillArea((void *)(getCurrentFrameBuffer() + ((x_rot + (width() * y_rot)) * sizeof(uint16_t))), 1, 1, color);
}

#if __has_include("lvgl.h")
void lvgl_displayFlushing(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p) {
    uint32_t width      = lv_area_get_width(area);
    uint32_t height     = lv_area_get_height(area);
    uint32_t offsetPos  = (area->x1 + (disp->hor_res * area->y1)) * sizeof(uint16_t);

    stm32_LCD_DrawImage((void *) color_p, (void *)(getCurrentFrameBuffer() + offsetPos), width, height, DMA2D_INPUT_RGB565);
    lv_disp_flush_ready(disp);         /* Indicate you are ready with the flushing*/
}

void H7_Video::attachLVGLTouchCb(void (*touch_cb)(void*,void*)) {
  static lv_indev_drv_t indev_drv;                                                /* Descriptor of a input device driver */
  lv_indev_drv_init(&indev_drv);                                                  /* Basic initialization */
  indev_drv.type = LV_INDEV_TYPE_POINTER;                                         /* Touch pad is a pointer-like device */
  indev_drv.read_cb = (void(*)(lv_indev_drv_t *, lv_indev_data_t *))(touch_cb);   /* Set your driver function */
  lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);                      /* Register the driver in LVGL and save the created input device object */
}
#endif