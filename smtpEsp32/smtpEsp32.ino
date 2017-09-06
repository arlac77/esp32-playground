#include <WiFi.h>
#include <time.h>
#include <esp_deep_sleep.h>
//#include <soc/rtc.h>

WiFiClient client;
char formatBuffer[256];
struct tm timeinfo;
time_t now;

typedef struct sample {
    float battery_voltage;
    float panel_voltage;
} Sample;

const unsigned int valueSamples = 60;
Sample values[valueSamples];


#include "config.h"

/*
const int   timezone     = -2*3600;
const char* wifiSSID     = "";
const char* wifiPassword = "";
const char* ndpSever     = "pool.ntp.org";
const char* hostname     = "esp32";
const char* domain       = "";
const char* smtpSever    = "";
const char* recipient    = "";
const char* subject      = "";
const char* smtpUsername = "";
const char* smtpPassword = "";
*/

void sendString(const char* message) {
  Serial.print(message);
  client.write(message, strlen(message));
}

void receiveServerResponse() {
  sendString("\r\n");
  client.flush();
  // while(client.read() != '\n');
  while(1) {
    if(!client.available())
      continue;
    char c = client.read();
    Serial.print(c);
    if(c == '\n')
      break;
  }
}

void sendReport() {
  client.connect(smtpSever, 25);
  Serial.print("SMPT Server: ");
  Serial.println(smtpSever);
  while(!client.connected())
    delay(1000);
  Serial.println("connected");

  sendString("EHLO ");
  sendString(hostname);
  sendString(domain);
  receiveServerResponse();
  sendString("AUTH LOGIN");
  receiveServerResponse();
  sendString(smtpUsername);
  receiveServerResponse();
  sendString(smtpPassword);
  receiveServerResponse();

  sendString("MAIL FROM: noreply@");
  sendString(hostname);
  receiveServerResponse();
  sendString("RCPT TO: ");
  sendString(recipient);
  receiveServerResponse();
  sendString("DATA");
  receiveServerResponse();

  sendString("From: noreply@");
  sendString(hostname);
  sendString("\r\nTo: ");
  sendString(recipient);
  sendString("\r\nSubject: ");
  sendString(subject);
  sendString("\r\nDate: ");
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(formatBuffer, sizeof(formatBuffer), "%F %T", &timeinfo);
  sendString(formatBuffer);
  sendString("\r\n");

  for(unsigned int i = 0; i < valueSamples; ++i) {
    sprintf(formatBuffer, "%f %f\n", values[i].battery_voltage, values[i].panel_voltage);
    sendString(formatBuffer);
  }

  sendString("\r\n.");
  receiveServerResponse();
  sendString("QUIT");
  receiveServerResponse();

  client.stop();
}

float measure(int pin) {
  const unsigned int samples = 50;
  float value = 0;
  for(unsigned int i = 0; i < samples; ++i) {
    value += analogRead(pin);
    delay(10);
  }
  return value/(samples*4096)*27.4;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(wifiSSID, wifiPassword);
  WiFi.setHostname(hostname);
  WiFi.waitForConnectResult();
  configTime(timezone, 0, ndpSever);


  for(unsigned int i = 0; i < valueSamples; i += 1) {
    values[i].battery_voltage = measure(34);
    values[i].panel_voltage = measure(35);
  }
  sendReport();


  // TODO: https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/soc/soc/rtc.h
  // rtc_init, rtc_sleep_init, rtc_sleep_set_wakeup_time, RTC_TIMER_TRIG_EN

/*
  struct rtc_sleep_config_t slc = RTC_SLEEP_CONFIG_DEFAULT(RTC_SLEEP_PD_RTC_MEM_FOLLOW_CPU);
  rtc_sleep_init(slc);

  uint64_t n = rtc_time_get();
  rtc_sleep_set_wakeup_time(n + 1000000);
  rtc_sleep_start(RTC_TIMER_TRIG_EN, 0);
  */
}

void loop() {
}


// https://gist.github.com/igrr/54f7fbe0513ac14e1aea3fd7fbecfeab ?

RTC_DATA_ATTR int wake_count;

void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    // Add additional functionality here

    static RTC_RODATA_ATTR const char fmt_str[] = "Wake count %d\n";
    ets_printf(fmt_str, wake_count++);
}


