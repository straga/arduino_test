#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <Wire.h>
#include <TAMC_GT911.h>


#define GFX_BL 2

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40, 41, 39, 42,
    8, 3, 46, 9, 1,
    5, 6, 7, 15, 16, 4,
    45, 48, 47, 21, 14,
    0, 8, 4, 8,
    0, 8, 4, 8,
    1, 16000000);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, rgbpanel);


#define TOUCH_SDA 19
#define TOUCH_SCL 20
#define TOUCH_INT 18
#define TOUCH_RST 38
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, 800, 480);


lv_obj_t * ta;
lv_obj_t * kb;
lv_obj_t * slider_label;


lv_style_t style_big_text;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t * drv, lv_indev_data_t * data) {
  ts.read();
  if (ts.isTouched) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = ts.points[0].x;
    data->point.y = ts.points[0].y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

static void slider_event_cb(lv_event_t * e) {
  lv_obj_t * slider = lv_event_get_target(e);
  int val = lv_slider_get_value(slider);

  char buf[16];
  snprintf(buf, sizeof(buf), "Value: %d", val);
  lv_label_set_text(slider_label, buf);

  uint8_t r = val * 2.55;
  uint8_t g = 255 - r;
  lv_obj_set_style_bg_color(lv_event_get_current_target(e), lv_color_make(r, g, 0), 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(BLACK);

  lv_init();

  static lv_disp_draw_buf_t draw_buf;
  static lv_color_t *disp_draw_buf;

  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * 800 * 480/4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, 800 * 480/4);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 800;
  disp_drv.ver_res = 480;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  ts.begin();
  ts.setRotation(ROTATION_INVERTED);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  lv_style_init(&style_big_text);
  lv_style_set_text_font(&style_big_text, &lv_font_montserrat_20);

  lv_obj_t * tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);
  lv_obj_set_size(tabview, 800, 480);

  lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Keyboard");
  lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Slider");

  ta = lv_textarea_create(tab1);
  lv_obj_set_size(ta, 300, 50);
  lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_add_style(ta, &style_big_text, 0);

  kb = lv_keyboard_create(tab1);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(kb, ta);

  lv_obj_t * slider = lv_slider_create(tab2);
  lv_obj_set_width(slider, 300);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, tab2);

  slider_label = lv_label_create(tab2);
  lv_label_set_text(slider_label, "Value: 0");
  lv_obj_align(slider_label, LV_ALIGN_CENTER, 0, -50);
  lv_obj_add_style(slider_label, &style_big_text, 0);
}

void loop() {
  lv_timer_handler();
  delay(1);
}