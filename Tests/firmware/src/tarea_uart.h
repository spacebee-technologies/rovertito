/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2022
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripción: Tarea encargada de recibir los datos por uart y procesarlos
 *===========================================================================*/

/*=======================[Definiciones]================================*/
  #ifndef _TAREA_UART_H
  #define _TAREA_UART_H
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
  void TAREA_UART_Initialize ( void );
  void TAREA_UART_Tasks( void );

/*=====================[Variables]================================*/
  typedef enum                  //Sirve para definir constantes enteras con nombre en C
  {
      TAREA_UART_STATE_INIT=0,
      TAREA_UART_STATE_SERVICE_TASKS,
  } TAREA_UART_STATES;          //Numeracion para establecer el estado de la tarea


  typedef struct
  {
      TAREA_UART_STATES state;  //Variable del tipo Numeracion para establecer el estado de la tarea
  } TAREA_UART_DATA;            //Estructura que contiene los datos de la tarea



//DOM-IGNORE-BEGIN
  #ifdef __cplusplus
  }
  #endif
//DOM-IGNORE-END

#endif /* _TAREA_UART_H */

