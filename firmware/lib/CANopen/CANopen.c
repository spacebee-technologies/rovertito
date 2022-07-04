/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 30/06/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripcion: Libreria CANopen utilizando preiferico MCAN con libreria mcan_fd_interrupt.c
 *              Soporte SDO en modo Expedited Read y Expedited Write. No soporta lectura y escritura por segmentos.
 *              Proximas mejores: Optimizar diccionario, crear soporte PDO, sincronizacion, SDO abort code, codigos de error.
*===========================================================================*/

/*=====================[ Inclusiones ]============================*/
  #include "CANopen.h"
  #include "Dictionary.h"
  #define Boot_up 1     //1 Si se quiere enviar mensaje Boot-up luego de pasar al estado pre-operacional  o 0 para desactivar

/*=====================[Variables]================================*/
    typedef enum
    {
        CANopen_INITIALIZATION,             //Estado de inicializacion, luego de configurarse se pasa al siguiente estado
        CANopen_PRE_OPERATIONAL,            //Estado pre operacional (solo responde a comandos NMT, SDO, SYNC, Time stamp o Hearbeat)
        CANopen_OPERATIONAL,                //Estado operacional (se activa el soporte de PDOs ademas de los comandos anteriores)
        CANopen_STOPPED,                    //Estado parado, deja inactivos todos los objetos de comunicación posibles (no se pueden enviar ni recibir PDOs ni SDOs), excepto a los comandos NMT que pueden cambiar a cualquiera de los estados restantes y Heartbeat. Es decir, un nodo solo puede realizar vigilancia de nodos o latidos, pero no puede recibir ni transmitir mensajes.
    } CANopen_STATES;                       //Enumaracion de los estados posibles de la maquina de estado CANopen

    volatile static CANopen_STATES state = CANopen_INITIALIZATION; //Variable para guardar el estado de la aplicaciÃ³n
    
    

    //Variables para periferico CAN
    SemaphoreHandle_t canMutexLock;                 //Mutex de semaforo utilizado para proteger el recurso compartido de CAN con otras tareas
    uint8_t Can1MessageRAM[MCAN1_MESSAGE_RAM_CONFIG_SIZE] __attribute__((aligned (32)))__attribute__((space(data), section (".ram_nocache")));
    static uint32_t rx_messageID = 0;
    static uint8_t rx_message[64] = {0};
    static uint8_t rx_messageLength = 0;
    static uint16_t timestamp = 0;
    static MCAN_MSG_RX_FRAME_ATTRIBUTE msgFrameAttr = MCAN_MSG_RX_DATA_FRAME;

/*========================[Prototipos]=================================*/    
    bool CANopen_BootUp(void);

/*=====================[Implementaciones]==============================*/

/*========================================================================
  Funcion: CANopen_init
  Descripcion: Establece la configuracion del periferico mcan0 segun establece la libreria mcan_fd_interrupt.c y cambia el estado de la maquina de estado CANopen
  Sin parametro de entrada
  Rertorna: uint8_t   0 = resultado ok
                      1 = No se pudo crear el bloqueo mutex
                      2 = Error al mandar mensaje Boot_Up
  ========================================================================*/
uint8_t CANopen_init(void){
    mcan_fd_interrupt_config(Can1MessageRAM);               //Configuro memoria ram de mensaje can
    canMutexLock = xSemaphoreCreateMutex();                 //Creo semaforo para proteger el recurso compartido de CAN con otras tareas
    if(canMutexLock == NULL)                                //Si no se creo el semaforo
    {
        return 1; /* No habia suficiente almacenamiento dinamico de FreeRTOS disponible para que el semaforo se creara correctamente. */
    }

    mcan_fd_interrupt_config(Can1MessageRAM);               //Configuro memoria ram de mensaje can
    
    state = CANopen_PRE_OPERATIONAL;                                //Cambio estado de la maquina de estado de CANopen
    if(Boot_up == 1){                                       //Si esta activo el Boot_up
        if(CANopen_BootUp()==false){ return 2; }            //Envio mensaje de inicio y verifico errores
    }
    return 0;                                               //Retorno OK
}

