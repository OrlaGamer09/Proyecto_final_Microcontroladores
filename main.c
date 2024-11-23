/*
 * File:   main_final.c
 * Author: Orlando Arroyo, Andres, Fabregas, María V. Lopez.
 *
 * Created on 9 de noviembre de 2024, 07:05 AM
 * nota: codigo 100% funcional
 */



// CONFIG:
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.


// CRYSTAL:
#define _XTAL_FREQ 3579500

//BUZZER:
// Definición de notas musicales para PR2 y CCPR1L
#define DO_PR2      239    // 262Hz
#define DO_CCPR1L   0x77
#define RE_PR2      211    // 294Hz
#define RE_CCPR1L   0x6A
#define MI_PR2      188    // 330Hz
#define MI_CCPR1L   0x5F
#define FA_PR2      178    // 349Hz
#define FA_CCPR1L   0x59
#define SOL_PR2     159    // 392Hz
#define SOL_CCPR1L  0x4F
#define LA_PR2      142    // 440Hz
#define LA_CCPR1L   0x46
#define SI_PR2      130    // 494Hz
#define SI_CCPR1L   0x3D

// Duraciones

NEGRA  =   400;
BLANCA =  800;
CORCHEA =  200;


// ULTRASONIDO:
#define Trigger RA1 // Pin de Trigger del sensor ultrasónico
#define Echo RA2 // Pin de Echo del sensor ultrasónico
int time_taken;
int distance;
int threshold_distance = 10; // Ajusta el umbral para probar diferentes distancias PULGADAS

// MOTOR: 
#define speed 3 // Speed Range 10 to 1  10 = lowest , 1 = highest
#define steps 250     // how much step it will take
#define clockwise 0 // clockwise direction macro
#define anti_clockwise 1 // anti clockwise direction macro
int motor_active = 0;  // Variable de control para el motor

// LCD:
#define firstbyte   0b00110011  //output 0011 to the high nibble of the display data bus
#define secondbyte  0b00110011
#define fourBitOp   0b00110010 // Configuracion de LCD de 4bits
#define twoLines    0b00101100 // Configura 2 lineas para LCD
#define incPosition   0b00000110 // Mueve el cursor despues que se muestra dato
#define cursorNoBlink   0b00001100 // apaga cursor para que no blink
#define clearScreen   0b00000001 // limpia display
#define returnHome   0b00000010 // envia cursor al inicio del display
#define lineTwo      0b11000000 // mueve el cursor a 2da linea de display
#define doBlink      0b00001111 // hace que blink el cursor
#define shiftLeft    0b00010000 // hace que blink el cursor
#define shiftRight   0b00010100 // hace que blink el cursor
#define lcdPort   PORTD // define el puerto al que se conecta el LCB
#define eBit  PORTDbits.RD5 // define el puerto al que se conecta E
#define rsBit  PORTDbits.RD4 // define el puerto al que se conecta RS
unsigned char lcdInfo, lcdTempInfo, rsOr; // 8 bit location for 3 variables
unsigned char n; // 8 bit location
char str[80]; //80 posiciones de memoria para enviar string a LCD
char lcdInitialise[8]=   //8 posiciones de memoria y carga con algun dato inicial
{
    firstbyte,
    secondbyte,
    fourBitOp, 
    twoLines, 
    incPosition,
    cursorNoBlink, 
    clearScreen,
    returnHome,
    };

// LIBRERIAS:

#include <stdio.h>
#include <math.h>
#include <xc.h>





///////////////////
//Definicion Subrutinas

//MOTOR:

void full_drive (char direction) {
    int repeat_steps = 15; // Ajusta este valor para más pasos en cada llamada
    for (int i = 0; i < repeat_steps; i++) {
        if (direction == anti_clockwise) {
            PORTB = 0b00000011;
            __delay_ms(speed);
            PORTB = 0b00000110;
            __delay_ms(speed);
            PORTB = 0b00001100;
            __delay_ms(speed);
            PORTB = 0b00001001;
            __delay_ms(speed);
        }
        if (direction == clockwise) {
            PORTB = 0b00001001;
            __delay_ms(speed);
            PORTB = 0b00001100;
            __delay_ms(speed);
            PORTB = 0b00000110;
            __delay_ms(speed);
            PORTB = 0b00000011;
            __delay_ms(speed);
        }
    }
}



