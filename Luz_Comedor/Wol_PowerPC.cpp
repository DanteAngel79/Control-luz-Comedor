#include "Libreria.h"

enum EstadoApagadoHttp {
  APAGADO_HTTP_IDLE = 0,
  APAGADO_HTTP_DRENANDO = 1
};

static WiFiClient clienteApagadoHttp;
static const PcConfig *pcApagadoPendiente = nullptr;
static String macApagadoHttp = "";
static String rutaApagadoHttp = "/";
static uint16_t puertoApagadoHttp = 80;
static unsigned long deadlineApagadoHttpMillis = 0;
static int estadoApagadoHttp = APAGADO_HTTP_IDLE;

static uint16_t extraerPuertoUrl(const char *url) {
  if (url == nullptr) {
    return 80;
  }

  String valor = url;
  int inicioHost = valor.indexOf("://");
  inicioHost = inicioHost >= 0 ? inicioHost + 3 : 0;
  int finHost = valor.indexOf('/', inicioHost);
  if (finHost < 0) {
    finHost = valor.length();
  }

  int separadorPuerto = valor.indexOf(':', inicioHost);
  if (separadorPuerto < 0 || separadorPuerto > finHost) {
    return 80;
  }

  String puertoTexto = valor.substring(separadorPuerto + 1, finHost);
  int puerto = puertoTexto.toInt();
  return puerto > 0 ? static_cast<uint16_t>(puerto) : 80;
}

static String extraerRutaUrl(const char *url) {
  if (url == nullptr) {
    return "/";
  }

  String valor = url;
  int inicioHost = valor.indexOf("://");
  inicioHost = inicioHost >= 0 ? inicioHost + 3 : 0;
  int inicioRuta = valor.indexOf('/', inicioHost);
  if (inicioRuta < 0) {
    return "/";
  }

  return valor.substring(inicioRuta);
}

static void limpiarApagadoHttpPendiente() {
  if (clienteApagadoHttp.connected()) {
    clienteApagadoHttp.stop();
  }
  pcApagadoPendiente = nullptr;
  macApagadoHttp = "";
  rutaApagadoHttp = "/";
  puertoApagadoHttp = 80;
  deadlineApagadoHttpMillis = 0;
  estadoApagadoHttp = APAGADO_HTTP_IDLE;
}

