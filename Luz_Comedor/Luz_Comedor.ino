#include "Libreria.h"
//ultima actualizacion 11 sep 2025
//Variables para el control del pulsador usando debounce para evitar lecturas erróneas por rebote y flanco ascendente para evitar múltiples activaciones.
unsigned long lastToggleTime = 0;
const unsigned long debounceDelay = 750;
int attempts = 0;
bool lastButtonState = LOW; // Estado anterior del botón

// Definición de variables globales (declaradas como extern en Libreria.h)
int varOnOff = 1;
unsigned long contadooR = 0;
bool VeriTurn_Automatic = false; // Verifica si el riego está encendido
// Inicia variables Estado Reles
bool estadoDelLED = false;
bool estado = false;

// NTP - Definición de variables globales
WiFiUDP udp;
NTPClient ntpverano(udp, "ntp.shoa.cl", (-3 * 3600), 3600);
NTPClient ntpinvierno(udp, "ntp.shoa.cl", (-4 * 3600), 3600);
String hora = "00:00:00";
String horaVirtual = "00:00:00";
String hourS = "00:00:00";
String MesCortado = "Aug";                         // Inicialización por defecto
String MesPalabras = "AGOSTO - Sistema Iniciando"; // Inicialización por defecto
int MesActual = 8;                                 // Agosto por defecto
int h = 0;
int m = 0;
int s = 0;
const PcConfig PCS[] = {
  {"PC1", "AQUI_TU_MAC", "http://AQUI_TU_IP_LOCAL:3000/apagar", IPAddress(192, 168, 0, 1)},
  {"PC2", "AQUI_TU_MAC", "http://AQUI_TU_IP_LOCAL:3000/apagar", IPAddress(192, 168, 0, 1)}
};
const size_t PC_COUNT = sizeof(PCS) / sizeof(PCS[0]);

// Variables para debugging de recuperación automática (mostrar en web)
String debugInfo = "";
String ultimaVerificacion = "";
String estadoActual = "";
String horaEncendidoActual = "";
String horaApagadoActual = "";
bool deberiaEstarEncendida = false;
unsigned long ultimaVerificacionMillis = 0;
bool recuperacionActiva = false; // ✅ NUEVA: Protege contra interferencias

// Variables de control
bool evitarDeepSleep = true;
bool ActivaLuz = false;
bool EstadoDeseadoLuz = false; // Estado que Alexa quiere (true=encender, false=apagar)
bool telnetHabilitado = false;
AlexaDebugState alexaDebugState = ALEXA_DEBUG_IDLE;
String alexaUltimoDispositivo = "-";
String alexaUltimaAccion = "espera";
String alexaUltimoDetalle = "sin eventos";
unsigned long alexaUltimoCambioMillis = 0;

bool Activa_Pc_Piso1 = false;
bool EstadoDeseadoPc_Piso1 = false; // Estado que Alexa quiere para el PC piso 1 (true=encender, false=apagar)
bool Activa_Pc_Piso2 = false;
bool EstadoDeseadoPc_Piso2 = false; // Estado que Alexa quiere para el PC piso 1 (true=encender, false=apagar)
bool EstadoActualPc_Piso1 = false;
bool EstadoActualPc_Piso2 = false;
const unsigned long pingIntervalPCs = 3600000;
const unsigned long pingStepDelayPCs = 1500;
unsigned long ultimoPingPCsMillis = 0;
unsigned long siguientePasoPingMillis = 0;
int estadoPingPCs = 0;
bool pingInicialPendiente = true;
// Variables para manejar el delay entre ReleLuz
bool ReleLuzEncendido = false;
unsigned long DelayLuzLed = 0;

// ✅ AÑADIDO: Control de tiempo no bloqueante para loop()
unsigned long previousMillis = 0;
const unsigned long loopInterval = 100; // Ejecutar funciones principales cada 100ms