void stop_motor() {
    PORTB = 0b00000000; // Desactiva todos los pines de control del motor
}



//LCD:

//Subrutina para envio de datos a LCD
void sendInfo()
{
    lcdTempInfo=(lcdTempInfo << 4 | lcdTempInfo >> 4); 
    //cambia los nibbles en lcdTempInfo listo para ser enviados
    lcdInfo=lcdTempInfo &  0x0F; 
    //Ignora  los ultimos 4 bits y carga el resto a lcdInfo
    
    lcdInfo= lcdInfo | rsOr; 
    //Or logico entre lcdinfo y rsOr (es instruccion o datos?)
    
    lcdPort= lcdInfo;
    //envio de datos al LCD
    
    eBit=1;
    // le dice al driver de LCD que nuevos datos se enviaron
    
    eBit=0;
            //debe ser high then low.
    //Uso del retardo 2 ms  para que LCD procese
    __delay_ms(3); // 2ms Delay
    
   //Fin de sendInfo
 };

 //Subrutina lcdOut: organiza datos para enviar a sendInfo
 void lcdOut()
{
     lcdTempInfo= lcdInfo; 
     // guarda info en lugar temporal
     
     sendInfo(); 
     //llama a la subrutina para enviar los high nibbles.
     
     sendInfo();
     //llama a la subrutina para enviar los low nibbles.
       
//Fin de lcdOUt
 };
 
 //Subrutina para configurar LCD, modo instrucciones
 void setUpTheLCD()
 {
     //introducir un retardo de 32 ms
      __delay_ms(32); // 2ms Delay
      
      n=0;
      // valor inicial de n
      
      rsOr= 0x00;
      // se asegura que RS pin sea 0
      while(n<8)
      {
          lcdInfo=lcdInitialise[n];
          // carga lcdInfo en la locacion n del array
          
          lcdOut();   
          //ejecuta subrutina lcdout 
          
          n++;
          //inc n
          //fin while 
      }
      rsOr=0x10; 
      //cambio de RS pin para que este en modo datos.
      
      //fin de subrutina setUp
 }
 
 //Subrutina para enviar el cursor a la segunda linea
 void line2()
 {
     rsOr=0x00;
     //RS pin esta en modo instruccion
     lcdInfo=lineTwo;
  //envia el comando de cursor a 2da linea
     
     lcdOut();
     //llama subrutina enviando el comando
     rsOr=0x10;
     //RS pin en modo datos
//Fin de subrutina line2 
 }
 

 
  //Inicio subrutina limpiar pantalla
 void clearTheScreen()
 {
    rsOr=0x00;
     //RS pin modo instruccion
     
    lcdInfo=clearScreen;
     //carga la variabla lcd Info con el valor definido por clearscreen
     
    lcdOut();
     //envia el comando al lcd
     
    lcdInfo=returnHome;
     
    lcdOut();
     //envia el comando al lcd
     
    rsOr=0x10;
     //RS pin en modo datos
             
      //Fin de subrutina Clear the screen
 }
 

 
void writeString(const char *str)
{     
  int i=0;
  while(str[i]!='\0')
  {
    // loop will go on till the NULL character in the string
    lcdInfo=str[i];
    //carga lo que esta siendo apuntado por el apuntador
    lcdOut();
    i++; //sube el apuntador
    }
    
     //fin writeString 
 }

// LCD nuevo
void run_lcd() {
    
    // configurar el lcd
     __delay_ms(500); // 500ms Delay wait powerup
    setUpTheLCD();
    //Escribir en el LCD
     __delay_ms(500); // 500ms Delay
    // Inicio de accion LCD
    writeString("Merry Xmas!"); // Escribe el mensaje en la primera línea
    __delay_ms(1000); // Espera 1 segundo

    line2(); // Cambia a la segunda línea
    writeString("HO, HO, HO..."); // Escribe el mensaje en la segunda línea
    __delay_ms(2000); // Espera 2 segundos

    clearTheScreen(); // Limpia la pantalla
    writeString("Wish you"); // Escribe el mensaje en la primera línea
    __delay_ms(1000); // Espera 1 segundo

    line2(); // Cambia a la segunda línea
    writeString("Orli Mavi Andres"); // Escribe el mensaje en la segunda línea
    __delay_ms(1000); // Espera 1 segundo
}


    //LEDs PWM
