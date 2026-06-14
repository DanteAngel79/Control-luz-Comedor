#include "Libreria.h"

// Escribir el fragmento de estados directo al stream (sin String grande en heap)
void escribirFragmentoEstados(Print &out) {
    // Iluminación
    out.print(F("<section class='card'><h2>LUZ COMEDOR</h2><p>Estado: "));
    if (estadoDelLED) {
        out.print(F("<span class='on'>ENCENDIDA</span></p><button class='btn danger' onclick=\"accion('/apagarLuz',this)\">APAGAR LUZ</button>"));
    } else {
        out.print(F("<span class='off'>APAGADA</span></p><button class='btn ok' onclick=\"accion('/encenderLuz',this)\">ENCENDER LUZ</button>"));
    }
    out.print(F("</section>"));

    // Control de Computadoras WOL
    out.print(F("<section class='card'><h2>CONTROL COMPUTADORAS</h2>"));
    for (size_t index = 0; index < PC_COUNT; index++) {
        out.print(F("<div class='sub'><h3>"));
        out.print(PCS[index].nombreAlexa);
        out.print(F("</h3><div class='row'>"
            "<button class='btn ok' onclick=\"accion('/wol?mac="));
        out.print(PCS[index].mac);
        out.print(F("',this)\">ENCENDER</button>"
            "<button class='btn danger' onclick=\"accion('/PcsOff?mac="));
        out.print(PCS[index].mac);
        out.print(F("',this)\">APAGAR</button>"
            "</div></div>"));
    }
    out.print(F("</section>"));

    // Estado de Recuperación Automática
    out.print(F("<section class='card'><h2>ESTADO DE RECUPERACION</h2>"
            "<p>Recuperaci&oacute;n Autom&aacute;tica: "));
    if (recuperacionActiva && !recuperacionBloqueada()) {
        out.print(F("<span class='on'>FUNCIONANDO</span></p>"));
        out.print(F("<p class='small'>Sin bloqueos activos. El sistema puede encender o apagar la luz autom&aacute;ticamente seg&uacute;n horario.</p>"));
    } else {
        out.print(F("<span class='off'>PAUSADA</span></p>"));
        if (Detect_Pulsador) {
            out.print(F("<p>Origen del bloqueo: <span class='off'>PULSADOR</span></p>"));
            out.print(F("<p class='small'>Hay una acci&oacute;n intencional manual sobre la luz. La automatizaci&oacute;n no corregir&aacute; el rele hasta el reset programado.</p>"));
        } else if (bloqueoRecuperacionRemota) {
            out.print(F("<p>Origen del bloqueo: <span class='off'>SERVER / ALEXA</span></p>"));
            out.print(F("<p class='small'>Hay una acci&oacute;n intencional remota sobre la luz. La automatizaci&oacute;n no corregir&aacute; el rele hasta el reset programado.</p>"));
        } else {
            out.print(F("<p class='small'>La recuperaci&oacute;n est&aacute; pausada temporalmente.</p>"));
        }
        out.print(F("<p class='small'>Reset programado seg&uacute;n la acci&oacute;n bloqueada: 02:10:20 o 06:20:20.</p>"));
    }
    out.print(F("</section>"));

    // Controles sistema
    out.print(F("<section class='card'><h2>HERRAMIENTAS DEL SISTEMA</h2><div class='row'>"
            "<button class='btn neutral' onclick=\"accion('/actualizaHora',this)\">SYNC HORA</button>"
            "<button class='btn danger' onclick=\"accion('/reset',this)\">REINICIAR</button></div>"));
    out.print(F("<div class='telnet-inline'><span class='small telnet-label'>Monitor Alexa "));
    out.print(obtenerEstadoTelnet());
    out.print(F("</span><span class='telnet-cmd'>CMD: telnet "));
    out.print(WiFi.localIP());
    out.print(F(" 23</span><button class='btn ghost' onclick=\"accion('/toggleTelnet',this)\">"));
    out.print(telnetHabilitado ? F("Ocultar") : F("Abrir"));
    out.print(F("</button></div></section>"));
}