/*
* Send a Wake-On-LAN packet for the given MAC address, to the given IP
* address. Often the IP address will be the local broadcast.
*/
void sendWOL(const IPAddress ip, const byte mac[]) {
  byte preamble[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  int beginPacketOk = udp.beginPacket(ip, 9);
  if (beginPacketOk != 1) {
    Serial.println("[WOL] ERROR beginPacket fallo");
    return;
  }

  size_t totalEscrito = udp.write(preamble, 6);
  for (uint8_t i = 0; i < 16; i++) {
    totalEscrito += udp.write(mac, 6);
  }

  int endPacketOk = udp.endPacket();
  String macHex = "";
  for (uint8_t i = 0; i < 6; i++) {
    if (mac[i] < 0x10) {
      macHex += '0';
    }
    macHex += String(mac[i], HEX);
    if (i < 5) {
      macHex += ':';
    }
  }
  macHex.toUpperCase();

  Serial.printf("[WOL] UDP debug begin=%d bytes=%u end=%d target=%s:9 mac=%s\n",
                beginPacketOk,
                static_cast<unsigned int>(totalEscrito),
                endPacketOk,
                ip.toString().c_str(),
                macHex.c_str());

  if (totalEscrito != 102 || endPacketOk != 1) {
    Serial.println("[WOL] ERROR paquete UDP incompleto o no confirmado");
  }
  // Delay eliminado para evitar bloqueos en callbacks web
}


byte valFromChar(char c) {
  if (c >= 'a' && c <= 'f') return ((byte)(c - 'a') + 10) & 0x0F;
  if (c >= 'A' && c <= 'F') return ((byte)(c - 'A') + 10) & 0x0F;
  if (c >= '0' && c <= '9') return ((byte)(c - '0')) & 0x0F;
  return 0;
}

bool esMacHexValida(const String &mac) {
  if (mac.length() != 12) {
    return false;
  }

  for (int i = 0; i < mac.length(); i++) {
    char c = mac.charAt(i);
    bool esHex = (c >= '0' && c <= '9') ||
                 (c >= 'a' && c <= 'f') ||
                 (c >= 'A' && c <= 'F');
    if (!esHex) {
      return false;
    }
  }

  return true;
}

static const PcConfig *obtenerPcPorMac(const String &mac)
{
  for (size_t index = 0; index < PC_COUNT; index++) {
    if (mac == PCS[index].mac) {
      return &PCS[index];
    }
  }

  return nullptr;
}

static bool *obtenerEstadoActualPc(const PcConfig &pc)
{
  if (&pc == &PCS[0]) {
    return &EstadoActualPc_Piso1;
  }

  if (&pc == &PCS[1]) {
    return &EstadoActualPc_Piso2;
  }

  return nullptr;
}

enum EstadoPingPc {
  PING_PCS_IDLE = 0,
  PING_PCS_PC1 = 1,
  PING_PCS_PC2 = 2
};

void actualizarEstadoAlexaPC(const String &mac, bool encendido) {
  const PcConfig *pc = obtenerPcPorMac(mac);

  if (pc == nullptr) {
    return;
  }

  bool *estadoActualPc = obtenerEstadoActualPc(*pc);
  if (estadoActualPc != nullptr) {
    *estadoActualPc = encendido;
  }

  publishMqttDeviceState(pc == &PCS[0] ? "pc_intel" : "pc_amd", encendido);
  actualizarEstadoAlexa(ALEXA_DEBUG_SYNC, pc->nombreAlexa, encendido ? "sync-on" : "sync-off", "Estado publicado por MQTT");
}

void actualizarEstadoAlexaPCs() {
  for (size_t index = 0; index < PC_COUNT; index++) {
    bool *estadoActualPc = obtenerEstadoActualPc(PCS[index]);
    bool encendido = estadoActualPc != nullptr ? *estadoActualPc : false;
    publishMqttDeviceState(index == 0 ? "pc_intel" : "pc_amd", encendido);
  }
}

void procesarPingPCs() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  unsigned long ahora = millis();

  if (estadoPingPCs == PING_PCS_IDLE) {
    bool correspondePing = pingInicialPendiente || (ahora - ultimoPingPCsMillis >= pingIntervalPCs);
    if (correspondePing) {
      estadoPingPCs = PING_PCS_PC1;
      siguientePasoPingMillis = ahora;
      pingInicialPendiente = false;
      Serial.println("[PING] Iniciando ciclo de verificación de PCs");
    }
    return;
  }

  if (ahora < siguientePasoPingMillis) {
    return;
  }

  if (estadoPingPCs == PING_PCS_PC1) {
    bool encendido = Ping.ping(PCS[0].ip, 1);
    actualizarEstadoAlexaPC(PCS[0].mac, encendido);
    Serial.println(String("[PING] ") + PCS[0].nombreAlexa + ": " + (encendido ? "RESPONDE" : "SIN RESPUESTA"));
    estadoPingPCs = PING_PCS_PC2;
    siguientePasoPingMillis = ahora + pingStepDelayPCs;
    return;
  }

  if (estadoPingPCs == PING_PCS_PC2) {
    bool encendido = Ping.ping(PCS[1].ip, 1);
    actualizarEstadoAlexaPC(PCS[1].mac, encendido);
    Serial.println(String("[PING] ") + PCS[1].nombreAlexa + ": " + (encendido ? "RESPONDE" : "SIN RESPUESTA"));
    estadoPingPCs = PING_PCS_IDLE;
    ultimoPingPCsMillis = ahora;
    Serial.println("[PING] Ciclo de verificación de PCs completado");
  }
}

/*
* Very simple converter from a String representation of a MAC address to
* 6 bytes. Does not handle errors or delimiters, but requires very little
* code space and no libraries.
*/
void macStringToBytes(const String mac, byte *bytes) {
  if (esMacHexValida(mac)) {
    for (int i = 0; i < 6; i++) {
      bytes[i] = (valFromChar(mac.charAt(i * 2)) << 4) | valFromChar(mac.charAt(i * 2 + 1));
    }
  } else {
    Serial.println("Incorrect MAC format.");
  }
}


