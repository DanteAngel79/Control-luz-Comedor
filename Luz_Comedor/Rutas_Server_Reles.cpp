#include "Libreria.h"

// ================================
// FUNCIONES AUXILIARES DE CONTROL
// ================================
// ✅ RUTA AJAX: Devuelve solo el fragmento de estados (streaming)
void rutaEstado(AsyncWebServerRequest *request)
{
    AsyncResponseStream *response = request->beginResponseStream("text/html; charset=utf-8");
    escribirFragmentoEstados(*response);
    request->send(response);
}

// ✅ CONFIGURACIÓN DEL SERVIDOR WEB
void Prepara_Rutas()
{
    // Ruta principal
    webServer.on("/", mostrarInterfazHTML);
    
    // ✅ RUTA DINÁMICA PARA FRAGMENTO DE ESTADOS
    webServer.on("/estado", rutaEstado);
    
    // Rutas de control de relés (AsyncWebServer)
    webServer.on("/encenderLuz", rutaEncenderLed);
    webServer.on("/apagarLuz", rutaApagarLed);
    webServer.on("/wol", rutaWakeOnLan);
    webServer.on("/PcsOff", rutaApagaComputador);
    webServer.on("/reset", rutaReset);
    webServer.on("/actualizaHora", rutaActuaHora);
    webServer.on("/toggleTelnet", rutaToggleTelnet);
    webServer.on("/banderaLuz", [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(Bandera_Rele_luz));
    });

    webServer.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    webServer.begin();
    Serial.println("[WEB] Servidor web iniciado en puerto 80");
    Serial.print("[WEB] Acceso web: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
}

// Función que manejará la ruta raíz "/"
void mostrarInterfazHTML(AsyncWebServerRequest *request)
{
    AsyncResponseStream *response = request->beginResponseStream("text/html; charset=utf-8");
    escribirInterfazHTML(*response);
    request->send(response);
}

// ================================
// RUTAS WEB ASYNC (Corregidas para AsyncWebServer)
// ================================

void rutaWakeOnLan(AsyncWebServerRequest *request) {
    Serial.println("========================================");
    Serial.println("[WOL] Ruta /wol activada");
    
    // Verificar que la petición no sea nula
    if (!request) {
        Serial.println("[ERROR] Request nulo en rutaWakeOnLan");
        return;
    }
    
    String mac = "";
    if (request->hasParam("mac")) {
        mac = request->getParam("mac")->value();
        mac.trim(); // Eliminar espacios
        Serial.println("[WOL] Parámetro MAC recibido: '" + mac + "'");
    } else {
        Serial.println("[WOL] ERROR: No se recibió parámetro MAC");
        request->send(400, "text/plain", "Falta parametro MAC");
        return;
    }
    
    // Verificación más estricta para evitar crashes
    if (esMacHexValida(mac)) {
        Serial.print("[WOL] MAC válida, longitud: ");
        Serial.println(mac.length());
        Serial.print("[WOL] Ejecutando WOL para MAC: ");
        Serial.println(mac);
        
        enviarWoL(mac);
        
        Serial.println("[WOL] ✅ Comando WOL ejecutado, enviando respuesta OK");
        request->send(200, "text/plain", "WOL_OK");
    } else {
        Serial.println("[WOL] ❌ MAC inválida, longitud: " + String(mac.length()));
        request->send(400, "text/plain", "MAC invalida");
    }
    Serial.println("========================================");
}

void programarApagadoDiferido(const String &mac, const String &origen)
{
    macApagadoPendiente = mac;
    apagadoPendiente = true;
    tiempoApagado = millis() + 1000;

    Serial.println(String("[APAGAR] Apagado diferido programado desde ") + origen + " para MAC: " + mac);
    Serial.println("[APAGAR] Ejecucion programada en 1 segundo");
}

