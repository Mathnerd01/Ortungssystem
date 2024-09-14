  #include <Arduino.h>
  #include <WiFi.h>
  #include "esp_wifi.h"
  #include "esp_system.h"
  #include "esp_event.h"
  #include "nvs_flash.h"
  #include <ArduinoJson.h>
  #include <HTTPClient.h>


  //===== SETTINGS =====//
  #define CHANNEL 1
  #define CHANNEL_HOPPING true
  #define MAX_CHANNEL 11
  #define HOP_INTERVAL 214 // in ms
  #define MAX_PACKETS 50 // Maximum number of packets to store

  //===== WIFI =====//
  #define WIFI_NETWORK "Fingerprint1"
  #define WIFI_PASSWORD "Fingerprint"
  #define WIFI_TIMEOUT_MS 20000

  //===== Run-Time Variables =====//
  int ch = CHANNEL;
  unsigned long lastChannelChange = 0;
  unsigned long lastPrintTime = 0;
  const unsigned long PRINT_INTERVAL = 5000; // Print every 5 seconds
  unsigned long packetCount = 0;

  // Structure to store packet information
  struct PacketInfo {
      char src_mac[18];
      int8_t rssi;
      unsigned long timestamp;
      uint8_t type;
  };

  // Array to store multiple packet infos
  PacketInfo packetInfos[MAX_PACKETS];
  int packetIndex = 0;

  //===== Function Definitions =====//
  void connectToWiFi() {
      Serial.print("Connecting to WiFi");
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

      unsigned long startAttemptTime = millis();

      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
          Serial.print(".");
          delay(100);
      }

      if (WiFi.status() != WL_CONNECTED) {
          Serial.println(" Failed!");
      } else {
          Serial.println(" Connected!");
          Serial.print("Local IP: ");
          Serial.println(WiFi.localIP());
      }
  }

  void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
      const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
      const uint8_t *frame = pkt->payload;
      const uint8_t frame_type = frame[0] & 0b00001100;

      if (packetIndex < MAX_PACKETS) {
          getMacAddress(&frame[10], packetInfos[packetIndex].src_mac);
          packetInfos[packetIndex].rssi = pkt->rx_ctrl.rssi;
          packetInfos[packetIndex].timestamp = millis();
          packetInfos[packetIndex].type = frame_type;
          packetIndex++;
      }

      packetCount++;
  }

  void getMacAddress(const uint8_t* addr, char* mac) {
        snprintf(mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X", 
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
  }

  void printPacketInfo() {
      if (millis() - lastPrintTime > PRINT_INTERVAL) {
          Serial.println("\n--- Packet Information ---");
          Serial.printf("Total packets captured: %lu\n", packetCount);
          Serial.printf("Packets in current batch: %d\n", packetIndex);
          Serial.println("Type | Source MAC      | RSSI | Time (ms)");
          Serial.println("-----|-----------------|------|----------");

          for (int i = 0; i < packetIndex; i++) {
              Serial.printf("%4d | %-15s | %4d | %9lu\n", 
                            packetInfos[i].type, packetInfos[i].src_mac, 
                            packetInfos[i].rssi, packetInfos[i].timestamp);
          }

          Serial.println("-------------------------");
          packetIndex = 0; // Reset the packet index
          lastPrintTime = millis();
      }
  }

  void channelHop() {
      if (CHANNEL_HOPPING && millis() - lastChannelChange > HOP_INTERVAL) {
          ch = (ch % MAX_CHANNEL) + 1;
          esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
          lastChannelChange = millis();
      }
  }

  String createJson() {
      DynamicJsonDocument doc(4096);
      doc["device_id"] = "ESP32_3";  // Unique identifier for this ESP32
      doc["total_packets"] = packetCount;
      doc["batch_size"] = packetIndex;

      JsonArray packets = doc.createNestedArray("packets");
      for (int i = 0; i < packetIndex; i++) {
          JsonObject packet = packets.createNestedObject();
          packet["type"] = packetInfos[i].type;
          packet["src_mac"] = packetInfos[i].src_mac;
          packet["rssi"] = packetInfos[i].rssi;
          packet["timestamp"] = packetInfos[i].timestamp;
      }

      String jsonString;
      serializeJson(doc, jsonString);
      return jsonString;
  }

void sendJson(const String& jsonString) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Attempting to reconnect...");
        connectToWiFi();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Failed to reconnect to WiFi. Aborting sendJson.");
            return;
        }
    }

    Serial.println("Sending data to server...");
    Serial.println("Server URL: http://143.93.153.168:5100/ESP32_3");
    Serial.println("JSON data: " + jsonString);

    HTTPClient http;
    http.begin("http://143.93.153.168:5100/ESP32_3");
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode > 0) {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
        String response = http.getString();
        Serial.println("Server response: " + response);
    } else {
        Serial.printf("Error sending HTTP POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}

  //===== Setup =====//
  void setup() {
      Serial.begin(115200);
      delay(2000);
      connectToWiFi() ;
      Serial.println("\n<<START>>");

      // Initialize NVS
      esp_err_t ret = nvs_flash_init();
      if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
          ESP_ERROR_CHECK(nvs_flash_erase());
          ret = nvs_flash_init();
      }
      ESP_ERROR_CHECK(ret);

      // Initialize WiFi in promiscuous mode
      wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
      ESP_ERROR_CHECK(esp_wifi_init(&cfg));
      ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
      ESP_ERROR_CHECK(esp_wifi_start());
      ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
      ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&sniffer));
      ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));

      Serial.println("WiFi sniffer started");
  }

  //===== Loop =====//
  void loop() {
    channelHop();
    
    // Create and send JSON string periodically
    if (millis() - lastPrintTime > PRINT_INTERVAL) {
        Serial.println("\n--- Preparing to send data ---");
        String jsonData = createJson();
        
        // Print the JSON string to Serial
        Serial.println("JSON Data:");
        Serial.println(jsonData);

        // Send the JSON data
        sendJson(jsonData);
        
        // Reset the packet index and update the last print time
        packetIndex = 0;
        lastPrintTime = millis();
        Serial.println("--- Data sending cycle completed ---\n");
    }
}
  
      
