#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>
#include <IRremote.h>
#include <NewPing.h>
#include <Servo.h>

//Sonar pin
#define TRIGGER_PIN1 6
#define ECHO_PIN1 7
#define TRIGGER_PIN2 8 
#define ECHO_PIN2 11
#define TRIGGER_PIN3 12
#define ECHO_PIN3 13
#define BUBBLEGUN_SERVO_PIN 9

#define MAX_DISTANCE 300 // Maximum distance (in cm) to ping.
#define PING_INTERVAL 50
#define NUMBEROFMISSREAD 2
#define PRESENCE_THRESHOLD 50
#define STRONGTRUE 2
#define AMBIGUOUS 1 
#define STRONGFALSE 0
#define STRONGFALSE_TIME 5000
#define TRIG_TIME 1000
#define TIME_THRESHOLD 1000
#define UNCHECKED_SENDING_PEOPLE_TIME 3000
#define SENDING_NO_PEOPLE_TIME 1000
#define SONAR_FURTHER_READING_DELAY 50
#define PEOPLE_PACKET 0xE0E0906F
#define NO_PEOPLE_PACKET 0xE0E0906A
#define DELAY_BETWEEN_IR_SENDING 30
#define TRIG_POSITION 180
#define UN_TRIG_POSITION 70
#define RX_PIN 4
#define TX_PIN 2
#define DETACH_TIME 300
#define MIN_VOLUME 15
#define MAX_VOLUME 30


int i = 0;
//SONAR READ VALUE
unsigned int cm_sonar1, cm_sonar2, cm_sonar3;
/*-------------------------------------------------------------------------
 * TIMER
 -------------------------------------------------------------------------*/
unsigned long sonar_timer, sending_people_timer, sending_no_people_timer, sonar_waiting_timer = 0;
unsigned long ir_timer = 0, strongfalse_timer = 0, bubble_gun_timer = 0, detach_timer = 0;

/*-----------------------------------------------------------------------------
 * FLAG RELATED TO TIMER
 -----------------------------------------------------------------------------*/
bool timer_flag = false, send_people_flag = false, send_no_people_flag = false;
bool strongfalse_flag = true, bubble_gun_trig = false;
bool detach_flag = false;

//TABLE HISTORY OF PEOPLE PRESENCE
bool people_presence_history [NUMBEROFMISSREAD] = {false, false};

//Used to store the actual state : STRONGTRUE, STRONGFALSE, AMIBUGUOUS (WHEN A READ SEE PRESENCE AND THE FOLLOWING/PREVIOUS DOES NOT)
int state = 0;

//first trigger pin, second echo pin
NewPing sonar1(TRIGGER_PIN1, ECHO_PIN1, MAX_DISTANCE);
NewPing sonar2(TRIGGER_PIN2, ECHO_PIN2, MAX_DISTANCE);
NewPing sonar3(TRIGGER_PIN3, ECHO_PIN3, MAX_DISTANCE);

//----------RX arduino ---> TX dfPlayer
SoftwareSerial mySerial(RX_PIN, TX_PIN); // RX, TX
byte dfplayer_volume = 30;

//sender object, pin 3

IRsend ir_sender;


//Servo variable:

Servo shell;
Servo trigger_bubble_gun;

void setup() {
  //mp3 serial
  dfplayer_volume = 30;
  mySerial.begin (9600);
  mp3_set_serial (mySerial);
  Serial.begin(115200);
  //mp3 set volume
  mp3_set_volume (dfplayer_volume);
         
  mp3_play (1);
}