void rutaApagaComputador(AsyncWebServerRequest *request) {
    Serial.println("========================================");
    Serial.println("[APAGAR] Ruta /PcsOff activada");
    
    // Verificar que la petición no sea nula
    if (!request) {
        Serial.println("[ERROR] Request nulo en rutaApagaComputador");
        return;
    }
    
    String mac = "";
    if (request->hasParam("mac")) {
        mac = request->getParam("mac")->value();
        mac.trim(); // Eliminar espacios
        Serial.println("[APAGAR] Parámetro MAC recibido: '" + mac + "'");
    } else {
        Serial.println("[APAGAR] ERROR: No se recibió parámetro MAC");
        request->send(400, "text/plain", "Falta parametro MAC");
        return;
    }
    
    // Verificación más estricta
    if (esMacHexValida(mac)) {
        Serial.print("[APAGAR] MAC válida, longitud: ");
        Serial.println(mac.length());
        Serial.print("[APAGAR] Programando apagado diferido para MAC: ");
        Serial.println(mac);
        
        // ✅ RESPONDER INMEDIATAMENTE PARA EVITAR STACK OVERFLOW
        request->send(200, "text/plain", "APAGAR_OK");
        
        // ✅ PROGRAMAR APAGADO DIFERIDO PARA EVITAR CONFLICTO HTTP
        programarApagadoDiferido(mac, "web");

        Serial.println("[APAGAR] OK Apagado programado para ejecutarse en 1 segundo");
        Serial.println("[APAGAR] Conexión HTTP cerrada, esperando momento seguro...");
    } else {
        Serial.println("[APAGAR] ❌ MAC inválida, longitud: " + String(mac.length()));
        request->send(400, "text/plain", "MAC invalida");
    }
    Serial.println("========================================");
}

// ================================
// FUNCIONES AUXILIARES DE CONTROL LED
// ================================

void ejecutarEncenderLed()
{
    estadoDelLED = true;
    digitalWrite(ReleLuz, LOW);  // LOW activa el relé y enciende la luz
    Bandera_Rele_luz = 1;
    sincronizarEstadoAlexaLuz();
    publishMqttDeviceState("luz_tele", true);
    DelayLuzLed = millis();
    estado = false;
    ReleLuzEncendido = true;
    registrarIntervencionRemota();
    Serial.println("[DEBUG] handleLuz: ENCENDIDA - bloqueo remoto activado");
}

void ejecutarApagarLed()
{
    estadoDelLED = false;
    digitalWrite(ReleLuz, HIGH); // HIGH desactiva el relé y apaga la luz
    Bandera_Rele_luz = 0;
    sincronizarEstadoAlexaLuz();
    publishMqttDeviceState("luz_tele", false);
    DelayLuzLed = millis();
    estado = true;
    ReleLuzEncendido = false;
    registrarIntervencionRemota();
    Serial.println("[DEBUG] handleLuz: APAGADA - bloqueo remoto activado");
}

// ================================
// RUTAS WEB (AJAX - Responden JSON)
// ================================

void rutaEncenderLed(AsyncWebServerRequest *request)
{
    ejecutarEncenderLed();
    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"luz_encendida\"}");
}

void rutaApagarLed(AsyncWebServerRequest *request)
{
    ejecutarApagarLed();
    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"luz_apagada\"}");
}

void rutaReset(AsyncWebServerRequest *request)
{
    Serial.println("[RESET] Reinicio del sistema solicitado desde interfaz web");
    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"reiniciando\"}");
    
    // Programar reinicio diferido para evitar problemas con AsyncWebServer
    sincronizacionPendiente = false; // Cancelar cualquier sincronización pendiente
    tiempoReset = millis() + 2000;   // Reiniciar en 2 segundos
    resetPendiente = true;
}

void rutaToggleTelnet(AsyncWebServerRequest *request)
{
    toggleTelnet();
    request->send(200, "application/json", String("{\"status\":\"ok\",\"telnet\":\"") + obtenerEstadoTelnet() + "\"}");
}

// ================================
// SINCRONIZACIÓN NTP DIFERIDA
// ================================

void rutaActuaHora(AsyncWebServerRequest *request)
{
    Serial.println("========================================");
    Serial.println("[WEB] Ruta ActualizaHora activada desde interfaz web");
    Serial.println("[NTP] Programando sincronización diferida...");

    // Mostrar hora antes de la actualización
    Serial.println("[NTP] Hora antes de actualizar: " + hora);
    Serial.println("[NTP] Variables tiempo antes: h=" + String(h) + ", m=" + String(m) + ", s=" + String(s));

    // ✅ RESPONDER INMEDIATAMENTE JSON PARA AJAX
    request->send(200, "application/json", "{\"status\":\"ok\",\"action\":\"sync_programado\"}");

    // ✅ PROGRAMAR SINCRONIZACIÓN PARA DESPUÉS DE CERRAR LA CONEXIÓN HTTP
    sincronizacionPendiente = true;
    tiempoSincronizacion = millis() + 2000; // Ejecutar en 2 segundos

    Serial.println("[NTP] ✅ Sincronización programada para ejecutarse en 2 segundos");
    Serial.println("[NTP] Conexión HTTP cerrada, esperando momento seguro...");
    Serial.println("========================================");
}

