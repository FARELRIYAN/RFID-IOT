#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 17
MFRC522 mfrc522(SS_PIN, RST_PIN);

#include <PubSubClient.h>

const char *ssid = "RIYAN";
const char *password = "ryanriyadi";
const char *mqtt_server = "broker.emqx.io";

String nama_tim = "calon_haji";
String rfid = nama_tim + "/esp/rfid";
String is_online = nama_tim + "/esp/is_online";
String onlined = nama_tim + "/esp/onlined";

String getUIDString(MFRC522::Uid uid)
{
  String uidString = "";
  for (byte i = 0; i < uid.size; i++)
  {
    uidString += String(uid.uidByte[i] < 0x10 ? "0" : "");
    uidString += String(uid.uidByte[i], HEX);
  }
  return uidString;
}

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Data masuk untuk ");
  Serial.print(topic);
  Serial.print(". berisi : ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == is_online)
  {
    client.publish(onlined.c_str(), "1");
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(nama_tim.c_str()))
    {
      Serial.println("connected");
      client.subscribe(is_online.c_str());
      client.publish(onlined.c_str(), "1");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void blink(void *pvParameters)
{
    pinMode(LED_BUILTIN, OUTPUT);
    while (1)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void RFID_task(void *pvParameter)
{
  while (1)
  {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
      String uid = getUIDString(mfrc522.uid);
      Serial.print("Kartu RFID terdeteksi, UID: ");
      Serial.println(uid);
      client.publish(rfid.c_str(), uid.c_str());
      mfrc522.PICC_HaltA();
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void MQTT_task(void *pvParameter)
{
  while (1)
  {
    if (!client.connected())
    {
      reconnect();
    }
    client.loop();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  xTaskCreate(&blink, "blink", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(&MQTT_task, "MQTT_task",  4096, NULL, 2, NULL);
  xTaskCreate(&RFID_task, "RFID_task",  4096, NULL, 2, NULL);
}

void loop()
{
}