// Función para controlar el duty cycle en subida y bajada
void Pwm_Leds() {
    PR2 = 255;            // Configura el periodo del PWM (valor máximo de 8 bits)
    static unsigned char dutyCycle = 0; // Variable para controlar el duty cycle
    static int increasing = 1;          // Flag para la dirección (subida/bajada)
    // Actualiza el duty cycle en CCPR1L
    CCPR1L = dutyCycle;

    // Controla la dirección del cambio
    if (increasing) {
        dutyCycle += 51; // Incrementa el duty cycle en aproximadamente 15%
        if (dutyCycle >= 255) {
            dutyCycle = 255; // Limita al 100%
            increasing = 0;  // Cambia la dirección a bajada
        }
    } else {
        dutyCycle -= 51; // Decrementa el duty cycle en aproximadamente 15%
        if (dutyCycle <= 0) {
            dutyCycle = 0; // Limita al 0%
            increasing = 1; // Cambia la dirección a subida
        }
    }

    __delay_ms(200); // Retardo de 500 ms entre cada cambio de duty cycle
}

//LDR:
unsigned int leerADC(void) {
    ADCON0bits.GO = 1;         // Inicia la conversión
    while (ADCON0bits.GO);     // Espera a que termine la conversión
    return ((ADRESH << 2) | (ADRESL >> 6)); // Combina ADRESH y los 2 bits más altos de ADRESL
}


void parpadearLEDs(void) {
    CCP1CON = 0x00;  // Configura el módulo CCP1 en modo PWM para LEDS
    for (unsigned char i = 0; i < 10; i++) {
        RC2 = 1; // Enciende LED en RC2
        RC3 = 1; // Enciende LED en RC3
        __delay_ms(400); // Retardo de 500 ms
        RC2 = 0; // Apaga LED en RC2
        RC3 = 0; // Apaga LED en RC3
        __delay_ms(400); // Retardo de 500 ms
    }
}
  
////////////////////////////////////////////////////
// INICIO CODIGO PRINCIPAL
////////////////////////////////////////////////////




