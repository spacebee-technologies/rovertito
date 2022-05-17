/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2021 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripción: Obtiene la letra recibida por uart y envia una trama por can
 *===========================================================================*/

/*==================[Definiciones]================================*/
  #ifndef _TAREA_PRINCIPAL_H
  #define _TAREA_PRINCIPAL_H
  // DOM-IGNORE-BEGIN
  #ifdef __cplusplus  // Provide C++ Compatibility

  extern "C" {

  #endif
  // DOM-IGNORE-END

/*=====================[ Inclusiones ]============================*/
  #include <stdint.h>
  #include <stdbool.h>
  #include <stddef.h>
  #include <stdlib.h>
  #include "configuration.h"

/*=================[Prototipos de funciones]======================*/
  void TAREA_PRINCIPAL_Initialize ( void );
  void TAREA_PRINCIPAL_Tasks( void );

/*=====================[Variables]================================*/
typedef enum                                //Sirve para definir constantes enteras con nombre en C
{
    TAREA_PRINCIPAL_STATE_INIT=0,
    TAREA_PRINCIPAL_STATE_SERVICE_TASKS,
} TAREA_PRINCIPAL_STATES;                   //Numeracion para establecer el estado de la tarea

typedef struct
{
    TAREA_PRINCIPAL_STATES state;           //Variable del tipo Numeracion para establecer el estado de la tarea
} TAREA_PRINCIPAL_DATA;                     //Estructura que contiene los datos de la tarea


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _TAREA_PRINCIPAL_H */

