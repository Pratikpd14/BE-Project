/*  Project: Sensor based Fire Smoke detection System.
 *  Last modified : 24-05-2024
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include "base64.h"

#define LED_BUILTIN 4   
#define    

#define in1 12
#define in2 13
#define in3 15
#define in4 14

const int halfStepSequence[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

// Pin definition for CAMERA_MODEL_AI_THINKER
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

const char *ssid = "ESP32_AP";
const char *password = "12345678";

WiFiServer server(80);
int i=0;
int rot = 0;
int opt;
int ind;
void stepMotor(int stepIndex){
  digitalWrite(in1, halfStepSequence[stepIndex][0]);
  digitalWrite(in2, halfStepSequence[stepIndex][1]);
  digitalWrite(in3, halfStepSequence[stepIndex][2]);
  digitalWrite(in4, halfStepSequence[stepIndex][3]);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MQ2_SENSOR_PIN, INPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  WiFi.softAP(ssid, password);

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
  IPAddress myIP = WiFi.softAPIP();
  server.begin();
}

void loop() {
  if (i==8){
    i = 0;
  }
  ind = i;
  if (rot>2448){
    ind = 7-i;
    if(rot == 4896){
      rot = 0;
    }
  }
  if((rot>371 and rot<2077) or (rot>2819 and rot<4525)){
    stepMotor(ind);
    delay(2);   
  }else{
    stepMotor(ind);
    delay(100);       
  }
  rot+=1;
  i+=1;
  
  Serial.println(i);
  delay(2);
  WiFiClient client = server.available();   

  if (client) {                             
    String currentLine = "";                
    while (client.connected()) {            
      if (client.available()) {                     
        char c = client.read();             
        if (c == '\n') {                    
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            int sensorValue = digitalRead(MQ2_SENSOR_PIN);
            client.print(sensorValue);          
            client.println("<br>");

            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
              String encodedImage = "Camera capture failed";
              return;
            }
            String encodedImage = base64Encode(fb->buf, fb->len);
            client.print(encodedImage);
            esp_camera_fb_return(fb);
            
            break;
          } else {    
            currentLine = "";
          }
        } else if (c != '\r') {  
          currentLine += c;      
        }
      }
    }
    client.stop();
  }
}

String base64Encode(uint8_t *data, size_t len) {
  return base64::encode(data, len);
}
