#include "Libreria.h"

static const char *nombreEstadoAlexa(AlexaDebugState estado)
{
  switch (estado) {
    case ALEXA_DEBUG_IDLE:
      return "IDLE";
    case ALEXA_DEBUG_VOICE_COMMAND:
      return "VOICE_COMMAND";
    case ALEXA_DEBUG_ACTION_PENDING:
      return "ACTION_PENDING";
    case ALEXA_DEBUG_ACTION_DONE:
      return "ACTION_DONE";
    case ALEXA_DEBUG_SYNC:
      return "SYNC";
    case ALEXA_DEBUG_ERROR:
      return "ERROR";
    case ALEXA_DEBUG_TRACE:
      return "TRACE";
    default:
      return "UNKNOWN";
  }
}

static AlexaDebugState estadoDesdeTraza(const String &linea)
{
  String mayus = linea;
  mayus.toUpperCase();

  if (mayus.indexOf("ERROR") >= 0 || mayus.indexOf("FALLO") >= 0 || mayus.indexOf("❌") >= 0) {
    return ALEXA_DEBUG_ERROR;
  }

  return ALEXA_DEBUG_TRACE;
}

static String categoriaDesdeTraza(const String &linea)
{
  if (linea.startsWith("[")) {
    int cierre = linea.indexOf(']');
    if (cierre > 1) {
      return linea.substring(1, cierre);
    }
  }

  return "SYSTEM";
}

void DebugTelnetLogger::begin(unsigned long baudRate)
{
  (void)baudRate;
}

size_t DebugTelnetLogger::print(const String &value)
{
  return writeText(value, false);
}

size_t DebugTelnetLogger::print(const char *value)
{
  return writeText(value == nullptr ? String("") : String(value), false);
}

size_t DebugTelnetLogger::print(char value)
{
  return writeText(String(value), false);
}

size_t DebugTelnetLogger::print(int value)
{
  return writeText(String(value), false);
}

size_t DebugTelnetLogger::print(unsigned int value)
{
  return writeText(String(value), false);
}

size_t DebugTelnetLogger::print(long value)
{
  return writeText(String(value), false);
}

size_t DebugTelnetLogger::print(unsigned long value)
{
  return writeText(String(value), false);
}

size_t DebugTelnetLogger::print(bool value)
{
  return writeText(value ? "true" : "false", false);
}

size_t DebugTelnetLogger::print(const IPAddress &value)
{
  return writeText(value.toString(), false);
}

size_t DebugTelnetLogger::println()
{
  return writeText("", true);
}

int DebugTelnetLogger::printf(const char *format, ...)
{
  char buffer[256];
  va_list args;
  va_start(args, format);
  int written = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  if (written < 0) {
    return written;
  }

  writeText(String(buffer), false);
  return written;
}

size_t DebugTelnetLogger::writeText(const String &value, bool appendNewLine)
{
  for (size_t i = 0; i < value.length(); i++) {
    char current = value.charAt(i);

    if (current == '\r') {
      flushLine();
      continue;
    }

    if (current == '\n') {
      flushLine();
      continue;
    }

    lineBuffer += current;
  }

  if (appendNewLine) {
    flushLine();
  }

  return value.length() + (appendNewLine ? 1 : 0);
}

void DebugTelnetLogger::flushLine()
{
  if (lineBuffer.length() == 0) {
    return;
  }

  registrarTrazaTelnet(lineBuffer);
  lineBuffer = "";
}

static void enviarLineaTelnet(const String &linea)
{
  if (!telnetHabilitado || !telnetClient || !telnetClient.connected()) {
    return;
  }

  telnetClient.println(linea);
}

void setupTelnet()
{
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  actualizarEstadoAlexa(ALEXA_DEBUG_IDLE, "sistema", "telnet-ready", "Puerto 23 disponible");
}

