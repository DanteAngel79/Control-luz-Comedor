#include "Libreria.h"

const char *MQTT_HOST = "AQUI_TU_HOST_MQTT";
const uint16_t MQTT_PORT = 8883;
const char *MQTT_USER = "AQUI_TU_USUARIO_MQTT";
const char *MQTT_PASS = "AQUI_TU_PASSWORD_MQTT";
const char *MQTT_TOPIC_CMD_WILDCARD = "alexa/cmd/+";
const char *MQTT_TOPIC_STATE_PREFIX = "alexa/state/";

static void connectMqttAlexa();
static void mqttAlexaCallback(char *topic, byte *payload, unsigned int length);
static void handleMqttDeviceCommand(const String &deviceId, const String &command);
static bool isTurnOnCommand(const String &command);
static bool isTurnOffCommand(const String &command);

void setupMqttAlexa()
{
  mqttSecureClient.setInsecure();
  mqttSecureClient.setBufferSizes(4096, 512);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttAlexaCallback);
  mqttClient.setSocketTimeout(2);
}

void handleMqttAlexa()
{
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  connectMqttAlexa();

  if (mqttClient.connected()) {
    mqttClient.loop();
  }
}

void publishMqttDeviceState(const char *deviceId, bool powerOn)
{
  if (!mqttClient.connected() || deviceId == nullptr) {
    return;
  }

  char topic[64];
  snprintf(topic, sizeof(topic), "%s%s", MQTT_TOPIC_STATE_PREFIX, deviceId);

  char payload[160];
  snprintf(
    payload,
    sizeof(payload),
    "{\"deviceId\":\"%s\",\"power\":\"%s\",\"source\":\"esp8266-comedor\"}",
    deviceId,
    powerOn ? "ON" : "OFF"
  );

  mqttClient.publish(topic, payload, true);
}

void publishAllMqttStates()
{
  publishMqttDeviceState("luz_tele", estadoDelLED);
  publishMqttDeviceState("pc_intel", EstadoActualPc_Piso1);
  publishMqttDeviceState("pc_amd", EstadoActualPc_Piso2);
}

static void connectMqttAlexa()
{
  if (mqttClient.connected()) {
    return;
  }

  unsigned long ahora = millis();
  if (lastMqttRetry != 0 && ahora - lastMqttRetry < mqttRetryMillis) {
    return;
  }

  lastMqttRetry = ahora;

  String clientId = String("comedor-") + String(ESP.getChipId(), HEX);
  bool conectado;

  if (strlen(MQTT_USER) > 0) {
    conectado = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
  } else {
    conectado = mqttClient.connect(clientId.c_str());
  }

  if (!conectado) {
    Serial.print("[MQTT] Desconectado. rc=");
    Serial.println(mqttClient.state());
    return;
  }

  mqttClient.subscribe(MQTT_TOPIC_CMD_WILDCARD);
  publishAllMqttStates();
  Serial.println("[MQTT] Conectado y suscrito a alexa/cmd/+");
}

static void mqttAlexaCallback(char *topic, byte *payload, unsigned int length)
{
  String topicString = topic == nullptr ? "" : String(topic);
  String command;
  command.reserve(length);

  for (unsigned int index = 0; index < length; index++) {
    command += static_cast<char>(payload[index]);
  }

  command.trim();

  const String prefix = "alexa/cmd/";
  if (!topicString.startsWith(prefix)) {
    Serial.println(String("[MQTT] Topic ignorado: ") + topicString);
    return;
  }

  String deviceId = topicString.substring(prefix.length());

  Serial.print("[MQTT] RX [");
  Serial.print(topicString);
  Serial.print("]: ");
  Serial.println(command);

  handleMqttDeviceCommand(deviceId, command);
}

static void handleMqttDeviceCommand(const String &deviceId, const String &command)
{
  if (command.equalsIgnoreCase("STATE")) {
    publishAllMqttStates();
    return;
  }

  bool turnOn = isTurnOnCommand(command);
  bool turnOff = isTurnOffCommand(command);

  if (!turnOn && !turnOff) {
    Serial.println(String("[MQTT] Comando no soportado: ") + command);
    return;
  }

  bool desiredState = turnOn;

  if (deviceId == "luz_tele") {
    registrarEventoAlexa("Luz Tele", desiredState ? "mqtt-on" : "mqtt-off", "Skill Smart Home");
    ActivaLuz = true;
    EstadoDeseadoLuz = desiredState;
    actualizarEstadoAlexa(ALEXA_DEBUG_ACTION_PENDING, "Luz Tele", desiredState ? "pendiente-on" : "pendiente-off", "MQTT");
    return;
  }

  if (deviceId == "pc_intel") {
    registrarEventoAlexa(PCS[0].nombreAlexa, desiredState ? "mqtt-on" : "mqtt-off", "Skill Smart Home");
    Activa_Pc_Piso1 = true;
    EstadoDeseadoPc_Piso1 = desiredState;
    actualizarEstadoAlexa(ALEXA_DEBUG_ACTION_PENDING, PCS[0].nombreAlexa, desiredState ? "pendiente-on" : "pendiente-off", "MQTT");
    return;
  }

  if (deviceId == "pc_amd") {
    registrarEventoAlexa(PCS[1].nombreAlexa, desiredState ? "mqtt-on" : "mqtt-off", "Skill Smart Home");
    Activa_Pc_Piso2 = true;
    EstadoDeseadoPc_Piso2 = desiredState;
    actualizarEstadoAlexa(ALEXA_DEBUG_ACTION_PENDING, PCS[1].nombreAlexa, desiredState ? "pendiente-on" : "pendiente-off", "MQTT");
    return;
  }

  Serial.println(String("[MQTT] Dispositivo desconocido: ") + deviceId);
}

static bool isTurnOnCommand(const String &command)
{
  return command.equalsIgnoreCase("ON") || command.indexOf("TurnOn") >= 0;
}

static bool isTurnOffCommand(const String &command)
{
  return command.equalsIgnoreCase("OFF") || command.indexOf("TurnOff") >= 0;
}
