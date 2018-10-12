#include <Arduino.h>
#include "NuestroLED.h"

NuestroLED::NuestroLED(const int pin){
	this->pin=pin;
	pinMode(this->pin, OUTPUT);
  this->modo=SOFT_PWM;
}

NuestroLED::NuestroLED(const int pin, const int modo){
	this->pin=pin;
  pinMode(this->pin, OUTPUT);
	this->modo=modo;
}

void NuestroLED::setModo(const int modo){
    this->modo=modo;
    this->isON=true; //Reinicia
    this->enSubida=true; //Reinicia
    this->start=0; //Reinicia
}

int NuestroLED::getModo(){
    return this->modo;
}

void NuestroLED::activar(const int intensidadPWMManual){
     switch ( this->modo )  
      {  
         case PRENDE_APAGA:
            onOff();
            break;  
         case INTENSIDAD_VARIABLE:
            manual(intensidadPWMManual);
            break;
         case SOFT_PWM:
            soft();
            break;
         case SIEMPRE_PRENDIDO:
            setIntensidadBoolON();
            on();
            break;
         case SIEMPRE_APAGADO:
         this->isON=false;
         setIntensidadBoolOFF();
            off();
            break;  
         default:  
            break; //IDK
      }  
}

void NuestroLED::setIntensidadPWM(){
  analogWrite(this->pin,this->intensidad);
}

void NuestroLED::setIntensidadBoolON(){
  digitalWrite(this->pin,HIGH);
}

void NuestroLED::setIntensidadBoolOFF(){
  digitalWrite(this->pin,LOW);
}

void NuestroLED::onOff(){
  if(this->isON && this->start % 2000 == 1000){ //Cuando pase un segundo, si está prendido, se apaga
    setIntensidadBoolOFF();
  }
  if(!this->isON && this->start % 2000 == 0){ //Cuando pase otro segundo, si está apagado, se prende
    setIntensidadBoolON();
  }
}

void NuestroLED::manual(const int intensidadPWMManual){
  this->intensidad = intensidadPWMManual;
  setIntensidadPWM(); //Talvez necesite un espaciado con millis, nose
}

void NuestroLED::soft(){
  
}

void NuestroLED::on(){
  if(!this->isON){
    setIntensidadBoolON();
  }
}

void NuestroLED::off(){
  if(this->isON){
    setIntensidadBoolOFF();
  }
}