void handleTelnet()
{
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (telnetServer.hasClient()) {
    WiFiClient nuevoCliente = telnetServer.available();

    if (!telnetHabilitado) {
      nuevoCliente.println("Telnet deshabilitado");
      nuevoCliente.stop();
    } else {
      if (telnetClient && telnetClient.connected()) {
        telnetClient.println("Nueva sesion Telnet detectada. Cerrando sesion actual.");
        telnetClient.stop();
      }

      telnetClient = nuevoCliente;
      telnetClient.println("Alexa monitor activo");
      telnetClient.println(String("Estado actual: ") + nombreEstadoAlexa(alexaDebugState));
      telnetClient.println(String("Dispositivo: ") + alexaUltimoDispositivo + " | Accion: " + alexaUltimaAccion);
      telnetClient.println(String("Detalle: ") + alexaUltimoDetalle);
    }
  }

  if (telnetClient && !telnetClient.connected()) {
    telnetClient.stop();
  }

  while (telnetClient && telnetClient.available()) {
    telnetClient.read();
  }
}

void setTelnetHabilitado(bool habilitado)
{
  telnetHabilitado = habilitado;

  if (!telnetHabilitado && telnetClient) {
    telnetClient.println("Telnet deshabilitado desde servidor web");
    telnetClient.stop();
  }

  actualizarEstadoAlexa(ALEXA_DEBUG_IDLE, "sistema", habilitado ? "telnet-on" : "telnet-off", habilitado ? "Monitoreo remoto habilitado" : "Monitoreo remoto deshabilitado");
}

void toggleTelnet()
{
  setTelnetHabilitado(!telnetHabilitado);
}

String obtenerEstadoTelnet()
{
  return telnetHabilitado ? "ACTIVO" : "INACTIVO";
}

void actualizarEstadoAlexa(AlexaDebugState nuevoEstado, const String &dispositivo, const String &accion, const String &detalle)
{
  alexaDebugState = nuevoEstado;
  alexaUltimoDispositivo = dispositivo;
  alexaUltimaAccion = accion;
  alexaUltimoDetalle = detalle;
  alexaUltimoCambioMillis = millis();

  String linea = "[ALEXA-STATE] ";
  linea += nombreEstadoAlexa(nuevoEstado);
  linea += " | dispositivo=" + dispositivo;
  linea += " | accion=" + accion;
  if (detalle.length() > 0) {
    linea += " | detalle=" + detalle;
  }

  enviarLineaTelnet(linea);
}

void registrarEventoAlexa(const String &dispositivo, const String &accion, const String &detalle)
{
  actualizarEstadoAlexa(ALEXA_DEBUG_VOICE_COMMAND, dispositivo, accion, detalle);
}

void registrarTrazaTelnet(const String &linea)
{
  String limpia = linea;
  limpia.trim();

  if (limpia.length() == 0) {
    return;
  }

  if (limpia == "========================================") {
    enviarLineaTelnet(limpia);
    return;
  }

  AlexaDebugState estado = estadoDesdeTraza(limpia);
  String categoria = categoriaDesdeTraza(limpia);
  actualizarEstadoAlexa(estado, categoria, "trace", limpia);
}

void sincronizarEstadoAlexaLuz(bool forzar)
{
  static bool estadoAnterior = false;
  static bool inicializado = false;

  if (!forzar && inicializado && estadoAnterior == estadoDelLED) {
    return;
  }

  actualizarEstadoAlexa(ALEXA_DEBUG_SYNC, "Luz Tele", estadoDelLED ? "sync-on" : "sync-off", forzar ? "Sincronizacion MQTT forzada" : "Sincronizacion MQTT automatica");
  estadoAnterior = estadoDelLED;
  inicializado = true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("[OTA] Iniciando actualización " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] Actualización completada");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progreso: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Fallo de autenticación");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Fallo al iniciar");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Fallo de conexión");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Fallo al recibir");
    } else if (error == OTA_END_ERROR) {
      Serial.println("Fallo al finalizar");
    }
  });
  ArduinoOTA.begin();
  Serial.println("[OTA] OTA habilitado con hostname: " + String(OTA_HOSTNAME));
}