/*========================================================================
  Funcion: Verify_writing_by_can
  Descripcion: Funcion para verificar el estado del can y el envio de mensaje can
  Sin parametro de entrada
  Retorna:     0 = si el mensaje se envio correctamente
               1 = si el mnesaje no se envio correctamente
  ========================================================================*/
uint8_t Verify_writing_by_can(void){
    volatile static CAN_ESTADO estado = CAN_LIBRE;         //Variable para guardar el estado de la aplicación CAN
    uint8_t interaccion=0;
    uint8_t retornar=9;                                        //Inicializo en un valor no utilizado
    while(true){                                              //Bloque para verificar envio correcto por periferico can. Posee timeout
        estado=Resultado();
        if(estado==CAN_TRANSMICION_OK){                       //Si la transmicion fue correcta
            mcan_fd_interrupt_habilitar();                    //Habilito mcan para seguir operando en el proximo mensaje sdo
            retornar=0;                                        //Se debe retornar ok
            break;                                            //Salgo del while
        }else{
            if(estado==CAN_TRANSMICION_ERROR){
                mcan_fd_interrupt_habilitar();                //Habilito mcan para seguir operando en el proximo mensaje sdo
                retornar=1;                                    //Se debe retornar error
                break;                                        //Salgo del while
            }else{
                interaccion++;                                //Incremento interaccion para timeout
            }
        }
        if(interaccion*1>=CAN_send_timeout){
            mcan_fd_interrupt_habilitar();                    //Habilito mcan para seguir operando en el proximo mensaje sdo
            retornar=1;                                        //Se debe retornar error
            break;                                            //Salgo del while
        }
        vTaskDelay(1 / portTICK_PERIOD_MS );                  //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
    }       

    return retornar; 
}


/*========================================================================
  Funcion: CANopen_SDO_Expedited_Write
  Descripcion: Envia un mensaje SDO
  Parametro de entrada:     uint8_t node_id  = id del nodo con el cual se desea comunicar si esta en modo cliente o el propio id del dispositivo si se esta en modo servidor
                            uint8_t command  = Describe la operacion del comando SDO (lectura/escritura), puede ser:
                                                0x22 = (rx) Solicitud de escritura > 4 bytes (no soportado en esta libreria)
                                                0x23 = (rx) Solicitud de escritura de 4 bytes
                                                0x2B = (rx) Solicitud de escritura de 2 bytes
                                                0x2F = (rx) Solicitud de escritura de 1 byte
                                                0x60 = (tx) Confirmacion de escritura a cliente (maestro) desde servidor (esclavo)
                                                0x40 = (rx) Solicitud de lectura (upload) - solicitud de parametro a servidor (esclavo)
                                                0x43 = (tx) Respuesta envio de parametro a cliente (4 bytes)
                                                0x4B = (tx) Respuesta envio de parametro a cliente (2 bytes)
                                                0x4F = (tx) Respuesta envio de parametro a cliente (1 byte)
                                                0x80 = (tx) Transferencia de datos abortada, envio mensaje de error a cliente (No sopoerado en esta libreria)
                            uint16_t index   = Indice del diccionario al que se desea acceder (2 bytes)
                            uint8_t subindex = Subindice deldiccionario al que se desea acceder (1 byte)
                            uint8_t data     = Dato a escribir en el diccionario, si es peticion de informacion data tiene que contener todos ceros (4 bytes) y luego se pasan los datos recibidos a esta variable
                            uint8_t mode     = Modo cliente (maestro) = 0  Modo servidor(esclavo) = 1
  Retorna:                  0  = si se envio y recibio correctamente
                            1  = si no se pudo enviar el mensaje
                            2  = si no se obtuvo respuesta del servidor (solo funciona con mensajes SDO en modo cliente)
                            3  = si la respuesta del servidor no corresponde con el mensaje enviado (solo funciona con mensajes SDO en modo cliente)
                            4  = si la maquina de estados no paso la inicializacion
  ========================================================================*/