void main() {
    //Configuracion de puertos
    
    PORTA=0x00; //
    PORTB=0x0F;
    PORTC=0x00;
    PORTD=0x00;
    PORTE=0x00;
    
    TRISA=0xFD; // Configura todos los pines de PORTA como entradas excepto RA1
    TRISB=0x00; // Todo salidas
    TRISC=0x00; // Todo salidas
    TRISD=0x00; // Todo salidas
    TRISE=0x00; // Todo salidas
    

    //ADCON0 = 0x00; // Apaga el ADC
    //ADCON1 = 0x06; //Define el puerto A como digital
    
    ADCON0 = 0x41;  //ActiVa el ADC en AN0 01000001
    ADCON1 = 0x4E;  // habilita el AN0 justificado a la izquierda 01001110

    CCP1CON = 0x0C;  // Configura el módulo CCP1 en modo PWM para LEDS
    T1CON = 0x10; // Timer1 sin preescalador, modo sincrónico, reloj interno
    
    CCP2CON = 0X2C;  // Configura el módulo CCP2 en modo PWM para Buzzer
    T2CON = 0X06; 
    PR2 = 0;
    CCPR2L = 0;
    CCPR1L = 0;
    
    unsigned int valor_lectura; 
    
    while(1) {
        CCP1CON = 0x0C;  // Configura el módulo CCP1 en modo PWM para LEDS
        //primero mensaje de bienvenida
        run_lcd();
        
        
        // luego se evalua con el LDR si esta oscuro, si es afirmativo pasa a modo noche 
        // se ejecuta esto:
        //clearTheScreen();
        //writeString("Night mode!");
        // Control de LED con PWM
        //Pwm_Leds();
        // si es negativo se sale del bucle y empieza a leer el ultrasonico y realiza las tareas con normalidad

         
         // Inicio de accion Ultrasonido 
        TMR1H = 0; 
        TMR1L = 0; // Limpia el valor del timer
        
        // Envía el pulso de Trigger
        Trigger = 1; 
        __delay_us(10);           
        Trigger = 0;  
        
        // Espera a que el Echo empiece y activa el Timer1
        while (Echo == 0);
        TMR1ON = 1;
        
        // Espera a que el Echo termine y apaga el Timer1
        while (Echo == 1);
        TMR1ON = 0;
        
        // Calcula el tiempo transcurrido y la distancia
        time_taken = (TMR1L | (TMR1H << 8)); 
        distance= (0.0272*time_taken)/2;  // Ajustado para 3579500 Hz y mejor precisión
        
        valor_lectura = leerADC(); // Leer valor analógico de AN0
        
        

        if (valor_lectura > 512) { //Modo noche
            clearTheScreen();
            writeString("Night mode!");
            parpadearLEDs(); // Parpadea LEDs si el valor supera el umbral
        }else{  // Modo día
        
                    // Enciende el LED si la distancia es menor que el umbral
            if (distance <= threshold_distance) {
                RD7 = 1; // Enciende el Flag
            } else {
                RD7 = 0; // Apaga el Flag
            }
            
             // Inicio de accion Motor
            // Verifica si RD2 está en alto y el motor no está activo
            if (RD7 == 1 && motor_active == 0) {
                //PR2 = 0; // Limpio la frecuencia
                clearTheScreen(); // Limpia la pantalla
                writeString("Santa's Coming!"); // Escribe el mensaje en la primera línea
                line2(); // Cambia a la segunda línea
                writeString("Happy Holidays!"); // Escribe el mensaje en la segunda línea
                motor_active = 1;  // Activa el motor

                unsigned int duration = 15000;  // Duración total en ms
                unsigned int elapsed = 0;
                int buzzer_step = 0; // Paso de sonido en run_buzzer


                while (elapsed < duration) {


                    full_drive(anti_clockwise);


                 // Ejecuta un paso de la función del buzzer
                    switch (buzzer_step) {
                        case 0: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L;  __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 1: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 2: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(NEGRA); elapsed += NEGRA; break;

                        case 3: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 4: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 5: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(NEGRA); elapsed += NEGRA; break;

                        case 6: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 7: PR2 = SOL_PR2; CCPR2L = SOL_CCPR1L; CCPR1L = SOL_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 8: PR2 = DO_PR2; CCPR2L = DO_CCPR1L; CCPR1L = DO_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 9: PR2 = RE_PR2; CCPR2L = RE_CCPR1L; CCPR1L = RE_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;

                        case 10: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(BLANCA); elapsed += BLANCA; break;

                        case 11: PR2 = FA_PR2; CCPR2L = FA_CCPR1L; CCPR1L = FA_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 12: PR2 = FA_PR2; CCPR2L = FA_CCPR1L; CCPR1L = FA_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 13: PR2 = FA_PR2; CCPR2L = FA_CCPR1L; CCPR1L = FA_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;

                        case 14: PR2 = FA_PR2; CCPR2L = FA_CCPR1L; CCPR1L = FA_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 15: PR2 = FA_PR2; CCPR2L = FA_CCPR1L; CCPR1L = FA_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 16: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L;  __delay_ms(CORCHEA); elapsed += CORCHEA; break;


                        case 17: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L;  __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 18: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 19: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(NEGRA); elapsed += NEGRA; break;  

                        case 20: PR2 = RE_PR2; CCPR2L = RE_CCPR1L; CCPR1L = RE_CCPR1L; __delay_ms(NEGRA); elapsed += NEGRA; break;
                        case 21: PR2 = RE_PR2; CCPR2L = RE_CCPR1L; CCPR1L = RE_CCPR1L; __delay_ms(NEGRA); elapsed += NEGRA; break;
                        case 22: PR2 = MI_PR2; CCPR2L = MI_CCPR1L; CCPR1L = MI_CCPR1L; __delay_ms(CORCHEA); elapsed += CORCHEA; break;
                        case 23: PR2 = RE_PR2; CCPR2L = RE_CCPR1L; CCPR1L = RE_CCPR1L; __delay_ms(NEGRA); elapsed += NEGRA; break;
                        case 24: PR2 = SOL_PR2; CCPR2L = SOL_CCPR1L; CCPR1L = SOL_CCPR1L; __delay_ms(NEGRA); elapsed += NEGRA; break;
                        }
                    // Limpia el PWM después de cada nota
                    PR2 = 0;
                    CCPR2L = 0;
                    // Incrementa el paso de la melodía y resetea si es necesario
                    buzzer_step = (buzzer_step + 1) % 25; 

                    }


                        CCPR1L = 0;
                        motor_active = 0;  // Resetea el estado después de 10 segundos
                        stop_motor();      // Llama a la función para detener el motor
                        clearTheScreen(); // Limpia la pantalla
            }
        
        }
        
                 
    }
    
 return; 
}




