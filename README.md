# ESP32-S3 LG Klima Kumandası

ESP32-S3 üzerinde çalışan, WiFi üzerinden erişilen bir web arayüzüyle LG klimasını IR (kızılötesi) sinyalleriyle kontrol eden bir uzaktan kumanda projesi. Telefon veya bilgisayarınızın tarayıcısından klimanızı açıp kapatabilir; mod, sıcaklık, fan hızı ve salınım ayarlarını yapabilirsiniz.

## Özellikler

- 📱 Mobil uyumlu, karanlık temalı web arayüzü (harici bağımlılık yok, her şey cihaz üzerinde)
- 🔌 Aç/Kapat kontrolü
- 🌡️ Sıcaklık ayarı (16–30 °C)
- ❄️ Mod seçimi: Soğutma / Isıtma / Nem / Fan / Oto
- 💨 Fan hızı: Oto / Düşük / Orta / Yüksek
- ↕️ Dikey salınım (açık/kapalı)
- 📡 LG IR protokolü ( [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) ile )

Gerçek bir klima kumandası gibi, her komutta klimanın **tüm durumu** yeniden gönderilir. Cihaz mevcut durumu hafızasında tutar ve her değişiklikte tekrar iletir.

## Donanım

| Bileşen | Detay |
|---|---|
| Kart | LOLIN S3 Mini (ESP32-S3) |
| IR LED | GPIO **4** üzerinden sürülür |
| Besleme | USB (yerleşik USB CDC) |

IR LED'i GPIO 4'e (uygun bir seri direnç / transistör sürücü ile) bağlayın ve klimaya doğrultun.

## Kurulum

Bu proje [PlatformIO](https://platformio.org/) ile yapılandırılmıştır.

### 1. WiFi bilgilerini girin

[src/main.cpp](src/main.cpp) içindeki ağ bilgilerinizi düzenleyin:

```cpp
const char* WIFI_SSID     = "WIFI-SSID";
const char* WIFI_PASSWORD = "WIFI-PASSWORD";
```

### 2. Derleyip yükleyin

```bash
pio run --target upload
```

### 3. Seri monitörü açın

```bash
pio device monitor
```

Kart WiFi'ye bağlandığında seri çıkışta cihazın IP adresini göreceksiniz:

```
[WiFi] IP: 192.168.1.42
[Server] http://192.168.1.42
```

### 4. Arayüzü açın

Aynı ağdaki bir cihazdan tarayıcıda bu IP adresini açın; kumanda arayüzü karşınıza gelecektir.

## Bağımlılıklar

`platformio.ini` içinde tanımlıdır ve otomatik indirilir:

- `crankyoldgit/IRremoteESP8266` — IR gönderim ve klima protokolleri
- `ESP32Async/ESPAsyncWebServer` — asenkron web sunucusu
- `ESP32Async/AsyncTCP` — asenkron TCP katmanı

## HTTP API

Web arayüzü aşağıdaki uç noktaları kullanır:

| Uç Nokta | Yöntem | Açıklama |
|---|---|---|
| `/` | GET | Kumanda web arayüzü (HTML) |
| `/state` | GET | Mevcut durumu JSON olarak döner |
| `/set` | GET | Durumu günceller ve IR sinyalini gönderir |

`/set` parametreleri:

| Parametre | Değerler |
|---|---|
| `power` | `0` / `1` |
| `mode` | `cool` / `heat` / `dry` / `fan` / `auto` |
| `temp` | `16`–`30` |
| `fan` | `auto` / `low` / `medium` / `high` |
| `swing` | `0` / `1` |

Örnek:

```
http://192.168.1.42/set?power=1&mode=cool&temp=22&fan=auto&swing=0
```

## Proje Yapısı

```
├── platformio.ini      # PlatformIO yapılandırması ve bağımlılıklar
├── src/
│   └── main.cpp        # Firmware: WiFi, web sunucusu, arayüz ve IR gönderimi
├── include/            # Başlık dosyaları
├── lib/                # Proje kütüphaneleri
└── test/               # Testler
```

## Notlar

- IR protokolü **LG** olarak ayarlıdır ve bu ünitede test edilmiştir. Farklı bir marka/model için `IRremoteESP8266` protokolünü değiştirmeniz gerekebilir.
- Cihaz statik değil DHCP ile IP alır; kolay erişim için yönlendiricinizden sabit IP atamanız önerilir.
