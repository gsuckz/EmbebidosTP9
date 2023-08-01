#include <stdbool.h>
#include "control.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define PRESCALER 1 //Se usa para accelerar ael reloj para las pruebas
typedef struct Control{
    Poncho_p poncho;
    Reloj * reloj;
    ESTADOS estado;
    int TimeOut;
    uint8_t temp [6];
    bool parpadeo;
}Control;



/**
 * @brief Configura un tiempo de TimeOut en segundos
 * 
 * @param controlador 
 * @param segundos 
 */

/*
static void setTimeOut(Control * controlador, int segundos){
    controlador->TimeOut = segundos * CANTIDAD_TICKS_POR_SEGUNDO_RELOJ ;
}
*/

static void setTimeOut(Control * controlador, int segundos){
    controlador->TimeOut = segundos * TICKS_SEG_SO + xTaskGetTickCount();
}

/**
 * @brief Borra (escribiendo un caracter desconocido) en los minutos
 * 
 * @param hhmm 
 */
static void parpadeoMM(uint8_t * hhmm,Control * controlador){
    if (controlador->parpadeo) {
        hhmm[2] = -1;
        hhmm[3] = -1;
    }
}
/**
 * @brief Borra (escribiendo un caracter desconocido) en las horas
 * 
 * @param hhmm 
 */
static void parpadeoHH(uint8_t * hhmm,Control * controlador){
    if (controlador->parpadeo) {
        hhmm[0] = -1;
        hhmm[1] = -1;
    }
}
/**
 * @brief 
 * @brief Borra (escribiendo un caracter desconocido) en el display
 * 
 * @param hhmm 
 */
static void parpadeoHHMM(uint8_t * hhmm,Control * controlador){
    parpadeoHH(hhmm,controlador);
    parpadeoMM(hhmm,controlador);
}
Control * crearControlador(int ticks_seg, void (*ControladorAlarma)(bool), Poncho_p poncho){
    Control * controlador = malloc(sizeof(Control));
    controlador->poncho = poncho;
    controlador->reloj = relojCrear(ticks_seg/PRESCALER, ControladorAlarma);
    controlador->estado = E_RESET;
    controlador->TimeOut =0;
    controlador->parpadeo =0;
    for(int i=0;i<6;i++) controlador->temp[i] = 0;
    return controlador;
    }



void guardarPantalla(Control * controlador){  
    uint8_t hhmm[6] = {0,0,0,0,0,0};
    switch (controlador->estado)
    { 
        case E_MOD_ALARMA_HOR:
        case E_MOD_ALARMA_MIN:    //FALLTHRU
        case E_MOD_ALARMA_HOR_R: //FALLTHRU
        case E_MOD_ALARMA_MIN_R://FALLTHRU
            PonchoPuntoMode(controlador->poncho,0,1);
            PonchoPuntoMode(controlador->poncho,1,1);
            PonchoPuntoMode(controlador->poncho,2,1);
            PonchoPuntoMode(controlador->poncho,3,1);
        case E_MOD_HORARIO_HOR:      //FALLTRHU
        case E_MOD_HORARIO_MIN:     //FALLTHRU
        case E_MOD_HORARIO_HOR_R:  //FALLTRHU
        case E_MOD_HORARIO_MIN_R: //FALLTHRU
            for (int i=0; i<=3 ;i++) {
                hhmm[i] = controlador->temp[i];
            }              
        void (*parpadear)(uint8_t * hhmm,Control * controlador) =((controlador->estado == E_MOD_ALARMA_MIN)    || 
                                                                  (controlador->estado == E_MOD_ALARMA_MIN_R)  ||
                                                                  (controlador->estado == E_MOD_HORARIO_MIN_R) ||
                                                                  (controlador->estado == E_MOD_HORARIO_MIN)  
        ) ? parpadeoMM : parpadeoHH; 
        parpadear(hhmm,controlador);            
        break;case E_RESET:
        case E_ESPERA_MOD_ALARMA_R:   //FALLTHRU 
        case E_ESPERA_MOD_HORARIO_R: //FALLTHRU        
            parpadeoHHMM(hhmm,controlador);
            PonchoPuntoMode(controlador->poncho,0,0);
            PonchoPuntoMode(controlador->poncho,1,0);
            PonchoPuntoMode(controlador->poncho,2,!controlador->parpadeo);                
            PonchoPuntoMode(controlador->poncho,3,0);            
        break;case E_MOSTRAR_HORA:
        case E_ESPERA_MOD_ALARMA:   //FALLTHRU
        case E_ESPERA_MOD_HORARIO:  //FALLTHRU
            relojHorario(controlador->reloj,hhmm);
            PonchoPuntoMode(controlador->poncho,0,0);
            PonchoPuntoMode(controlador->poncho,1,0);
            PonchoPuntoMode(controlador->poncho,2,!controlador->parpadeo);                
            PonchoPuntoMode(controlador->poncho,3,0);            
        break;default:
            for (int i=0; i<=3 ;i++) {
                hhmm[i] = -1;
            }             
            PonchoPuntoMode(controlador->poncho,0,1);   
            
        break;
    }
    if ((getEstadoAlarma(controlador->reloj)) == READY) {
        PonchoPuntoMode(controlador->poncho,0,1);
    }
    PonchoWriteDisplay(controlador->poncho, hhmm); 
    //PonchoDrawDisplay(controlador->poncho); 
}

