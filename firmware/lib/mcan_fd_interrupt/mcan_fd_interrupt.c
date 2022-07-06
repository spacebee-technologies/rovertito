/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 16/05/2022 
 * Update: 19/05/2022
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * DescripciÃ³n: Libreria CAN bus en modo fd por interrupcion
 *              Funcionamiento: paso 1: Se configura la ram del mensaje can mcan_fd_interrupt_config() y la maquina de estado de can empieza como APP_STATE_MCAN_USER_INPUT
 *                              paso 2: Llamar mcan_fd_interrupt_enviar() o mcan_fd_interrupt_recibir()
 *                                      cuando se llama a estas funciones, el estado can cambia de APP_STATE_MCAN_USER_INPUT a  APP_STATE_MCAN_IDLE (can incativo)
 *                              paso 3: Cuando se finaliza la transmicion o recepcion, se llama automaticamente a la funcion APP_MCAN_Callback() la cual cambia el estado de la maquina de estados de can
 *                                      a APP_STATE_MCAN_XFER_SUCCESSFUL si todo salio bien o a APP_STATE_MCAN_XFER_ERROR si algo salio mal
 *                              paso 4: Verificar con una tarea el estado de la maquina de estados de mcan y cuando sea APP_STATE_MCAN_XFER_SUCCESSFUL procesar el dato recibido.
 *===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include "mcan_fd_interrupt.h"
  #include <stddef.h>                       //Define NULL
  #include <stdbool.h>                      //Define true
  #include <stdlib.h>                       //Define EXIT_FAILURE
  #include <stdio.h>                        //Para sizeof
  #include "definitions.h"                  //Prototipos de funciones SYS

/*=====================[Variables]================================*/
    typedef enum
    {
        APP_STATE_MCAN_RECEIVE,             //Estado Recibiendo mensaje
        APP_STATE_MCAN_TRANSMIT,            //Estado transmitiendo mensaje
        APP_STATE_MCAN_IDLE,                //Estado mcan inactivo
        APP_STATE_MCAN_XFER_SUCCESSFUL,     //Estado mensaje recibido o transmitido correctamente
        APP_STATE_MCAN_XFER_ERROR,          //Estado mensaje recibido o transmitido erroneamente
        APP_STATE_MCAN_USER_INPUT           //Esperando al usuario para enviar o recibir mensaje
    } APP_STATES;                           //Enumaracion de los estados posibles

    static uint32_t xferContext = 0;        //Contexto global de can
    volatile static APP_STATES state = APP_STATE_MCAN_USER_INPUT; //Variable para guardar el estado de la aplicaciÃ³n

/*========================[Prototipos]=================================*/    


 
/*=====================[Implementaciones]==============================*/

/*========================================================================
  Funcion: APP_MCAN_Callback
  Descripcion: Envia mensaje por canbus
  Parametro de entrada:
                        uintptr_t context:
   No retorna nada
  ========================================================================*/
void APP_MCAN_Callback(uintptr_t context)
{
    xferContext = context;                                  //Valorizo el contexto global
    uint32_t status = MCAN1_ErrorGet();                     //Comprueba el estado de MCAN
    if (((status & MCAN_PSR_LEC_Msk) == MCAN_ERROR_NONE) || ((status & MCAN_PSR_LEC_Msk) == MCAN_PSR_LEC_NO_CHANGE))    //Si no hay error can o si no hay cambio can
    {
        switch ((APP_STATES)context)                        //Segun el contexto:
        {
            case APP_STATE_MCAN_RECEIVE:
            case APP_STATE_MCAN_TRANSMIT:                   //Si el contexto del callback es por transmicion o recepcion de can
            {
                state = APP_STATE_MCAN_XFER_SUCCESSFUL;     //Estado mensaje recibido o transmitido correctamente
                break;                                      //Salgo de switch
            }
            default:                                        //En cualquier otro contexto
                break;                                      //Salgo de switch
        }
    }
    else                                                    //
    {
        state = APP_STATE_MCAN_XFER_ERROR;                  //Estado mensaje recibido o transmitido erroneamente
    }
}


/*========================================================================
  Funcion: mcan_fd_interrupt_config
  Descripcion: Establece la configuraciÃ³n de RAM de mensajes can. Previamente se debe haber llamado a MCAN1_Initialize para la instancia de MCAN asociada.
  Parametro de entrada:
                        uint8_t *msgRAMConfigBaseAddress: Puntero a la direcciÃ³n base del bÃºfer asignada a la aplicaciÃ³n. La aplicaciÃ³n debe asignar el bÃºfer desde la memoria contigua no almacenada en cachÃ© y el tamaÃ±o del bÃºfer debe ser MCAN1_MESSAGE_RAM_CONFIG_SIZE
                                Ej: uint8_t Can1MessageRAM[MCAN1_MESSAGE_RAM_CONFIG_SIZE] __attribute__((aligned (32)))__attribute__((space(data), section (".ram_nocache")));
  No retorna nada
  ========================================================================*/
