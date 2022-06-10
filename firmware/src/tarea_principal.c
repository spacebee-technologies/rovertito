/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 16/05/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripción: Obtiene la letra recibida por uart y envia una trama por can
 *===========================================================================*/

/*=====================[ Inclusiones ]============================*/
#include "tarea_principal.h"
#include "definitions.h"
#include <string.h>
#include <stdio.h>
#include "mcan_fd_interrupt.h"

/*=====================[Variables]================================*/
  TAREA_PRINCIPAL_DATA tarea_principalData;       //Estructura que contiene la informacion de la tarea como por ejemplo, el estado de esta
  SemaphoreHandle_t uartMutexLock;                //Mutex de semaforo utilizado para proteger el recurso compartido de UART con otras tareas
  SemaphoreHandle_t canMutexLock;                 //Mutex de semaforo utilizado para proteger el recurso compartido de CAN con otras tareas
  extern uint8_t readByte_global;                 //Variable externa para guadar el byte leido por uart globalmente.
  TaskHandle_t xTAREA_Can;                        //Puntero hacia la tarea can
  TaskHandle_t xTAREA_Can1;                       //Puntero hacia la tarea can1
  TaskHandle_t xTAREA_Can2;                       //Puntero hacia la tarea can2
  uint8_t Can1MessageRAM[MCAN1_MESSAGE_RAM_CONFIG_SIZE] __attribute__((aligned (32)))__attribute__((space(data), section (".ram_nocache")));
  
  static uint32_t rx_messageID = 0;
  static uint8_t rx_message[64] = {0};
  static uint8_t rx_messageLength = 0;
  static uint16_t timestamp = 0;
  static MCAN_MSG_RX_FRAME_ATTRIBUTE msgFrameAttr = MCAN_MSG_RX_DATA_FRAME;


/*===================[Prototipos de funciones]=========================*/
  void TAREA_Can(void *pvParameters );
  void TAREA_Can1(void *pvParameters );
  void TAREA_Can2(void *pvParameters );
  
/*=====================[Implementaciones]==============================*/

  
/*========================================================================
  Funcion: TAREA_PRINCIPAL_Initialize
  Descripcion: Tarea para iniciar la maquina de estado de la tarea y para crear semaforo para proteger el recurso compartido de uart
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/  
void TAREA_PRINCIPAL_Initialize ( void )
{
    tarea_principalData.state = TAREA_PRINCIPAL_STATE_INIT; //Se inicia la maquina de estado mediante su estructura. Se establece en 1
    uartMutexLock = xSemaphoreCreateMutex();                //Creo semaforo para proteger el recurso compartido de UART con otras tareas
    if( uartMutexLock == NULL)                              //Si no se creo el semaforo
    {
        /* No había suficiente almacenamiento dinámico de FreeRTOS disponible para que el semáforo se creara correctamente. */
        USART1_Write((uint8_t*)"No se pudo crear el bloqueo mutex\r\n", strlen("No se pudo crear el bloqueo mutex\r\n"));  //Escribo por uart
    }
    
    canMutexLock = xSemaphoreCreateMutex();                //Creo semaforo para proteger el recurso compartido de CAN con otras tareas
    if(canMutexLock == NULL)                               //Si no se creo el semaforo
    {
        /* No había suficiente almacenamiento dinámico de FreeRTOS disponible para que el semáforo se creara correctamente. */
        USART1_Write((uint8_t*)"No se pudo crear el bloqueo mutex2\r\n", strlen("No se pudo crear el bloqueo mutex2\r\n"));  //Escribo por uart
    }
}


