//8266Many2OneReceiver2.ino
//for Ashlee's controller base station 2/23/2023
//ESP-NOW Receiver Code
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-many-to-one-esp8266-nodemcu/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <U8g2lib.h>

// Structure example to receive data
// Must match the sender structure
  // Must match the receiver structure
typedef struct struct_message {
	uint8_t  id;
	uint8_t  event;
	uint8_t  status;
	uint8_t  e1Score;
	uint8_t  e2Score;
	uint8_t  e3Score;
	uint8_t  e4Score;
	uint8_t  messFails;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create a structure to hold the readings from each board
struct_message board1;
struct_message board2;
struct_message board3;

// Create an array with all the structures
struct_message boardsStruct[3] = {board1, board2, board3};
long heartBeats[3] = {0, 0, 0};
long heartTime = 0;
int comm1Loss = 0;
int comm2Loss = 0;
int comm3Loss = 0;
int commLossCode = 0;

// define constants for events to be used in myData.event
#define P1H_EVENT 1  // player 1 head shot code
#define P1B_EVENT 2  // player 1 body shot code
#define P2H_EVENT 3  // player 2 head shot code
#define P2B_EVENT 4  // player 2 body shot code

bool eventFlag = false;  //mmanages whether an event is within the window
unsigned long eventTime;  // holds time since event started 
unsigned long EventWindow = 250;  // window size for vetting event

bool scoreFlag = false;  // flag indicating score vetted
bool scored = false;  // flag indicating that score was scored (recorded)

uint8_t  scoreEvent = 0;  // the collected, vetted score

int p1Hscore = 0;  // collected individual event scores
int p1Bscore = 0;
int p2Hscore = 0;
int p2Bscore = 0;
int player1Total = 0;  // sum of p1H and p1B scores
int player2Total = 0;  // sum of 2H and p2B scores
int p1adder = 0;       // adder to p1 score by keyboard
int p2adder = 0;       // adder to p2 score by keyboard

//variable for 2 minute timer
unsigned long nextTick = 0;   // keeps the time until the next tenth second
int myTenths = 0;
int mySeconds = 0;
int myMinutes = 2;

// the button to start and stop the 2 minute timer
int  timerStartStop = D6;   
int MATCHTIMEmins = 2;  //not a constant,  can be changed remotely
int MATCHTIMEsecs = 0;
bool timerButtonStat = false;
bool timerToggle = false;
bool timerRun = false;

// the button to reset the score and time at the beginning of the match
int timeScoreReset = D3;    
bool scoreStat = false;

// switch to for 1 judge mode variables for input locations
int judge = D7;   // pin 3
bool oneJudge = false;  // create program variables for switch status

//output for end of match beeper
int beeper = D0;

unsigned long updateTime;    // manages when to update scoring display
unsigned long ShowTime = 500;  // how often displlay is updated

// Create a U8G2 display Object for specific display and comm interface being used
//U8G2_SSD1306_64X48_ER_F_HW_I2C u8g2(U8G2_R0, 22, 21);
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, 22, 21);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C  u8g2(U8G2_R0, 20, 19);


/****************
** start of OnDataRecv program that automatically is called when message is received
**
** this is a Callback function that will be executed when data is received
*/
void OnDataRecv(uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) 
{
char macStr[18];

	memcpy(&myData, incomingData, sizeof(myData));
//	Serial.printf("Board ID %u: %u bytes\n", myData.id, len);
  
  // Update the structures with all the new incoming data
	boardsStruct[myData.id-1].id = myData.id;
	boardsStruct[myData.id-1].event = myData.event;
	boardsStruct[myData.id-1].status = myData.status;
	boardsStruct[myData.id-1].e1Score = myData.e1Score;
	boardsStruct[myData.id-1].e2Score = myData.e2Score;
	boardsStruct[myData.id-1].e3Score = myData.e3Score;
	boardsStruct[myData.id-1].e4Score = myData.e4Score;
	boardsStruct[myData.id-1].messFails = myData.messFails;
  heartBeats[myData.id-1] = millis();  // record the last time thhis node sent a message

 // Serial.printf("event %u:  messF %u: \n", myData.event, myData.messFails );

  if(timerRun){
	  if ((myData.event > 0) && !eventFlag) 
	  {
		  eventTime = millis();
		  eventFlag = true;
	  }
  }

}