void mcan_fd_interrupt_config(uint8_t *msgRAMConfigBaseAddress){
    MCAN1_MessageRAMConfigSet(msgRAMConfigBaseAddress);     //Establece configuraciÃ³n de RAM de mensajes
}


/*========================================================================
  Funcion: mcan_fd_interrupt_enviar
  Descripcion: Envia mensaje por canbus
  Parametro de entrada:
                        uint32_t messageID:     Id can a donde enviar el mensaje de 11 bits/29 bits.
                        uint8_t *message:       Puntero del mensaje a enviar
                        uint8_t messageLength:  TamaÃ±o del arreglo a enviar
                        MCAN_MODE MCAN_MODE:    Modo de operacion can
                                                Mensajes FD (can FD) hata 64 bytes:                   MCAN_MODE_FD_WITH_BRS
                                                Mensajes estandar normal (can clasico) hasta 8 bytes: MCAN_MODE_NORMAL
                        MCAN_MSG_ATTR_TX:       Trama de datos o trama remota usando Tx FIFO o Tx Buffer.
                                                MCAN_MSG_ATTR_TX_FIFO_DATA_FRAME
  Retorna: dato bool indicando si se pudo transmitir el mensaje true o false.
  ========================================================================*/
bool mcan_fd_interrupt_enviar(uint32_t messageID , uint8_t *message, uint8_t messageLength, MCAN_MODE MCAN_MODE_L, MCAN_MSG_TX_ATTRIBUTE MCAN_MSG_ATTR_TX){
    if (state == APP_STATE_MCAN_USER_INPUT)                                                     //Si se esta esperando al usuario para enviar o recibir mensaje
        {           
            MCAN1_TxCallbackRegister( APP_MCAN_Callback, (uintptr_t)APP_STATE_MCAN_TRANSMIT );  //Establece el puntero a la funciÃ³n (y su contexto) que se llamarÃ¡ cuando ocurran los eventos de transferencia del MCAN dado. Previamente se debe haber llamado a MCAN1_Initialize para la instancia de MCAN asociada.
            state = APP_STATE_MCAN_IDLE;                                                        //Establezco el estado en estado mcan inactivo    
            bool Resultado = MCAN1_MessageTransmit(messageID, messageLength, message, MCAN_MODE_L, MCAN_MSG_ATTR_TX); //Transmite un mensaje al bus CAN. Previamente se debe haber llamado a MCAN1_Initialize para la instancia de MCAN asociada.
            return Resultado;                                                                   //Retorno resultado. True si se puedo trasmitir o false si no se puedo transmitir.
        }
    return false;                                                                              //Retorno falso si no se esperaba al usuario para enviar o recibir mensaje
}


/*========================================================================
  Funcion: mcan_fd_interrupt_recibir
  Descripcion: Recibe mensaje por canbus
  Parametro de entrada:
                        uint32_t *rx_messageID:     Id can del mensaje recibido (de 11 bits/29 bits).
                        uint8_t *rx_message:        Puntero de la variable donde guardar el mensaje
                        uint8_t *rx_messageLength:  Puntero de la variable donde guardar el tamaÃ±o del mensaje
                        uint16_t *timestamp:        Puntero a la marca de tiempo del mensaje Rx, el valor de la marca de tiempo es 0 si la marca de tiempo estÃ¡ deshabilitada
                        MCAN_MSG_ATTR_RX:           Trama de datos o trama remota usando Tx FIFO o Tx Buffer. Mensaje para ser leÃ­do desde Rx FIFO0 o Rx FIFO1 o Rx Buffer
                                                    MCAN_MSG_ATTR_RX_BUFFER
                                                    MCAN_MSG_ATTR_RX_FIFO0
                                                    MCAN_MSG_ATTR_RX_FIFO1
                        msgFrameAttr:               Trama de datos o trama remota a recibir  ej:  MCAN_MSG_RX_DATA_FRAME
  Retorna: dato bool indicando si se pudo transmitir el mensaje true o false.
  ========================================================================*/