void loop() {
    cm_sonar1 = sonar1.ping_cm();
    cm_sonar2 = sonar2.ping_cm();
    cm_sonar3 = sonar3.ping_cm();
    sonar_waiting_timer = millis();
    update_people_presence_history();
    Serial.println (cm_sonar1);
    Serial.println (cm_sonar2);
    Serial.println (cm_sonar3);
    state = check_presence_table();
    switch (state)
    {
      case STRONGTRUE : Serial.println("STRONGTRUE"); break;
      case STRONGFALSE : Serial.println("STRONGFALSE"); break;
      case AMBIGUOUS : Serial.println("AMBIGUOUS"); break;
    }
    if (send_people_flag == true)
    {
      //send packet
      send_people_packet();
      //decrease volume until it reach the min value
      if(dfplayer_volume > MIN_VOLUME)
      {
        dfplayer_volume--;
        mp3_set_volume (dfplayer_volume);        
      }
      Serial.println("Sending Packet");
      //checking if time is ended
      if ( millis() - sending_people_timer > UNCHECKED_SENDING_PEOPLE_TIME)
      {
        if (state == STRONGTRUE )
        {
          sending_people_timer = millis();
        }
        else if (state == STRONGFALSE)
        {
          //SEND FIRST NO PEOPLE PACKET
          //pop_up_shell();
          send_people_flag = false;
          send_no_people_flag = true;
          sending_no_people_timer = millis ();
          Serial.println("First no people packet sent");
          //send no people packet
          send_no_people_packet();
          strongfalse_flag = true;
          strongfalse_timer = millis();
          //INCREASE VOLUME UNTIL IT REACH A CERTAIN VALUE
          
        }
      }

    }

    else if (state == STRONGTRUE)
    {

      if (timer_flag == false)
      {
        //FIRST STRONGTRUE SEEN
        strongfalse_flag = false;
        if(bubble_gun_trig == true)
        {
          un_trig_bubble_gun();
          bubble_gun_trig = false;
        }
        Serial.println("Start presence Timer");
        sonar_timer = millis();
        timer_flag = true;
      }
      else if (timer_flag == true)
      {
        if (millis() - sonar_timer >= TIME_THRESHOLD)
        {
          //FIRST PACKET OF PRESENCE SENT
          un_trig_bubble_gun();
          timer_flag = false;
          send_no_people_flag = false;
          send_people_flag = true;
          send_people_packet();
          //send packet
          Serial.println("First packet Sent");
          //initialize timer
          sending_people_timer = millis();
          //pop_down_shell();
        }
      }
    }
    else if (state == AMBIGUOUS)
    {
      if (timer_flag == true)
      {
        if (millis() - sonar_timer >= TIME_THRESHOLD)
        {
          //FIRST PACKET OF PRESENCE SENT
          un_trig_bubble_gun();
          timer_flag = false;
          send_no_people_flag = false;
          send_people_flag = true;
          send_people_packet();
          //send packet
          Serial.println("First packet Sent");
          //initialize timer
          sending_people_timer = millis();
          //pop_down_shell();
        }
      }
      else if (strongfalse_flag == true)
      {
        if (millis() - strongfalse_timer > STRONGFALSE_TIME)
        {
          Serial.println("SERVO ATTIVATO");
          strongfalse_flag = false;
          trig_bubble_gun();
          bubble_gun_trig = true;
          bubble_gun_timer = millis();
        }
      }
      else if (bubble_gun_trig == true)
      {
        if ( millis() - bubble_gun_timer > TRIG_TIME)
        {
          Serial.println("DISATTIVATO SERVO");
          un_trig_bubble_gun();
          bubble_gun_trig = false;
        }
      }
      
      
    }
    else if (state == STRONGFALSE)
    {
      if(timer_flag == true)
      {
        Serial.println("Stop Sonar Timer");
        timer_flag = false;
        Serial.println("START STRONGFALSETIMER");
        strongfalse_flag = true;
        strongfalse_timer = millis();
      }
      else if (strongfalse_flag == true)
      {
        if (millis() - strongfalse_timer > STRONGFALSE_TIME)
        {
          Serial.println("SERVO ATTIVATO");
          strongfalse_flag = false;
          trig_bubble_gun();
          bubble_gun_trig = true;
          bubble_gun_timer = millis();
        }
      }
      else if (bubble_gun_trig == true)
      {
        if ( millis() - bubble_gun_timer > TRIG_TIME)
        {
          Serial.println("DISATTIVATO SERVO");
          un_trig_bubble_gun();
          bubble_gun_trig = false;
          
        Serial.println("START STRONGFALSETIMER");
          strongfalse_flag = true;
          strongfalse_timer = millis();
        }
      }
    }
    if (send_no_people_flag == true)
    {
      Serial.println("No people packet");
      // send no people packet
      send_no_people_packet();
      if ( millis () - sending_no_people_timer > SENDING_NO_PEOPLE_TIME)
      {
        Serial.println("Stop no people packet");
        send_no_people_flag = false;
      }
    }

    if(detach_flag)
    {
      if(millis() - detach_timer > DETACH_TIME)
      {
        trigger_bubble_gun.detach();
      }
    }
    //NO PEOPLE RECORDED IN FRONT OF THE OYSTER
    if(send_people_flag == false)
    {
      if (dfplayer_volume < MAX_VOLUME)
          {
            dfplayer_volume++;
            mp3_set_volume (dfplayer_volume);            
          }
    }
  //delay(300);
}
/*-------------------------------------------------
 * STRONG TRUE ACTION
 --------------------------------------------------*/

void update_people_presence_history()
{
  people_presence_history[0] = people_presence_history[1]; //shifting, in case of a greater number of allowed missread, will be substituted with a proper function
  if (presence_perceived(cm_sonar1) || presence_perceived(cm_sonar2) || presence_perceived(cm_sonar3))
  {
    people_presence_history [NUMBEROFMISSREAD - 1] = true;
  }
  else
  {
    people_presence_history[NUMBEROFMISSREAD - 1] = false;
  }
}



int check_presence_table()
{
  if (people_presence_history[0] == true && people_presence_history[1] == true) return STRONGTRUE;
  else if (people_presence_history[0] == false && people_presence_history[1] == false) return STRONGFALSE;
  else return AMBIGUOUS;
}



void trig_bubble_gun()
{
  trigger_bubble_gun.attach(BUBBLEGUN_SERVO_PIN);
  trigger_bubble_gun.write(TRIG_POSITION);
  detach_flag = true;
  detach_timer = millis();
}


void un_trig_bubble_gun()
{
  trigger_bubble_gun.attach(BUBBLEGUN_SERVO_PIN);
  trigger_bubble_gun.write(UN_TRIG_POSITION);
  
  detach_flag = true;
  detach_timer = millis();
  
}


bool presence_perceived( unsigned int distance)
{
  if (distance > 0 && distance < PRESENCE_THRESHOLD) return true;
  else return false;

}

void send_no_people_packet()
{
  if(millis() - ir_timer >= DELAY_BETWEEN_IR_SENDING)
  {
      ir_sender.sendSony(NO_PEOPLE_PACKET, 32);
      ir_timer = millis();
  }
}

void send_people_packet()
{
  if(millis() - ir_timer >= DELAY_BETWEEN_IR_SENDING)
  {
    ir_sender.sendSony(PEOPLE_PACKET, 32);
    ir_timer = millis ();
  }  
}

