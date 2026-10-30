#define TINY_GSM_MODEM_SIM7600
#define SerialMon Serial
#define SerialAT Serial1

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include <esp_now.h>

#define MODEM_TX 26
#define MODEM_RX 25
#define MODEM_PWKEY 4
#define MODEM_RESET_PIN 27
#define MODEM_POWER_ON 12 

#define TIME_TO_SLEEP 480

//Hálózati és szerver adatok
const char apn[] = "SZOLGALTATO_APN_IDE";
const char server[] = "SAJAT_SZERVER_CIME.HU"; 
const int  port = 80; 
const char SMS_TARGET[] = "+36300000000";

//ESP32 Mac címe
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
  int kaptar_id;
  float homerseklet;
  float frekvencia;
  int kijaro; 
} struct_message;

typedef struct command_message {
  int kijaro_parancs; 
} command_message;

struct_message bejovoAdat;
struct_message kaptarAdatok[10]; 
bool vanFrissAdat[10] = {false};
command_message kimenoParancs;
esp_now_peer_info_t peerInfo;

TinyGsm modem(SerialAT);
TinyGsmClient client(modem, 0); 
HttpClient http(client, server, port);

//Bejövő adatok feldolgozása
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  memcpy(&bejovoAdat, incomingData, sizeof(bejovoAdat));
  
  int index = bejovoAdat.kaptar_id - 1; 
  if (index >= 0 && index < 10) {
    kaptarAdatok[index] = bejovoAdat;
    vanFrissAdat[index] = true;
    
    SerialMon.print("\nAdat erkezett! Kaptar: ");
    SerialMon.print(bejovoAdat.kaptar_id);
    SerialMon.print(" | Ho: ");
    SerialMon.println(bejovoAdat.homerseklet);
  }
}

//Deep Sleep
void melyAlvasInditasa() {
  SerialMon.println("\n⚙️ Hardverek lekapcsolasa...");
  if (modem.isGprsConnected()) modem.gprsDisconnect();
  
  pinMode(MODEM_PWKEY, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW); delay(100);
  digitalWrite(MODEM_PWKEY, HIGH); delay(3000); 
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_POWER_ON, LOW); 

  SerialMon.println("Deep Sleep");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  //Rendszer indítása
  SerialMon.begin(115200);
  delay(500);
  SerialMon.println("LILYGO INDUL");

  //ESP-NOW beállítása
  WiFi.mode(WIFI_STA); 
  if (esp_now_init() != ESP_OK) return;
  
  esp_now_register_recv_cb(OnDataRecv);
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  //GSM Modem bekapcsolása
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_POWER_ON, HIGH);
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, LOW); delay(200);
  digitalWrite(MODEM_RESET_PIN, HIGH); delay(3000);
  
  pinMode(MODEM_PWKEY, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW); delay(100);
  digitalWrite(MODEM_PWKEY, HIGH); delay(1000);
  digitalWrite(MODEM_PWKEY, LOW);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  
  //Hálózati csatlakozás
  modem.restart(); 
  modem.waitForNetwork(120000);
  modem.gprsConnect(apn, "", "");
  SerialMon.println("\nVarakozas az adatra...");
}

unsigned long ebredesiIdo = millis();

void loop() {
  //Beérkezett adatok ellenőrzése
  for (int i = 0; i < 10; i++) {
    if (vanFrissAdat[i] == true) {
      if (!modem.isGprsConnected()) modem.gprsConnect(apn, "", "");

      //Adatküldés a PHP szerverre
      String url = "/api.php?kid=" + String(kaptarAdatok[i].kaptar_id) + 
                   "&ho=" + String(kaptarAdatok[i].homerseklet) +
                   "&frek=" + String(kaptarAdatok[i].frekvencia) +
                   "&kij=" + String(kaptarAdatok[i].kijaro);
      
      int err = http.get(url);
      if (err == 0) {
        String body = http.responseBody();
        SerialMon.println("Szerver válasz: " + body);
        
        //SMS riasztás küldése
        if (body.indexOf("R:1") != -1) {
            String smsText = " VIGYAZAT! A(z) " + String(kaptarAdatok[i].kaptar_id) + ". kaptar frekvenciaja megugrott!";
            modem.sendSMS(SMS_TARGET, smsText);
        }

        //Kijáró parancs visszaküldése a Kaptárnak (ESP-NOW)
        if (body.indexOf("KIJARO:1") != -1) {
            SerialMon.println("Parancs küldése a kaptárnak: NYITÁS");
            kimenoParancs.kijaro_parancs = 1;
            esp_now_send(broadcastAddress, (uint8_t *) &kimenoParancs, sizeof(kimenoParancs));
        } else if (body.indexOf("KIJARO:0") != -1) {
            SerialMon.println("Parancs küldése a kaptárnak: ZÁRÁS");
            kimenoParancs.kijaro_parancs = 0;
            esp_now_send(broadcastAddress, (uint8_t *) &kimenoParancs, sizeof(kimenoParancs));
        }

        http.stop();
        melyAlvasInditasa();
      } else {
        http.stop();
      }
    }
  }

  //Időtullépés
  if (millis() - ebredesiIdo > 720000) {
    melyAlvasInditasa();
  }
  delay(500); 
}