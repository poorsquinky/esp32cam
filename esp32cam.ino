#include "esp_camera.h"
#include <WiFi.h>

#include <WiFiUdp.h>
#include <Syslog.h>

// used only to attempt to shut the bt system off:
#include <esp_bt.h>

#include "private_wifi.h"

// How many seconds to we wait without sending video frames before we do a hardware reset
#define WATCHDOG_TIMEOUT 300

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

// 40,80,160 (full speed for ai_thinker)
#define CPU_SPD_MHZ 160

#include "camera_pins.h"

void startCameraServer();

extern unsigned long frame_count;
extern unsigned long coulombs_pos;
extern unsigned long coulombs_neg;
#define CC_INT_PIN GPIO_NUM_13
#define CC_POL_PIN GPIO_NUM_14

static void IRAM_ATTR coulombInterrupt(void* arg) {
  static boolean polarity;
  polarity = digitalRead(CC_POL_PIN);
  if (polarity) {
    coulombs_neg++;
  } else {
    coulombs_pos++;
  }
}

#define SYSLOG_SERVER "10.1.1.50"
#define SYSLOG_PORT 514
#define DEVICE_HOSTNAME "esptestcam2"
#define APP_NAME "camera"

WiFiUDP udpClient;

Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_HOSTNAME, APP_NAME, LOG_KERN);

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

void log_metrics() {
  syslog.logf("{ \"cpu_temp\": %d, \"signal_strength\": %d, \"frame_count\": %u }", temprature_sens_read(), WiFi.RSSI(), frame_count);
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }

  /*
    //drop down frame size for higher initial frame rate
    s->set_framesize(s, FRAMESIZE_QVGA);
  */

  // ES: my settings
  s->set_framesize(s, FRAMESIZE_VGA);
  // s->set_awb_gain(s, 0);

  // power consumption?  https://www.savjee.be/2019/12/esp32-tips-to-increase-battery-life/
  // setCpuFrequencyMhz(CPU_SPD_MHZ);
  btStop();
  esp_bt_controller_disable();


  /*
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  */

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  char hostname[64];
  byte mac[6];
  char macstr[20]; // 17 really but whatever
  // https://www.arduino.cc/en/Reference/WiFiMACAddress <-- this implies you can't get your mac address without connecting but that sounds like bullshit
  WiFi.macAddress(mac);
  Serial.print("Wifi MAC Address: ");
  snprintf(macstr, sizeof(macstr), "%x:%x:%x:%x:%x:%x", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  Serial.printf("%s\n", macstr);
  if (strcmp(macstr, "impossible") == 0) {
    strncpy(hostname, "esp32-impossible-host", sizeof(hostname));
  } else if (strcmp(macstr, "c4:4f:33:3a:5d:55") == 0) {
    strncpy(hostname, "esp32-cam1", sizeof(hostname));  
  } else if (strcmp(macstr, "c:67:1:ab:f4:98") == 0) {
    strncpy(hostname, "esp32-cam2", sizeof(hostname));
  } else if (strcmp(macstr, "c4:4f:33:3a:5d:55") == 0) {
    strncpy(hostname, "esp32-cam3", sizeof(hostname));
  } else if (strcmp(macstr, "c4:1f:a2:96:2b:c8") == 0) {
    strncpy(hostname, "esp32-cam4", sizeof(hostname));
    // f0:a4:a0:96:2b:c8
    // c4:1f:a2:96:2b:c8
  } else {
    strncpy(hostname, "esp32-cam", sizeof(hostname));
  }

  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // needed because of a bug in the dhcp stack: https://github.com/espressif/arduino-esp32/issues/2537#issuecomment-590029109
  WiFi.setHostname(hostname);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  syslog.logf("Camera started and ready");

  // set up the coulomb counter
    pinMode(CC_INT_PIN,INPUT);
    pinMode(CC_POL_PIN,INPUT);
  /*
    attachInterrupt(CC_INT_PIN,coulombInterrupt,FALLING);
  */

    // https://github.com/espressif/esp-who/issues/90#issuecomment-518142982
    err = gpio_isr_handler_add(CC_INT_PIN, &coulombInterrupt, (void*) CC_INT_PIN);
    if (err != ESP_OK) {
    Serial.printf("handler add failed with error 0x%x \r\n", err);
    }
    err = gpio_set_intr_type(CC_INT_PIN, GPIO_INTR_NEGEDGE);
    if (err != ESP_OK) {
    Serial.printf("set intr type failed with error 0x%x \r\n", err);
    }

}

int watchdog_cycles = 0;

void loop() {
  int prev_frame_count = frame_count;
  // put your main code here, to run repeatedly:
  //  log_metrics();
  delay(10000);

  // The watchdog timer will increment if wifi is not connected or no video frames were sent
  if ((frame_count == prev_frame_count) || (WiFi.status() != WL_CONNECTED)) {
    watchdog_cycles += 1;
  } else {
    watchdog_cycles = 0;
  }
  if (watchdog_cycles * 10 > WATCHDOG_TIMEOUT) {
    ESP.restart();
  }
}
