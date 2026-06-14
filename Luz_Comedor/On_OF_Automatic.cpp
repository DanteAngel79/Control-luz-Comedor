#include "Libreria.h"

// Variables globales para controlar el timing de 5 minutos para recuperación automática
unsigned long previousMillisRecuperacion = 0;
const unsigned long interval = 300000; // 5 minutos en milisegundos

namespace {

enum BloqueoRecuperacionTipo {
  BLOQUEO_RECUPERACION_NINGUNO = 0,
  BLOQUEO_RECUPERACION_ENCENDIDO = 1,
  BLOQUEO_RECUPERACION_APAGADO = 2
};

enum BloqueoRecuperacionOrigen {
  BLOQUEO_ORIGEN_NINGUNO = 0,
  BLOQUEO_ORIGEN_MANUAL = 1,
  BLOQUEO_ORIGEN_REMOTO = 2
};

BloqueoRecuperacionTipo bloqueoRecuperacionTipo = BLOQUEO_RECUPERACION_NINGUNO;
BloqueoRecuperacionOrigen bloqueoRecuperacionOrigen = BLOQUEO_ORIGEN_NINGUNO;

void sincronizarIndicadoresBloqueo()
{
  Detect_Pulsador = bloqueoRecuperacionTipo != BLOQUEO_RECUPERACION_NINGUNO && bloqueoRecuperacionOrigen == BLOQUEO_ORIGEN_MANUAL;
  bloqueoRecuperacionRemota = bloqueoRecuperacionTipo != BLOQUEO_RECUPERACION_NINGUNO && bloqueoRecuperacionOrigen == BLOQUEO_ORIGEN_REMOTO;
}

void establecerBloqueoRecuperacion(BloqueoRecuperacionTipo nuevoTipo, BloqueoRecuperacionOrigen nuevoOrigen)
{
  bloqueoRecuperacionTipo = nuevoTipo;
  bloqueoRecuperacionOrigen = (nuevoTipo == BLOQUEO_RECUPERACION_NINGUNO) ? BLOQUEO_ORIGEN_NINGUNO : nuevoOrigen;
  recuperacionActiva = (bloqueoRecuperacionTipo == BLOQUEO_RECUPERACION_NINGUNO);
  sincronizarIndicadoresBloqueo();
}

void obtenerHorarioRecuperacionMesActual(String &horaEncendido, String &horaApagado)
{
  horaEncendido = "";
  horaApagado = "";

  switch (MesActual)
  {
  case 1:
    horaEncendido = "21:30:20";
    horaApagado = "01:15:20";
    break;
  case 2:
    horaEncendido = "21:00:20";
    horaApagado = "01:16:20";
    break;
  case 3:
    horaEncendido = "21:15:20";
    horaApagado = "01:17:20";
    break;
  case 4:
    horaEncendido = "18:30:20";
    horaApagado = "01:18:20";
    break;
  case 5:
    horaEncendido = "18:00:20";
    horaApagado = "01:19:20";
    break;
  case 6:
    horaEncendido = "17:45:20";
    horaApagado = "01:20:20";
    break;
  case 7:
    horaEncendido = "18:00:20";
    horaApagado = "01:21:20";
    break;
  case 8:
    horaEncendido = "18:30:20";
    horaApagado = "01:22:20";
    break;
  case 9:
    horaEncendido = "20:10:20";
    horaApagado = "01:23:20";
    break;
  case 10:
    horaEncendido = "20:30:20";
    horaApagado = "01:24:20";
    break;
  case 11:
    horaEncendido = "21:00:20";
    horaApagado = "01:25:20";
    break;
  case 12:
    horaEncendido = "21:15:20";
    horaApagado = "01:26:20";
    break;
  }
}

BloqueoRecuperacionTipo determinarBloqueoRecuperacionActual()
{
  String horaEncendido;
  String horaApagado;
  obtenerHorarioRecuperacionMesActual(horaEncendido, horaApagado);

  if (horaEncendido.length() == 0 || horaApagado.length() == 0)
  {
    return BLOQUEO_RECUPERACION_NINGUNO;
  }

  int horaActualMinutos = convertirHoraAMinutosSilencioso(normalizarHora(hora));
  bool deberiaEstarEncendidaAhora = estaEnRangoEncendidoSilencioso(horaActualMinutos, horaEncendido, horaApagado);

  if (estadoDelLED == deberiaEstarEncendidaAhora)
  {
    return BLOQUEO_RECUPERACION_NINGUNO;
  }

  return estadoDelLED ? BLOQUEO_RECUPERACION_APAGADO : BLOQUEO_RECUPERACION_ENCENDIDO;
}

const char *obtenerNombreBloqueo(BloqueoRecuperacionTipo tipo)
{
  switch (tipo)
  {
  case BLOQUEO_RECUPERACION_ENCENDIDO:
    return "auto-encendido";
  case BLOQUEO_RECUPERACION_APAGADO:
    return "auto-apagado";
  default:
    return "sin bloqueo";
  }
}

const char *obtenerNombreOrigen(BloqueoRecuperacionOrigen origen)
{
  switch (origen)
  {
  case BLOQUEO_ORIGEN_MANUAL:
    return "manual (pulsador)";
  case BLOQUEO_ORIGEN_REMOTO:
    return "server/alexa";
  default:
    return "sin origen";
  }
}

void registrarIntervencionEnRele(BloqueoRecuperacionOrigen origen)
{
  BloqueoRecuperacionTipo nuevoBloqueo = determinarBloqueoRecuperacionActual();
  establecerBloqueoRecuperacion(nuevoBloqueo, origen);

  if (nuevoBloqueo == BLOQUEO_RECUPERACION_NINGUNO)
  {
    Serial.println("[RECUPERACION] Intervencion " + String(obtenerNombreOrigen(origen)) + " sin bloqueo activo: el rele coincide con el horario automatico");
    return;
  }

  Serial.println("[RECUPERACION] Intervencion " + String(obtenerNombreOrigen(origen)) + " bloquea " + String(obtenerNombreBloqueo(nuevoBloqueo)));
}

bool accionAutomaticaBloqueada(BloqueoRecuperacionTipo tipoAccion)
{
  return bloqueoRecuperacionTipo == tipoAccion;
}

void ejecutarAccionAutomatica(bool encender, const String &horaObjetivo)
{
  BloqueoRecuperacionTipo tipoAccion = encender ? BLOQUEO_RECUPERACION_ENCENDIDO : BLOQUEO_RECUPERACION_APAGADO;

  if (accionAutomaticaBloqueada(tipoAccion))
  {
    Serial.println("[AUTO] " + String(encender ? "Encendido" : "Apagado") + " automático omitido a las " + horaObjetivo + " por intervención " + String(obtenerNombreOrigen(bloqueoRecuperacionOrigen)));
    return;
  }

  digitalWrite(ReleLuz, encender ? LOW : HIGH);
  Bandera_Rele_luz = encender ? 1 : 0;
  estadoDelLED = encender;
  VeriTurn_Automatic = encender;
  publishMqttDeviceState("luz_tele", encender);
  recuperacionActiva = true;
  limpiarBloqueosRecuperacion();
  Serial.println("[AUTO] Luz " + String(encender ? "encendida" : "apagada") + " automáticamente a las " + horaObjetivo);
}

} // namespace

