/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 04/07/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripcion: Libreria driver EPOS4
 *===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include "../CANopen/CANopen.h"
  
/*=====================[Implementaciones]==============================*/

/*========================================================================
  Funcion: Epos4_read_actual_position
  Descripcion: Mando por canbus un mensaje SDO de solicitud de lectura (0x40) mediante can open para obtener la posicion
               Index 0x6064, Subindex 0x00
  Parametro de entrada: uint8_t EPOS4_id:      Id del driver
                        uint8_t *resultado:    Puntero a la variable uint8 de 4 bytes donde se va a guardar la posicion actual
  Retorno   true:   si se pudo leer correctamente
            false:  si no se pudo leer
  ========================================================================*/
bool Epos4_read_actual_position(uint8_t EPOS4_id, uint8_t *resultado){
  uint8_t data[3]={0};
  uint8_t res = CANopen_SDO_Expedited_Write(EPOS4_id, 0x40, 0x6064, 0x00, data, CANopen_SDO_mode_client);
  resultado=data;
  if(res==0){
    return true;
  }else{
    return false;
  }
}

/*========================================================================
  Funcion: Epos4_write_target_position()
  Descripcion: Mando por canbus un mensaje SDO de 4 bytes (0x23) mediante can open indicando la posicion
               Index 0x607A, Subindex 0x00: Data 0x000008AE  2222dec
  Parametro de entrada: uint8_t EPOS4_id: Id del driver
                        uint8_t *data:    Posicion a mandar
  Retorno   true:   si se pudo escribir correctamente
            false:  si no se pudo escribir
  ========================================================================*/
bool Epos4_write_target_position(uint8_t EPOS4_id,uint8_t *data){
  uint8_t res = CANopen_SDO_Expedited_Write(EPOS4_id, 0x23, 0x607A, 0x00, data, CANopen_SDO_mode_client);
  if(res==0){
    return true;
  }else{
    return false;
  }
}