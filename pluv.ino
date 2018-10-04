#include <avr/sleep.h>
#include <avr/power.h>

//pino responsavel pelo power up e down do raspberry pi 
int power_pin = 7;

//pino reset serve para resetar o contador na memoria e esperar um tempo e dar shutdown na energia do raspberry
int reset_pin = 6;

//clock e data serve para enviar dados ao raspberri de forma sincronizada
int clock_pin = 5;
int data_pin = 4;

//pluvi pin quando em low conta como evento de contagem
int pluv_pin = 2;

//variaveis do modulo de envio sincrono
int clockState = LOW;
int internalClock = LOW;
int bufferPosition = LOW;
int bitPosition = LOW;


//contador que vai no buffer de dados
int counterBuffer = 0;
//tempo entre as leituras
int loop_dt = 5;

// is_rasp significa que o raspberry esta ligado e o arduino esta esperando o sinal de reset

bool is_rasp = false; // esta em comunicacao com o raspberry pi ?

//contador do pluviometro
int counter = 0;


void event(void)
{
  //avisa que houve evento
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  counter = counter + 1;
  //Serial.print("event ");
  //Serial.println(counter);
}


int listen = 0;
void pin2Interrupt(void)
{
  /* This will bring us back from sleep. */
  /* We detach the interrupt to stop it from
   * continuously firing while the interrupt pin
   * is low.
   */
  detachInterrupt(0);
  //Serial.println("awake");
  event();
}


void enterSleep(void)
{
  // Serial.println("sleep");

  /* Setup pin2 as an interrupt and attach handler. */
  attachInterrupt(0, pin2Interrupt, LOW);
  digitalWrite(13, LOW);
  digitalWrite(power_pin , LOW);
  delay(100);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  /* The program will continue from here. */
  /* First thing to do is disable sleep. */
  sleep_disable();
}




void setup() {
  //Serial.begin(9600);
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  pinMode(data_pin,  OUTPUT);
  pinMode(pluv_pin,  INPUT);
  pinMode(power_pin, OUTPUT);
  pinMode(reset_pin, INPUT);

  
  is_rasp = false;

  digitalWrite(power_pin , LOW);
  
  internalClock = 0;
}


//rotina que cuida dos envios dos dados para o raspberry pi
void SendData()
{
  internalClock = internalClock + 1;
  if (internalClock < 10) return;  //poucos contadores .. nem se de ao trabalho  

  internalClock = 0;
  if (clockState == LOW) { clockState = HIGH; }
  else { clockState = LOW; }

  if (clockState == HIGH)
  {
    if (counterBuffer > 0) 
      {
         digitalWrite(data_pin, HIGH);
         digitalWrite(13, HIGH);
      }
    else
    {
      digitalWrite(data_pin, LOW);
      digitalWrite(13, LOW);
      counterBuffer = counter + 1;
    }
    counterBuffer = counterBuffer - 1;
  }
  //tem que ser depois
  digitalWrite(clock_pin, clockState);

}

int seconds = 0; //secondos em espera
volatile int old_val = HIGH;

//rotina que le o sinal do pluviometro quando acordado
void listem_pluv()
{
  int  val = digitalRead(pluv_pin);
  if (old_val == HIGH && val == LOW)
  {
    event(); //add o contador
    seconds = 0; //sera o contador
  }
  old_val = val;

  //se o raspberry estiver desligado inicie a contagem para voltar a durmir
  if (is_rasp == false)
  {
    seconds++;
    if (loop_dt*seconds > 10 * 1000)   //10 seg para desligar a forca e dormir
    {
      //entra em sono
      delay(200);
      seconds = 0;   
      digitalWrite(power_pin , LOW);   //desliga a forca
      enterSleep();
    }
  }

}


//rotina que decide se deve enviar dados para o raspberrypi
void send_to_rap()
{
  is_rasp = false;
  if (counter > 4)
  {
    is_rasp = true;
    digitalWrite(power_pin , HIGH); 
    SendData();
  }
  if (digitalRead(reset_pin) == HIGH)
  {
    is_rasp = false;    //desliga a variavel que segura o raspberry pi  
    counter = 0;
  }
  
   digitalWrite(13, LOW); //desliga o led   
}

 

void loop()
{
  send_to_rap();
  listem_pluv(); 
  delay(loop_dt);
}