uint8_t CANopen_SDO_Expedited_Write(uint8_t node_id, uint8_t command, uint16_t index, uint8_t subindex,  uint8_t *data, uint8_t mode){
    if(state==CANopen_PRE_OPERATIONAL || state==CANopen_OPERATIONAL){         //Si el estado del CANopen permite mensajes SDO
        xSemaphoreTake(canMutexLock, portMAX_DELAY);                         //Tomo semaforo para proteger el bus can ya que es un recurso compartico con otras tareas
        static uint8_t message[8] = {0}; 
        message[0]=command;                                                  //Byte 0 (comando SDO)
        message[1]=index & 0xFF;                                             //Byte 1 Index LSB (byte menos significativo)
        message[2]=index >> 8;                                               //Byte 2 Index MSB (byte mas significativo)
        message[3]=subindex;                                                 //Byte 3 Subindex
        message[4]=data[0];                                                  //Byte 4 = byte data 0
        message[5]=data[1];                                                  //Byte 4 = byte data 1
        message[6]=data[2];                                                  //Byte 4 = byte data 2
        message[7]=data[3];                                                  //Byte 4 = byte data 3
        uint32_t id =0x000;

        if(mode==CANopen_SDO_mode_client){
           id = 0x600 + node_id;
        }else{
           id = 0x580 + node_id;
        }
        
        //Envio mensaje por can
        bool retorno = mcan_fd_interrupt_enviar((uint32_t) id, message, 8, MCAN_MODE_FD_WITH_BRS, MCAN_MSG_ATTR_TX_FIFO_DATA_FRAME); //Envio trama por can bus
        if ( retorno == false){
            return 1;
        }

        if(Verify_writing_by_can()==1){
            return 1;
        }

        //Si esta en modo cliente, espero respuesta desde el servidor
        if(mode==CANopen_SDO_mode_client){
            //Configuro recepcion de mensaje can
            bool retorno3 = mcan_fd_interrupt_recibir(&rx_messageID, rx_message, &rx_messageLength, &timestamp, MCAN_MSG_ATTR_RX_BUFFER, &msgFrameAttr);
            if (retorno3 == false)  
            {
                xSemaphoreGive(canMutexLock);                      //Libero semaforo
                return 2;                                          //Retorno desde la funcion
            }
            volatile static CAN_ESTADO estado = CAN_LIBRE;         //Variable para guardar el estado de la aplicación CAN
            //Bucle infinito con timeout para verificar la recepcion en periferico can
            uint8_t interacciones=0;
            uint8_t retornar=9;                                    //Inicializo en un valor no utilizado
            while (true){
                estado=Resultado();
                switch (estado)
                {
                    case CAN_RECEPCION_OK:                                    
                    {
                        //Verifico que el mensaje recibido sea el correcto
                        if(rx_messageID!=0x580 + node_id){         //Si la respuesta no es del servidor o no es un mensaje sdo tx 
                            retornar = 3;
                            break;                                 //Salgo del switch
                        }
                        if(rx_message[0]==0x60||rx_message[0]==0x43||rx_message[0]==0x4B||rx_message[0]==0x4F){ //Si la respuesta es un comando de respuesta
                            if(rx_message[0]==0x60){               //Si el mensaje es de confirmacion de escritura
                                retornar = 0;
                                break;                             //Salgo del switch
                            }else{                                 //Sino verifico que el diccionario de respuesta corresponda con el enviado
                                if((rx_message[1]==(index & 0xFF)) && (rx_message[2]==(index >> 8))){
                                    //paso el dato recibido a la variable apuntada en la funcion
                                    data[0]=rx_message[4];
                                    data[1]=rx_message[5];
                                    data[2]=rx_message[6];
                                    data[3]=rx_message[7];
                                    retornar = 0;
                                    break;                         //Salgo del switch
                                }else{
                                    retornar = 3;
                                    break;                         //Salgo del switch
                                }
                            }
                        }else{                                     //Si la respuesta no es un comando de respuesta
                            retornar = 3;
                            break;                                 //Salgo del switch
                        }
                        
                    }
                    case CAN_RECEPCION_ERROR:                                    
                    {
                        retornar = 2;
                        break;                                      //Salgo del switch
                    }
                    default: 
                        interacciones++;
                        break;                                      //Salgo del switch
                }
                if(retornar == 0 || retornar == 2 || retornar == 3){//Si en el switch se establecio retorno
                    mcan_fd_interrupt_habilitar();                  //Habilito mcan para seguir operando en el proximo mensaje sdo
                    break;                                          //Salgo del while true
                }
                if(interacciones*1>=CANopen_SDO_timeout){
                    retornar = 2;
                    mcan_fd_interrupt_habilitar();                  //Habilito mcan para seguir operando en el proximo mensaje sdo
                    break;                                          //Salgo del while true
                }
                vTaskDelay(1 / portTICK_PERIOD_MS );                //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
            }
            xSemaphoreGive(canMutexLock);
            return retornar;                                        //Retorno desde la funcion
        }
        xSemaphoreGive(canMutexLock);                               //Libero semaforo
        return 0;                                                   //Retorno desde la funcion
    }else{
        return 4;                                                   //Retorno desde la funcion
    }
}

