/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripción: Libreria CAN bus en modo fd por interrupcion
 *===========================================================================*/

/*==================[Definiciones]================================*/
  #ifndef _MCAN_FD_INTERRUPT_H
  #define _MCAN_FD_INTERRUPT_H
  // DOM-IGNORE-BEGIN
  #ifdef __cplusplus  // Provide C++ Compatibility

  extern "C" {

  #endif
  // DOM-IGNORE-END

/*=====================[ Inclusiones ]============================*/
  #include <stddef.h>                     //Define NULL
  #include <stdbool.h>                    //Define true
  #include <stdlib.h>                     //Define EXIT_FAILURE
  #include "definitions.h"                //Prototipos de funciones SYS

/*=================[Prototipos de funciones]======================*/
 void mcan_fd_interrupt_config(uint8_t *msgRAMConfigBaseAddress);
 bool mcan_fd_interrupt_enviar(uint32_t messageID , uint8_t *message, uint8_t messageLength, MCAN_MODE MCAN_MODE, MCAN_MSG_TX_ATTRIBUTE MCAN_MSG_ATTR_TX);
 bool mcan_fd_interrupt_recibir(uint32_t *rx_messageID, uint8_t *rx_message, uint8_t *rx_messageLength, uint16_t *timestamp, MCAN_MSG_RX_ATTRIBUTE MCAN_MSG_ATTR_RX, MCAN_MSG_RX_FRAME_ATTRIBUTE *msgFrameAttr);
 uint8_t Resultado();
 void mcan_fd_interrupt_habilitar();


/*=====================[Variables]================================*/


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _TAREA_PRINCIPAL_H */

