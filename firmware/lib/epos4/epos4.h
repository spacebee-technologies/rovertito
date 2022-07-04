/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 04/07/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripcion: Libreria driver EPOS4
 *===========================================================================*/

/*==================[Definiciones]================================*/
  #ifndef _epos4_H
  #define _epos4_H

/*=====================[ Inclusiones ]============================*/
  #include <stddef.h>                     //Define NULL
  #include <stdbool.h>                    //Define true
  #include <stdlib.h>                     //Define EXIT_FAILURE
  #include "definitions.h"                //Prototipos de funciones SYS
  #include "../CANopen/CANopen.h"
  
/*=================[Prototipos de funciones]======================*/
    bool Epos4_read_actual_position(uint8_t EPOS4_id, uint8_t *resultado);
    bool Epos4_write_target_position(uint8_t EPOS4_id,uint8_t *data);
/*=====================[Variables]================================*/
  
/*=====================[Definiciones]================================*/

  #endif