/*========================================================================
  Funcion: CANopen_SDO_Expedited_Read
  Descripcion: Recibe un mensaje can y comprueba que sea un mensaje SDO
  Parametro de entrada:     
  Retorna:                  0 = Mensaje SDO recibido y es escritura sobre diccionario
                            1 = Si se presenta error al recibir mensaje
                            2 = Si fallo la recepcion
                            3 = Si el mensaje recibido no es SDO Rx
                            4 = Mensaje SDO recibido y paramtros enviados correctamente
                            5 = Mensaje SDO recibido pero no es para este dispositivo (id diferente)
                            6 = Mensaje SDO recibido y paramtros enviados incorrectamente
  ========================================================================*/
uint8_t CANopen_SDO_Expedited_Read(uint16_t *index, uint8_t *subindex){
    xSemaphoreTake(canMutexLock, portMAX_DELAY);                    //Tomo semaforo para proteger el bus can ya que es un recurso compartico con otras tareas
    //Configuro recepcion de mensaje can
    bool retorno = mcan_fd_interrupt_recibir(&rx_messageID, rx_message, &rx_messageLength, &timestamp, MCAN_MSG_ATTR_RX_BUFFER, &msgFrameAttr);
    if (retorno == false)  
    {
        xSemaphoreGive(canMutexLock);                               //Libero semaforo
        return 1;                                                   //Retorno desde la funcion
    }
    volatile static CAN_ESTADO estado = CAN_LIBRE;                  //Variable para guardar el estado de la aplicación CAN
    //Bucle infinito con timeout para verificar la recepcion en periferico can
    uint8_t interacciones=0;                                        //Interacciones para timeout
    uint8_t retornar=9;                                             //Inicializo en un valor no utilizado
    while (true){
        estado=Resultado();
        switch (estado)
        {
            case CAN_RECEPCION_OK:                                    
            {
                //Verifico que el mensaje recibido sea el correcto
                if(rx_messageID>=0x601 && rx_messageID<=0x67F){     //Si el mensaje tiene cobid SDO Rx
                    if(rx_messageID==(0x600+CANopen_nodeid)){       //El mensaje esta dedicado a este nodo
                        
                        if(rx_message[0]==0x40){                    //Si el comando SDO es lectura
                            *index=(rx_message[2]<<8)+rx_message[1];//Paso index
                            *subindex=rx_message[3];                //Paso subindex
                            retornar = 4;                           //Indico que se retorna Mensaje SDO recibido y es necesario enviar parametros del diccionario
                            break;                                  //Salgo del switch
                        }

                        if(rx_message[0]==0x43){           //Si el comando SDO es escritura de 4 bytes
                            *index=(rx_message[2]<<8)+rx_message[1];//Paso index
                            *subindex=rx_message[3];                //Paso subindex
                            uint32_t data=(rx_message[7]<<24)+(rx_message[6]<<16)+(rx_message[5]<<8)+rx_message[4];
                            CANopen_Write_Dictionary(*index, *subindex, data, 32); //Escribo diccionario de objetos
                            retornar = 0;                           //Indico que se retorna ok
                            break;                                  //Salgo del switch
                        }

                        if(rx_message[0]==0x4B){                    //Si el comando SDO es escritura de 2 bytes
                            *index=(rx_message[2]<<8)+rx_message[1];//Paso index
                            *subindex=rx_message[3];                //Paso subindex
                            uint16_t data=(rx_message[5]<<8)+rx_message[4];
                            CANopen_Write_Dictionary(*index, *subindex, data, 16);//Escribo diccionario de objetos
                            retornar = 0;                           //Indico que se retorna ok
                            break;                                  //Salgo del switch
                        }

                        if(rx_message[0]==0x4F){                    //Si el comando SDO es escritura de 1 byte
                            *index=(rx_message[2]<<8)+rx_message[1];//Paso index
                            *subindex=rx_message[3];                //Paso subindex
                            uint8_t data=rx_message[4];
                            CANopen_Write_Dictionary(*index, *subindex, data, 16); //Escribo diccionario de objetos
                            retornar = 0;                           //Indico que se retorna ok
                            break;                                  //Salgo del switch
                        }
                    }else{
                        retornar = 5;                               //Indico que se retorna que el mensaje sdo es para otro nodo id
                        break;                                      //Salgo del switch
                    }
                    break;                                          //Salgo del switch
                }else{
                    retornar = 3;                                   //Indico que se retorna que no es mensaje con cobid SDO Rx
                    break;                                          //Salgo del switch
                }
            }
            case CAN_RECEPCION_ERROR:                                    
            {
                retornar = 2;                                       //Indico que se retorna error en mensaje de recepcion
                break;                                              //Salgo del switch
            }
            default:                                                //Sino
                interacciones++;                                    //Incremento intecciones
                break;                                              //Salgo del switch
        }

        if(retornar == 0 || retornar == 2 || retornar == 3 || retornar == 4 || retornar == 5){ //Si en el switch se establecio retorno
            mcan_fd_interrupt_habilitar();                          //Habilito mcan para seguir operando en el proximo mensaje sdo
            break;                                                  //Salgo del while true
        }

        if(interacciones*1>=CANopen_SDO_timeout){
            retornar = 2;                                           //Indico que se retorna error en mensaje de recepcion
            mcan_fd_interrupt_habilitar();                          //Habilito mcan para seguir operando en el proximo mensaje sdo
            break;                                                  //Salgo del while true
        }
        vTaskDelay(1 / portTICK_PERIOD_MS );                        //Deje que la tarea quede inactiva por un tiempo determinado dejando que se produzca el cambio de contexto a otra tarea.
    }
    xSemaphoreGive(canMutexLock);

    if(retornar==0){                                                //Si el comando recibido era de escritura y se escribio con exito, Envio mensaje de confirmacion
        uint8_t data_ff[3]={0};
        CANopen_SDO_Expedited_Write(CANopen_nodeid, 0x60, *index, *subindex,  data_ff, CANopen_SDO_mode_server);
    }

    if(retornar==4){                                                //Si el comando recibido era de lectura, envio dato desde el diccionario
        uint32_t dataa;                                             //Variable para guardar el dato del diccionario
        
        if(rx_message[0]==0x23){
            if(CANopen_Read_Dictionary(*index,*subindex, &dataa, 32)==true){ //Si existe el dato en el diccionario
                uint8_t data_byte[3]={0};  
                //En el diccionario se guarda el dato siendo primero el byte mas siginificativo. Para transmitir por CANopen es necesario transmitir primero el byte menos significativo (por eso se espeja el dato)
                data_byte[0]=(uint8_t)(dataa & 0x0F);
                data_byte[1]=(uint8_t)((dataa>>8) & 0x0F);
                data_byte[2]=(uint8_t)((dataa>>16) & 0x0F);
                data_byte[3]=(uint8_t)((dataa>>24) & 0x0F);
                if(CANopen_SDO_Expedited_Write(CANopen_nodeid, 0x60, *index, *subindex,  data_byte, CANopen_SDO_mode_server)!=0){   //Si no se envio correctamente el dato
                    retornar=6;
                }
            }else{
                retornar=6;
            }
        } 

        if(rx_message[0]==0x2B){
            if(CANopen_Read_Dictionary(*index,*subindex, &dataa, 16)==true){ //Si existe el dato en el diccionario
                uint8_t data_byte[3]={0};  
                //En el diccionario se guarda el dato siendo primero el byte mas siginificativo. Para transmitir por CANopen es necesario transmitir primero el byte menos significativo (por eso se espeja el dato)
                data_byte[0]=(uint8_t)(dataa & 0x0F);
                data_byte[1]=(uint8_t)((dataa>>8) & 0x0F);
                if(CANopen_SDO_Expedited_Write(CANopen_nodeid, 0x60, *index, *subindex,  data_byte, CANopen_SDO_mode_server)!=0){   //Si no se envio correctamente el dato
                    retornar=6;
                }
            }else{
                retornar=6;
            }
        }   

        if(rx_message[0]==0x2F){
            if(CANopen_Read_Dictionary(*index,*subindex, &dataa, 8)==true){  //Si existe el dato en el diccionario
                uint8_t data_byte[3]={0};  
                //En el diccionario se guarda el dato siendo primero el byte mas siginificativo. Para transmitir por CANopen es necesario transmitir primero el byte menos significativo (por eso se espeja el dato)
                data_byte[0]=(uint8_t)(dataa & 0x0F);
                if(CANopen_SDO_Expedited_Write(CANopen_nodeid, 0x60, *index, *subindex,  data_byte, CANopen_SDO_mode_server)!=0){   //Si no se envio correctamente el dato
                    retornar=6;
                }
            }else{
                retornar=6;
            }
        }                           
        
    }
    return retornar;                                                //Retorno desde la funcion
}

