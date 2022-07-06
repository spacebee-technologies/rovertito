/*=============================================================================
 * Author: Spacebeetech - Navegacion
 * Date: 30/056/2022 
 * Board: Atmel ARM Cortex-M7 Xplained Ultra Dev Board ATSAMV71-XULT ATSAMV71Q21B
 * Entorno de programacion: MPLABX - Harmony
 *
 * Descripcion: diccionario basico para CANopen
*===========================================================================*/

/*=====================[Variables]================================*/  
typedef struct diccionario32 {                        //Diccionario para datos de 4 bytes
        uint16_t Index;
        uint8_t  Subindex;
        uint8_t  Attribute;
        uint32_t Data;
        } diccionario32;

typedef struct diccionario16 {                        //Diccionario para datos de 2 bytes
        uint16_t Index;
        uint8_t  Subindex;
        uint8_t  Attribute;
        uint16_t Data;
        } diccionario16;

typedef struct diccionario8 {                        //Diccionario para datos de 1 byte
            uint16_t Index;
            uint8_t  Subindex;
            uint8_t  Attribute;
            uint8_t Data;
            } diccionario8;  


//Inicializacion del diccionario

diccionario32 dictionary32[]={
    //Index     Subindex     Attribute        Data
    { 0x00,         0,           0,          0x0000},
    { 0x00,         1,           0,          0x0000},
    { 0X01,         0,           0,          0x0000}
};      

diccionario16 dictionary16[]={
    //Index     Subindex     Attribute       Data
    { 0x00,         0,           0,          0x00},
    { 0x00,         1,           0,          0x00},
    { 0X01,         0,           0,          0x00}
};  

diccionario8 dictionary8[]={
    //Index     Subindex     Attribute      Data
    { 0x00,         0,           0,          0},
    { 0x00,         1,           0,          0},
    { 0X01,         0,           0,          0}
};  