bool recuperacionBloqueada()
{
  return bloqueoRecuperacionTipo != BLOQUEO_RECUPERACION_NINGUNO;
}

void registrarIntervencionManual()
{
  registrarIntervencionEnRele(BLOQUEO_ORIGEN_MANUAL);
}

void registrarIntervencionRemota()
{
  registrarIntervencionEnRele(BLOQUEO_ORIGEN_REMOTO);
}

void limpiarBloqueosRecuperacion()
{
  establecerBloqueoRecuperacion(BLOQUEO_RECUPERACION_NINGUNO, BLOQUEO_ORIGEN_NINGUNO);
}

void onoff()
{
  contadooR = contadooR + 1;
  hora = normalizarHora(hora);
  
  // 🔍 DEBUG: Confirmar que onoff() se ejecuta (solo cada 30 segundos para no saturar)
  static unsigned long lastDebugOnOff = 0;
  if (millis() - lastDebugOnOff >= 30000) {
    Serial.println("[DEBUG-ONOFF] Funcion onoff() ejecutandose - Contador: " + String(contadooR) + " | Hora: " + hora);
    lastDebugOnOff = millis();
  }

  // Variables para control de tiempo de recuperación
  unsigned long currentMillis = millis();

  String horaEncendido;
  String horaApagado;
  obtenerHorarioRecuperacionMesActual(horaEncendido, horaApagado);

  if (hora == horaEncendido)
  {
    ejecutarAccionAutomatica(true, horaEncendido);
  }

  if (hora == horaApagado)
  {
    ejecutarAccionAutomatica(false, horaApagado);
  }

  // Verificación centralizada de recuperación automática 
  if (currentMillis - previousMillisRecuperacion >= interval)
  {
    previousMillisRecuperacion = currentMillis;
    verificarRecuperacionParaMesActual();
  }

  // ✅ SINCRONIZACIÓN NTP OPTIMIZADA cada 14400 segundos (4 horas)
  if (contadooR >= 14400) // ✅ CORREGIDO: cada 4 horas para evitar desbordamiento y ser más eficiente
  {
    Serial.println("[AUTO-NTP] Sincronización automática cada 4h iniciada...");
    Serial.println("[AUTO-NTP] Contador alcanzó: " + String(contadooR) + " segundos");

    // Usar la función Hrs() que ya tiene los delays necesarios
    Hrs();

    // Actualizar reloj virtual
    RelojDigital();
    hora = horaVirtual;

    contadooR = 0; // Resetear contador
    Serial.println("[AUTO-NTP] ✅ Sincronización completada: " + hora);
    Serial.println("[AUTO-NTP] Próxima sincronización en 4 horas");
  }

  // ✅ RESETEO AUTOMÁTICO DE Detect_Pulsador 
  // Para APAGADO MANUAL: Reset a las 02:10:20.
  // Para ENCENDIDO MANUAL: Reset a las 06:20:20.
  
  // 🔍 DEBUG: Verificar hora actual cada minuto para diagnóstico
  if (hora.endsWith(":00:20"))  // Cada minuto en punto
  {
    Serial.println("[DEBUG] Hora actual: " + hora + " | Detect_Pulsador: " + String(Detect_Pulsador) + " | recuperacionActiva: " + String(recuperacionActiva));
  }
  
  // ✅ CORREGIDO: Extraer solo HH:MM:SS del formato con milisegundos
  String horaFormateada = hora.length() >= 8 ? hora.substring(0, 8) : hora; // Tomar solo "HH:MM:SS"
  
  // ✅ CORREGIDO: Usar comparación más flexible para manejar formatos de hora
  if (horaFormateada == "02:10:20")  // Para APAGADO MANUAL: Reset a las 2:10:20 (MADRUGADA)
  {
    // ⚠️ DEBUG solo cuando detectamos el horario específico
    Serial.println("[RESET-DEBUG] Hora actual completa: '" + hora + "' | Hora formateada: '" + horaFormateada + "'");
    Serial.println("[RESET] ⏰ Horario de reset detectado a las " + horaFormateada + " (apagado manual - MADRUGADA)");
    if (bloqueoRecuperacionTipo == BLOQUEO_RECUPERACION_ENCENDIDO)
    {
      Serial.println("[RESET] ⏰ Limpiando bloqueo de " + String(obtenerNombreBloqueo(bloqueoRecuperacionTipo)) + " originado por " + String(obtenerNombreOrigen(bloqueoRecuperacionOrigen)));
      Serial.println("[RESET] La recuperación de ENCENDIDO automático volverá a funcionar");
      limpiarBloqueosRecuperacion();
      verificarRecuperacionParaMesActual();
      Serial.println("[RESET] ✅ recuperacionActiva reactivada tras reset de apagado manual");
    }
    else if (recuperacionBloqueada())
    {
      Serial.println("[RESET] ⏭️ Hay un bloqueo activo, pero corresponde al auto-apagado. Se mantiene hasta las 06:20:20");
    }
    else
    {
      Serial.println("[RESET] ✅ No hay bloqueo de auto-encendido pendiente");
    }
  }
  else if (horaFormateada == "06:20:20")  // Para ENCENDIDO MANUAL: Reset a las 06:20:20 (MADRUGADA)
  {
    // ⚠️ DEBUG solo cuando detectamos el horario específico
    Serial.println("[RESET-DEBUG] Hora actual completa: '" + hora + "' | Hora formateada: '" + horaFormateada + "'");
    Serial.println("[RESET] ⏰ Horario de reset detectado a las " + horaFormateada + " (encendido manual - MADRUGADA)");
    if (bloqueoRecuperacionTipo == BLOQUEO_RECUPERACION_APAGADO)
    {
      Serial.println("[RESET] ⏰ Limpiando bloqueo de " + String(obtenerNombreBloqueo(bloqueoRecuperacionTipo)) + " originado por " + String(obtenerNombreOrigen(bloqueoRecuperacionOrigen)));
      Serial.println("[RESET] La recuperación de APAGADO automático volverá a funcionar");
      limpiarBloqueosRecuperacion();
      verificarRecuperacionParaMesActual();
      Serial.println("[RESET] ✅ recuperacionActiva reactivada tras reset de encendido manual");
    }
    else if (recuperacionBloqueada())
    {
      Serial.println("[RESET] ⏭️ Hay un bloqueo activo, pero corresponde al auto-encendido. Se mantiene hasta las 02:10:20 siguiente");
    }
    else
    {
      Serial.println("[RESET] ✅ No hay bloqueo de auto-apagado pendiente");
    }
  }
}



