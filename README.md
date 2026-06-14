# Control Luz Comedor + Wake-on-LAN (ESP8266)

Controlador domótico del **comedor** sobre **ESP8266 (NodeMCU)**. Además de la iluminación, despierta y monitorea PCs por la red:

- 💡 Control de **luz** del comedor (relé)
- 💻 **Wake-on-LAN** de dos PCs (`pc_intel`, `pc_amd`) con verificación de estado por **ping**
- 🗣️ **Alexa** vía MQTT + TLS (`TecnomaticoSkill`)
- 🕐 **Reloj NTP**
- 🌐 Interfaz **web asíncrona**
- 📡 **OTA** y **mDNS**
- ⏱️ Encendido/apagado **automático** por horario

## 🔌 Hardware

- ESP8266 (NodeMCU 1.0 / ESP-12E)
- Módulo de relé para la luz
- Red local con los PCs objetivo (WOL habilitado en BIOS/SO)

## 📚 Librerías necesarias

- `ESP8266` board package
- `ESPAsyncWebServer` + `ESPAsyncTCP`
- `PubSubClient` (MQTT)
- `NTPClient`
- `ESP8266Ping`
- `ArduinoOTA`, `ESP8266mDNS` (incluidas en el core)

## ⚙️ Configuración

Edita [`Luz_Comedor/Libreria.h`](Luz_Comedor/Libreria.h) y reemplaza los marcadores de WiFi y OTA:

```cpp
#define STA_SSID   "AQUI_TU_RED"
#define STA_PASS   "AQUI_TU_PASSWORD_WIFI"
#define OTA_PASSWORD "AQUI_TU_PASSWORD_OTA"
```

Configura las **MAC e IP** de los PCs a despertar (WOL) y los datos MQTT según tu montaje.

## 🚀 Carga del firmware

1. Abre `Luz_Comedor/Luz_Comedor.ino` en Arduino IDE (con todos los `.cpp`/`.h` de la carpeta).
2. Placa **NodeMCU 1.0 (ESP-12E)**, selecciona el puerto.
3. Compila y sube por USB; luego puedes actualizar por **OTA**.

## 📁 Estructura

| Archivo | Función |
|---|---|
| `Luz_Comedor.ino` | Setup/loop y lógica principal |
| `Wol_PowerPC.cpp` | Wake-on-LAN y estado de los PCs |
| `Mqtt_Alexa.cpp` / `Ota_Alexa.cpp` | Alexa (MQTT) y OTA |
| `On_OF_Automatic.cpp` | Encendido/apagado por horario |
| `Rutas_Server_Reles.cpp` / `Pagina_Web_New.cpp` | Servidor e interfaz web |
| `Reloj.cpp` | Reloj NTP |
