#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Base64.h>
#include <base64.hpp>

// WiFi-uppgifter
const char* ssid = "iPSK-UMU";
const char* pwd = "HejHopp!!";
WiFiClient wifiClient;
const int LED_OUTPUT_PIN = 32;
const int LED_OUTPUT_PIN_2 = 21; 
const int BUTTON_INPUT_PIN = 14;
const int PIEZO_TRIGGER_PIN = 15; // Pin-nummer för piezo-signalen
const uint16_t PIEZO_FREQUENCY_open = 2000; 
const uint16_t PIEZO_FREQUENCY_knack = 1000; 
const uint16_t PIEZO_DURATION = 10000;  // Varaktighet för piezo-signalen (till exempel 1 sekund)

// MQTT-broker uppgifter
const char* broker = "eu1.cloud.thethings.network";
int port = 1883;
const char* mqtt_usr = "adamslora@ttn";
const char* mqtt_pwd = "NNSXS.O6U2WYOZ6UDCZC7PQZ3OIJLSDWELT2AQTMHAGRQ.EZOPUCRKIWTKI4UGOFW4VE3S3JYNWRU6O3LEBVHQIXLRNY23H6SA";
const char* mqttTopic = "v3/adamslora@ttn/devices/eui-2cf7f12032304992/up";

PubSubClient client(wifiClient);
unsigned long lastMsg = 0;
#define MSG_INTERVAL 10000  // Frekvens av meddelanden (t.ex., 10000 ms = 10 sekunder)

void setup() {
  Serial.begin(115200);

  // Ansluta till WiFi
  Serial.println("Ansluter till WiFi...");
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  Serial.println("\nAnsluten till WiFi");
  pinMode(PIEZO_TRIGGER_PIN, OUTPUT);
  pinMode(LED_OUTPUT_PIN, OUTPUT);
  pinMode(LED_OUTPUT_PIN_2, OUTPUT);

  // setup för mqtt
  client.setServer(broker, port);
  client.setCallback(callback);

  // Ansluta till MQTT
  while (!client.connected()) {
    Serial.println("Ansluter till MQTT...");
    if (client.connect("ESP32Client", mqtt_usr, mqtt_pwd )) {
      Serial.println("MQTT connected");
      client.setBufferSize(4048);
      client.subscribe(mqttTopic);
      Serial.println("MQTT subscribed, waiting for message...");
    }
  else {
    Serial.print("Anslutningen misslyckades, rc=");
    Serial.print(client.state());
    delay(2000);
  }
 }
 
}

void loop() {
  client.loop();

  if (!client.connected()) { 
    reconnect();
  }
  
  if(digitalRead(BUTTON_INPUT_PIN) == 0)
  {
    digitalWrite(LED_OUTPUT_PIN, LOW);    // Stänga av LED
    digitalWrite(LED_OUTPUT_PIN_2, LOW);    // Stänga av LED_2
    noTone(PIEZO_TRIGGER_PIN);
  }
}

//Callback aktiveras efter MQTT-meddelande har tagits emot
void callback(char* topic, byte* message, unsigned int length)  
{
  delay(1000);                   

  // Skapar en temporär variabel för att lagra det mottagna meddelandet

  String messageTemp;

  //Skriver över meddelandet till messageTemp strängen
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  // Skapa ett objekt för med en storlek på 3072 bytes för att lagra JSON-data
  DynamicJsonDocument doc(1024 * 3); // Allocera tillräckligt med minne

  // Deserialisera JSON-meddelandet 
  DeserializationError error = deserializeJson(doc, messageTemp);

  if (error) {
    Serial.print(F("deserializeJson() misslyckades: "));
    Serial.println(error.f_str());
    return;
  }
  // söker efter värdet acossierat med nyckeln uplink message i översta lagret, hitta sedan frm_payload värdet
  const char* payload_base64_const = doc["uplink_message"]["frm_payload"];

  String decodedMessage; 

  if (payload_base64_const) {
    // Konvertera const char* till char* genom att skapa en kopia av data
    int inputLength = strlen(payload_base64_const);
    unsigned char* payload_base64 = new unsigned char[inputLength + 1]; // +1 för null-terminator
    strcpy((char*)payload_base64, payload_base64_const); // Vi behöver använda en cast här

    unsigned char decodedPayload[2 * inputLength]; // Stor nog buffer, för säkerhetens skull

    // Utför avkodningen
    int decodedLength = decode_base64(payload_base64, decodedPayload);

    // Skriv ut den avkodade payloaden
    Serial.print("Dekodad Payload: ");
    for (int i = 0; i < decodedLength; i++) 
    {
        decodedMessage += (char)decodedPayload[i];  // Spara i sträng
        Serial.print((char)decodedPayload[i]);
    }
    Serial.println();

    decodedPayload[0] = 0;
    delete[] payload_base64; // Frigör minnet! 
  } else {
    Serial.println("Ingen payload hittades i meddelandet.");
  }

  Serial.print(decodedMessage);
  if (decodedMessage == "knack") 
  {
    digitalWrite(LED_OUTPUT_PIN, LOW);  
    digitalWrite(LED_OUTPUT_PIN_2, HIGH);   
    tone(PIEZO_TRIGGER_PIN, PIEZO_FREQUENCY_knack); 
    delay(1000);
    noTone(PIEZO_TRIGGER_PIN);   
  } 
  else if (decodedMessage == "open")
  {
    tone(PIEZO_TRIGGER_PIN, PIEZO_FREQUENCY_open);
    digitalWrite(LED_OUTPUT_PIN, HIGH); 
    digitalWrite(LED_OUTPUT_PIN_2, LOW);   
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Försöker ansluta till MQTT...");
    if (client.connect("Adamski", mqtt_usr, mqtt_pwd)) {
      Serial.println("ansluten");
      client.setBufferSize(4096);
      client.subscribe(mqttTopic);
    } else {
      Serial.print("misslyckades, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}