/****************
** start of setup
*/
 
void setup() 
{

   // Initialize Serial Monitor
	Serial.begin(115200);
  
  // configute IO to input button info
	pinMode(judge, INPUT_PULLUP);
	pinMode(timerStartStop, INPUT_PULLUP);
	pinMode(timeScoreReset, INPUT_PULLUP);
  pinMode(beeper, OUTPUT);
  
	u8g2.begin();
//	Serial.println("Display initialized"); 

  // Set device as a Wi-Fi Station
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();

  // Init ESP-NOW
	if (esp_now_init() != 0) 
	{
//		Serial.println("Error initializing ESP-NOW");
		return;
	}
	else 
	{
//		Serial.println();
//		Serial.println( " ESP-Now good!");
	}
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packet info
	esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
	esp_now_register_recv_cb(OnDataRecv);
}

/****************
** start of main program
*/
void loop()
{
	
  // manipulate event window timing flags 
  // and clear data after event window
	if ((millis() - eventTime) > EventWindow)
	{
		eventFlag = false;
		scoreFlag = false;
		scored = false;   //if I clear flags when scored  do I need this flag/??
		scoreEvent = 0;
		boardsStruct[0].event = 0;
		boardsStruct[1].event = 0;
		boardsStruct[2].event = 0;
	}

  // eventFlag says something happened
  // Access the variables for scoring within the event window
	if (eventFlag)
	{    // eventFlag stays true until end of eventTime
		oneJudge = !digitalRead(judge);   // read switch to see if there is only one judge

		if (oneJudge == true)
		{
		scoreEvent = myData.event; 	
		scoreFlag = true;  // scoreFlag will indicate a mach has been found
		}
		else 
		{
			// make sure two controllers agree on an event during event window
			if (boardsStruct[0].event == boardsStruct[1].event) 
			{
				scoreEvent = boardsStruct[0].event;  // scoreEvent will hold code from boards
				scoreFlag = true;  // scoreFlag will indicate a mach has been found
			} 
			else if (boardsStruct[0].event == boardsStruct[2].event) 
			{
				scoreEvent = boardsStruct[0].event;
				scoreFlag = true;
			}
			else if (boardsStruct[1].event == boardsStruct[2].event) 
			{
				scoreEvent = boardsStruct[1].event;
				scoreFlag = true;
			} 
			else 
			{                // no scoring match found
				scoreEvent = 0;     // if no match found clear scoreEvent to prevent error
			}
		}
	}

// determine who scored from event codes
	if (scoreFlag && !scored )
	{
		scored = true;    // set flag to score only once per event
      // determine event type from codes
		switch(scoreEvent)
		{
			case P1H_EVENT:  //player 1 head shot code 
				p1Hscore += 2;
			break;
			
			case P1B_EVENT:  //player 1 body shot code
			p1Bscore++;
			break;
			
			case P2H_EVENT:  //player 2 head shot code
			p2Hscore += 2;
			break;
			
			case P2B_EVENT:  //player 2 body shot code
			p2Bscore++;
			break;
			
			default:
			break;
		}
	}

  //calculate the total score for each player
	player1Total = p1Hscore + p1Bscore + p1adder;  
	player2Total = p2Hscore + p2Bscore + p2adder;

// Check that you haven't lost comm with any remote
  heartTime = millis();
  if ((heartTime - heartBeats[0]) > 1100) 
  {
    comm1Loss = 1;
  }
  else
  {
    comm1Loss = 0;
  }

  if ((heartTime - heartBeats[1]) > 1100) 
  {
    comm2Loss = 2;
  }
  else
  {
    comm2Loss = 0;
  }

 if ((heartTime - heartBeats[2]) > 1100) 
  {
    comm3Loss = 4;
  }
  else
  {
    comm3Loss = 0;
  }

  commLossCode = comm1Loss +  comm2Loss + comm3Loss;  //data encoded to send to Processing

  // Create a count-down timer
  if(timerRun){
		if((millis() - nextTick) > 99){
			nextTick = millis();
			
			myTenths++;
			if (myTenths > 9){
				myTenths = 0;
				mySeconds--;
			}
			if(mySeconds < 0){
				mySeconds = 59;
				myMinutes--;
			}
			if((myMinutes == 0) && (mySeconds == 0)){  // stops when timer runs down
				timerRun = false;
				sendSerialData(player1Total, player2Total, myMinutes, mySeconds, commLossCode);
				sendTotalData();
				beepTwice();
			}
		}
	}
	

// read button to start of stop timer
	timerButtonStat = !digitalRead(timerStartStop);   
	if(timerButtonStat && !timerToggle){
		timerRun = !timerRun;
		timerToggle = true;
	}
	if(!timerButtonStat){
		timerToggle = false;		
	}

// read button to reset time and score at beginning of match
	scoreStat = !digitalRead(timeScoreReset);   
	if(scoreStat){
		resetMatch();
	}

// send info to display every ShowTime seconds; Shoe time is equal to 500ms
	if((millis() - updateTime) > ShowTime)
	{
		updateTime = millis();
		displayScore(player1Total, player2Total, myMinutes, mySeconds);
    sendSerialData(player1Total, player2Total, myMinutes, mySeconds, commLossCode);
	}
 
// check for a serial input from Processing
	if (Serial.available() != 0) {
	// then you have something to read
		readSerialKey();
	}


}  // end of main loop

