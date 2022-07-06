/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 05/07/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripcion: Codigo de ejemplo para manejo de mensajes SDO en modo server mediante CANopen usando el periferico MCAN de la samv71
 *              Esta codigo funciona para FreeRTOS
*===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include "../CANopen/CANopen.h"
  
/*=====================[Implementaciones]==============================*/

/*========================================================================
  Funcion: TAREA
  Descripcion: Recibo mensaje SDO por CANopen
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/

void TAREA(void *pvParameters ){
  //Inicializo CANopen
  uint8_t resultado=CANopen_init();
  if (resultado == 0){ Uart1_println("CANopen was initialized and is in pre-operational mode"); }
  if (resultado == 1){ Uart1_println("No se pudo crear el bloqueo mutex"); }
  if (resultado == 2){ Uart1_println("Error al mandar mensaje Boot_Up");  CANopen_STOP(); }  
  
  while(true){

    //Verifica que llegue mensaje SDO para el id de este nodo server y en caso de llegar:
    //Si es de escritura, escribe el diccionario e guarda en las siguientes variables el index y subindex modificado y envia mensaje SDO de confirmacion por can.
    //Si es de lectura, busca en el diccionario y envia el dato leido por can. Ademas guarda en las siguientes variables el index y subindex donde se tomo el dato enviado
    uint16_t index=0x0000;
    uint8_t  subindex=0x00;

    uint8_t res = CANopen_SDO_Expedited_Read(uint16_t *index, uint8_t *subindex);

    if(res==0){
        Uart1_println("Mensaje SDO recibido yse escribio dato sobre diccionario");
    }else{
        if(res==4){
        Uart1_println("Mensaje SDO recibido y dato de diccioanrio enviado correctamente");
        }else{
            Uart1_println("Error");
        }
    }

    vTaskDelay(1500 / portTICK_PERIOD_MS );               //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
  }
  
  
  if(xTAREA != NULL){vTaskDelete(xTAREA); xTAREA=NULL;} //Elimino esta tarea
}

/*========================================================================
  Funcion: TAREA2
  Descripcion: Leo diccionario para obterner variable modificada por otro nodo CANopen y poder usarla en algun proceso
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void TAREA2(void *pvParameters ){
  while(true){

    //Lee diccionario para usar el dato de 32 bits de la variable que previamente se modifico mediante CANopen
    int8_t data[3]={0};
    uint16_t index=0x607A;
    uint8_t subindex=0x00;
    bool res = CANopen_Read_Dictionary(index, subindex, data, 32);
    
    if(res == true){
        Uart1_println("Lectura correcta");
        //El dato leido se almacena en data[]
    }else{
        Uart1_println("Error al leer diccionario");
    }

    vTaskDelay(1500 / portTICK_PERIOD_MS );               //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
  }
  
  if(xTAREA != NULL){vTaskDelete(xTAREA); xTAREA=NULL;} //Elimino esta tarea
}