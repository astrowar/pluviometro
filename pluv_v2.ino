 
//codigo do pluviometro de sao jose


#include <Wire.h>
#include <DS3232RTC.h>
#include "LowPower.h"

#include <avr/sleep.h>
#include <avr/power.h>

#define LED_PIN (13)

const int csensor = 2;  //pino do pluviometro

int cpin = 8;  //pino que liga e desliga o relogio
int rpin = 9;  //liga e desliga o raspberry pi

int cdata = 4;   //data no raspbery
int cclock = 5;  //clock , pois eh syncrono
int creset = 6;  //resete que vem do arduino


volatile int state;
int reset_state = 0;


String logstr; //string que contem os hash

bool force_log = false;
int last_mhour = 0;

void wakeUp()
{
 // seta o stato para dispara o event log
  state = 1;
  detachInterrupt(0);
}


//variaveis do logger serial para oraspberry
int char_i = 0;
int bit_i = 0;
int counter_clock = 0;


void send_log()
{

  digitalWrite(rpin, HIGH); //mantem o arduino ligado

  counter_clock += 1;
  //inicio do ciclo, baixa o pin
  if (counter_clock < 2)
  {
    digitalWrite(cclock, LOW);
  }

  //meio do ciclo, aciona
  if (counter_clock == 10)
  {
    //next bit
    bit_i += 1;
    if (bit_i >= 8)
    {
      bit_i = 0;
      char_i = char_i + 1;
      if (char_i > logstr.length())
      {
        char_i = 0; //reset string
      }
    }

    if (logstr.length() == 0)
    {
      //nao tem mais log ! reset foi acionado 
      force_log = false;
      counter_clock = 0;
      return;
    }

    char  c = logstr.charAt(char_i);
    int bb = bitRead(c, bit_i);
    if (bb == 0) { digitalWrite(cdata, LOW); }
    else { digitalWrite(cdata, HIGH); }
    digitalWrite(cclock, HIGH);

  }

  if (counter_clock >= 20)
  {
    counter_clock = 0; //final do ciclo, reseta
  }

}

 

time_t get_tempo()
{
  digitalWrite(cpin, HIGH);
  delay(100); //da um tempo para iniciar
  time_t t = (RTC.get());
  digitalWrite(cpin, LOW);
  TWCR = 0; //IMPORTANTE , reseta as linhas I2C

  return t;
}


void adquire_log()
{
  // Serial.print("log");
  //puxa a data e adiciona ao log global das horas

  time_t t = get_tempo();

  //constroe o evento atual
  String  event_log = String(year(t), DEC) +
    String(month(t), DEC) +
    String(day(t), DEC) +
    String(hour(t), DEC) +
    String(minute(t), DEC) +
    String(second(t), DEC) + " ";
  //adiciona  ao log total
  logstr = logstr + event_log;
  //fila ja esta muito grande, descarregue no raspebrry pi
  if (logstr.length() > 30) force_log = true;
  last_mhour = 24 * day(t) + hour(t);
  Serial.println(logstr);

}



void verifica_log_remainder()
{
  //funcao que verifica se ha algum log no buffer e se esse log esta a muito tempo
  if (logstr.length() > 2)
  {
    time_t t = get_tempo();

    //mais de 4 horas parado na fila ...
    int  mhour = 24 * day(t) + hour(t);
    if (abs(mhour - last_mhour) > 0) force_log = true;
  }

}


bool wait_lower;
int wake_cicles;
int sleep_cicles; // ciclos no qual o sistema durmiu e acordou,450 ciclos fazem uma hora

void setup()
{
  logstr = "";
  last_mhour = 0; //momento do ultimo LOG

  state = 0;
  pinMode(csensor, INPUT_PULLUP);

  pinMode(cpin, OUTPUT);
  pinMode(rpin, OUTPUT);

  pinMode(cdata, OUTPUT);
  pinMode(cclock, OUTPUT);
  pinMode(creset, INPUT);

 

  // No setup is required for this library
  Serial.begin(9600);
  Wire.begin();

  wait_lower = false;
  wake_cicles = 0;

  digitalWrite(cpin, LOW);
  digitalWrite(rpin, LOW);

  sleep_cicles = 0;

}


 
void check_reset()
{
   
  if (digitalRead(creset) == HIGH)
  {
    reset_state = 1;
  }
  else
  {
    //LOW
    if (reset_state == 1)
    {
      //ok, temos um sinal de reset
      reset_state = 0;

      // desliga o arduino
      digitalWrite(rpin, LOW);


      // limpa o cache
      logstr = "";
      force_log = false; //para de enviar log para o raspberry

    }
  }
}


void check_sensor()
{
  //verifica se ha disparo durante o estado acordado

   
   
    if (digitalRead(csensor) == HIGH) //pino esta alto
    {
      wait_lower = true;
    }  //espera ele baixa
    else
    {
      if (wait_lower)  //estava esperando baixar ?
      {

        adquire_log();  //aciona o log
        wait_lower = false;  //agora espera subir
      }
    }
   


}





void loop()
{


  check_reset(); //verifica se o reset pin esta flipando

  //foi acordado pelo pluviometro
  if (state == 1)
  {
    adquire_log();
    state = 0;
  }
  else
  {
      check_sensor(); ///verifica se tem sinal do sensor de chuva
  }

  if (sleep_cicles > 120) //apos 15min, verifica se nao precisa descarregar o log
  {
    verifica_log_remainder();
    sleep_cicles = 0;
  }


  //sistema requer o log ?
  if (force_log)
  {
    send_log();
  }




  if (force_log == false)
  {
    wake_cicles = wake_cicles + 1;
    if (wake_cicles > 20) //muitos ciclos acordados ?
    {

      Serial.println("sleep");
      delay(10);

      attachInterrupt(0, wakeUp, FALLING);
      // Enter power down state for 8 s with ADC and BOD module disabled
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

      detachInterrupt(0);
      Serial.println("wake");

      wake_cicles = 0;
      sleep_cicles = sleep_cicles + 1;

    }
  }

  delay(10);



}