/*========================================================================
  Funcion: CANopen_BootUp
  Descripcion: Envia mensaje de inicio BootUp por MCAN
  Sin parametro de entrada
  Retorna:   true  = si se envio correctamente
             false = si no se pudo enviar
  ========================================================================*/
bool CANopen_BootUp(void){
    xSemaphoreTake(canMutexLock, portMAX_DELAY);                            //Tomo semaforo para proteger el bus can ya que es un recurso compartico con otras tareas
    static uint8_t message[1] = {0}; message[0]=0;
    uint32_t id = 0x700 + CANopen_nodeid;
    bool retorno = mcan_fd_interrupt_enviar((uint32_t) id, message, 1, MCAN_MODE_FD_WITH_BRS, MCAN_MSG_ATTR_TX_FIFO_DATA_FRAME); //Envio trama por can bus
    if ( retorno == false){
        xSemaphoreGive(canMutexLock);                                        //Libero semaforo
        return false;
    }  
    if(Verify_writing_by_can()==1){
        xSemaphoreGive(canMutexLock);                                        //Libero semaforo
        return false;
    }        
    xSemaphoreGive(canMutexLock);                                           //Libero semaforo
    return true;
}

/*========================================================================
  Funcion: CANopen_STOP
  Descripcion: Pasa al estado stop de la maquina de estados de CANopen
  Sin parametro de entrada
  No retorna nada
  ========================================================================*/
