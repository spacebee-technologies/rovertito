/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2022
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 *===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include "tarea_2.h"
  #include "definitions.h"

/*=====================[Variables]================================*/
  TAREA_2_DATA tarea_2Data;

/*===============[Implementaciones]==============================*/
void TAREA_2_Initialize ( void )
{
    tarea_2Data.state = TAREA_2_STATE_INIT;
}

void TAREA_2_Tasks ( void )
{
   vTaskDelay(500 / portTICK_PERIOD_MS );
}
