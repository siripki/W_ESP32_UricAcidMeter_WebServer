//SPIFFS, libraray untuk file system webserver
#include "SPIFFS.h"
#include "FS.h"

//WebServer and WiFi Parameter, library untuk wifi dan webserver
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
AsyncWebServer server(80);           //membuat objek untuk GUI server
AsyncEventSource events("/events");  //membuat SSE Event Source on /events untuk update nilai asam urat di webserver secara berkala

#include <Wire.h>
#include <LiquidCrystal_I2C.h> //library lcd
#include <Adafruit_ADS1X15.h>  //library eksternal adc ads1115

LiquidCrystal_I2C lcd(0x27, 16, 2);  //objek lcd 16x2 dengan alamat i2c 0x27
Adafruit_ADS1115 ads; //membuat objek ads1115

#define buzzer 15 //pin buzzer
int initADC, startADC; //variabel untuk inisialisasi pengukuran

// Parameter ketentuan yang tertampil di webserver
String mode;
String saran = "--";
String puasaAnak = "3,4 &ndash; 3,7 mg/dL";
String puasaDewasa = "3,7 &ndash; 9,0 mg/dL";
String puasaLansia = "&ge; 5,9 mg/dL";
String tidakPuasaAnak = "2,2-5 &ndash; 5 mg/dL";
String tidakPuasaDewasa = "2,7 &ndash; 8,5 mg/dL";
String tidakPuasaLansia = "&ge; 7,3 mg/dL";
float asamUrat = 0.0;  //nilai asam urat

void setup() {
  Serial.begin(115200); //memulai komunikasi serial, untuk debugging via USB
  pinMode(buzzer, OUTPUT);  //set pin buzzer sebagai output
  digitalWrite(buzzer, HIGH); //matikan buzzer 

  spiffSetup(); //inisialisasi file sistem
  readSettings(); //membaca setingan alat

  WiFi.mode(WIFI_AP); //merubah mode ke AP
  WiFi.softAP("Uric Acid Meter", "12345678"); //akses point dengan SSID dan Pass tersebut
  serverSetup();  //memulai web server

  xTaskCreatePinnedToCore(  //mengaktifkan Core 0
    loop2,   /* Task function. */
    "loop2", /* name of task. */
    20000,   /* Stack size of task */
    NULL,    /* parameter of the task */
    1,       /* priority of the task */
    NULL,    /* Task handle to keep track of created task */
    0);      /* pin task to core 0 */

}

void loop() {  //core 1 handle webserver
}

void loop2(void* pvParameters) { //core 0 handle pengukuran, lcd, dan buzzer
  lcdSetup();
  while (1) {
    if (ads.readADC_SingleEnded(1)>=1000) { //mendeteksi apakah strip terpasang
      lcd.setCursor(0, 0);
      lcd.print("  Ready To Use  ");
      lcd.setCursor(0, 1);
      lcd.print("Masukan Strip !!");
    }
    else if (ads.readADC_SingleEnded(1)<=190) {  //jika dimasukan strip guladarah/kolesterol maka menampilkan strip tidak didukung
      lcd.setCursor(0, 0);
      lcd.print("      Strip     ");
      lcd.setCursor(0, 1);
      lcd.print(" Tidak Didukung ");
    }
    else if (ads.readADC_SingleEnded(1)<=380) { //mendeteksi strip asam urat
      vTaskDelay(1000);
      startADC = ads.readADC_SingleEnded(0);  //ambil nilai adc strip tanpa darah
      initADC = startADC + 10;  //mencatat nilai awal
      while (ads.readADC_SingleEnded(1)<=380) {
        uric(); //mengeksekusi fungsi uric
      }
    }
    vTaskDelay(200);
    // Serial.print("ADC tipe Strip : "); Serial.println(ads.readADC_SingleEnded(1));
    Serial.print("ADC Uric Strip : "); Serial.println(ads.readADC_SingleEnded(0)); //menampilkan nilai adc strip
  }
}

