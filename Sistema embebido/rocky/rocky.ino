#include <SoftwareSerial.h>
#include <stdlib.h>
#include "nuestroServo.h"
#include "NuestroPotenciometro.h"
#include "NuestroLED.h"
#include "ColorRocklet.h"
#include "NuestraBarreraLaser.h"
#include "NuestroPulsador.h"

#define CANT_COLORES 6

/*Definicion de estados*/
#define EN_ESPERA 0
#define BUSCANDO 1
#define LLEVANDO 2
#define SENSANDO 3
#define TOBOGAN_A 4
#define TOBOGAN_M 5
#define DESPACHANDO 6

/*Defino los modos de operación*/
#define AUTO 0
#define MANUAL 1
#define CELULAR 2

/*Definicion de pines*/
#define PIN_0_LECTOR_COLOR 4
#define PIN_1_LECTOR_COLOR 5
#define PIN_2_LECTOR_COLOR 6
#define PIN_3_LECTOR_COLOR 7
#define PIN_SALIDA_LECTOR_COLOR 8
#define PIN_LASER 2
#define PIN_FLAME_DETECTOR 3
#define PIN_LED 9
#define PIN_POTENCIOMETRO 0
#define PIN_PULSADOR 12
#define PIN_SERVO_CINTA 11
#define PIN_SERVO_TOBOGAN 10

/*Tiempos de espera*/
#define TBUSCAR 1000
#define TLLEVAR 1000
#define TDESPACHO 1000
#define TACOMODAR 1000

/* Definicion de limites para servos*/
#define MAX_NEGRO 190
#define MIN_NEGRO 0
#define MAX_AZUL 254
#define MIN_AZUL 60

/* Comandos que pueden llegar por Bluetooth */
#define PASAR_A_CELU '0'
#define SALIR_DE_CELU '1'
#define PAUSAR '2'
#define SEGUIR '3'
#define SOLTAR '4'
#define POSICIONAR '5'

/*Seteo de variables y pines*/
ColorRocklet lectorColor = ColorRocklet(
		PIN_0_LECTOR_COLOR, PIN_1_LECTOR_COLOR,
		PIN_2_LECTOR_COLOR, PIN_3_LECTOR_COLOR,
		PIN_SALIDA_LECTOR_COLOR); //Sensor de color
NuestraBarreraLaser barreraLaser = NuestraBarreraLaser(
		PIN_LASER,
		PIN_FLAME_DETECTOR); //Barrera con laser y detector de flama
NuestroLED led = NuestroLED(PIN_LED);
NuestroPotenciometro potenciometro = NuestroPotenciometro(PIN_POTENCIOMETRO);
NuestroPulsador pulsador = NuestroPulsador(PIN_PULSADOR);
NuestroServo servoCinta = NuestroServo(
		PIN_SERVO_CINTA,
		MIN_NEGRO, MAX_NEGRO); //Servo de estaciones RECEPCION, COLOR y CAIDA
NuestroServo servoTobogan = NuestroServo(
		PIN_SERVO_TOBOGAN,
		MIN_AZUL, MAX_AZUL); //Servo de estaciones de colores
SoftwareSerial bluetooth(0, 1); //Modulo bluetooth

bool play; //Para play y pausa del celu
int estadoActual;
int modo; //Automatico o manual
unsigned long inicioEsperaServo; //Tiempo en que el servo comenzo a moverse, para saber hasta que tiempo esperar
bool sensado; //El sensorColor fue sensado
int color; //Color del rocklet leído
int cantColores[CANT_COLORES] = { 0 }; //Contador de cantidad de cada color
const int estaciones[] = { NuestroServo::ST_1, NuestroServo::ST_2,
		NuestroServo::ST_3, NuestroServo::ST_4, NuestroServo::ST_5,
		NuestroServo::ST_6 }; //Estaciones segun el color
int posPotenciometro; // Posicion leida del potenciometro
int posCelular; //Posicion recibida por bluetooth para modo celular
bool despachoPendiente; //Por si me llega la orden de despachar desde el bt antes de que este en modo TOBOGAN_M