// ============ FUNCIÓN DE RECUPERACIÓN AUTOMÁTICA OPTIMIZADA ============

// Función optimizada que solo verifica el mes actual Y ACTUALIZA INFO WEB
void verificarRecuperacionParaMesActual()
{
  String horaEncendido, horaApagado;

  // Actualizar información para mostrar en web
  ultimaVerificacion = hora;
  ultimaVerificacionMillis = millis();
  estadoActual = estadoDelLED ? "ENCENDIDO" : "APAGADO";

  obtenerHorarioRecuperacionMesActual(horaEncendido, horaApagado);
  if (horaEncendido.length() == 0 || horaApagado.length() == 0)
  {
    debugInfo = "ERROR: Mes inválido: " + String(MesActual);
    return;
  }

  // Guardar horarios para mostrar en web
  horaEncendidoActual = horaEncendido;
  horaApagadoActual = horaApagado;

  // Convertir hora actual a minutos para facilitar comparación
  int horaActualMinutos = convertirHoraAMinutosSilencioso(normalizarHora(hora));
  deberiaEstarEncendida = estaEnRangoEncendidoSilencioso(horaActualMinutos, horaEncendido, horaApagado);

  // Actualizar información de debug para web
  debugInfo = "Mes: " + String(MesActual) + " | Hora: " + hora + " | Estado: " + estadoActual +
              " | Debería: " + (deberiaEstarEncendida ? "ENCENDIDO" : "APAGADO") +
              " | Rango: " + horaEncendido + " a " + horaApagado +
              " | HoraMinutos: " + String(horaActualMinutos) +
              " | EncenderMin: " + String(convertirHoraAMinutosSilencioso(horaEncendido)) +
              " | ApagarMin: " + String(convertirHoraAMinutosSilencioso(horaApagado));

  // Recuperar el estado correcto de la luz después de un reseteo
  if (deberiaEstarEncendida && !estadoDelLED && !recuperacionBloqueada())
  {
    // Debería estar encendida pero está apagada - RECUPERAR
    Serial.println("[RECUPERACION] ANTES: Pin ReleLuz = " + String(digitalRead(ReleLuz)) + ", estadoDelLED = " + String(estadoDelLED));
    digitalWrite(ReleLuz, LOW);
    Bandera_Rele_luz = 1;
    estadoDelLED = true;
    VeriTurn_Automatic = true;
    recuperacionActiva = true; // ✅ PROTEGER contra interferencias
    Serial.println("[RECUPERACION] DESPUES: Pin ReleLuz = " + String(digitalRead(ReleLuz)) + ", estadoDelLED = " + String(estadoDelLED));
    debugInfo += " | ACCIÓN: Luz encendida automáticamente";
  }
  else if (!deberiaEstarEncendida && estadoDelLED && !recuperacionBloqueada())
  {
    // Debería estar apagada pero está encendida - CORREGIR (solo si no es manual)
    Serial.println("[RECUPERACION] ANTES: Pin ReleLuz = " + String(digitalRead(ReleLuz)) + ", estadoDelLED = " + String(estadoDelLED));
    digitalWrite(ReleLuz, HIGH);
    Bandera_Rele_luz = 0;
    estadoDelLED = false;
    VeriTurn_Automatic = false;
    recuperacionActiva = true; // ✅ MANTENER activa - acabamos de hacer una recuperación
    Serial.println("[RECUPERACION] DESPUES: Pin ReleLuz = " + String(digitalRead(ReleLuz)) + ", estadoDelLED = " + String(estadoDelLED));
    debugInfo += " | ACCIÓN: Luz apagada automáticamente";
  }
  else
  {
    Serial.println("[RECUPERACION] Estado correcto - Pin ReleLuz = " + String(digitalRead(ReleLuz)) + ", estadoDelLED = " + String(estadoDelLED));
    
    // Verificar si hay bloqueo manual activo
    if (recuperacionBloqueada())
    {
      // Si hay intervención manual, recuperación debe estar desactivada
      recuperacionActiva = false;
      Serial.println("[RECUPERACION] DEBUG: Hay bloqueo activo, desactivando recuperacionActiva");

      String origenBloqueo = String(obtenerNombreOrigen(bloqueoRecuperacionOrigen));
      if (bloqueoRecuperacionTipo == BLOQUEO_RECUPERACION_ENCENDIDO)
      {
        debugInfo += " | ACCIÓN: Auto-encendido bloqueado por " + origenBloqueo + " - Reset 02:10:20";
        Serial.println("[RECUPERACION] Auto-encendido bloqueado por " + origenBloqueo + " hasta 02:10:20");
      }
      else if (bloqueoRecuperacionTipo == BLOQUEO_RECUPERACION_APAGADO)
      {
        debugInfo += " | ACCIÓN: Auto-apagado bloqueado por " + origenBloqueo + " - Reset 06:20:20";
        Serial.println("[RECUPERACION] Auto-apagado bloqueado por " + origenBloqueo + " hasta 06:20:20");
      }
    }
    else
    {
      // Si no hay intervención manual, la recuperación debe estar activa
      recuperacionActiva = true;
      Serial.println("[RECUPERACION] DEBUG: Sin bloqueos activos, activando recuperacionActiva");
      debugInfo += " | ACCIÓN: Estado correcto - Sistema funcionando normalmente";
      Serial.println("[RECUPERACION] OK Sistema funcionando normalmente - recuperacionActiva = true");
    }
  }
}