// Página principal dinámica enviada por stream para evitar un String grande en heap.
void escribirInterfazHTML(Print &out)
{
    out.print(F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"));
    out.print(F("<meta name='viewport' content='width=device-width,initial-scale=1'>"));
    out.print(F("<title>Control Luz Comedor</title><style>"));
    out.print(F("body{margin:0;padding:16px;background:#1e2124;color:#dcdcdc;font:15px Segoe UI,Arial,sans-serif;}"));
    out.print(F(".wrap{max-width:440px;margin:0 auto;}h1{margin:0 0 12px;font-size:24px;text-align:center;color:#8bc34a;}"));
    out.print(F(".stamp{margin:0 0 16px;padding:0 0 12px;text-align:center;font-size:13px;color:#a3a6aa;border-bottom:1px solid #3c3f44;}"));
    out.print(F(".card{background:#2c2f33;border-radius:12px;padding:16px;margin:0 0 16px;box-shadow:0 4px 10px rgba(0,0,0,.28);}"));
    out.print(F("h2{margin:0 0 10px;padding-bottom:8px;border-bottom:1px solid #3c3f44;font-size:17px;color:#fff;}h3{margin:0 0 8px;font-size:14px;color:#8bc34a;text-align:center;}"));
    out.print(F("p{margin:0 0 12px;line-height:1.4}.row{display:flex;gap:8px}.sub{margin:0 0 12px;padding:10px;background:#23272a;border-radius:8px}.sub:last-child{margin-bottom:0}"));
    out.print(F(".btn{width:100%;padding:10px 12px;border:0;border-radius:8px;font-size:12px;font-weight:700;color:#fff;text-transform:uppercase;cursor:pointer}.ok{background:#43b581}.danger{background:#f04747}.neutral{background:#72767d}.loading{background:#a3a6aa;cursor:not-allowed}"));
    out.print(F(".ghost{width:auto;padding:6px 10px;background:transparent;border:1px solid #4b5158;color:#8f979f;font-size:10px;letter-spacing:.08em}.telnet-inline{margin-top:12px;display:flex;align-items:center;justify-content:space-between;gap:8px}.telnet-label{text-align:left;font-style:normal}.telnet-cmd{font:10px Consolas,monospace;color:#8bc34a;white-space:nowrap}.on{font-weight:700;color:#43b581}.off{font-weight:700;color:#f04747}.small{font-size:12px;color:#a3a6aa;text-align:center;font-style:italic}.footer{text-align:center;font-size:11px;color:#72767d;margin-top:18px}"));
    out.print(F("</style></head><body><div class='wrap'><h1>Luz Comedor</h1><p class='stamp'><strong>"));
    out.print(hourS);
    out.print(F("</strong> | "));
    out.print(MesPalabras);
    out.print(F("</p><div id='estados'><section class='card'><p class='small'>Cargando...</p></section></div>"));
    out.print(F("<script>let refresco=false,timer=null;"));
    out.print(F("function plan(ms){clearTimeout(timer);timer=setTimeout(()=>estado(),ms)}"));
    out.print(F("function estado(force=false){if(refresco&&!force){plan(5000);return}refresco=true;fetch('/estado',{cache:'no-store'}).then(r=>r.text()).then(h=>document.getElementById('estados').innerHTML=h).finally(()=>{refresco=false;plan(15000)})}"));
    out.print(F("function accion(url,btn){let txt=btn.textContent;btn.disabled=true;btn.textContent='Cargando...';btn.classList.add('loading');fetch(url,{cache:'no-store'}).then(r=>{btn.textContent=r.ok?'OK':'ERROR';setTimeout(()=>{estado(true);btn.textContent=txt;btn.disabled=false;btn.classList.remove('loading')},1200)}).catch(()=>{btn.textContent='ERROR RED';setTimeout(()=>{btn.textContent=txt;btn.disabled=false;btn.classList.remove('loading')},2000)})}"));
    out.print(F("estado(true);</script><div class='footer'>Interfaz Luz Comedor</div></div></body></html>"));
}