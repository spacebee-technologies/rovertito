/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 16/05/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 *===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include <stddef.h>                     // Defines NULL
  #include <stdbool.h>                    // Defines true
  #include <stdlib.h>                     // Defines EXIT_FAILURE
  #include "definitions.h"                // SYS function prototypes


/*=====================[Implementaciones]==============================*/

/*========================================================================
  Funcion: main
  Descripcion: Inicia todos los modulos generados en Harmony. Crea las tareas e inicia el planificador
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
int main ( void )
{
    SYS_Initialize ( NULL );    //Inicializa todos los modulos generado en el Harmony

    while ( true )
    {
        SYS_Tasks ( );        //Mantenga las maquinas de estado de todos los modulos MPLAB Harmony sondeados.
                              //Si se utiliza FreeRTOS, en esta funcion se crean las tareas y se llama al planificador para que este realize los cambio de contextos.
                              //Si se utiliza FreeRTOS, esta funcion nunca retorna ya que esta misma llama al planificador y este ultimo intercambia el contexto de ejecucion entre las tareas creadas previamente y las nuevas tareas que se creen durante su ejecucion.
                              // Cuando se inicia el planificador de FreeRTOS no se retorna nunca al menos que el micro se quede sin ram. Por lo tanto lo que sigue de aca en adelante no se ejecutara nunca.
        /* La ejecucion no deberaa venir aqui� durante el funcionamiento normal si no se usa FreeRtos*/
    }
    return ( EXIT_FAILURE );
}