void enviarPeticionApagarComputador(String mac) {
  const PcConfig *pc = obtenerPcPorMac(mac);

  if (WiFi.status() != WL_CONNECTED) {
    actualizarEstadoAlexa(ALEXA_DEBUG_ERROR, "PC", "shutdown-error", "WiFi no conectado");
    Serial.println("[ERROR] WiFi no conectado, no se puede apagar computador");
    return;
  }

  if (pc == nullptr) {
    actualizarEstadoAlexa(ALEXA_DEBUG_ERROR, mac, "shutdown-error", "MAC no reconocida");
    Serial.println("[ERROR] MAC address not recognized for shutdown: " + mac);
    return;
  }

  if (estadoApagadoHttp != APAGADO_HTTP_IDLE) {
    actualizarEstadoAlexa(ALEXA_DEBUG_ERROR, pc->nombreAlexa, "apagar-error", "Ya hay un apagado en curso");
    Serial.println("[HTTP] ERROR ya hay una peticion de apagado en curso");
    return;
  }

  macApagadoHttp = mac;
  pcApagadoPendiente = pc;
  rutaApagadoHttp = extraerRutaUrl(pc->urlApagado);
  puertoApagadoHttp = extraerPuertoUrl(pc->urlApagado);
  deadlineApagadoHttpMillis = millis() + 750;
  estadoApagadoHttp = APAGADO_HTTP_DRENANDO;

  Serial.println(String("[HTTP] Iniciando peticion de apagado no bloqueante a: ") + pc->urlApagado);
  yield();

  clienteApagadoHttp.stop();
  clienteApagadoHttp.setNoDelay(true);

  if (!clienteApagadoHttp.connect(pc->ip, puertoApagadoHttp)) {
    actualizarEstadoAlexa(ALEXA_DEBUG_ERROR, pc->nombreAlexa, "apagar-error", "No se pudo abrir socket HTTP");
    Serial.println("[HTTP] ERROR no se pudo abrir socket al servidor remoto");
    limpiarApagadoHttpPendiente();
    return;
  }

  String hostHeader = pc->ip.toString() + ":" + String(puertoApagadoHttp);
  String request = String("GET ") + rutaApagadoHttp + " HTTP/1.1\r\n"
      + "Host: " + hostHeader + "\r\n"
      + "Connection: close\r\n"
      + "User-Agent: ESP8266-WOL\r\n\r\n";
  size_t bytesEnviados = clienteApagadoHttp.print(request);
  yield();

  if (bytesEnviados != request.length()) {
    actualizarEstadoAlexa(ALEXA_DEBUG_ERROR, pc->nombreAlexa, "apagar-error", "No se envio la solicitud completa");
    Serial.printf("[HTTP] ERROR bytes enviados incompletos: %u/%u\n",
                  static_cast<unsigned int>(bytesEnviados),
                  static_cast<unsigned int>(request.length()));
    limpiarApagadoHttpPendiente();
    return;
  }

  actualizarEstadoAlexaPC(mac, false);
  actualizarEstadoAlexa(ALEXA_DEBUG_ACTION_DONE, pc->nombreAlexa, "apagar", "Solicitud HTTP enviada sin bloqueo");
  ultimoPingPCsMillis = millis();
  Serial.println("[HTTP] Solicitud de apagado enviada; la respuesta se drenara en background");
}

void procesarApagadoHttpPendiente() {
  if (estadoApagadoHttp == APAGADO_HTTP_IDLE) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    actualizarEstadoAlexa(ALEXA_DEBUG_ERROR,
                          pcApagadoPendiente == nullptr ? "PC" : pcApagadoPendiente->nombreAlexa,
                          "apagar-error",
                          "WiFi desconectado durante apagado");
    Serial.println("[HTTP] ERROR WiFi desconectado durante el apagado remoto");
    limpiarApagadoHttpPendiente();
    return;
  }

  while (clienteApagadoHttp.available() > 0) {
    clienteApagadoHttp.read();
    yield();
  }

  if (!clienteApagadoHttp.connected() || millis() >= deadlineApagadoHttpMillis) {
    Serial.println("[HTTP] Limpieza de socket de apagado completada");
    limpiarApagadoHttpPendiente();
  }
}

void enviarWoL(String mac) {
  // Verificar conexión WiFi antes de enviar WOL
  if (WiFi.status() != WL_CONNECTED) {
    actualizarEstadoAlexa(ALEXA_DEBUG_ERROR, "PC", "wol-error", "WiFi no conectado");
    Serial.println("[ERROR] WiFi no conectado, no se puede enviar WOL");
    return;
  }
  
  IPAddress target_ip = WiFi.localIP();
  target_ip[3] = 255;  // Direccion de broadcast
  
  Serial.println("[WOL] Enviando Wake-on-LAN a: " + mac);
  Serial.println("[WOL] IP de broadcast: " + target_ip.toString());

  byte target_mac[6];
  macStringToBytes(mac, target_mac);
  const PcConfig *pc = obtenerPcPorMac(mac);

  // Enviar el paquete WOL
  sendWOL(target_ip, target_mac);
  actualizarEstadoAlexaPC(mac, true);
  actualizarEstadoAlexa(ALEXA_DEBUG_ACTION_DONE, pc == nullptr ? "PC" : pc->nombreAlexa, "encender", "Paquete WOL enviado");
  ultimoPingPCsMillis = millis();
  
  Serial.println("[WOL] OK Paquete WOL enviado");
}

