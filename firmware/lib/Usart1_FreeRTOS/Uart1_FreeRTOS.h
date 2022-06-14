/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 16/05/2022
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripcion: Tarea encargada de recibir los datos por uart y procesarlos
 *===========================================================================*/

/*=======================[Definiciones]================================*/
#ifndef _Uart1_FreeRTOS_H
#define _Uart1_FreeRTOS_H

/*=====================[ Inclusiones ]============================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "configuration.h"

/*=====================[Variables]================================*/
typedef enum
   {
       TAREA_UART_STATE_INIT=0,
       TAREA_UART_STATE_SERVICE_TASKS,
   } TAREA_UART_STATES;


typedef struct
  {
      TAREA_UART_STATES state;  //Variable del tipo Numeracion para establecer el estado de la tarea
  } TAREA_UART_DATA;            //Estructura que contiene los datos de la tarea


/*=================[Prototipos de funciones]======================*/
void Uart1_FreeRTOS_Config ( uint8_t prioridad );
void Uart1_print (const char * mensaje);
void Uart1_println (const char * mensaje);
uint8_t Uart1_Recibir (void);

#endif /* _TAREA_UART_H */