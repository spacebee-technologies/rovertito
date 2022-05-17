/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2021 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripción: Tarea encargada de recibir los datos por uart y procesarlos
 *===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include "tarea_uart.h"
  #include "definitions.h"
  #include "portmacro.h"
  #include <string.h>

/*=================[Prototipos de funciones]======================*/

/*=====================[Variables]================================*/

  TAREA_UART_DATA tarea_uartData;             //Estructura que contiene la informacion de la tarea como por ejemplo, el estado de esta
  static SemaphoreHandle_t dataRxSemaphore;   //Variable Mutex para semaforo binario para bloquer tarea uart hasta que se llame a la funcion de interupcion de uart. (uartReadEventHandler)
  uint8_t readByte_global=' ';                //Variable para guadar el byte leido globalmente.

/*=====================[Implementaciones]==============================*/

/*========================================================================
  Funcion: uartReadEventHandler
  Descripcion: Callback que se llama al ocurrir una recepcio de datos por uart, esta funcion libera el semaforo que desbloquea la tarea uart
              para poder obtener el dato que se encuentra en el buffer del driver uart.
  Parametro de entrada: 
                          USART_EVENT event: tipo de evento del callback
                          uintptr_t context.
  No retorna nada
  ========================================================================*/
void uartReadEventHandler(USART_EVENT event, uintptr_t context )
{
    if (event == USART_EVENT_READ_THRESHOLD_REACHED)                         //UMBRAL DE LECTURA DE EVENTO USART ALCANZADO
    {
        BaseType_t xHigherPriorityTaskWoken;                                 //Variable para determinar si luego de la interrupcion se realiza cambio de contexto o no

        xSemaphoreGiveFromISR( dataRxSemaphore, &xHigherPriorityTaskWoken ); //Desbloqueo la tarea uart soltando el semáforo. El semáforo debe haber sido creado previamente
                                                                              //xSemaphoreGiveFromISR() establecerá *xHigherPriorityTaskWoken en pdTRUE si al proporcionar el semáforo se desbloqueó una tarea y la tarea desbloqueada tiene una prioridad más alta que la tarea que se está ejecutando actualmente. Si xSemaphoreGiveFromISR() establece este valor en pdTRUE, se debe solicitar un cambio de contexto antes de salir de la interrupción.
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );                      //Si xHigherPriorityTaskWoken se estableció en verdadero, deberíamos ceder
                                                                              //Pase el valor xHigherPriorityTaskWoken a portYIELD_FROM_ISR(). Si xHigherPriorityTaskWoken se configuró en pdTRUE dentro de xTimerPendFunctionCallFromISR(), al llamar a portYIELD_FROM_ISR() se solicitará un cambio de contexto. Si xHigherPriorityTaskWoken sigue siendo pdFALSE, llamar a portYIELD_FROM_ISR() no tendrá ningún efecto.
    } 
}



/*========================================================================
  Funcion: TAREA_UART_Initialize
  Descripcion: inicia la maquina de estado de la tarea uart
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void TAREA_UART_Initialize ( void )
{
    tarea_uartData.state = TAREA_UART_STATE_INIT; //Se inicia la maquina de estado mediante su estructura. Se establece en 1
}

/*========================================================================
  Funcion: TAREA_UART_Tasks
  Descripcion: Tarea para configurar y leer el uart
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void TAREA_UART_Tasks ( void )
{
    uint8_t readByte;                                                          //Variable para guardar el bit recibido por uart
    bool status = false;                                                       //Variable para indicar el estado de la tarea
    //TickType_t timeNow;

    USART1_ReadCallbackRegister(uartReadEventHandler, 0);                      //Se indica la funcion a llamar por interrupcion de uart y el contexto
    USART1_ReadThresholdSet(1);                                                //Esta API permite que la aplicación establezca un nivel de umbral en la cantidad de bytes de datos disponibles en el búfer de recepción. Una vez que se alcanza el umbral, se envía una notificación a la aplicación si está habilitada. 
    USART1_ReadNotificationEnable(true, false);                                //Esta API permite que la aplicación active o desactive la recepción de notificaciones. Además, la aplicación puede optar por recibir notificaciones de forma persistente hasta que la condición del umbral sea verdadera. Por ejemplo, si el umbral de recepción se establece en 5, se envía una notificación cuando el búfer de recepción interno tiene 5 bytes. Si la notificación persistente está desactivada, la aplicación recibe una notificación solo una vez cuando hay 5 bytes sin leer en el búfer de recepción. Sin embargo, si la notificación persistente está activada, la aplicación recibe una notificación cada vez que se recibe un byte y el búfer de recepción tiene 5 o más caracteres sin leer. En este caso, la notificación se detendrá cuando la aplicación lea los datos del búfer de recepción y el número de bytes que se generan en el búfer de recepción sea inferior a 5.
                                                                               //bool USART1_ReadNotificationEnable(bool isEnabled, bool isPersistent)
    dataRxSemaphore = xSemaphoreCreateBinary();                                //Creo semaforo binario. Ese se libara en la funcion uartReadEventHandler cuando el uart alcanza los byte del buffer e interrumpe

    if (dataRxSemaphore != NULL)                                               //Si se creo el semaforo
    {
        status = true;                                                        //Establesco en true el estado de la tarea
    }

    while (status == true)                                                    //Fin de la configuracion y bucle infinito
    {
        /* Block until a character is received on the terminal */
        if( xSemaphoreTake( dataRxSemaphore, portMAX_DELAY ) == pdTRUE )     //Bloquea esta tarea hasta recibir en numero de bytes configurados en el uart
        {
            while (USART1_Read(&readByte, 1) == true)                        //Mientras se pueda guardar el dato recibido en readByte
            {                                                                //size_t USART1_Read(uint8_t* pRdBuffer, const size_t size)
                portENTER_CRITICAL();                                        //Seccion critica para evitar que se ejecute cambio de contexto alterando el proceso de guardado de la variable
                readByte_global=readByte;                                    //Cambio variable local a global
                portEXIT_CRITICAL();                                         //Salgo de seccion critica
            }

        }
    }
}