void CANopen_STOP(void){
   state = CANopen_STOPPED;
}

/*========================================================================
  Funcion: CANopen_Write_Dictionary
  Descripcion: Escribe dato en el diccionario de objetos
  Parametro de entrada:  uint16_t index     = indice donde se desea escribir
                         uint8_t subindex   = subindice donde se desea escribir
                         uint32_t data      = dato a escribir
                         uint8_t dictionary = diccionario al cual se desea escribir (32, 16, 8)
  Retorna:               True si se puedo escribir
                         False si no se puedo escribir
  ========================================================================*/
bool CANopen_Write_Dictionary(uint16_t index, uint8_t subindex, uint32_t data, uint8_t dictionary){
    bool retorno = false;
    portENTER_CRITICAL();                              //Seccion critica para evitar que se ejecute cambio de contexto alterando el proceso de guardado de la variable
    if(dictionary==32){
        uint16_t  tamanio_dic=sizeof(dictionary32)/8;       //Obtengo numeros de elementos del diccionario
        for(uint16_t i=0; i<tamanio_dic;i++){               //Recorro el diccionario
                                                            //Busco index y subindex indicados
            if(dictionary32[i].Index==index && dictionary32[i].Subindex==subindex){
                if(dictionary32[i].Attribute==0){           //Si el atributo permite escritura
                    dictionary32[i].Data=data;              //Escribo dato en diccionario
                    retorno=true;
                }
            }
        }
    }
    if(dictionary==16){
        uint16_t  tamanio_dic=sizeof(dictionary16)/6;       //Obtengo numeros de elementos del diccionario
        for(uint16_t i=0; i<tamanio_dic;i++){               //Recorro el diccionario
                                                            //Busco index y subindex indicados
            if(dictionary16[i].Index==index && dictionary16[i].Subindex==subindex){
                if(dictionary16[i].Attribute==0){           //Si el atributo permite escritura
                    dictionary16[i].Data=(uint16_t)data;    //Escribo dato en diccionario
                    retorno=true;
                }
            }
        }
    }
    if(dictionary==8){
        uint16_t  tamanio_dic=sizeof(dictionary8)/5;        //Obtengo numeros de elementos del diccionario
        for(uint16_t i=0; i<tamanio_dic;i++){               //Recorro el diccionario
                                                            //Busco index y subindex indicados
            if(dictionary8[i].Index==index && dictionary8[i].Subindex==subindex){
                if(dictionary8[i].Attribute==0){            //Si el atributo permite escritura
                    dictionary8[i].Data=(uint8_t)data;      //Escribo dato en diccionario
                    retorno=true;
                }
            }
        }
    }
    portEXIT_CRITICAL();                               //Salgo de la seccion critica
    return retorno;
}

