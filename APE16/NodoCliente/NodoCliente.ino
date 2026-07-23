#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>

// Credenciales de tu red Wi-Fi
const char* ssid = "TU_RED_WIFI";
const char* password = "TU_CONTRASEÑA";

// ¡IMPORTANTE! Reemplaza con la IP que te dio el Monitor Serial del Servidor
IPAddress server_ip(192, 168, 1, 100); 

WiFiUDP udp;
Coap coap(udp);
unsigned long ultimoEnvio = 0;

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  Serial.print("Conectando al WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡Cliente Conectado!");
  
  coap.start();
}

void loop() {
  coap.loop(); // Necesario para procesar respuestas del servidor

  // Enviar datos cada 5 segundos (5000 ms)
  if (millis() - ultimoEnvio > 5000) {
    ultimoEnvio = millis();

    // 1. Crear la estructura de datos
    StaticJsonDocument<200> doc;
    doc["id_nodo"] = "ESP32_Cliente_01";
    doc["temperatura"] = 24.5;
    doc["humedad"] = 60.2;
    doc["estado"] = "activo";

    // 2. Serialización 
    char payload[256];
    
    // --- MODO JSON (Activo) ---
    size_t payload_size = serializeJson(doc, payload);
    
    // --- MODO MESSAGEPACK (Comenta la línea de arriba y descomenta la de abajo) ---
    // size_t payload_size = serializeMsgPack(doc, payload);

    Serial.print("\nEnviando POST a /sensores... Tamaño: ");
    Serial.print(payload_size);
    Serial.println(" bytes.");

    // 3. Medir tiempo de respuesta y enviar por CoAP
    unsigned long tiempo_inicio = millis();
    
    // Enviar petición POST a través de CoAP
    // Parámetros: IP, puerto CoAP (5683), recurso, tipo, método, token, tokenlen, payload, longitud
    coap.send(server_ip, 5683, "sensores", COAP_CON, COAP_POST, NULL, 0, (uint8_t*)payload, payload_size);
    
    unsigned long tiempo_fin = millis();
    Serial.print("Tiempo estimado de procesamiento de envío: ");
    Serial.print(tiempo_fin - tiempo_inicio);
    Serial.println(" ms");
  }
}