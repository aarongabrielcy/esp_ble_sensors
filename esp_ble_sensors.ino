#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <HTTPClient.h>

int scanTime = 5;  // En segundos
BLEScan *pBLEScan;

String sensor_temp_1 = "";
const char* targetMAC[] = {
  "7c:d9:f4:10:b2:23",
  "7c:d9:f4:14:bb:24",
  "7c:d9:f4:11:77:86",
  "7c:d9:f4:10:31:68",
  "7c:d9:f4:13:9c:df"
};
const int targetMACCount = sizeof(targetMAC) / sizeof(targetMAC[0]);
const char* ssid = "INFINITUM471D";
const char* password = "aQyq73cYxk";
const char* serverUrl = "http://192.168.1.73:8081/endpoint"; //casa
String chipIdStr;
const char* head = "IOT";

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    for (int i = 0; i < targetMACCount; i++) {
      // Verificar si la dirección MAC coincide
      if (strcmp(advertisedDevice.getAddress().toString().c_str(), targetMAC[i]) == 0) {
        // Mostrar los datos del fabricante
        Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

        // Obtener los datos del fabricante como un String
        String manufacturerData = advertisedDevice.getManufacturerData();

        if (manufacturerData.length() > 0) {
          // Convertir los datos a hexadecimal
          String tempData = "";
          for (size_t j = 0; j < manufacturerData.length(); j++) {
            char buffer[3];
            sprintf(buffer, "%02X", (unsigned char)manufacturerData[j]);
            tempData += buffer;
          }

          // Convertir la dirección MAC al formato sin dos puntos
          String macAddressFormatted = advertisedDevice.getAddress().toString().c_str();
          macAddressFormatted.replace(":", "");

          // Agregar la dirección MAC y los datos del sensor
          String dataToSend = String(head) + ";" + chipIdStr +";" + macAddressFormatted + ";" + tempData;
          sensor_temp_1 = dataToSend;
          Serial.println(sensor_temp_1);

          // Enviar los datos al servidor
          sendDataToServer(sensor_temp_1);
        }
      }
    }
  }

  void sendDataToServer(const String& data) {
    if(WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl); // Especifica la URL

      // Enviar el dato en formato JSON
      http.addHeader("Content-Type", "application/json");
      String payload = "{\"sensor_data\": \"" + data + "\"}";
      int httpResponseCode = http.POST(payload);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Response code: " + String(httpResponseCode));
        Serial.println("Response: " + response);
      } else {
        Serial.println("Error sending POST request");
        Serial.println("Error code: " + String(httpResponseCode));
      }

      http.end(); // Liberar recursos
    } else {
      Serial.println("WiFi not connected");
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  uint64_t chipId = ESP.getEfuseMac();  // Obtén la dirección MAC
  chipIdStr = String((uint16_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
  
  // Configura la conexión Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.println("IP Address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }

  // Configura el escaneo BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  // Ejecutar el escaneo
  BLEScanResults* foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices->getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults();  // delete results from BLEScan buffer to release memory
  delay(2000);  // Esperar 2 segundos antes de la siguiente exploración
}
