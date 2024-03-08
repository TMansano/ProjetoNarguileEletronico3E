// original library source:
// www.ladyada.net/learn/sensors/thermocouple
/*
 * Arduino code to read temperature using MAX6675 chip and k-type thermocouple
 * and display it on serial monitor
 * This code is "AS IS" without warranty or liability. Free to be used as long as you keep this note intact.* 
 * This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */



/*
//3E UEL  Universidade Estadual de Londrina 2024
------------------------------- Codificação de desenvolvimento de software de arduino ------------------
horimetro contador ultra preciso com controle automatico de escala e fator de erro previsivél

Este código se refere ao dispositivo desenvolvido pela 3E UEL Engenharia, na qual se refe a um contador de alta precisão que se automaticamente 
corrige com tempo, sendo assim um contador em malha fechada com relação a sua saida [numero de horas totais e ciclos], o mesmo também conta 
com sensor ultra rápido de tensão para aquisição dos dados de inicio e fim de contagem e também salvamento dos dados em um cartão de 
memória micro SD, o código é sequencial entretante após melhorias o código funcina com base em dois núcloeos do processodr presente 
na ESP32, a programação multicore foi necessaria para aumentar a velocidade de processamento do contador e melhorar sua precisão.

as bibliotecas necessaria estão abaixo com observação a biblioteca LiquidCrystal_I2C.h que deve ser instalada separadamente na maquina assim como a placa ESP32 Dev Module da ESP 32 de 36 pinos.
*/




#include <RotaryEncoder.h>
#include <Arduino.h>
#include <LiquidCrystal.h>  // Biblioteca LCD
#include <LiquidCrystal_I2C.h>
#include <wire.h>
#include "max6675.h"  // max6675.h file is part of the library that you should download from Robojax.com
int soPin = 4;        // SO=Serial Out
int csPin = 5;        // CS = chip select CS pin
int sckPin = 6;       // SCK = Serial Clock pin
int next_millis = 0;
int millis_offset=0;
int contador_millis=0;
int sec = 0;
int hrs = 0;
int minute = 0;
int bateria_atual = 0;
int temperatura_alvo = 0;
int celsius = 0;
int bateria_inicio=0;
int consumo_bateria=0;
int a,b=0;
int select=0;
int temperatura_resistencia=0;
int counter = 0;
int newPos=0;
int controle=0;
int aState;
int aLastState;
char buf[15];
#define CLK 6
#define DT 7//clk e 6 e dt é 7
#define mosfet_pino 6
#define pushbutton_pino 2
#define bateria_pino A7
#define mosfet_pino 6

//define a inicialização do LCD i2c
LiquidCrystal_I2C lcd(0x27, 16, 2);
MAX6675 robojax(sckPin, csPin, soPin);
// Setup a RoraryEncoder for pins A2 and A3:
RotaryEncoder encoder(A2, A3);

byte termometro_simbolo[8] = {
  B0100,
  B01010,
  B01010,
  B01010,
  B010001,
  B011111,
  B01110,
};


void setup() {
  //Serial.begin(9600);
  //-------------definição da inicialização dos sistemas--------------------------
  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, termometro_simbolo);
  carga_bateria();
  //-------------------------parte da tela 3 (menu principal de opções)-----------------
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  // Reads the initial state of the outputA
  aLastState = digitalRead(CLK);
  //----------------------------------------------------------------------
  //-------------Entradas e Saidas--------------------------
  pinMode(mosfet_pino, OUTPUT);        // mosfet
  pinMode(pushbutton_pino, INPUT_PULLUP);  //Reset BUTTON1
  //----------------------FUNÇÂO QUE ANALISA A TEMPERATURA DO SENSOR--------------
  temperatura_resistencia = robojax.readCelsius();
//----------registra o offset-------------------
  millis_offset = millis();
  bateria_inicio = bateria_atual;




  //----------tela 1----------(boot)
  lcd.print("    Narguile Elet. V1");
  lcd.setCursor(0, 1);
  lcd.print("  Leo");
  delay(20000);

  
}



