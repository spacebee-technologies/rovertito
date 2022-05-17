/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2021 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripción: Obtiene la letra recibida por uart y envia una trama por can
 *===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include "tarea_principal.h"
  #include "definitions.h"
  #include <string.h>

/*=====================[Variables]================================*/
  TAREA_PRINCIPAL_DATA tarea_principalData;       //Estructura que contiene la informacion de la tarea como por ejemplo, el estado de esta
  SemaphoreHandle_t uartMutexLock;                //Mutex de semaforo utilizado para proteger el recurso compartido de UART con otras tareas
  extern uint8_t readByte_global;                 //Variable externa para guadar el byte leido por uart globalmente.

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
}


/*========================================================================
  Funcion: TAREA_PRINCIPAL_Tasks
  Descripcion: Tarea para interpretar dato recibido por uart y enviar trama can
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void TAREA_PRINCIPAL_Tasks ( void )
{

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

            if (readByte == 'L' || readByte == 'l')            //Si el dato recibido es el caracter 'L' o 'l' realizo blink
            {
                LED1_Toggle();
            }

            portENTER_CRITICAL();                             //Seccion critica para evitar que se ejecute cambio de contexto alterando el proceso de guardado de la variable
            readByte_global = ' ';                            //Limpio la variable para esperar a un nuevo dato recibido
            portEXIT_CRITICAL();                              //Salgo de la seccion critica

        }else{

            xSemaphoreTake(uartMutexLock, portMAX_DELAY);     //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
            USART1_Write((uint8_t*)"Esperando tecla...\r\n", 20); //Envio 20 bytes por uart
            xSemaphoreGive(uartMutexLock);                    //Libero semaforo
        }

        vTaskDelay(10 / portTICK_PERIOD_MS );                //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
    }
}
