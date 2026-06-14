#ifndef Libreria_h
#define Libreria_h

#include <Arduino.h> // Es buena práctica incluir esto
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266Ping.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <stdarg.h>

enum AlexaDebugState {
	ALEXA_DEBUG_IDLE = 0,
	ALEXA_DEBUG_VOICE_COMMAND = 1,
	ALEXA_DEBUG_ACTION_PENDING = 2,
	ALEXA_DEBUG_ACTION_DONE = 3,
	ALEXA_DEBUG_SYNC = 4,
	ALEXA_DEBUG_ERROR = 5,
	ALEXA_DEBUG_TRACE = 6
};

struct PcConfig {
	const char *nombreAlexa;
	const char *mac;
	const char *urlApagado;
	IPAddress ip;
};

class DebugTelnetLogger {
public:
	void begin(unsigned long baudRate);
	size_t print(const String &value);
	size_t print(const char *value);
	size_t print(char value);
	size_t print(int value);
	size_t print(unsigned int value);
	size_t print(long value);
	size_t print(unsigned long value);
	size_t print(bool value);
	size_t print(const IPAddress &value);
	size_t println();

	template <typename T>
	size_t println(const T &value) {
		size_t written = print(value);
		written += println();
		return written;
	}

	int printf(const char *format, ...);

private:
	String lineBuffer;

	size_t writeText(const String &value, bool appendNewLine);
	void flushLine();
};

void onoff();
void verificarRecuperacionParaMesActual();
void escribirInterfazHTML(Print &out);
void Configura();
void printLocalTime();
void convierteMes();
void RelojDigital();
void HrsAsync();
void Hrs();
void Prepara_Rutas();
void ejecutarEncenderLed();
void ejecutarApagarLed();
void mostrarInterfazHTML(AsyncWebServerRequest *request);
void rutaEncenderLed(AsyncWebServerRequest *request);
void rutaApagarLed(AsyncWebServerRequest *request);
void rutaActuaHora(AsyncWebServerRequest *request);
void procesarSincronizacionDiferida(); // ✅ NUEVA FUNCIÓN para sincronización diferida
void rutaReset(AsyncWebServerRequest *request);
void handleLuz();
void handlePC_Piso1();
void handlePC_Piso2();
void setupMqttAlexa();
void handleMqttAlexa();
void publishMqttDeviceState(const char *deviceId, bool powerOn);
void publishAllMqttStates();
void setupOTA();
String normalizarHora(String horaStr);
void sincronizarRelojVirtual(int horas, int minutos, int segundos);
int convertirHoraAMinutosSilencioso(String horaStr);
bool estaEnRangoEncendidoSilencioso(int horaActualMinutos, String horaEncendido, String horaApagado);
int convertirHoraAMinutos(String horaStr);
bool estaEnRangoEncendido(int horaActualMinutos, String horaEncendido, String horaApagado);
void prevenirDeepSleep(uint64_t tiempo);
void updateNTPAsync();
void Pulsador();
bool recuperacionBloqueada();
void registrarIntervencionManual();
void registrarIntervencionRemota();
void limpiarBloqueosRecuperacion();
void escribirFragmentoEstados(Print &out);
void rutaEstado(AsyncWebServerRequest *request);
void sendWOL(const IPAddress ip, const byte mac[]);
void macStringToBytes(const String mac, byte *bytes);
bool esMacHexValida(const String &mac);
void actualizarEstadoAlexaPCs();
void actualizarEstadoAlexaPC(const String &mac, bool encendido);
void sincronizarEstadoAlexaLuz(bool forzar = false);
void setupTelnet();
void handleTelnet();
void setTelnetHabilitado(bool habilitado);
void toggleTelnet();
String obtenerEstadoTelnet();
void actualizarEstadoAlexa(AlexaDebugState nuevoEstado, const String &dispositivo, const String &accion, const String &detalle = "");
void registrarEventoAlexa(const String &dispositivo, const String &accion, const String &detalle = "");
void registrarTrazaTelnet(const String &linea);
void procesarPingPCs();
void enviarWoL(String mac);
void enviarPeticionApagarComputador(String mac);
void procesarApagadoHttpPendiente();
void programarApagadoDiferido(const String &mac, const String &origen);
void rutaWakeOnLan(AsyncWebServerRequest *request);
void rutaApagaComputador(AsyncWebServerRequest *request);
void rutaToggleTelnet(AsyncWebServerRequest *request);
void onWiFiConnect(const WiFiEventStationModeGotIP& event);
void onWiFiDisconnect(const WiFiEventStationModeDisconnected& event);

