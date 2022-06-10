/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2022
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripción: Obtiene la letra recibida por uart y envia una trama por can
 *===========================================================================*/

/*==================[Definiciones]================================*/
#ifndef _TAREA_PRINCIPAL_H
#define _TAREA_PRINCIPAL_H

/*=====================[ Inclusiones ]============================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "configuration.h"

  
// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

/*=====================[Variables]================================*/
typedef enum
{
    TAREA_PRINCIPAL_STATE_INIT=0,
    TAREA_PRINCIPAL_STATE_SERVICE_TASKS,
} TAREA_PRINCIPAL_STATES;

typedef struct
{
    TAREA_PRINCIPAL_STATES state;
} TAREA_PRINCIPAL_DATA;


/*=================[Prototipos de funciones]======================*/
void TAREA_PRINCIPAL_Initialize ( void );
void TAREA_PRINCIPAL_Tasks( void );

//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _TAREA_PRINCIPAL_H */

/*******************************************************************************
 End of File
 */