// ✅ CONTROL DE TIEMPO ASÍNCRONO PARA NTP
unsigned long previousNTPUpdate = 0;
const unsigned long ntpUpdateInterval = 3600000; // Actualizar NTP cada hora (3600000ms)
unsigned long lastNTPAttempt = 0;
const unsigned long ntpRetryInterval = 30000; // Reintentar NTP cada 30 segundos si falla
bool ntpInitialized = false;
bool ntpUpdateInProgress = false;
bool currentButtonState = false;
bool Detect_Pulsador = false;
bool bloqueoRecuperacionRemota = false;
int Bandera_Rele_luz = 0; // Variable para controlar el estado del relé de la luz (0=apagado, 1=encendido)
// ✅ VARIABLE PARA DETECCIÓN DE RECONEXIÓN WIFI AUTOMÁTICA
bool wifiConnectedPreviously = false;

// Variables faltantes para WOL
bool resetPendiente = false;
unsigned long tiempoReset = 0;
bool sincronizacionPendiente = false;
unsigned long tiempoSincronizacion = 0;
// Variables para apagado diferido (evita stack overflow)
bool apagadoPendiente = false;
unsigned long tiempoApagado = 0;
String macApagadoPendiente = "";
int estadoPulsador = 0;


AsyncWebServer webServer(80);
WiFiClientSecure mqttSecureClient;
PubSubClient mqttClient(mqttSecureClient);
unsigned long lastMqttRetry = 0;
const unsigned long mqttRetryMillis = 5000UL;
WiFiServer telnetServer(23);
WiFiClient telnetClient;
DebugTelnetLogger debugTelnet;

// Definiciones de constantes y objetos IPAddress
const char *ntpServer = "time1.google.com";
const long gmtOffset_sec = 3600;            // Replace with your GMT offset (seconds)
const int daylightOffset_sec = (-3 * 3600); // Replace with your daylight offset (seconds)

IPAddress local_IP(192, 168, 0, 1); // AQUI_TU_IP_LOCAL
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // DNS primario
IPAddress secondaryDNS(1, 1, 1, 1); // DNS secundario

// ✅ CALLBACK PARA EVENTO DE CONEXIÓN WIFI
void onWiFiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println();
  Serial.println("[WIFI] ✅ Conectado y IP asignada: " + WiFi.localIP().toString());
  wifiConnectedPreviously = true;
  pingInicialPendiente = true;
  estadoPingPCs = 0;
  siguientePasoPingMillis = 0;
}

void onWiFiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("[WIFI] ❌ Desconectado. Razón: " + String(event.reason));
  wifiConnectedPreviously = false;
  estadoPingPCs = 0;
}