void setup() {
	//Serial.begin(9600);
    pinMode(0, INPUT);
    pinMode(1, OUTPUT);
	bluetooth.begin(9600);
	estadoActual = EN_ESPERA;
	modo = AUTO;
    play = true;
    despachoPendiente = false;
    posCelular=128; //Para inicializarlo con algo
}

void loop() {
	loDeSiempre(); //Chequea y hace todo lo que tiene que hacer en cada loop

    if(play){
    	//Decide qué más ejecutar según el estado actual
    	switch (estadoActual) {
    	case EN_ESPERA:/* Condicion de la barrera laser */
    		barreraLaser.activarBarrera();
    		if (barreraLaser.detecta()){
    			barreraLaser.desactivarBarrera();
    			aBuscando();
    		}
    		break;
    		
    	case BUSCANDO:/* Condicion de el servo cinta  para BUSCANDO*/
    		if (millis() - inicioEsperaServo >= TBUSCAR)
    			aLlevando();
    		break;
    		
    	case LLEVANDO:/* Condicion del el servo cinta para LLEVANDO */
    		if (millis() - inicioEsperaServo >= TLLEVAR) {
    			//Pasa a SENSANDO
    			estadoActual = SENSANDO;
    			setearLED();
    		}
    		break;
    		
    	case SENSANDO:/* Condicion del sensor color */
    		if (!sensado) { //Si todavia no lo identifique, hago otra lectura
    			lectorColor.prenderSensor();
    			sensado = lectorColor.hacerLectura();
    
    		} else { //Si ya lo identifique
    			sensado = false;
    			lectorColor.apagarSensor();
    
    			//Obtengo el color, sumo y reporto
    			color = lectorColor.getColor();
    			if (color != ColorRocklet::NO_IDENTIFICADO)
    				cantColores[color]++;
    			reportarColores();
    
    			//Paso al tobogan segun el estado
    			if (modo == AUTO) 
    				aToboganA();
    			else //Manual o Celular
    				aToboganM();
    		}
    		break;
    		
    	case TOBOGAN_A:/* Tobogan en modo auto*/
    		if (modo == AUTO) {
    			if (millis() - inicioEsperaServo >= TACOMODAR)
    				aDespachando();
    		} else //Modo Manual o Celular
    			aToboganM();
    		break;
    		
    	case TOBOGAN_M:/* Tobogan en modo manual */
            switch(modo){
            case MANUAL:
                if (!pulsador.detectaCorto()) {
                    //Mientras no haya pulso corto, muevo el servo segun el potenciometro
                    posPotenciometro = potenciometro.getPosicion();
                    servoTobogan.irAAnalogico(posPotenciometro);
                } else
                    aDespachando();
                break;
            case CELULAR: //El pase a Despachando se maneja desde la recepcion del bt o desde el inicio de TOBOGAN_M
                servoTobogan.irA256(posCelular);
                break;
            default: //Automatico
                aToboganA();
            }
    		break;
    		
    	case DESPACHANDO:/* Despacho */
    		if (millis() - inicioEsperaServo >= TDESPACHO) {
    			//Pasa a EN_ESPERA
    			estadoActual = EN_ESPERA;
    			setearLED();
    		}
    	}
    }
}

/**
 * Setea el modo del led segun el estado actual. Debe ser llamada
 * en cada cambio de estado.
 */
void setearLED() {
	/* Seteo de LED */
	if (modo == MANUAL || modo == CELULAR) {
		if (estadoActual == TOBOGAN_M) {
			led.setModo(NuestroLED::INTENSIDAD_VARIABLE);
		} else
			led.setModo(NuestroLED::PRENDE_APAGA);
	} else {
		if (estadoActual == EN_ESPERA)
			led.setModo(NuestroLED::SOFT_PWM);
		else {
			if (estadoActual == TOBOGAN_A || estadoActual == DESPACHANDO)
				led.setModo(NuestroLED::SIEMPRE_PRENDIDO);
			else
				//BUSCANDO || LLEVANDO || SENSANDO
				led.setModo(NuestroLED::SIEMPRE_APAGADO);
		}
	}
}

/**
 * Hace los chequeos y acciones comunes a todos los loops
 */