// Función auxiliar para convertir hora en formato "HH:MM:SS" a minutos (SILENCIOSA para web)
int convertirHoraAMinutosSilencioso(String horaStr)
{
  horaStr = normalizarHora(horaStr);
  int primerSeparador = horaStr.indexOf(':');
  int segundoSeparador = horaStr.indexOf(':', primerSeparador + 1);
  if (primerSeparador < 0 || segundoSeparador < 0)
  {
    return 0;
  }
  int horas = horaStr.substring(0, primerSeparador).toInt();
  int minutos = horaStr.substring(primerSeparador + 1, segundoSeparador).toInt();
  return horas * 60 + minutos;
}

// Función auxiliar para verificar si la hora actual está en el rango de encendido (SILENCIOSA para web)
bool estaEnRangoEncendidoSilencioso(int horaActualMinutos, String horaEncendido, String horaApagado)
{
  int minutosEncendido = convertirHoraAMinutosSilencioso(horaEncendido);
  int minutosApagado = convertirHoraAMinutosSilencioso(horaApagado);

  // Si el rango cruza medianoche (ej: 21:15 a 3:35)
  if (minutosEncendido > minutosApagado)
  {
    return (horaActualMinutos >= minutosEncendido) || (horaActualMinutos <= minutosApagado);
  }
  // Si el rango no cruza medianoche
  else
  {
    return (horaActualMinutos >= minutosEncendido) && (horaActualMinutos <= minutosApagado);
  }
}

