#include <WiFi.h>

// Credenciales de la red
const char* ssid = "-";
const char* password = "-";

// Establecer el número de puerto del servidor web en 80
WiFiServer server(80);

// Variables para almacenar la solicitud HTTP
String header;

// Pines para el sensor ultrasonido
const int trigPin = 12;
const int echoPin = 13;

// Variables para medir la distancia
long duration;
int distance;

// Estado del espacio de estacionamiento
int availableSpaces = 0;

// Tiempo actual y anterior para el temporizador
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  
  // Configurar los pines del sensor ultrasonido
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Conectar a la red Wi-Fi con SSID y contraseña
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Imprimir la dirección IP local e iniciar el servidor web
  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {
  // Medir la distancia utilizando el sensor ultrasonido
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  
  // Determinar si el espacio está ocupado
  if (distance < 400) { // Ajustar el umbral según sea necesario
    availableSpaces = 0; // Espacio ocupado
  } else {
    availableSpaces = 1; // Espacio libre
  }

  WiFiClient client = server.available();   
  if (client) {                             
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("Nuevo Cliente.");
    String currentLine = "";   
    header = "";  // Asegúrate de reiniciar el header antes de usarlo            
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  
      currentTime = millis();
      if (client.available()) {             
        char c = client.read();             
        Serial.write(c);                    
        header += c;
        if (c == '\n') {                    
          if (currentLine.length() == 0) {
            if (header.indexOf("GET /status") >= 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type: application/json");
              client.println("Connection: close");
              client.println();
              client.print("{\"availableSpaces\":");
              client.print(availableSpaces);
              client.println("}");
            } else {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              // Mostrar la página web HTML
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta charset='UTF-8'><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              client.println("<style>");
              client.println("body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f0f0f0; color: #333; }");
              client.println("header { background-color: #4CAF50; color: white; padding: 1em 0; text-align: center; }");
              client.println(".container { padding: 2em; text-align: center; }");
              client.println(".status { font-size: 2em; margin-top: 1em; }");
              client.println("</style></head>");

              // Encabezado de la página web
              client.println("<body>");
              client.println("<header><h1>Monitoreo de Estacionamiento</h1></header>");
              client.println("<div class='container'>");
              client.println("<p class='status'>Espacios disponibles: <span id=\"spaces\">Cargando...</span></p>");
              client.println("<script>");
              client.println("setInterval(function() {");
              client.println("  fetch('/status').then(response => response.json()).then(data => {");
              client.println("    document.getElementById('spaces').innerText = data.availableSpaces;");
              client.println("  });");
              client.println("}, 1000);");
              client.println("</script>");
              client.println("</div>");
              client.println("</body></html>");

              client.println();
            }
            break;
          } else { 
            currentLine = "";
          }
        } else if (c != '\r') {  
          currentLine += c;      
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Cliente desconectado.");
    Serial.println("");
  }
  
  // Imprimir el estado del espacio en la consola serial
  Serial.print("Espacios disponibles: ");
  Serial.println(availableSpaces);
  
  // Esperar un segundo antes de la siguiente lectura
  delay(1000);
}