void loop() {
  lcd.clear();      //limpa a tela
  carga_bateria();  //analisa a carga da bateria
  contador_millis = millis() - millis_offset;

  if(controle=1)
  {
    controle_temperatura();
  }
  
  //----------------------FUNÇÂO QUE ANALISA A TEMPERATURA DO SENSOR--------------
  temperatura_resistencia = robojax.readCelsius();
  //-----------------"contador acompanhador"---------------//string a ser impressa na LCD
  //sprintf(buf, "%02d:%02d:%02d     ",(contador_millis / 3600), ((restodadivisaopor % 3600) / 60), (((restodadivisaopor % 3600) % 60) / 60));
  //--------------------------------------------------tela 2----------(tela principal)------------------------------------
  lcd.write(byte(0) );
  lcd.print(temperatura_resistencia);
  lcd.print("/");
  lcd.print(temperatura_alvo);
  lcd.print((char)223);

  lcd.print("   B ");
  lcd.print(bateria_atual);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Sessao ");
  lcd.print(buf);
  //------------------------------------------------------------------------------------------------



  //botao = digitalRead(6);
  if (digitalRead(6) == LOW) {
    select = 1;
    encoder.setPosition(1);
    lcd.clear();
    delay(500);
  }
  if (select == 1) {
    menu_opcoes();
  }
}


void menu_opcoes() {

  //static int pos = 0;
  static int pos = 0;
  encoder.tick();
  newPos = encoder.getPosition();
  //----------------limites da seta do menu----------------
  if (newPos > 5) { encoder.setPosition(2); }
  if (newPos < 2) { encoder.setPosition(5); }

  if (pos != newPos) {
    lcd.clear();
    //lcd.setCursor(0,0);
    //lcd.print(newPos);
    pos = newPos;
  }

  //botao = digitalRead(6);
  if (digitalRead(6) == LOW) {
    select = newPos;
    lcd.clear();
    delay(200);
  }


  //parte do cursor
  if (newPos == 2 or 4) {
    a = 0;
    b = 0;
  }
  if (newPos == 3 or 5) {
    a = 0;
    b = 1;
  }
  lcd.setCursor(a, b);
  lcd.print(">");

  if (newPos < 3) {
    lcd.setCursor(1, 0);
    lcd.print("Aquece/Ajustar");
    lcd.setCursor(1, 1);
    lcd.print("Esfriar");
  } 
  else 
  {
    lcd.setCursor(1, 0);
    lcd.print("Bateria");
    lcd.setCursor(1, 1);
    lcd.print("Voltar");
  }


  if (select == 2) { temperatura_set(); }
  if (select == 3) {  controle=0;}
  if (select == 4) { bateria(); }
  if (select == 5) { voltar(); }
}


void temperatura_set() {
  //t----------tela 4----------(temperatural alvo)------------------------------------------------------------------------------------
  aState = digitalRead(CLK);  // Reads the "current" state of the outputA
  // If the previous and the current state of the outputA are different, that means a Pulse has occured
  if (aState != aLastState) {
    // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
    if (digitalRead(DT) != aState) {
      temperatura_alvo++;
      //temperatura_alvo=counter;
      //lcd.clear();
    } else {
      temperatura_alvo--;
      //lcd.clear();
    }
  }
  aLastState = aState;  // Updates the previous state of the outputA with the current state

  lcd.print("   Temperatura");
  lcd.setCursor(0, 1);
  lcd.print(temperatura_alvo);
  lcd.print( (char)223);

  controle=1;

  //fim (fom do cógigo "temperatura Alvo" -----------------------------------------------------------------------------------------------------------------------------------
}

void bateria() {
  //t----------tela 4----------(menu bateria)
  consumo_bateria = bateria_inicio - baterial_atual;
  lcd.print("Consumo de ");
  lcd.print(consumo_bateria);
  lcd.print("%em");
  lcd.setCursor(0, 1);
  lcd.print(buf);
  lcd.print(", Atual ");
  lcd.print(baterial_atual);
  lcd.print(" %");
  //---------------------------------------------------
}







void carga_bateria() {
  bateria_atual = ((analogRead(bateria_pino) * 5) / 1024) * 20;
}


void controle_temperatura() {
  //------------------------------PARTE DE CONTROLE (Bang Bang) ---------------------------------
  if (temperatura_resistencia < temperatura_alvo) {
    digitalWrite(mosfet_pino, HIGH);  //polariza o mosfet (Aquece)
  } else {
    digitalWrite(mosfet_pino, LOW);  //despolariza (esfria)
  }
}
