#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <bh1750.h>
#include <wire.h>
#include <DHTesp.h>


#define WIFI_SSID "Chiko"
#define WIFI_PASSWORD "chikojuga"

#define MQTT_BROKER  "broker.emqx.io"
#define MQTT_TOPIC_PUBLISH_DHT_suhu   "esp32_chiko/data/dht_suhu"
#define MQTT_TOPIC_PUBLISH_DHT_humid   "esp32_chiko/data/dht_humid"
#define MQTT_TOPIC_PUBLISH_LUX   "esp32_chiko/data/lux"
#define MQTT_TOPIC_SUBSCRIBE "esp32_chiko/cmd"  
#define led_hijau 19
#define led_merah 22
#define led_kuning 21

BH1750 cahaya;
DHTesp dht;

char g_szDeviceId[30];
Ticker timerMqtt;
WiFiClient espClient;
PubSubClient mqtt(espClient);
boolean mqttConnect();
void WifiConnect();

void onPublishMessage_cahaya(void * parameters)
{
  for(;;){
    char szLux[50];
    float lux = cahaya.readLightLevel();
    if (lux<400)
    {
      digitalWrite(led_hijau, HIGH);
      digitalWrite(led_merah, LOW);
      sprintf(szLux, "Safe door closed: %f", lux);
    }else
    {
      digitalWrite(led_hijau, LOW);
      digitalWrite(led_merah, HIGH);
      sprintf(szLux, "Warning door opened: %f", lux);
    }
    mqtt.publish(MQTT_TOPIC_PUBLISH_LUX, szLux);
    vTaskDelay(4000/portTICK_PERIOD_MS);
  }
  
}

void onPublishMessage_suhu(void * parameters)
{
  for(;;){
    char szSuhu[50];
    float suhu = dht.getTemperature();
    sprintf(szSuhu, "Suhu: %f", suhu);
    mqtt.publish(MQTT_TOPIC_PUBLISH_DHT_suhu, szSuhu);
    vTaskDelay(5000/portTICK_PERIOD_MS);
  }
  
}

void onPublishMessage_humid(void * parameters)
{
  for(;;){
    char szHumid[50];
    float humid = dht.getHumidity();
    sprintf(szHumid, "Humidity: %f", humid);
    mqtt.publish(MQTT_TOPIC_PUBLISH_DHT_humid, szHumid);
    vTaskDelay(7000/portTICK_PERIOD_MS);
  }
  
}

void mttqloop(void * parameters)
{
  for(;;){
      mqtt.loop();
  }
  
}

void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(led_hijau, OUTPUT);
  pinMode(led_merah, OUTPUT);
  pinMode(led_kuning, OUTPUT);

  WifiConnect();
  mqttConnect();
  dht.setup(5,DHTesp::DHT11);
  Wire.begin(15,18);
  cahaya.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23,&Wire);


  xTaskCreate(onPublishMessage_cahaya, "onPublishMessage_cahaya", 2048, NULL, 1, NULL);
  xTaskCreate(mttqloop, "mttqloop", 2048, NULL, 1, NULL);
  xTaskCreate(onPublishMessage_suhu, "onPublishMessage_suhu", 2048, NULL, 1, NULL);
  xTaskCreate(onPublishMessage_humid, "onPublishMessage_humid", 2048, NULL, 1, NULL);

}

void loop()
{
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {

payload[len] = '\0';
const char* p =  (char *)payload;
Serial.println(p);
Serial.println(len);
if (strcmp(p,"led-on")==0)
{
  digitalWrite(led_kuning, HIGH);
  Serial.println("led on");
}else if (strcmp(p,"led-off")==0)
{
  digitalWrite(led_kuning, LOW);
  Serial.println("led off");

}
}
boolean mqttConnect() {
  sprintf(g_szDeviceId, "esp32_%08X",(uint32_t)ESP.getEfuseMac());
  mqtt.setServer(MQTT_BROKER, 1883);
  mqtt.setCallback(mqttCallback);
  Serial.printf("Connecting to %s clientId: %s\n", MQTT_BROKER, g_szDeviceId);

  boolean fMqttConnected = false;
  for (int i=0; i<3 && !fMqttConnected; i++) {
    Serial.print("Connecting to mqtt broker...");
    fMqttConnected = mqtt.connect(g_szDeviceId);
    if (fMqttConnected == false) {
      Serial.print(" fail, rc=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }

  if (fMqttConnected)
  {
    Serial.println(" success");
    mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
    Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);
    
  }
  return mqtt.connected();
}

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }  
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}