void loDeSiempre() {
	//Chequeo el bluetooth
	if (bluetooth.available())
		recibirDatos();

    if(play) {
    	/* Checkeo el pulsador */
    	pulsador.chequear();
    	//Si hace falta, cambio de modo
    	if (!(modo==CELULAR) && pulsador.detectaLargo()) {
    		if (modo == AUTO) {
    			modo = MANUAL;
    			bluetooth.println("$m");
    		} else {
    			modo = AUTO;
    			bluetooth.println("$a");
    		}
    	}
    
    	//Mando un tick al LED
    	led.activar(posPotenciometro);
    }
}

/**
 * Envia el nuevo color y los contadores actualizados a la app
 */
void reportarColores() {
	bluetooth.print("#");
    bluetooth.print(color); //Mando el color identificado
    //Mando los contadores de cada color
	for (int i = 0; i < CANT_COLORES; i++) {
		bluetooth.print("-");
		bluetooth.print(cantColores[i]);
	}
	bluetooth.println();
}

/**
 * Recibe datos del bluetooth y actua en consecuencia
 */
void recibirDatos(){
    char c = bluetooth.read();
    if(!play){
        if(c == SEGUIR)
            play = true;
        return;
    }
    switch(c){
    case PASAR_A_CELU:
        modo = CELULAR;
        break;
    case SALIR_DE_CELU:
        /*Sólo se sale del modo celular si no está en pausa*/
        if(play){
          modo = AUTO;
          bluetooth.println("$a");
          despachoPendiente = false;
        }
        break;
    case PAUSAR:
        play = false;
        break;
    case SOLTAR:
        if(modo == CELULAR){
            if(estadoActual == TOBOGAN_M)
                aDespachando();
            else 
                despachoPendiente = true;
        } 
        break;
    case POSICIONAR:
      /*Sólo lee y procesa los datos del movimiento si está en modo CELULAR y estado TOBOGAN_MANUAL*/
        if(modo == CELULAR){
            //Leo el numero            
            int aux= bluetooth.read(); //Read da un char, lo promociono a int que va a tener entre 0 y 255
            if(aux > 0 && aux <= 254) //Reduciendo ruido
                posCelular = aux;
        }
        break;
    case 'x':
        if(modo == AUTO)
            bluetooth.println("$a");
        else
            bluetooth.println("$m");
    }
}

/**
 * Pasa al estado BUSCANDO.
 * Llamado desde EN_ESPERA
 */
void aBuscando(){
	estadoActual = BUSCANDO;
	setearLED();
	
	//Seteo el servo para que busque el rocklet
	servoCinta.irA(NuestroServo::RECEPCION_ST);
	inicioEsperaServo = millis();
}

/**
 * Pasa a LLEVANDO.
 * Llamado desde BUSCANDO.
 */
void aLlevando(){
	estadoActual = LLEVANDO;
	setearLED();
	
	//Seteo el servo para que lleve el rocklet
	servoCinta.irA(NuestroServo::COLOR_ST);
	inicioEsperaServo = millis();
}

/**
 * Pasa a TOBOGAN_A.
 * Llamado desde SENSANDO y TOBOGAN_M
 */
void aToboganA(){
	estadoActual = TOBOGAN_A;
	setearLED();
	
	//Los colores estan del 0 al 5, para servir de indices en los vectores
	if (color != ColorRocklet::NO_IDENTIFICADO) //Si reconoci un color
		servoTobogan.irA(estaciones[color]);
	else //Sino lo mando a alguna estacion definida para los no reconocidos
		servoTobogan.irA(NuestroServo::ST_1);
	inicioEsperaServo = millis();
}

/**
 * Pasa a TOBOGAN_M.
 * Llamado desde SENSANDO y TOBOGAN_A
 */
void aToboganM(){
    if(despachoPendiente){
        despachoPendiente = false;
        aDespachando();
        return;
    }
	estadoActual = TOBOGAN_M;
	setearLED();
}

/**
 * Pasa a DESPACHANDO.
 * Llamado desde TOBOGAN_A, TOBOGAN_M, recibirDatos con SOLTAR y aToboganM con despachoPendiente
 */
void aDespachando(){
	estadoActual = DESPACHANDO;
	setearLED();
	
	//Seteo el servo para que lleve el rocklet
	servoCinta.irA(NuestroServo::CAIDA_ST);
	inicioEsperaServo = millis();
}
