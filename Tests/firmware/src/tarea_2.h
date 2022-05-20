/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2022
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 *===========================================================================*/
/*==================[Definiciones]================================*/
  #ifndef _TAREA_2_H
  #define _TAREA_2_H
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

/*=====================[Variables]================================*/
typedef enum
{
    /* Application's state machine's initial state. */
    TAREA_2_STATE_INIT=0,
    TAREA_2_STATE_SERVICE_TASKS,
    /* TODO: Define states used by the application state machine. */

} TAREA_2_STATES;

typedef struct
{
    /* The application's current state */
    TAREA_2_STATES state;

    /* TODO: Define any additional data used by the application. */

} TAREA_2_DATA;

/*=================[Prototipos de funciones]======================*/
void TAREA_2_Initialize ( void );
void TAREA_2_Tasks( void );


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _TAREA_2_H */