// Función para ejecutar la sincronización diferida (DEBE LLAMARSE DESDE loop())
void procesarSincronizacionDiferida()
{
    if (sincronizacionPendiente && millis() >= tiempoSincronizacion)
    {
        Serial.println("========================================");
        Serial.println("[NTP] ⏰ Ejecutando sincronización diferida (conexión HTTP cerrada)");

        // ✅ SINCRONIZACIÓN COMPLETA: Actualizar MES y HORA
        Serial.println("[NTP] Actualizando mes desde servidor NTP...");
        printLocalTime(); // Esto actualiza MesCortado y llama convierteMes()
        
        Serial.println("[NTP] Ejecutando función Hrs() sin interferencia HTTP...");
        Hrs(); // Esto actualiza la hora

        // Actualizar reloj virtual
        RelojDigital();
        hora = horaVirtual;

        // Mostrar resultado
        Serial.println("[NTP] ✅ Sincronización diferida completada exitosamente");
        Serial.println("[NTP] Hora actualizada: " + hora);
        Serial.println("[NTP] Variables tiempo después: h=" + String(h) + ", m=" + String(m) + ", s=" + String(s));
        Serial.println("[NTP] Mes actual: " + MesCortado + " (" + String(MesActual) + ")");
        Serial.println("========================================");

        // Marcar como completada
        sincronizacionPendiente = false;
    }
    
    // ✅ PROCESAR APAGADO DIFERIDO PARA EVITAR STACK OVERFLOW
    if (apagadoPendiente && millis() >= tiempoApagado)
    {
        Serial.println("========================================");
        Serial.println("[APAGAR] ⏰ Ejecutando apagado diferido (conexión HTTP cerrada)");
        Serial.println("[APAGAR] Ejecutando apagado para MAC: " + macApagadoPendiente);
        
        enviarPeticionApagarComputador(macApagadoPendiente);
        
        Serial.println("[APAGAR] Solicitud entregada al procesador HTTP no bloqueante");
        Serial.println("========================================");
        
        // Marcar como completado
        apagadoPendiente = false;
        macApagadoPendiente = "";
    }
    
    // Procesar reinicio diferido
    if (resetPendiente && millis() >= tiempoReset)
    {
        Serial.println("[RESET] Ejecutando reinicio diferido...");
        ESP.restart();
    }
}

// ================================
// CONTROL ALEXA (MANTIENE LÓGICA ORIGINAL)
// ================================

void handleLuz()
{
    // Solo ejecuta si ActivaLuz está activa
    if (!ActivaLuz)
        return;

    // Usa el estado deseado de Alexa en lugar del estado actual
    if (EstadoDeseadoLuz == true)
    {
        ejecutarEncenderLed();
        actualizarEstadoAlexa(ALEXA_DEBUG_ACTION_DONE, "Luz Tele", "encender", "Accion aplicada sobre rele");
    }
    else
    {
        ejecutarApagarLed();
        actualizarEstadoAlexa(ALEXA_DEBUG_ACTION_DONE, "Luz Tele", "apagar", "Accion aplicada sobre rele");
    }

    // Después de procesar la acción, desactiva el flag
    ActivaLuz = false;
}

void handlePC_Piso1() {
    // Solo ejecuta si la activación está pendiente
    if (!Activa_Pc_Piso1) return;
    
    if (EstadoDeseadoPc_Piso1 == true) {
        enviarWoL(PCS[0].mac);  // Computador Primer Piso
    } else {
        programarApagadoDiferido(PCS[0].mac, "alexa");  // Computador Primer Piso
    }
    
    // Después de procesar la acción, desactiva el flag
    Activa_Pc_Piso1 = false;
}

void handlePC_Piso2() {
    // Solo ejecuta si la activación está pendiente
    if (!Activa_Pc_Piso2) return;
    
    if (EstadoDeseadoPc_Piso2 == true) {
        enviarWoL(PCS[1].mac);  // Computador Segundo Piso
    } else {
        programarApagadoDiferido(PCS[1].mac, "alexa");  // Computador Segundo Piso
    }
    
    // Después de procesar la acción, desactiva el flag
    Activa_Pc_Piso2 = false;
}

// ================================
// RUTA PARA RESETEAR MANUALMENTE EL PULSADOR (PRUEBA)
// ================================