/*========================================================================
  Funcion: TAREA_PRINCIPAL_Tasks
  Descripcion: Tarea para interpretar dato recibido por uart y enviar trama can
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void TAREA_PRINCIPAL_Tasks ( void )
{
    mcan_fd_interrupt_config(Can1MessageRAM);               //Configuro memoria ram de mensaje can
    xTaskCreate((TaskFunction_t) TAREA_Can, "TAREA_Can", 512, NULL, 5, &xTAREA_Can);
    while (1)
    {
        portENTER_CRITICAL();                                  //Seccion critica para evitar que se ejecute cambio de contexto alterando el proceso de guardado de la variable
        uint8_t readByte = readByte_global;                    //Paso de variable global a variable local
        portEXIT_CRITICAL();                                   //Salgo de seccion critica

        if (readByte != ' ')                                   //Si el byte es distinto del caracter ' '
        { 
            xSemaphoreTake(uartMutexLock, portMAX_DELAY);      //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
            USART1_Write((uint8_t*)"Tecla: ", 7);              //Escribo 7 bytes por uart
            USART1_Write(&readByte, 1);                        //Escribo 1 byte por uart
            USART1_Write((uint8_t*)"\r\n", 2);                 //Escribo 2 byte por uart
            xSemaphoreGive(uartMutexLock);                     //Libero semaforo

            if (readByte == '1')                               //Si el dato recibido es el caracter 1
            {
                xTaskCreate((TaskFunction_t) TAREA_Can1, "TAREA_Can1", 512, NULL, 4, &xTAREA_Can1); //Creo tarea para envio trama 1 por can
            }

            if (readByte == '2')                               //Si el dato recibido es el caracter 2
            {
                xTaskCreate((TaskFunction_t) TAREA_Can2, "TAREA_Can2", 512, NULL, 4, &xTAREA_Can2); //Creo tarea para envio trama 2 por can
            }

            if (readByte == '3')                               //Si el dato recibido es el caracter 2
            {
              xSemaphoreTake(canMutexLock, portMAX_DELAY);     //Tomo semaforo para proteger el bus can ya que es un recurso compartico con otras tareas
              bool retorno = mcan_fd_interrupt_recibir(&rx_messageID, rx_message, &rx_messageLength, &timestamp, MCAN_MSG_ATTR_RX_BUFFER, &msgFrameAttr);
              if (retorno == false)  
              {
                  xSemaphoreTake(uartMutexLock, portMAX_DELAY);     //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
                  USART1_Write((uint8_t*)"Message Reception Failed \r\n", strlen("Message Reception Failed \r\n")); 
                  xSemaphoreGive(uartMutexLock);                    //Libero semaforo
              }
              xSemaphoreGive(canMutexLock);                                        //Libero semaforo
            }

            portENTER_CRITICAL();                              //Seccion critica para evitar que se ejecute cambio de contexto alterando el proceso de guardado de la variable
            readByte_global = ' ';                             //Limpio la variable para esperar a un nuevo dato recibido
            portEXIT_CRITICAL();                               //Salgo de la seccion critica

        }else{

            xSemaphoreTake(uartMutexLock, portMAX_DELAY);     //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
            USART1_Write((uint8_t*)"Esperando tecla...\r\n", 20); //Envio 20 bytes por uart
            xSemaphoreGive(uartMutexLock);                    //Libero semaforo
        }

        vTaskDelay(1500 / portTICK_PERIOD_MS );               //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
    }
}


/*========================================================================
  Funcion: TAREA_Can1
  Descripcion: Envio la trama 1 por canbus
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void TAREA_Can1(void *pvParameters ){
  xSemaphoreTake(canMutexLock, portMAX_DELAY);                         //Tomo semaforo para proteger el bus can ya que es un recurso compartico con otras tareas
  static uint8_t message[4] = {0}; message[0]='T'; message[1]='I'; message[2]='T'; message[3]='O';
  bool retorno = mcan_fd_interrupt_enviar((uint32_t) 0x45A, message, 4, MCAN_MODE_FD_WITH_BRS, MCAN_MSG_ATTR_TX_FIFO_DATA_FRAME); //Envio trama por can bus
  if ( retorno == false)
  {
    xSemaphoreTake(uartMutexLock, portMAX_DELAY);                      //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
    USART1_Write((uint8_t*)"Error\r\n", 7);                            //Envio 7 bytes por uart
    xSemaphoreGive(uartMutexLock);                                     //Libero semaforo
  }             
  //mcan_fd_interrupt_habilitar();                                     //Libero la maquina de estado del mcan para que otra tarea o funcion pueda enviar o recibir por can
  xSemaphoreGive(canMutexLock);                                        //Libero semaforo
  if(xTAREA_Can1 != NULL){vTaskDelete(xTAREA_Can1); xTAREA_Can1=NULL;} //Elimino esta tarea
}

/*========================================================================
  Funcion: TAREA_Can2
  Descripcion: Envio la trama 2 por canbus
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void TAREA_Can2(void *pvParameters ){
  xSemaphoreTake(canMutexLock, portMAX_DELAY);                         //Tomo semaforo para proteger el bus can ya que es un recurso compartico con otras tareas
  static uint8_t message[64] = {0};
  message[0]='R'; message[1]='o'; message[2]='v'; message[3]='e'; message[4]='r';
  bool retorno = mcan_fd_interrupt_enviar((uint32_t) 0x469, message, 8, MCAN_MODE_NORMAL, MCAN_MSG_ATTR_TX_FIFO_DATA_FRAME); //Envio trama por can bus
  
  if ( retorno == false)
  {
    xSemaphoreTake(uartMutexLock, portMAX_DELAY);                      //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
    USART1_Write((uint8_t*)"Error\r\n", 7);                            //Envio 7 bytes por uart
    xSemaphoreGive(uartMutexLock);                                     //Libero semaforo
  }             
  //mcan_fd_interrupt_habilitar();                                     //Libero la maquina de estado del mcan para que otra tarea o funcion pueda enviar o recibir por can
  xSemaphoreGive(canMutexLock);                                        //Libero semaforo

  xSemaphoreTake(uartMutexLock, portMAX_DELAY);                        //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
  USART1_Write((uint8_t*)"Elimino\r\n", 9);                            //Envio 9 bytes por uart
  xSemaphoreGive(uartMutexLock);                                       //Libero semaforo
  if(xTAREA_Can2 != NULL){vTaskDelete(xTAREA_Can2); xTAREA_Can2=NULL;} //Elimino esta tarea
}


/*========================================================================
  Funcion: TAREA_Can
  Descripcion: Tarea para monitorear el estado del bus can
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void TAREA_Can(void *pvParameters ){
  volatile static CAN_ESTADO estado = CAN_LIBRE;                                //Variable para guardar el estado de la aplicación CAN
  while ( true )
  {
    estado=Resultado();
    switch (estado)
    {
        case CAN_LIBRE:                                    
        {
            break;
        }
        case CAN_RECEPCION_OK:                                    
        {
            xSemaphoreTake(uartMutexLock, portMAX_DELAY);     //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
            USART1_Write((uint8_t*)" New Message Received: \r\n", strlen(" New Message Received: \r\n")); 
            uint8_t length = rx_messageLength;
            //printf(" Message - Timestamp : 0x%x ID : 0x%x Length : 0x%x ", (unsigned int)timestamp, (unsigned int)rx_messageID, (unsigned int)rx_messageLength);
            while(length)
            {
                uint8_t dato=rx_message[rx_messageLength - length--];
                char destino[5]="     ";
                sprintf(destino, "0x%x ", dato);
                USART1_Write((uint8_t*)destino, 5);
            }
            USART1_Write((uint8_t*)"\r\n", strlen("\r\n"));
            xSemaphoreGive(uartMutexLock);                    //Libero semaforo
            mcan_fd_interrupt_habilitar();
            break;
        }
        case CAN_TRANSMICION_OK:                                        
        {
            xSemaphoreTake(uartMutexLock, portMAX_DELAY);     //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
            USART1_Write((uint8_t*)" Success\r\n", strlen(" Success\r\n")); 
            xSemaphoreGive(uartMutexLock);                    //Libero semaforo
            mcan_fd_interrupt_habilitar();
            break;
        }
        case CAN_RECEPCION_ERROR:                                    
        {
            xSemaphoreTake(uartMutexLock, portMAX_DELAY);     //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
            USART1_Write((uint8_t*)" Error in received message\r\n", strlen(" Error in received message\r\n")); 
            xSemaphoreGive(uartMutexLock);                    //Libero semaforo
            mcan_fd_interrupt_habilitar();
            break;
        }
        case CAN_TRANSMICION_ERROR:                                        
        {
            xSemaphoreTake(uartMutexLock, portMAX_DELAY);     //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
            USART1_Write((uint8_t*)" Failed\r\n", strlen(" Failed\r\n")); 
            xSemaphoreGive(uartMutexLock);                    //Libero semaforo
            mcan_fd_interrupt_habilitar();
            break;
        }
        default:                                                                
            break;
    }
    vTaskDelay(1500 / portTICK_PERIOD_MS );                   //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
  }
  if(xTAREA_Can != NULL){vTaskDelete(xTAREA_Can); xTAREA_Can=NULL;} //Elimino esta tarea
}