/*********************
**  routine to display score and time to OLED display
*/

void displayScore(int pscore1, int pscore2, int mins, int secs) 
{
char displayBuffer[20];
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13_tf );

//    u8g2.drawStr(40,10,"Scores");
//    u8g2.setFont(u8g2_font_9x18_tf );
    u8g2.drawStr(22,10,"BLUE"); 
    u8g2.drawStr(85,10,"RED"); 

//    Other fonts stuff
//    u8g2.setFont(u8g2_font_5x7_tf );  // little font
//    u8g2.setFont(u8g2_font_10x20_tf );  //other interesting bigger fonts
//    u8g2.setFont(u8g2_font_t0_22b_tf );
//    u8g2.setFont(u8g2_font_mystery_quest_28_tf );  // cool big font

    u8g2.setFont(u8g2_font_profont29_tf);   // professionsl big font
  
	u8g2.setCursor(20,36);    // (x,y) pixels
//   sprintf(displayBuffer,"Temp %2d pts",pscore); // example with added text   
    sprintf(displayBuffer,"%2d",pscore1); // using displayBuffer for formatted output   
    u8g2.print(displayBuffer);

    u8g2.setCursor(64,36);   
    sprintf(displayBuffer," %2d",pscore2);  // just the number 
    u8g2.print(displayBuffer);
	
    u8g2.setCursor(25,64);   
    sprintf(displayBuffer,"%2d:%02d",mins,secs);   
    u8g2.print(displayBuffer);


//    Other drawing stuff
//    u8g2.drawStr(0,30,displayBuffer);  ;; use draw string with char buffer
//    u8g2.drawPixel(30,58);  //  //(x, y)
//    u8g2.drawHLine(10,37,108);  // (xstart, ystart, length)
//    u8g2.drawCircle(80,50,10); // (centerx, centery, radius)
//    u8g2.drawBox(5,35,20,10);   //solid ( upperLeftx, upperLefty, xsize, ysize)
//    u8g2.drawFrame(30,18,20,20);   // hollow ( upperLeftx, upperLefty, xsize, ysize)
    u8g2.drawLine(64, 16, 64, 34);  //(xstart, ystart, xend, yend)
    
    u8g2.sendBuffer();
}

/*********************
**  routine to display on going score and time and all data TO processing program
*/
void sendSerialData(int pscore1, int pscore2, int mins, int secs, int commCode)
{
	char buffer[20];

	sprintf(buffer, "%d,%d,%d,%d,%d", pscore1, pscore2, mins, secs, commCode);
	Serial.println(buffer);
}



