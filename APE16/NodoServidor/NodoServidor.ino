#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

// Credenciales de tu red Wi-Fi
const char* ssid = "TU_RED_WIFI";
const char* password = "TU_CONTRASEÑA";

WiFiUDP udp;
Coap coap(udp);

// Función que se ejecuta cuando llega una petición al endpoint /sensores
void callback_sensores(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("\n[CoAP] Petición POST recibida en /sensores");
  
  // Extraer el payload (la carga útil)
  char payload_str[packet.payloadlen + 1];
  memcpy(payload_str, packet.payload, packet.payloadlen);
  payload_str[packet.payloadlen] = '\0';
  
  Serial.print("Tamaño del mensaje: ");
  Serial.print(packet.payloadlen);
  Serial.println(" bytes.");
  
  Serial.println("Contenido recibido:");
  Serial.println(payload_str);
  Serial.println("-----------------------------------");
  
  // Enviar confirmación al cliente
  coap.sendResponse(ip, port, packet.messageid, "Datos recibidos OK");
}

void setup() {
  Serial.begin(115200);
  
  // Conexión Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando al WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n¡Conectado!");
  Serial.print("Dirección IP del Servidor: ");
  Serial.println(WiFi.localIP());

  // Configurar y arrancar servidor CoAP
  coap.server(callback_sensores, "sensores");
  coap.start();
}

void loop() {
  // Mantener el servidor escuchando
  coap.loop();
}