bool mcan_fd_interrupt_recibir(uint32_t *rx_messageID, uint8_t *rx_message, uint8_t *rx_messageLength, uint16_t *timestamp, MCAN_MSG_RX_ATTRIBUTE MCAN_MSG_ATTR_RX, MCAN_MSG_RX_FRAME_ATTRIBUTE *msgFrameAttr){  
    if (state == APP_STATE_MCAN_USER_INPUT)                                                                     //Si se esta esperando al usuario para enviar o recibir mensaje
        {
            MCAN1_RxCallbackRegister( APP_MCAN_Callback, (uintptr_t)APP_STATE_MCAN_RECEIVE, MCAN_MSG_ATTR_RX); //Establece el puntero a la funciÃ³n (y su contexto) que se llamarÃ¡ cuando ocurran los eventos de transferencia del MCAN dado. Previamente se debe haber llamado a MCAN1_Initialize para la instancia de MCAN asociada.
            state = APP_STATE_MCAN_IDLE;                                                                       //Establezco el estado en estado mcan inactivo
            uint8_t rx_message_size=sizeof(rx_message);                                                        //Obtengo tamaÃ±o del mensaje
            memset(rx_message, 0x00, rx_message_size);                                                         //memset(void *str, int c, size_t n) copia el carÃ¡cter c (un carÃ¡cter sin signo) en los primeros n caracteres de la cadena a la que apunta el argumento str .
            bool resultado = MCAN1_MessageReceive(rx_messageID, rx_messageLength, rx_message, timestamp, MCAN_MSG_ATTR_RX, msgFrameAttr); //Recibe un mensaje del bus CAN. Previamente se debe haber llamado a MCAN1_Initialize para la instancia de MCAN asociada.
            return resultado;                                                                                  //Retorno resultado. True si se puedo recibir o false si no se puedo recibir.
        }
    return false;                                                                                              //Retorno falso si no se esperaba al usuario para enviar o recibir mensaje
}

/*========================================================================
  Funcion: Resultado
  Descripcion: Indica el resultado del can bus luego de llamar mcan_fd_interrupt_enviar() o mcan_fd_interrupt_recibir() para determina si un mensaje se envio o recibio correctamente. 
               Esta pensado para llamar a esta funcion continuamente en una tarea para determinar si hay un nuevo dato recibido correctamente luego de llamar mcan_fd_interrupt_recibir().
               Esta pensado para llamar a esta funcion continuamente en una tarea para determinar si se envio correctamente el mensaje can luego de llamar a la funcion mcan_fd_interrupt_enviar().
  Sin parameto de entrada.
  Retorna: uint8_t estado: indica el estado del can bus:
                        0 = no se esta esperando una transmision o recepcion
                        1 = Se recibio correctamente un dato por can bus luego de llamar a la funcion mcan_fd_interrupt_recibir()
                        2 = Se transmitio correctamente un dato por can bus luego de llamar a la funcion mcan_fd_interrupt_enviar()
                        3 = Error al recibir un dato por can bus luego de llamar a la funcion mcan_fd_interrupt_recibir()
                        4 = Error al transmitir un dato por can bus luego de llamar a la funcion mcan_fd_interrupt_enviar()
  ========================================================================*/
uint8_t Resultado(void){
    uint8_t resultado=0;                                                        //Variable para retornar
    switch (state)
    {
        case APP_STATE_MCAN_XFER_SUCCESSFUL:                                    //Si la transmicion o recepcion se realizo con exito
        {
            if ((APP_STATES)xferContext == APP_STATE_MCAN_RECEIVE)              //Si el contexto era de recepcion
            {
                resultado=1;                                                    //Se recibio correctamente
            } 
            else if ((APP_STATES)xferContext == APP_STATE_MCAN_TRANSMIT)        //Si el contexto era de transmision
            {
                resultado=2;                                                    //Se transmitio correctamente
            }
            break;
        }
        case APP_STATE_MCAN_XFER_ERROR:                                         //Si la transmicion o recepcion fue erronea
        {
            if ((APP_STATES)xferContext == APP_STATE_MCAN_RECEIVE)              //Si el contecto era de recepcion
            {
                resultado=3;                                                    //Error al recibir mensaje
            }
            else                                                                //Sino era de transmicion
            {
                resultado=4;                                                    //Error al enviar mensaje
            }
            break;
        }
        default:                                                                //En cualquier otro estado del can
            break;
    }
    return resultado;                                                           //Retorna el resultado
}

/*========================================================================
  Funcion: mcan_fd_interrupt_habilitar()
  Descripcion: Establece el estado de la maquina de estado de can en user_input para poder configurar una nueva transmicion o recepcion de datos can
               Recordar que las funciones mcan_fd_interrupt_recibir() y mcan_fd_interrupt_enviar() no se ejecutan si el estado de la maquina no es  APP_STATE_MCAN_USER_INPUT.
               Con esto se evita que si todavia no se tomo el dato que llego por can, no se sobreescriba.
  Sin parameto de entrada.
  No retorna nada
  ========================================================================*/
void mcan_fd_interrupt_habilitar(void){
    state = APP_STATE_MCAN_USER_INPUT;
}