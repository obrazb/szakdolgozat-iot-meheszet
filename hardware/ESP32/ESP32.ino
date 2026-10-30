#include <esp_now.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <driver/i2s.h>
#include "arduinoFFT.h"
#include <ESP32Servo.h> 

#define KAPTAR_ID 1                                                 
#define TIME_TO_SLEEP 600                                           
#define MEASURE_TIME 60000                                          

const int oneWireBus = 26; 
                                          
//Lilygo MAC címe
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};   

#define I2S_SCK 33  
#define I2S_WS  25  
#define I2S_SD  32  
#define I2S_PORT I2S_NUM_0
#define SZERVO_PIN 13 

const uint16_t samples = 512; 
const double samplingFrequency = 10000; 

double vReal[samples];
double vImag[samples];
int32_t raw_samples[samples];

typedef struct struct_message {
  int kaptar_id;
  float homerseklet;
  float frekvencia; 
  int kijaro; 
} struct_message;

typedef struct command_message {
  int kijaro_parancs; 
} command_message;

struct_message kaptarAdat;
command_message bejovoParancs;

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
esp_now_peer_info_t peerInfo;
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, samples, samplingFrequency);
Servo kijaroSzervo;
bool parancsErkezett = false;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\n[ESP-NOW] Küldés állapota: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SIKERES!" : "SIKERTELEN!");
}

//Kijaro vezerlese
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&bejovoParancs, incomingData, sizeof(bejovoParancs));
  parancsErkezett = true;
  
  if(bejovoParancs.kijaro_parancs == 1) {
    kijaroSzervo.write(90);
  } else if(bejovoParancs.kijaro_parancs == 0) {
    kijaroSzervo.write(0);
  }
}

void setup() {
  //Rendszer és szenzorok indítása
  Serial.begin(115200);
  delay(500);
  Serial.println("\nKaptár indul...");

  kijaroSzervo.attach(SZERVO_PIN);
  sensors.begin();
  WiFi.mode(WIFI_STA);

  //Mikrofon beállítása
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = samplingFrequency,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);

  //ESP-NOW beállítása
  if (esp_now_init() != ESP_OK) {
    Serial.println("Hiba az ESP-NOW indításakor");
    return;
  }

  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
  esp_now_register_recv_cb(OnDataRecv); 
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Nem sikerült elérni a LilyGo-t");
    return;
  }

  //Hang mérés és FFT feldolgozás
  Serial.println("Hangmintavételezés...");
  
  double frekvenciaOsszeg = 0.0;
  int ervenyesMeresekSzama = 0;
  double KUSZOB = 1000.0; 

  uint32_t startTime = millis();

  while (millis() - startTime < MEASURE_TIME) {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, &raw_samples, sizeof(raw_samples), &bytesRead, portMAX_DELAY);
    int samplesRead = bytesRead / sizeof(int32_t);

    for (int i = 0; i < samples; i++) {
      if (i < samplesRead) {
        vReal[i] = (double)(raw_samples[i] >> 14); 
      } else {
        vReal[i] = 0.0;
      }
      vImag[i] = 0.0;
    }

    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); 
    FFT.compute(FFT_FORWARD); 
    FFT.complexToMagnitude(); 

    double maxMagnitude = 0.0;
    double peakFreq = 0.0;
    
    for (int i = 2; i < (samples / 2); i++) {
        if (vReal[i] > maxMagnitude) {
            maxMagnitude = vReal[i];
            peakFreq = (i * 1.0 * samplingFrequency) / samples;
        }
    }

    if (maxMagnitude > KUSZOB && peakFreq >= 50.0 && peakFreq <= 2000.0) {
      frekvenciaOsszeg += peakFreq;
      ervenyesMeresekSzama++;
    }
    delay(10); 
  }

  float atlagFrekvencia = 0.0;
  if (ervenyesMeresekSzama > 0) {
    atlagFrekvencia = frekvenciaOsszeg / ervenyesMeresekSzama;
    Serial.print("1 perces átlagfrekvencia: ");
    Serial.print(atlagFrekvencia);
    Serial.print(" Hz ("); Serial.print(ervenyesMeresekSzama); Serial.println(" mintából)");
  } else {
    Serial.println("Nem volt értékelhető hang.");
    atlagFrekvencia = 0.0; 
  }

  //Hőmérséklet mérés
  sensors.requestTemperatures(); 
  float mertHo = sensors.getTempCByIndex(0); 
  if (mertHo == -127.00) {
    Serial.println("Hőmérő nem található!");
    mertHo = 0.0;
  }
  Serial.print("Mért hőmérséklet: "); Serial.print(mertHo); Serial.println(" °C");

  //Adatok küldése a LilyGo-nak
  kaptarAdat.kaptar_id = KAPTAR_ID; 
  kaptarAdat.homerseklet = mertHo; 
  kaptarAdat.frekvencia = atlagFrekvencia; 
  kaptarAdat.kijaro = 0; 

  Serial.println("Adatcsomag elküldése a LilyGO-nak...");
  esp_now_send(broadcastAddress, (uint8_t *) &kaptarAdat, sizeof(kaptarAdat));
  
  unsigned long waitStart = millis();
  while(millis() - waitStart < 20000) {
    if(parancsErkezett) {
      delay(10000); 
      break;
    }
    delay(100);
  }
 
  //Deep Sleep
  Serial.print("Deep Sleep....");
  
  i2s_driver_uninstall(I2S_PORT);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
}