/*********************
**  routine to send all board data TO processing program at match end
*/
void sendTotalData()
{
	char buffer[20];

	uint8_t id, ev, st, e1;
	uint8_t e2, e3, e4, fail;
	int boardNum = 3;
	int i = 0;

	for ( i = 0; i<3; i++)
	{	
//		id = boardsStruct[i].id;
		id = i+1;
		ev = boardsStruct[i].event;
		st = boardsStruct[i].status;
		e1 = boardsStruct[i].e1Score;
		e2 = boardsStruct[i].e2Score;
		e3 = boardsStruct[i].e3Score;
		e4 = boardsStruct[i].e4Score;
		fail = boardsStruct[i].messFails;
		
		sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d,%d", id,ev,st,e1,e2,e3,e4,fail);
		Serial.println(buffer);
	}

}


/*********************
**  routine to read keys FROM processing  
**
**  Ashlee want to be able to start and stop timer, add to score, and reset timer and score
*/
void readSerialKey()
{

char c;
	
	c = Serial.read();
		
	if(c == 'r')
	{
		resetMatch();
	}
	else if(c == 't')
	{
		timerRun = !timerRun;	
	}
	else if ( c == '1')
	{
		p1adder++;  // the number 1   added to Blue Score
	}
	else if ( c == '2')
	{
		p1adder--;      // Subtracted from Blue Score
    if (p1adder < 0)
    {
      p1adder = 0;
    }
	}
	else if ( c == '0')
	{
		p2adder++;   // added to Red score
	}
  else if ( c == '9')
	{
		p2adder--;    // subtracted from Red score
    if (p2adder < 0)
    {
      p2adder = 0;      
    }
	}
	else if ( c == 'l')
	{
		EventWindow += 10;  // lowercase L
	}
	else if ( c == 'k')
	{
		EventWindow -= 10;  //lowercase K
    if (EventWindow < 100)
    {
      EventWindow = 100;   // default 250, minimum 100
    }
	}
	else if ( c == 'z')
	{
		if( MATCHTIMEsecs == 30)  //addd 30 seconds to MatchTime  Default 2:00 minutes
    {
      MATCHTIMEsecs = 0;
      MATCHTIMEmins++;
		}
    else 
    {
      MATCHTIMEsecs = 30;
    }
  }
	else if ( c == 'x')
	{
    if ( MATCHTIMEsecs == 30)  //subtract 30 seconds frem MatchTime,  Default 2:00 minutes
    {
      MATCHTIMEsecs = 0;
    }
    else
    {
      if(MATCHTIMEmins > 1)  // Minumum MatchTIme = 1:00 minutes
      {
			  MATCHTIMEsecs = 30;
        MATCHTIMEmins--;          
      }
    }
	}
	else if ( c == 'd')
  {
    sendTotalData();
  }
	else
	{
			
	}
}


/*********************
**  routine to beep buzzer at end of 2 minute match  
***/
void beepTwice()
{
	//sound beeper for 1 sec
  digitalWrite(beeper,HIGH);
  delay(500);
  digitalWrite(beeper,LOW);
  delay(500);
  digitalWrite(beeper,HIGH);
  delay(500);
  digitalWrite(beeper,LOW);
}

/*********************
**  routine to beep buzzer at lost message  
**
*/
void beepOnce()
{
	//sound beeper for 1 sec
  digitalWrite(beeper,HIGH);
  delay(500);
  digitalWrite(beeper,LOW);
}


/*********************
**  routine to reset the time and score when either:
**    the reset button is pressed or reset receiived from processing
**
**  Ashlee want to be able to start and stop timer, add to score, and reset timer and score
*/
void resetMatch()
{
  //clear all scores
  p1Hscore = 0;
  p1Bscore = 0;
  p2Hscore = 0;
  p2Bscore = 0;
  p1adder = 0;
  p2adder = 0;

  // reset time
  myTenths = 0;
  myMinutes = MATCHTIMEmins;
  mySeconds = MATCHTIMEsecs;
  timerRun = false;
	
}

