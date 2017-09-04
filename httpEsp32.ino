#include <WiFi.h>
#include <time.h>

const char* ssid      = "";
const char* password  = "";
const char* ndpSever  = "pool.ntp.org";
const char* hostname  = "esp32";

WiFiServer server(80);
char linebuf[256];
struct tm timeinfo;
time_t now;

float measure(int pin) {
  const int count = 32;
  float value = 0;
  for(int i = 0; i < count; ++i) {
    value += analogRead(pin);
    delay(1);
  }
  return value/(count*4096)*27.4;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  WiFi.setHostname(hostname);
  WiFi.waitForConnectResult();

  Serial.print("WiFi IP address: ");
  Serial.println(WiFi.localIP());
  configTime(-3600*2, 0, ndpSever);
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if(!client) {
    delay(1000);
    return;
  }

  int bufferPos = 0;
  while(client.connected()) {
    if(!client.available())
      continue;
    char lastChar = client.read();
    if(bufferPos < sizeof(linebuf)-1)
      linebuf[bufferPos++] = lastChar;

    if(lastChar == '\n') {
      linebuf[bufferPos] = 0;
      if(bufferPos <= 2) {
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(linebuf, sizeof(linebuf), "%F %T", &timeinfo);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.print("Time: ");
        client.println(linebuf);
        break;
      }
      bufferPos = 0;
    }
  }
  delay(10);
  client.stop();
}