void uric(){ //pengambilan data asam urat
  int asamUratADC;
  if (ads.readADC_SingleEnded(0) >= initADC) { //jika nilai adc strip melebihi nilai adc awal, mulai memproses data
    beep();
    lcd.clear();
    for(int i=15 ; i>=1 ; i--){  //beri waktu 15 detik sebelum ambil data sampel
      // Serial.println("ADC Strip UA        : " + String(ads.readADC_SingleEnded(0) - startADC));
      lcd.setCursor(0, 0);
      lcd.print(" MEMPROSES DATA ");
      lcd.setCursor(7, 1);
      lcd.print("  ");
      lcd.setCursor(7, 1);
      lcd.print(i);
      vTaskDelay(975);
    }
    asamUratADC = ads.readADC_SingleEnded(0) - startADC; //nilai adc strip saat ini - nilai adc strip tanpa darah
    Serial.println("ADC Strip UA (blood): " + String(asamUratADC));
    asamUrat = getRegress(asamUratADC); //mengkalkulasi nilai asam urat menggunakan fungsi regresi
    getSaran(asamUrat); //menentukan saran berdasarkan nilai asam urat
    serverSentEvents(); //mengupload data baru ke webserver
    if(asamUrat>=7.0){ //jika sama atau lebih dari 200, buzzer nyala dua kali
      beepWarning();
      lcd.setCursor(0,0);
      lcd.print("===ASAM__URAT===");
      lcd.setCursor(0,1);
      lcd.print(" KADAR TINGGI ! ");
      vTaskDelay(2000);
    }
    else if (asamUrat<7.0) { //jika kurang dari 7 buzzer nyala sekali
      beep();
    }
    tampilHasil("ASAM  URAT", asamUrat, "mg/dL"); //memanggil fungsi display
  }
  else {
    lcd.setCursor(0,0);
    lcd.print("===ASAM__URAT===");
    lcd.setCursor(0,1);
    lcd.print(" Masukan Sample ");
  }
}

float getRegress(int &adc) { //rumus regresi asam urat hasil kalibrasi
  float result;
  result = 0.0959*adc + 0.2376; //y = 0,0959x + 0,2376
  if (result < 0.0) result = 0;
  return result;
}

void lcdSetup() { //fungsi untuk inisialisasi LCD dan ADS1115
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("  ALAT DETEKSI  ");
  lcd.setCursor(0,1);
  lcd.print("   ASAM  URAT   ");
  ads.begin(0x48);   //alamat i2c ads1115
  vTaskDelay(1000);
}

void tampilHasil(const char* mode, float hasil, const char* satuan){ //fungsi untuk menampilkan hasil pada LCD
  lcd.clear();
  while ( hasil<10000 && ads.readADC_SingleEnded(1)<=1000) { //hold tampilan hasil selama strip tidak dganti
    lcd.setCursor(3, 0);
    lcd.print(mode);
    lcd.setCursor(1, 1);
    lcd.print(hasil,1);
    lcd.setCursor(11, 1);
    lcd.print(satuan);
    vTaskDelay(300);
  }
}

void beep(){ //untuk menyalakan buzzer
  digitalWrite(buzzer, LOW);
  vTaskDelay(60);
  digitalWrite(buzzer, HIGH);
}

void beepWarning(){ //menyalakan buzzer beep 2 kali
  digitalWrite(buzzer, LOW);
  vTaskDelay(60);
  digitalWrite(buzzer, HIGH);
  vTaskDelay(100);
  digitalWrite(buzzer, LOW);
  vTaskDelay(1000);
  digitalWrite(buzzer, HIGH);
}

void getSaran(float &value) { //menentukan saran berdasarkan mode yang dipilih
  if (mode == "anak") {
    if (value >= 4 and value < 5) saran = "Konsultasi dengan dokter anak untuk evaluasi menyeluruh";
    else if (value >= 5 and value < 6) saran = "Perlu penanganan medis segera";
    else if (value >= 6.0) saran = "Ini adalah kondisi serius yang memerlukan perhatian medis segera";
    else saran = "Normal";
  }
  else if (mode == "dewasa") {
    if (value >= 4 and value < 5) saran = "Perlu memperhatikan pola makan dan gaya hidup";
    else if (value >= 5 and value < 6) saran = "Dianjurkan untuk segera berkonsultasi dengan dokter";
    else if (value >= 6.0) saran = "Kondisi ini memerlukan perhatian medis segera";
    else saran = "Normal";
  }
  else if (mode == "lansia") {
    if (value >= 4 and value < 5) saran = "Memerlukan konsultasi dengan dokter untuk evaluasi menyeluruh";
    else if (value >= 5 and value < 6) saran = "Perlu pemantauan berkala dengan dokter";
    else if (value >= 6.0) saran = "Memerlukan penanganan medis segera";
    else saran = "Normal";
  }
}
