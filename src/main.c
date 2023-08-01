/* Copyright 2022, Laboratorio de Microprocesadores
 * Facultad de Ciencias Exactas y Tecnología
 * Universidad Nacional de Tucuman
 * http://www.microprocesadores.unt.edu.ar/
 * Copyright 2022, Esteban Volentini <evolentini@herrera.unt.edu.ar>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/** \brief Simple sample of use LPC HAL gpio functions
 **
 ** \addtogroup samples Sample projects
 ** \brief Sample projects to use as a starting point
 ** @{ */
/* === Headers files inclusions =============================================================== */
#include "mybsp.h"
#include "poncho.h"
#include "reloj.h"
#include "control.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <stdbool.h>
/* === Macros definitions ====================================================================== */

/* === Private data type declarations ========================================================== */
/* === Private variable declarations =========================================================== */
Control * controlador;
Poncho_p poncho;
/* === Private function declarations =========================================================== */
/* === Public variable definitions ============================================================= */
/* === Private variable definitions ============================================================ */
/* === Private function implementation ========================================================= */

/* === Public function implementation ========================================================= */
void ControladorAlarma(bool estado){
    PonchoBuzzer(poncho,estado);
    return;
}
void Refresh_display_Task(void *none){
    while (1){
        PonchoDrawDisplay(poncho); 
        vTaskDelay(pdMS_TO_TICKS(5)); //refresca el display cada 5ms
    }
}
void Control_Task(void *none){
    while (1){
        int teclas = 0;
        if(PonchoBotonAceptar(poncho))      { //Funciones lentas para checkear los botones? Probar si necesita optimizar
            teclas +=ACEPTAR;}
        if(PonchoBotonCancelar(poncho))     {
            teclas +=CANCELAR;}
        if(PonchoBotonFuncion(poncho,1))    {
            teclas +=F1;}
        if(PonchoBotonFuncion(poncho,2))    {
            teclas +=F2;}
        if(PonchoBotonFuncion(poncho,3))    {
            teclas +=F3;}
        if(PonchoBotonFuncion(poncho,4))    {
            teclas +=F4;}
        if(isHighF(poncho,1))    {
            teclas +=F1H;}
        if(isHighF(poncho,2))    {
            teclas +=F2H;}

        if(teclas || getEstado(controlador)>E_MOSTRAR_HORA){
            procesar(controlador,teclas); //La funcion es llamada en los flancos altos de la detección, así toma los casos de isHigh
        }
        vTaskDelay(pdMS_TO_TICKS(20)); //verifica las teclas cada 20ms
    }
}
void Timer_Task(void *none){
    while(1){
    taskENTER_CRITICAL();
    timerCtrl(controlador);
    taskEXIT_CRITICAL();
    vTaskDelay(TICKS_SEG_SO / CANTIDAD_TICKS_POR_SEGUNDO_RELOJ); //Es mas preciso llamar poco?
    }
}      
int main(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock /(TICKS_SEG_SO));
    poncho = PonchoInit();
    controlador = crearControlador(CANTIDAD_TICKS_POR_SEGUNDO_RELOJ, ControladorAlarma ,poncho); 
        xTaskCreate(Refresh_display_Task, "Refresh_display_Task", 256, NULL, tskIDLE_PRIORITY + 1,NULL);
        xTaskCreate(Control_Task, "Control_Task", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
        xTaskCreate(Timer_Task, "Timer_Task", 256, NULL,tskIDLE_PRIORITY + 4, NULL);
        vTaskStartScheduler();
}
/* === End of documentation ==================================================================== */
/** @} End of module definition for doxygen */
