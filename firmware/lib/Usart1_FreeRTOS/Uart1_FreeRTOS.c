/*=============================================================================
 * Author: Spacebeetech - Navegación
 * Date: 16/05/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripción: Tarea encargada de recibir los datos por uart y procesarlos
 * Funcionamiento: La funcion Uart1_FreeRTOS_Config crea la TAREA_UART_Tasks y un semaforo para proteger el intercambio de datos con el periferico ya que es un recurso compartido entre diferentes tareas. 
 *                 TAREA_UART_Tasks al iniciarse configura el periferico uart como interrupcion cuando el buffer uart llega a x bytes previamente establecidos.
 *                 Ademas el iniciarse esta tarea tambien crea un semaforo binario, este semaforo sirve para bloquear o habilitar la tarea dependiendo si hay datos en el buffer para leer o no.
 *                 Luego de iniciar el semaforo y configurar la int de uart, se entra un bucle infinito. Dentro de este bucle se intenta tomar el semaforo, si el semaforo habilita, se leen los datos del buffer uart.
 *                 en caso de que no haya dato en el buffer, el semaforo no estara habilitado por la interrupcion y por ende bloqueara la tarea TAREA_UART_Tasks hasta que llegue el dato uart permitiendo que otras tareas entren en contexto y ejecución.
 *
 *                 Para recibir un dato, se debe llemar a la funcion Uart1_Recibir(). Esta funcion retorna el mensaje obtenido por la tarea explicada anteriormente.
 *                 Para enviar mensaje se puede usar Uart1_print() o Uart1_println()
 *===========================================================================*/

/*=====================[ Inclusiones ]============================*/
#include "Uart1_FreeRTOS.h"
#include "definitions.h"
#include "portmacro.h"
#include <string.h>

/*=================[Prototipos de funciones]======================*/
void uartReadEventHandler(USART_EVENT event, uintptr_t context );
void TAREA_UART_Initialize ( void );
void TAREA_UART_Tasks ( void *pvParameters );

/*=====================[Variables]================================*/
  TaskHandle_t xTAREA_UART_Tasks;                    //Puntero hacia la TAREA_UART_Tasks
  TAREA_UART_DATA tarea_uartData;                    //Estructura que contiene la informacion de la tarea como por ejemplo, el estado de esta
  static SemaphoreHandle_t dataRxSemaphore;          //Variable Mutex para semaforo binario para bloquer tarea uart hasta que se llame a la funcion de interupcion de uart. (uartReadEventHandler)
  static SemaphoreHandle_t Semaforo_uart;            //Mutex de semaforo utilizado para proteger el recurso compartido de UART con otras tareas
  uint8_t readByte=' ';                              //Variable para guardar el bit recibido por uart
    
/*=====================[Implementaciones]==============================*/

/*========================================================================
  Funcion: Uart1_FreeRTOS_Config
  Descripcion: Crea la TAREA_UART_Tasks para monitoriar continuamente recepcion de dato
  Parametro de entrada:
                        uint8_t prioridad: Priodirdad de la tarea a crear
  No retorna nada
  ========================================================================*/
void Uart1_FreeRTOS_Config (uint8_t prioridad)
{
    xTaskCreate((TaskFunction_t) TAREA_UART_Tasks, "TAREA_uart1", 512, NULL, prioridad, &xTAREA_UART_Tasks); //Creo tarea para uart1
    TAREA_UART_Initialize ();                               //Inicializo la maquina de estado del uart1
    Semaforo_uart = xSemaphoreCreateMutex();                //Creo semaforo para proteger el recurso compartido de UART con otras tareas
    if( Semaforo_uart == NULL)                              //Si no se creo el semaforo
    {
        /* No habi­a suficiente almacenamiento dinÃ¡mico de FreeRTOS disponible para que el semaforo se creara correctamente. */
        USART1_Write((uint8_t*)"No se pudo crear el bloqueo mutex", strlen("No se pudo crear el bloqueo mutex"));
    }
}

/*========================================================================
  Funcion: Uart1_print
  Descripcion: Envia una cadena de texto por el uart1
  Parametro de entrada:
                        uint8_t mensaje: Texto a enviar
  No retorna nada
  ========================================================================*/
void Uart1_print (const char * mensaje)
{
  xSemaphoreTake(Semaforo_uart, portMAX_DELAY);                      //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
  USART1_Write((uint8_t*)mensaje, strlen(mensaje));                  //Envio bytes por uart
  xSemaphoreGive(Semaforo_uart);                                     //Libero semaforo
}

/*========================================================================
  Funcion: Uart1_println
  Descripcion: Envia una cadena de texto por el uart1 con salto de linea y retorno de carro
  Parametro de entrada:
                        uint8_t mensaje: Texto a enviar
  No retorna nada
  ========================================================================*/
void Uart1_println (const char * mensaje)
{
  xSemaphoreTake(Semaforo_uart, portMAX_DELAY);                      //Tomo semaforo para proteger el envio por uart ya que es un recurso compartico con otras tareas
  USART1_Write((uint8_t*)mensaje, strlen(mensaje));                  //Envio bytes por uart
  USART1_Write((uint8_t*)"\r\n", strlen("\r\n"));                    //Envio salto de linea y retorno de carro
  xSemaphoreGive(Semaforo_uart);                                     //Libero semaforo
}

/*========================================================================
  Funcion: Uart1_Recibir
  Descripcion: Envia una cadena de texto por el uart1
  Sin parametro de entrada
  Retorna el mensaje recibido previamente en la tarea_uart_task mediante el periferico uart1
  Nota importante: Se usa seccion critica para evitar inconveniente ya que es un recurso compartido
  ========================================================================*/
uint8_t Uart1_Recibir (void)
{
  portENTER_CRITICAL();                                  //Seccion critica para evitar que se ejecute cambio de contexto alterando el proceso de guardado de la variable
  return readByte;                                       //Retorno mensaje recibido previamente en la tarea tasc_uart_task
  readByte=' ';                                          //Limpio la variable
  portEXIT_CRITICAL();                                   //Salgo de seccion critica
}

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
  Nota importante: Se usa seccion critica para evitar inconveniente ya que es un recurso compartido
  ========================================================================*/
void TAREA_UART_Tasks ( void *pvParameters )
{
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
            portENTER_CRITICAL();                                            //Seccion critica para evitar que se ejecute cambio de contexto alterando el proceso de guardado de la variable
            USART1_Read(&readByte, 1);                                       //Obtengo el dato del buffer del periferico y lo almacena en la variable readByte
            portEXIT_CRITICAL();                                             //Salgo de seccion critica
        }
    }
}