extern const char *ntpServer;
extern const long gmtOffset_sec;     // Replace with your GMT offset (seconds)
extern const int daylightOffset_sec; // Replace with your daylight offset (seconds)
extern int Bandera_Rele_luz;
// Version Beta, aun en test
//  Configura tu red doméstica aquí
#define STA_SSID "AQUI_TU_RED"
#define STA_PASS "AQUI_TU_PASSWORD_WIFI"

// RELES
// ReleLuz es activo en LOW: LOW energiza el relé y HIGH lo desenergiza.
const int ReleLuz = D2;
const int pulsadorPin = D1;
// FIN RELES

// Variables globales - usando extern para evitar múltiples definiciones
extern unsigned long contadooR;
extern int estadoPulsador;
extern bool estadoDelLED; // Estado lógico de la luz: true=encendida, false=apagada
// Inicia variables Estado Reles
extern bool estado;
extern bool Detect_Pulsador; //Variable detecte si se presiono intencionalmente con el fin de usar en la Recuperación el estado correcto de la luz después de un reseteo
extern bool bloqueoRecuperacionRemota;


// NTP - Variables globales extern
extern WiFiUDP udp;
extern NTPClient ntpverano;
extern NTPClient ntpinvierno;
extern String hora;
extern String horaVirtual;
extern String hourS;
extern String MesCortado;  // Inicialización por defecto
extern String MesPalabras; // Inicialización por defecto
extern int MesActual;      // Agosto por defecto
extern int h;
extern int m;
extern int s;
extern const PcConfig PCS[];
extern const size_t PC_COUNT;
extern bool Activa_Pc_Piso1;
extern bool EstadoDeseadoPc_Piso1;
extern bool Activa_Pc_Piso2;
extern bool EstadoDeseadoPc_Piso2;
extern bool EstadoActualPc_Piso1;
extern bool EstadoActualPc_Piso2;
extern bool VeriTurn_Automatic;
extern const unsigned long pingIntervalPCs;
extern const unsigned long pingStepDelayPCs;
extern unsigned long ultimoPingPCsMillis;
extern unsigned long siguientePasoPingMillis;
extern int estadoPingPCs;
extern bool pingInicialPendiente;

// Variables para debugging de recuperación automática (mostrar en web) - extern
extern String debugInfo;
extern String ultimaVerificacion;
extern String estadoActual;
extern String horaEncendidoActual;
extern String horaApagadoActual;
extern bool deberiaEstarEncendida;
extern unsigned long ultimaVerificacionMillis;
extern bool recuperacionActiva; // ✅ NUEVA: Protege contra interferencias
extern bool resetPendiente;
extern unsigned long tiempoReset;
extern bool sincronizacionPendiente;
extern unsigned long tiempoSincronizacion;
// Variables para apagado diferido (evita stack overflow)
extern bool apagadoPendiente;
extern unsigned long tiempoApagado;
extern String macApagadoPendiente;
// Configuración OTA
#define OTA_HOSTNAME "COMEDOR"
#define OTA_PASSWORD "AQUI_TU_PASSWORD_OTA"

// Puto ASKEY
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;   // DNS primario
extern IPAddress secondaryDNS; // DNS secundario

extern bool evitarDeepSleep;
extern bool ActivaLuz;
extern bool EstadoDeseadoLuz; // Estado que Alexa quiere (true=encender, false=apagar)
extern bool telnetHabilitado;
extern AlexaDebugState alexaDebugState;
extern String alexaUltimoDispositivo;
extern String alexaUltimaAccion;
extern String alexaUltimoDetalle;
extern unsigned long alexaUltimoCambioMillis;

// Variables para manejar el delay entre ReleLuz
extern bool ReleLuzEncendido;
extern unsigned long DelayLuzLed;

// ✅ AÑADIDO: Control de tiempo no bloqueante para loop() - extern
extern unsigned long previousMillis;
extern const unsigned long loopInterval; // Ejecutar funciones principales cada 100ms

// ✅ CONTROL DE TIEMPO ASÍNCRONO PARA NTP - extern
extern unsigned long previousNTPUpdate;
extern const unsigned long ntpUpdateInterval; // Actualizar NTP cada hora (3600000ms)
extern unsigned long lastNTPAttempt;
extern const unsigned long ntpRetryInterval; // Reintentar NTP cada 30 segundos si falla
extern bool ntpInitialized;
extern bool ntpUpdateInProgress;

extern AsyncWebServer webServer;
extern WiFiClientSecure mqttSecureClient;
extern PubSubClient mqttClient;
extern unsigned long lastMqttRetry;
extern const unsigned long mqttRetryMillis;
extern WiFiServer telnetServer;
extern WiFiClient telnetClient;
extern DebugTelnetLogger debugTelnet;
//////////////////////////////

#define Serial debugTelnet

#endif // Cierra el bloque #ifndef