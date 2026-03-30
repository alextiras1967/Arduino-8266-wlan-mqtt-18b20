#include <ESP8266WiFi.h>        
#include <PubSubClient.h>       
#include <OneWire.h>            
#include <DallasTemperature.h>  

// WiFi Einstellungen
const char* ssid = "ssid";           
const char* password = "pass";       

// MQTT Broker Einstellungen (Beispiel – lokaler Broker)
const char* mqtt_server = "ip";      // IP-Adresse des MQTT-Brokers (ersetzen Sie mit Ihrer IP)
const int mqtt_port = 1883;          // Port des MQTT-Brokers
const char* mqtt_user = "user";      // Benutzername für MQTT (falls benötigt)
const char* mqtt_pass = "pass";      // Passwort für MQTT (falls benötigt)

// DS18B20 Einstellungen
#define ONE_WIRE_BUS D2      // GPIO4, D2 auf NodeMCU (Pin für OneWire)
OneWire oneWire(ONE_WIRE_BUS);       // Erstellen eines OneWire-Objekts
DallasTemperature sensors(&oneWire); // Erstellen eines DallasTemperature-Objekts
DeviceAddress sensorAddress;         // Speicher für Adresse des Sensors

WiFiClient espClient;                // Erstellen eines WiFiClient-Objekts
PubSubClient client(espClient);      // Erstellen eines PubSubClient-Objekts

unsigned long lastMeasure = 0;       // Speichert Zeitpunkt der letzten Messung
const long measureInterval = 5000;   // Interval für Messung in Millisekunden (5 Sekunden)

void setup() {
  Serial.begin(115200);              // Serielle Kommunikation mit 115200 Baud

  // Initialisierung des Sensors
  sensors.begin();                   // Starten der Kommunikation mit DS18B20
  if (!sensors.getAddress(sensorAddress, 0)) {  // Prüfen, ob Sensor gefunden wurde
    Serial.println("DS18B20 nicht gefunden! Überprüfen Sie die Verbindung.");
    while (1);  // Programm stoppen, falls Sensor nicht gefunden
  }

  // Verbindung zum WiFi
  WiFi.mode(WIFI_STA);               // Setzt ESP8266 in Client-Modus
  WiFi.begin(ssid, password);        // Verbinden mit WLAN
  Serial.print("Verbinde mit WiFi...");
  while (WiFi.status() != WL_CONNECTED) {  // Warten auf Verbindung
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi verbunden!");

  // MQTT-Client einrichten
  client.setServer(mqtt_server, mqtt_port);  // MQTT-Broker-Adresse und Port festlegen
}

void reconnect() {
  while (!client.connected()) {      // Solange keine Verbindung besteht
    Serial.print("Verbinde mit MQTT...");
    if (client.connect("ESP8266_DS18B20", mqtt_user, mqtt_pass)) {  // Versuche MQTT-Verbindung
      Serial.println("ERFOLG");
    } else {
      Serial.print("FEHLER, Code=");
      Serial.print(client.state());  // Fehlercode ausgeben
      delay(5000);                   // Warte 5 Sekunden vor nächstem Versuch
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();  // Bei Verbindungsabbruch neu verbinden

  // Alle 5 Sekunden Temperatur messen
  if (millis() - lastMeasure > measureInterval) {
    sensors.requestTemperatures();           // Anforderung zur Temperaturmessung
    float tempC = sensors.getTempC(sensorAddress);  // Temperatur lesen (in Celsius)

    if (tempC != DEVICE_DISCONNECTED_C) {    // Überprüfen, ob Temperatur gelesen wurde
      char buffer[16];                       // Puffer für Textdarstellung der Temperatur
      dtostrf(tempC, 0, 2, buffer);          // Konvertiere Temperatur in String mit 2 Nachkommastellen
      String topic = "topic/sensor";         // MQTT-Topic für die Daten
      client.publish(topic.c_str(), buffer); // Sende Temperatur über MQTT
      Serial.print("Temperatur: ");
      Serial.println(buffer);
    } else {
      Serial.println("Fehler beim Lesen der Temperatur!");
    }

    lastMeasure = millis();  // Zeitpunkt der letzten Messung aktualisieren
  }

  client.loop();  // Wichtig: MQTT-Client aktivieren
}