void setup()
{
  pinMode(ReleLuz, OUTPUT);
  pinMode(pulsadorPin, INPUT_PULLUP);
  lastButtonState = digitalRead(pulsadorPin);
 
  // ✅ LEER EL NIVEL ACTUAL DEL PIN DEL RELÉ ANTES DE MODIFICARLO (importante para recuperación)
  bool estadoInicialLuz = digitalRead(ReleLuz);
  Serial.println("[INIT] Estado previo LED: " + String(estadoInicialLuz ? "HIGH" : "LOW"));

  // Relé activo-LOW: pin LOW = luz ON, pin HIGH = luz OFF.
  // Reescribir el mismo nivel que se leyó para preservar el estado físico.
  digitalWrite(ReleLuz, estadoInicialLuz);
  estadoDelLED = !estadoInicialLuz;              // LOW(false)→ON(true), HIGH(true)→OFF(false)
  Bandera_Rele_luz = estadoDelLED ? 1 : 0;

  Serial.println("[INIT] Estado inicial del relé preservado para evitar cambios espurios al arrancar");

  delay(1);
  
  // ✅ CONFIGURAR EVENTOS WIFI PARA MONITOREO
  WiFi.onStationModeGotIP(onWiFiConnect);
  WiFi.onStationModeDisconnected(onWiFiDisconnect);

  // ✅ CONFIGURACIÓN WIFI: Modo STA con reconexión automática del ESP8266
  WiFi.hostname("COMEDOR");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);  // El ESP8266 se reconecta automáticamente
  WiFi.persistent(true);        // Guardar credenciales en flash

  // IP FIJA
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("Fallo al configurar IP estática");
  }

  // comienza STA
  WiFi.begin(STA_SSID, STA_PASS);
  Serial.print("[WIFI] Conectando a WiFi");

  // ✅ ESPERA INICIAL BREVE - WiFi.setAutoReconnect(true) seguirá intentando en background
  while (WiFi.status() != WL_CONNECTED && attempts < 15)
  {
    Serial.print(".");
    Pulsador();
    attempts++;
    delay(500);
  }

  // ✅ CONFIGURAR SERVICIOS INDEPENDIENTEMENTE DEL ESTADO WIFI INICIAL
  Serial.println("\n[SYSTEM] Configurando servicios del sistema...");
  
  // Configurar servidor web; quedará accesible cuando la interfaz WiFi tenga IP.
  Prepara_Rutas();

  if (WiFi.status() == WL_CONNECTED)
  {
    wifiConnectedPreviously = true;
    Serial.println("[WIFI] ✅ Conexión inicial exitosa");
    Serial.print("[WIFI] IP: " + WiFi.localIP().toString());
    Serial.print(" | Gateway: " + WiFi.gatewayIP().toString());
    Serial.println(" | Subnet: " + WiFi.subnetMask().toString());

    // Configurar servicios que requieren WiFi
    setupTelnet();
    setupOTA();
    // ✅ 1. INICIALIZAR mDNS PRIMERO
    if (MDNS.begin("COMEDOR")) {
      Serial.println("[mDNS] ✅ mDNS iniciado correctamente");

      MDNS.addService("http", "tcp", 80);

      Serial.println("[mDNS] ✅ Servicio HTTP agregado");
    } else {
      Serial.println("[mDNS] ❌ Error al inicializar mDNS");
    }

    // ✅ CONFIGURACIÓN NTP (solo si hay WiFi)
    Serial.println("[NTP] Configurando NTP...");
    Configura();

    // ✅ INICIALIZACIÓN NTP ASÍNCRONA
    if ((MesCortado == "Sep") || (MesCortado == "Oct") || (MesCortado == "Nov") || (MesCortado == "Dec") || (MesCortado == "Jan") || (MesCortado == "Feb") || (MesCortado == "Mar"))
    {
      ntpverano.begin();
      Serial.println("[NTP] Cliente NTP verano iniciado");
    }
    else
    {
      ntpinvierno.begin();
      Serial.println("[NTP] Cliente NTP invierno iniciado");
    }

    // ✅ PRIMERA SINCRONIZACIÓN NTP
    Hrs();
    RelojDigital();
    hora = horaVirtual;
    Serial.println("[DEBUG] Hora sincronizada: " + hora);

    // ✅ EJECUTAR VERIFICACIÓN INICIAL
    Serial.println("[DEBUG] Ejecutando verificación inicial de recuperación automática...");
    verificarRecuperacionParaMesActual();
    Serial.println("[DEBUG] ✅ Verificación inicial completada");

    Serial.println("[SYSTEM] ✅ Servicios con WiFi iniciados correctamente");
    Serial.println("[SYSTEM] 🌐 Acceso web: http://" + WiFi.localIP().toString());
  }
  else
  {
    Serial.println("[WIFI] ⚠️ Sin conexión WiFi inicial");
    Serial.println("[WIFI] 🔄 WiFi.setAutoReconnect(true) seguirá intentando conectar automáticamente");
    Serial.println("[SYSTEM] ✅ Sistema iniciado en modo offline");

    // ✅ INICIALIZAR recuperacionActiva aunque no haya WiFi
    recuperacionActiva = true;
    Serial.println("[INIT] ✅ recuperacionActiva inicializada en true");
  }

  // ✅ ASEGURAR QUE recuperacionActiva ESTÉ INICIALIZADA CORRECTAMENTE
  if (!recuperacionBloqueada()) {
    recuperacionActiva = true;
    Serial.println("[INIT] ✅ recuperacionActiva establecida en true (sin bloqueos activos)");
  }

  setupMqttAlexa();

  Serial.println("[SYSTEM] 🚀 Sistema completamente iniciado");
}