static bool timeOutCheck(Control * controlador){  //optimizar?
    bool huboTimeOut = 0;
    if(controlador->TimeOut >= xTaskGetTickCount()){
        huboTimeOut = true;
    }
        return huboTimeOut;
}

void procesarBotones(Control * controlador, int teclas){            
            if ((getEstadoAlarma(controlador->reloj)) == ON){    //Los botones (ACEPTAR) y (CANCELAR) solo funcionan para 
                if(teclas & ACEPTAR) { // posoponer o apagar la alarma, cuando esta sonando. 
                    relojSnooze(controlador->reloj,5);
                    return;}
                if(teclas & CANCELAR) {
                    setAlarmaEstado(controlador->reloj,READY);
                    return;}
            }
            switch (controlador->estado){
            case E_RESET: 
                for (int i=0; i<=3 ;i++) {
                    controlador->temp[i] = 0;
                }                                           
                if(teclas & F1H) {
                    setTimeOut(controlador,3);
                    controlador->estado = E_ESPERA_MOD_HORARIO_R;
                }
                if(teclas & F2H) {
                    setTimeOut(controlador,3);
                    controlador->estado = E_ESPERA_MOD_ALARMA_R;
                }
            break;case E_MOSTRAR_HORA: //FALLTHRU
                if(teclas & F1H) {
                    setTimeOut(controlador,3);
                    controlador->estado = E_ESPERA_MOD_HORARIO;
                }
                if(teclas & F2H) {
                    setTimeOut(controlador,3);
                    controlador->estado = E_ESPERA_MOD_ALARMA;
                }
                if((teclas & ACEPTAR) && 
                !(getEstadoAlarma(controlador->reloj) == SNOOZE)) setAlarmaEstado(controlador->reloj,READY);
                if(teclas & CANCELAR) setAlarmaEstado(controlador->reloj,OFF);
            break;case E_ESPERA_MOD_ALARMA:
                if(!(teclas & F2H)) controlador->estado = E_MOSTRAR_HORA;
                if(!timeOutCheck(controlador)) { 
                    setTimeOut(controlador,30);
                    getAlarmaHora(controlador->reloj,controlador->temp);
                    controlador->estado = E_MOD_ALARMA_MIN;
                }
            break;case E_ESPERA_MOD_HORARIO:
                if(!(teclas & F1H)) controlador->estado = E_MOSTRAR_HORA;
                if(!timeOutCheck(controlador)) {
                    setTimeOut(controlador,30);
                    relojHorario(controlador->reloj,controlador->temp);
                    controlador->estado = E_MOD_HORARIO_MIN;
                }
            break;case E_MOD_ALARMA_MIN:
                if(teclas & F4){
                    setTimeOut(controlador,30);
                    incrementarMinutos(controlador->temp);
                }
                if(teclas & F3){
                    setTimeOut(controlador,30);
                    decrementarMinutos(controlador->temp);
                }
                if(teclas & CANCELAR || !timeOutCheck(controlador)){
                    controlador->estado = E_MOSTRAR_HORA;
                }
                if(teclas & ACEPTAR){
                    setTimeOut(controlador,30);
                    controlador->estado = E_MOD_ALARMA_HOR;
                }
            break;case E_MOD_ALARMA_HOR:
                if(teclas & F4){
                    setTimeOut(controlador,30);
                    incrementarHoras(controlador->temp);
                }
                if(teclas & F3){
                    setTimeOut(controlador,30);
                    decrementarHoras(controlador->temp);
                }
                if(teclas & CANCELAR || !timeOutCheck(controlador)){
                    controlador->estado = E_MOSTRAR_HORA;
                }   
                if(teclas & ACEPTAR){
                    setTimeOut(controlador,30);
                    setAlarmaHora(controlador->reloj,controlador->temp);
                    setAlarmaEstado(controlador->reloj,READY);
                    controlador->estado = E_MOSTRAR_HORA;
                }                    
            break;case E_MOD_HORARIO_MIN:
                if(teclas & F4){
                    setTimeOut(controlador,30);
                    incrementarMinutos(controlador->temp);
                }
                if(teclas & F3){
                    setTimeOut(controlador,30);
                    decrementarMinutos(controlador->temp);
                }
                if(teclas & CANCELAR || !timeOutCheck(controlador)){
                    controlador->estado = E_MOSTRAR_HORA;
                }
                if(teclas & ACEPTAR){
                    setTimeOut(controlador,30);
                    controlador->estado = E_MOD_HORARIO_HOR;
                }           
            break;case E_MOD_HORARIO_HOR:
                if(teclas & F4){
                    setTimeOut(controlador,30);
                    incrementarHoras(controlador->temp);
                }
                if(teclas & F3){
                    setTimeOut(controlador,30);
                    decrementarHoras(controlador->temp);
                }
                if(teclas & CANCELAR || !timeOutCheck(controlador)){
                    controlador->estado = E_MOSTRAR_HORA;
                }   
                if(teclas & ACEPTAR){
                    relojGuardarHora(controlador->reloj,controlador->temp);
                    controlador->estado = E_MOSTRAR_HORA;
                }              
            break;case E_ESPERA_MOD_ALARMA_R:
                if(!(teclas & F2H)) controlador->estado = E_RESET;
                if(!timeOutCheck(controlador)) {
                    setTimeOut(controlador,30);
                    getAlarmaHora(controlador->reloj,controlador->temp);
                    controlador->estado = E_MOD_ALARMA_MIN_R;
                }
            break;case E_ESPERA_MOD_HORARIO_R:
                if(!(teclas & F1H)) controlador->estado = E_RESET;
                if(!timeOutCheck(controlador)) {
                    setTimeOut(controlador,30);
                    relojHorario(controlador->reloj,controlador->temp);
                    controlador->estado = E_MOD_HORARIO_MIN_R;
                }
            break;case E_MOD_ALARMA_MIN_R:
                if(teclas & F4){
                    setTimeOut(controlador,30);
                    incrementarMinutos(controlador->temp);
                }
                if(teclas & F3){
                    setTimeOut(controlador,30);
                    decrementarMinutos(controlador->temp);
                }
                if(teclas & CANCELAR || !timeOutCheck(controlador)){
                    controlador->estado = E_RESET;
                }
                if(teclas & ACEPTAR){
                    setTimeOut(controlador,30);
                    controlador->estado = E_MOD_ALARMA_HOR_R;
                }
            break;case E_MOD_ALARMA_HOR_R:
                if(teclas & F4){
                    setTimeOut(controlador,30);
                    incrementarHoras(controlador->temp);
                }
                if(teclas & F3){
                    setTimeOut(controlador,30);
                    decrementarHoras(controlador->temp);
                }
                if(teclas & CANCELAR || !timeOutCheck(controlador)){
                    controlador->estado = E_RESET;
                }   
                if(teclas & ACEPTAR){
                    setTimeOut(controlador,30);
                    setAlarmaHora(controlador->reloj,controlador->temp);
                    setAlarmaEstado(controlador->reloj,READY);
                    controlador->estado = E_RESET;
                }                    
            break;case E_MOD_HORARIO_MIN_R:
                if(teclas & F4){
                    setTimeOut(controlador,30);
                    incrementarMinutos(controlador->temp);
                }
                if(teclas & F3){
                    setTimeOut(controlador,30);
                    decrementarMinutos(controlador->temp);
                }
                if(teclas & CANCELAR || !timeOutCheck(controlador)){
                    controlador->estado = E_RESET;
                }
                if(teclas & ACEPTAR){
                    setTimeOut(controlador,30);
                    controlador->estado = E_MOD_HORARIO_HOR_R;
                }           
            break;case E_MOD_HORARIO_HOR_R:
                if(teclas & F4){
                    setTimeOut(controlador,30);
                    incrementarHoras(controlador->temp);
                }
                if(teclas & F3){
                    setTimeOut(controlador,30);
                    decrementarHoras(controlador->temp);
                }
                if(teclas & CANCELAR || !timeOutCheck(controlador)){
                    controlador->estado = E_RESET;
                }   
                if(teclas & ACEPTAR){
                    while(!relojGuardarHora(controlador->reloj,controlador->temp)); //bloquea el programa si no puede guardar
                    controlador->estado = E_MOSTRAR_HORA;
                }              
            break;default:
            break;
        }    
    guardarPantalla(controlador);
}
/*
static void timeOutCheck(Control * controlador){  //optimizar?
    if(controlador->TimeOut>1) {
        controlador->TimeOut--;
        return; //Si es mayor que 1 solo decrementa
    }
    if (controlador->TimeOut == 1){
        controlador->TimeOut=0;
        procesarBotones(controlador,0); //Cuando hay un TimeOut cheuea los botones
    } 
}
*/



static void segRefParpadeo(Control * ctrl){
    ctrl->parpadeo = !ctrl->parpadeo;
    //ctrl->parpadeo = 0;
}
void timerCtrl(Control * ctrl){ //crítico para no perder la hora
    //timeOutCheck(ctrl);
    if (relojTick(relojDe(ctrl))) {
        segRefParpadeo(ctrl);
        guardarPantalla(ctrl);        
    } 
}
Poncho_p ponchoDe(Control * controlador){
    return controlador->poncho;
}
Reloj * relojDe(Control * controlador){
    return controlador->reloj;
}

ESTADOS getEstado(Control * controlador){
    return controlador->estado;
}