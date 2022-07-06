/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 05/07/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripcion: Codigo de ejemplo para manejo de mensajes SDO en modo cliente mediante CANopen usando el periferico MCAN de la samv71
 *              Esta codigo funciona para FreeRTOS
*===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include "../CANopen/CANopen.h"
  
/*=====================[Implementaciones]==============================*/

/*========================================================================
  Funcion: TAREA
  Descripcion: Envio y recibo mensaje por CANopen
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/

void TAREA(void *pvParameters ){
  //Inicializo CANopen
  uint8_t resultado=CANopen_init();
  if (resultado == 0){ Uart1_println("CANopen was initialized and is in pre-operational mode"); }
  if (resultado == 1){ Uart1_println("No se pudo crear el bloqueo mutex"); }
  if (resultado == 2){ Uart1_println("No se pudo crear la tarea_CAN"); }
  if (resultado == 3){ Uart1_println("Error al mandar mensaje Boot_Up");  CANopen_STOP(); }  
  
  while(true){

    //Envio mensaje CANopen SDO de escritura de deccionario de 32 bites (4 bytes)
    uint8_t  Server_id=0x02;
    uint8_t  data[3]={0};  data[0]=0x00; data[1]=0x00; data[2]=0x00; data[3]=0x00;
    uint16_t index=0x607A;
    uint8_t  subindex=0x00;

    uint8_t res = CANopen_SDO_Expedited_Write(Server_id, 0x23, index, subindex, data, CANopen_SDO_mode_client);
    
    if(res==0){
        Uart1_println("Escritura correcta");
    }else{
        Uart1_println("Error al escribir diccionario");
    }

    //Envio mensaje CANopen SDO de lectura de deccionario  de 32 bites (4 bytes)
    uint8_t  Server_id2=0x03;
    uint8_t  data[3]={0};
    uint16_t index=0x607A;
    uint8_t  subindex=0x00;

    uint8_t res = CANopen_SDO_Expedited_Write(Server_id2, 0x40, index, subindex, data, CANopen_SDO_mode_client);
    

    if(res==0){
        Uart1_println("Lectura correcta");
        //El dato leido se almacena en data[]
    }else{
        Uart1_println("Error al leer diccionario");
    }

    vTaskDelay(1500 / portTICK_PERIOD_MS );               //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
  }
  
  
  if(xTAREA != NULL){vTaskDelete(xTAREA); xTAREA=NULL;} //Elimino esta tarea
}
