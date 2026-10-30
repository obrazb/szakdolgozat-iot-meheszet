# szakdolgozat-iot-meheszet
Méhcsaládok állapotának monitorozása IoT okoskaptárral. Hardveres vezérlő kódok (C++), szerveroldali szkriptek, webes vizualizáció és Python alapú akusztikus adatelemzés.
# 🐝 Méhkaptár állapotfigyelő és beavatkozó rendszer IoT technológiákkal - Szakdolgozati Projekt

Ez a repo egy IoT alapú, távolról vezérelhető és monitorozható okoskaptár teljes szoftveres megvalósítását tartalmazza. A rendszer valós időben méri a kaptár belső hőmérsékletét és a méhek akusztikus aktivitását, majd ezeket az adatokat mobilhálózaton keresztül egy felhős adatbázisba továbbítja.

## 🌟 Főbb funkciók
- **Akusztikus analízis (FFT):** A belső ESP32 mikrofonnal figyeli a hangfrekvenciát, hogy kiszűrje a rendellenes viselkedést.
- **Környezeti monitorozás:** DS18B20 szenzor a kaptár belső hőmérséklet mérésére.
- **Távoli beavatkozás:** Szervomotoros kijáró-vezérlés a webes felületről.
- **Okos riasztások:** Automatikus SMS küldés kiugró értékek esetén.
- **Adatvizualizáció:** Interaktív dashboard és Python alapú korrelációs elemzés.

## 📂 Mappaszerkezet

* 📁 **`hardware/`** - Mikrovezérlők (C++) forráskódjai.
  * `ESP32/`: Az ESP32 mikrokontrolleres végpont, amely a méréseket, az FFT számítást és a szervó mozgatását végzi.
  * `LilyGO/`: A SIM7600 GSM átjáró, amely továbbítja az adatokat a szerver felé és kezeli az SMS riasztásokat.
* 📁 **`backend/`** - Szerveroldal (PHP). Az adatok fogadását és kiszolgálását végző API, valamint a MySQL adatbázis szerkezete.
* 📁 **`frontend/`** - Felhasználói felület. HTML/CSS/JS alapú reszponzív dashboard a valós idejű adatok követéséhez és a kijáró vezérléséhez.
* 📁 **`elemzes/`** - Adattudományi elemzés . Hőmérséklet és frekvencia összefüggéseit, valamint csúcsdetektálást végző szkript.

## 🛠️ Felhasznált technológiák
* **Hardware:** ESP32U, LilyGO T-Call A7670E, DS18B20, INMP441 Mikrofon, MG90 Szervomotor.
* **Backend:** C++, PHP 8, MySQL.
* **Frontend:** HTML5, CSS3 (Bootstrap 5), JavaScript (Fetch API, Chart.js).
* **Adatelemzés:** Python (Google Colab).

## 
Ez a szoftvercsomag szakdolgozati kutatás céljából jött létre (2026).