// Función auxiliar para convertir hora en formato "HH:MM:SS" a minutos CON DEBUG
int convertirHoraAMinutos(String horaStr)
{
  horaStr = normalizarHora(horaStr);
  Serial.println("[HORA] Convirtiendo: " + horaStr);

  int primerSeparador = horaStr.indexOf(':');
  int segundoSeparador = horaStr.indexOf(':', primerSeparador + 1);
  if (primerSeparador < 0 || segundoSeparador < 0)
  {
    Serial.println("[HORA] Formato inválido");
    return 0;
  }
  int horas = horaStr.substring(0, primerSeparador).toInt();
  int minutos = horaStr.substring(primerSeparador + 1, segundoSeparador).toInt();
  int totalMinutos = horas * 60 + minutos;

  Serial.println("[HORA] " + horaStr + " = " + String(totalMinutos) + " minutos (H:" + String(horas) + " M:" + String(minutos) + ")");

  return totalMinutos;
}

// Función auxiliar para verificar si la hora actual está en el rango de encendido CON DEBUG
bool estaEnRangoEncendido(int horaActualMinutos, String horaEncendido, String horaApagado)
{
  int minutosEncendido = convertirHoraAMinutos(horaEncendido);
  int minutosApagado = convertirHoraAMinutos(horaApagado);

  Serial.println("[RANGO] Minutos - Encender: " + String(minutosEncendido) + " | Apagar: " + String(minutosApagado) + " | Actual: " + String(horaActualMinutos));

  // Si el rango cruza medianoche (ej: 21:15 a 3:35)
  if (minutosEncendido > minutosApagado)
  {
    Serial.println("[RANGO] Rango cruza medianoche");
    bool resultado = (horaActualMinutos >= minutosEncendido) || (horaActualMinutos <= minutosApagado);
    Serial.println("[RANGO] Resultado: " + String(resultado ? "ENCENDER" : "APAGAR"));
    return resultado;
  }
  // Si el rango no cruza medianoche
  else
  {
    Serial.println("[RANGO] Rango normal (no cruza medianoche)");
    bool resultado = (horaActualMinutos >= minutosEncendido) && (horaActualMinutos <= minutosApagado);
    Serial.println("[RANGO] Resultado: " + String(resultado ? "ENCENDER" : "APAGAR"));
    return resultado;
  }
}
