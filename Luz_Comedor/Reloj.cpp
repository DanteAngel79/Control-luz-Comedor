#include "Libreria.h"

String normalizarHora(String horaStr)
{
  horaStr.trim();
  int primerSeparador = horaStr.indexOf(':');
  if (primerSeparador == 1)
  {
    horaStr = "0" + horaStr;
  }
  if (horaStr.length() >= 8)
  {
    return horaStr.substring(0, 8);
  }
  return horaStr;
}

void sincronizarRelojVirtual(int horas, int minutos, int segundos)
{
  h = horas;
  m = minutos;
  s = (segundos * 10) + 4;

  if (s >= 600)
  {
    s -= 600;
    m += 1;
  }
  if (m >= 60)
  {
    m -= 60;
    h = (h + 1) % 24;
  }
}
void Configura()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(1000); // ✅ TIEMPO MÍNIMO para configuración NTP
  printLocalTime();
}

void printLocalTime()
{
  String MesSincortar = "";
  int n;
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  delay(2000); // ✅ RESTAURADO: tiempo necesario para obtener datos del sistema
  
  MesSincortar = asctime(timeinfo);
  MesCortado = asctime(timeinfo);
  MesCortado.remove(0, 4);
  MesCortado.remove(3, 21);
  Serial.println("[NTP] Mes detectado: " + MesCortado);
  convierteMes();
}
void convierteMes()
{
  if (MesCortado == "Dec")
  {
    MesActual = 12;
    MesPalabras = "26-12-1939 - 24-08-2023, AMANDA, FELIZ CUMPLE !";
  }
  if (MesCortado == "Jan")
  {
    MesActual = 1;
    MesPalabras = "ENERO";
  }
  if (MesCortado == "Feb")
  {
    MesActual = 2;
    MesPalabras = "4 FEBRERO, RAFAEL, FELIZ CUMPLE !";
  }
  if (MesCortado == "Mar")
  {
    MesActual = 3;
    MesPalabras = "HUY MARZO";
  }
  if (MesCortado == "Apr")
  {
    MesActual = 4;
    MesPalabras = "1 ABRIL, EMILIO, FELIZ CUMPLE !";
  }
  if (MesCortado == "May")
  {
    MesActual = 5;
    MesPalabras = "1 MAYO, AQUI_TU_NOMBRE, FELIZ CUMPLE !";
  }
  if (MesCortado == "Jun")
  {
    MesActual = 6;
    MesPalabras = "JUNIO";
  }
  if (MesCortado == "Jul")
  {
    MesActual = 7;
    MesPalabras = "JULIO";
  }
  if (MesCortado == "Aug")
  {
    MesActual = 8;
    MesPalabras = "4 AGOSTO, GABRIEL, FELIZ CUMPLE !";
  }
  if (MesCortado == "Sep")
  {
    MesActual = 9;
    MesPalabras = "SEPTIEMBRE";
  }
  if (MesCortado == "Oct")
  {
    MesActual = 10;
    MesPalabras = "OCTUBRE";
  }
  if (MesCortado == "Nov")
  {
    MesActual = 11;
    MesPalabras = "NOVIEMBRE";
  }
}
void RelojDigital() // El reloj
{
  s = s + 1;
  if (s >= 600)
  {
    m = m + 1;
    s = 0;
    if (m == 60)
    {
      h = h + 1;
      m = 0;
    }
    if (h == 24)
    {
      h = 0;
    }
  }
  
  // ✅ FORMATEAR CORRECTAMENTE CON CEROS A LA IZQUIERDA Y CONVERTIR SEGUNDOS
  String hStr = (h < 10) ? "0" + String(h) : String(h);
  String mStr = (m < 10) ? "0" + String(m) : String(m);
  
  // Convertir s (0-599) a segundos reales (0-59) para el formato HH:MM:SS
  int segundosReales = s / 10;
  String sStr = (segundosReales < 10) ? "0" + String(segundosReales) : String(segundosReales);
  
  horaVirtual = hStr + ":" + mStr + ":" + sStr; // Formato correcto HH:MM:SS
  hourS = hStr + ":" + mStr + ":" + "00";
}

// ✅ FUNCIÓN NTP ASÍNCRONA CON TIEMPO SUFICIENTE PARA SINCRONIZACIÓN
void HrsAsync()
{
  if ((MesCortado == "Sep") || (MesCortado == "Oct") || (MesCortado == "Nov") || (MesCortado == "Dec") || (MesCortado == "Jan") || (MesCortado == "Feb") || (MesCortado == "Mar"))
  {
    // ✅ FORZAR ACTUALIZACIÓN Y ESPERAR SINCRONIZACIÓN
    ntpverano.forceUpdate();
    delay(1500); // ✅ RESTAURADO: tiempo necesario para sincronización NTP
    
    // ✅ MANTENER LÓGICA ORIGINAL: getFormattedTime() primero
    hora = normalizarHora(ntpverano.getFormattedTime());
    contadooR = 0;
    sincronizarRelojVirtual(int(ntpverano.getHours()), int(ntpverano.getMinutes()), int(ntpverano.getSeconds()));
    
    Serial.println("[NTP] Hora actualizada (verano): " + hora);
  }
  else
  {
    // ✅ FORZAR ACTUALIZACIÓN Y ESPERAR SINCRONIZACIÓN
    ntpinvierno.forceUpdate();
    delay(1500); // ✅ RESTAURADO: tiempo necesario para sincronización NTP
    
    // ✅ MANTENER LÓGICA ORIGINAL: getFormattedTime() primero
    hora = normalizarHora(ntpinvierno.getFormattedTime());
    contadooR = 0;
    sincronizarRelojVirtual(int(ntpinvierno.getHours()), int(ntpinvierno.getMinutes()), int(ntpinvierno.getSeconds()));
    
    Serial.println("[NTP] Hora actualizada (invierno): " + hora);
  }
}

// ✅ MANTENER LA FUNCIÓN ORIGINAL CON TIEMPO SUFICIENTE PARA SINCRONIZACIÓN
void Hrs()
{
  if ((MesCortado == "Sep") || (MesCortado == "Oct") || (MesCortado == "Nov") || (MesCortado == "Dec") || (MesCortado == "Jan") || (MesCortado == "Feb") || (MesCortado == "Mar"))
  {
    ntpverano.forceUpdate();
    delay(2000); // ✅ RESTAURADO: tiempo necesario para sincronización completa
    hora = normalizarHora(ntpverano.getFormattedTime());
    contadooR = 0;
    sincronizarRelojVirtual(int(ntpverano.getHours()), int(ntpverano.getMinutes()), int(ntpverano.getSeconds()));
  }
  else
  {
    ntpinvierno.forceUpdate();
    delay(2000); // ✅ RESTAURADO: tiempo necesario para sincronización completa
    hora = normalizarHora(ntpinvierno.getFormattedTime());
    contadooR = 0;
    sincronizarRelojVirtual(int(ntpinvierno.getHours()), int(ntpinvierno.getMinutes()), int(ntpinvierno.getSeconds()));
  }
}