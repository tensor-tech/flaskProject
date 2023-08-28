/*
   2022-03-22
   QQ交流群：566565915
   官网https://bemfa.com

  分辨率默认配置：config.frame_size = FRAMESIZE_UXGA;
  其他配置：
  FRAMESIZE_UXGA （1600 x 1200）
  FRAMESIZE_QVGA （320 x 240）
  FRAMESIZE_CIF （352 x 288）
  FRAMESIZE_VGA （640 x 480）
  FRAMESIZE_SVGA （800 x 600）
  FRAMESIZE_XGA （1024 x 768）
  FRAMESIZE_SXGA （1280 x 1024）

  config.jpeg_quality = 10;（10-63）越小照片质量最好
  数字越小表示质量越高，但是，如果图像质量的数字过低，尤其是在高分辨率时，可能会导致ESP32-CAM崩溃

  支持发布订阅模式，当图片上传时，订阅端会自动获取图片url地址，可做图片识别，人脸识别，图像分析
*/

//需要在arduino IDE软件中---工具-->管理库-->搜索arduinojson并安装
//需要在arduino IDE软件中---工具-->管理库-->搜索arduinojson并安装
//需要在arduino IDE软件中---工具-->管理库-->搜索arduinojson并安装
//建议使用sdk 1.0.6版本，新版2.0.3存在问题

#include "esp_http_client.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoJson.h>
/*********************需要修改的地方**********************/
const char* ssid = "tensor";           //WIFI名称
const char* password = "zhangliang123";     //WIFI密码
int capture_interval = 3 * 1000;      // 默认20秒上传一次，可更改（本项目是自动上传，如需条件触发上传，在需要上传的时候，调用take_send_photo()即可）
/********************************************************/

const char*  post_url = "http://81.68.181.82:5000/imageprocess"; // 默认上传地址
static String httpResponseString;//接收服务器返回信息
bool internet_connected = false;
long current_millis;
long last_capture_millis = 0;

unsigned long previousMillis = 0;//用于计时断网重连
unsigned long interval = 30000; //如果断网，重连时间间隔30秒

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup()
{
  Serial.begin(115200);

  if (init_wifi()) { // Connected to WiFi
    internet_connected = true;
    Serial.println("Internet connected");
  }

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

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}


/********初始化WIFI*********/
bool init_wifi()
{
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  return true;
}

/********http请求处理函数*********/
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  if (evt->event_id == HTTP_EVENT_ON_DATA)
  {
    httpResponseString.concat((char *)evt->data);
  }
  return ESP_OK;
}


/********推送图片*********/
static esp_err_t take_send_photo()
{
  Serial.println("Taking picture...");
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return ESP_FAIL;
  }

  httpResponseString = "";
  esp_http_client_handle_t http_client;
  esp_http_client_config_t config_client = {0};
  config_client.url = post_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;
  http_client = esp_http_client_init(&config_client);
  esp_http_client_set_post_field(http_client, (const char *)fb->buf, fb->len);//设置http发送的内容和长度
  esp_http_client_set_header(http_client, "Content-Type", "image/jpg"); //设置http头部字段

  esp_err_t err = esp_http_client_perform(http_client);//发送http请求
  esp_http_client_cleanup(http_client);
  esp_camera_fb_return(fb);
  return res;
}



void loop()
{
  // TODO check Wifi and reconnect if needed 断网重连
  if ((WiFi.status() != WL_CONNECTED) && (millis() - previousMillis >=interval)) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = millis();
  }

  //定时发送
  current_millis = millis();
  if (current_millis - last_capture_millis > capture_interval) { // Take another picture
    last_capture_millis = millis();
    take_send_photo();
  }
}