/*========================================================================
  Funcion: CANopen_Read_Dictionary
  Descripcion: Lee dato en el diccionario de objetos
  Parametro de entrada:  uint16_t index     = indice donde se desea leer
                         uint8_t subindex   = subindice donde se desea leer
                         uint32_t* data     = puntero a la variable donde se quiere guardar al dato
                         uint8_t dictionary = diccionario al cual se desea escribir (32, 16, 8)
  Retorna:               True si se puedo leer
                         False si no se puedo leer
  ========================================================================*/
bool CANopen_Read_Dictionary(uint16_t index, uint8_t subindex, uint32_t *data, uint8_t dictionary){
    bool retorno = false;
    portENTER_CRITICAL();                              //Seccion critica para evitar que se ejecute cambio de contexto alterando el proceso de guardado de la variable
    if(dictionary==32){
        uint16_t  tamanio_dic=sizeof(dictionary32)/8;       //Obtengo numeros de elementos del diccionario
        for(uint16_t i=0; i<tamanio_dic;i++){               //Recorro el diccionario
                                                            //Busco index y subindex indicados
            if(dictionary32[i].Index==index && dictionary32[i].Subindex==subindex){
                if(dictionary32[i].Attribute==0){           //Si el atributo permite leer
                    *data=dictionary32[i].Data;             //Escribo dato en diccionario
                    retorno=true;
                }
            }
        }
    }
    if(dictionary==16){
        uint16_t  tamanio_dic=sizeof(dictionary16)/6;       //Obtengo numeros de elementos del diccionario
        for(uint16_t i=0; i<tamanio_dic;i++){               //Recorro el diccionario
                                                            //Busco index y subindex indicados
            if(dictionary16[i].Index==index && dictionary16[i].Subindex==subindex){
                if(dictionary16[i].Attribute==0){           //Si el atributo permite leer
                    *data=(uint32_t)dictionary16[i].Data;   //Escribo dato en diccionario
                    retorno=true;
                }
            }
        }
    }
    if(dictionary==8){
        uint16_t  tamanio_dic=sizeof(dictionary8)/5;        //Obtengo numeros de elementos del diccionario
        for(uint16_t i=0; i<tamanio_dic;i++){               //Recorro el diccionario
                                                            //Busco index y subindex indicados
            if(dictionary8[i].Index==index && dictionary8[i].Subindex==subindex){
                if(dictionary8[i].Attribute==0){            //Si el atributo permite leer
                    *data=(uint32_t)dictionary8[i].Data;    //Escribo dato en diccionario
                    retorno=true;
                }
            }
        }
    }
    portEXIT_CRITICAL();                               //Salgo de la seccion critica
    return retorno;
}

/*========================================================================
  Funcion: CAN_get_message
  Descripcion: Pasa las variables locales de esta libreria a los punteros 
  Parametro de entrada:  uint32_t *messageID        = Puntero a la variable a donde se quiere guardar el id
                         uint8_t *message           = Puntero a la variable a donde se quiere guardar el mensaje
                         uint8_t *messageLength     = Puntero a la variable a donde se quiere guardar el tamaño del mensaje
  No retorna nada
  ========================================================================*/
void CAN_get_message(uint32_t *messageID, uint8_t *message, uint8_t *messageLength){
    *messageID=rx_messageID;
    message=rx_message;
    *messageLength=rx_messageLength;
}