void Pulsador() // Detectar flanco ascendente con debounce
{
  currentButtonState = digitalRead(pulsadorPin);
  
  if (lastButtonState == HIGH && currentButtonState == LOW && millis() - lastToggleTime > debounceDelay) 
  {
    bool releEstado = digitalRead(ReleLuz);
    digitalWrite(ReleLuz, !releEstado);      // Toggle: invierte el nivel del pin
    estadoDelLED = releEstado;               // Antes HIGH(OFF)→ahora LOW(ON)=true, antes LOW(ON)→ahora HIGH(OFF)=false
    Bandera_Rele_luz = estadoDelLED ? 1 : 0;
    sincronizarEstadoAlexaLuz();
    publishMqttDeviceState("luz_tele", estadoDelLED);
    lastToggleTime = millis();
    registrarIntervencionManual();
    Serial.println("[PULSADOR] Botón presionado - LED: " + String(estadoDelLED ? "ON" : "OFF"));
  }

  lastButtonState = currentButtonState;      // Actualizar estado del botón
}

void loop()
{
  unsigned long currentMillis = millis();
  
  // ✅ WATCHDOG: Reset del watchdog para evitar resets
  ESP.wdtFeed();
  
  // ✅ PULSADOR SIEMPRE FUNCIONA (SIN WIFI O CON WIFI)
  Pulsador();
  
  // ✅ DETECCIÓN DE RECONEXIÓN WIFI AUTOMÁTICA
  // WiFi.setAutoReconnect(true) se encarga de reconectar, solo detectamos el cambio
  bool currentWiFiStatus = (WiFi.status() == WL_CONNECTED);

  if (currentWiFiStatus != wifiConnectedPreviously) {
    if (currentWiFiStatus) {
      Serial.println("[WIFI] 🔄 Reconectado automáticamente por WiFi.setAutoReconnect(true)");
      Serial.println("[WIFI] IP: " + WiFi.localIP().toString());

      // ✅ REINICIALIZAR TODOS LOS SERVICIOS TRAS RECONEXIÓN
      Serial.println("[SYSTEM] 🔄 Reinicializando servicios de red...");

      // 0. Reiniciar Telnet
      setupTelnet();

      // 1. Reiniciar OTA
      setupOTA();
      Serial.println("[OTA] ✅ OTA reiniciado");

      // 2. Reiniciar MQTT para Skill Smart Home
      setupMqttAlexa();
      Serial.println("[MQTT] ✅ Alexa MQTT reiniciado");

      // 3. Reiniciar mDNS
      MDNS.close(); // Cerrar instancia anterior si existe
      if (MDNS.begin("COMEDOR")) {
        Serial.println("[mDNS] ✅ mDNS reiniciado correctamente");

        MDNS.addService("http", "tcp", 80);

        Serial.println("[mDNS] ✅ Servicio HTTP agregado");
      } else {
        Serial.println("[mDNS] ❌ Error al reinicializar mDNS");
      }

      // 4. Reinicializar NTP tras reconexión
      Serial.println("[NTP] Inicializando NTP tras reconexión...");
      Configura(); // Reconfigurar mes y parámetros NTP
      if ((MesCortado == "Sep") || (MesCortado == "Oct") || (MesCortado == "Nov") || (MesCortado == "Dec") || (MesCortado == "Jan") || (MesCortado == "Feb") || (MesCortado == "Mar"))
      {
        ntpverano.begin();
        Serial.println("[NTP] Cliente NTP verano reiniciado");
      } else {
        ntpinvierno.begin();
        Serial.println("[NTP] Cliente NTP invierno reiniciado");
      }
      Hrs();
      RelojDigital();
      hora = horaVirtual;
      ntpInitialized = true;
      Serial.println("[NTP] ✅ NTP sincronizado: " + hora);

      // 5. Ejecutar verificación de recuperación automática
      Serial.println("[DEBUG] Ejecutando verificación de recuperación tras reconexión...");
      verificarRecuperacionParaMesActual();
      Serial.println("[DEBUG] ✅ Verificación completada");

      Serial.println("[SYSTEM] ✅ Todos los servicios reiniciados correctamente");
    }
    wifiConnectedPreviously = currentWiFiStatus;
  }
  
  // Procesar servicios que requieren WiFi
  if (currentWiFiStatus)
  {
    handleTelnet();
    ArduinoOTA.handle();
    MDNS.update();
    handleMqttAlexa();
    procesarPingPCs();

    // ✅ ACTUALIZACIÓN NTP CONTROLADA - SOLO CADA 4 HORAS para no interferir con servidor web
    if (!ntpInitialized || (currentMillis - previousNTPUpdate >= 14400000)) // 4 horas = 14400000ms
    {
      if (!ntpUpdateInProgress) // Evitar múltiples actualizaciones simultáneas
      {
        ntpUpdateInProgress = true;
        Serial.println("[NTP] Iniciando sincronización programada...");

        // Ejecutar HrsAsync en background (con delays restaurados)
        HrsAsync();

        ntpInitialized = true;
        previousNTPUpdate = currentMillis;
        ntpUpdateInProgress = false;

        Serial.println("[NTP] ✅ Sincronización completada");
      }
    }
  }

  // ✅ EJECUTAR FUNCIONES PRINCIPALES CON CONTROL DE TIEMPO USANDO MILLIS()
  if (currentMillis - previousMillis >= loopInterval)
  {
    previousMillis = currentMillis;

    // Yield para evitar watchdog y stack overflow
    yield();
    
    handleLuz();
    handlePC_Piso1();
    handlePC_Piso2();
    
    yield(); // Dar tiempo al sistema
    
    RelojDigital(); // ✅ MANTENER - esencial para el reloj virtual
    hora = horaVirtual; // ✅ MANTENER - sincronización reloj virtual
    
    // Ejecutar onoff() con menos frecuencia para evitar stack overflow
    static unsigned long lastOnOff = 0;
    if (currentMillis - lastOnOff >= 1000) { // Solo cada segundo
      onoff();
      lastOnOff = currentMillis;
    }
  }

  // ✅ INTEGRACIÓN: PROCESAR SINCRONIZACIÓN DIFERIDA (AJAX NTP)
  // Esta función maneja la sincronización NTP solicitada desde la interfaz web
  // sin interferir con las conexiones HTTP activas
  procesarSincronizacionDiferida();
  procesarApagadoHttpPendiente();

  // Ceder CPU sin bloquear la atención del servidor Async.
  yield();
}

// ✅ FUNCIÓN NTP ASÍNCRONA QUE MANTIENE LÓGICA ORIGINAL
void updateNTPAsync()
{
  if (ntpUpdateInProgress)
    return; // Evitar múltiples actualizaciones simultáneas

  unsigned long currentMillis = millis();

  // Solo intentar actualización si ha pasado suficiente tiempo
  if (currentMillis - lastNTPAttempt < ntpRetryInterval)
    return;

  lastNTPAttempt = currentMillis;
  ntpUpdateInProgress = true;

  Serial.println("[NTP] Sincronización asíncrona...");

  // ✅ MANTENER LÓGICA ORIGINAL: usar HrsAsync que mantiene la funcionalidad de Hrs()
  HrsAsync();

  ntpUpdateInProgress = false;
}