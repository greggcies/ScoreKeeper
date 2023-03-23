//8266Many2OneSender2.ino
//ESP-NOW code for Ashlee's controllers 1/11/2023
//Sender Code

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

// REPLACE WITH RECEIVER MAC Address
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//uint8_t broadcastAddress[] = {0x2C, 0x3A, 0xE8, 0x22, 0x5D, 0xA6};  // breadboard version
uint8_t broadcastAddress[] = {0xBC, 0xFF, 0x4D, 0xF7, 0xD9, 0xDE};

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1  //  IE. myData.id

 
// define constants for controller events to be used in myData.event
#define Px_RELEASED 0
#define P1H_EVENT 1
#define P1B_EVENT 2
#define P2H_EVENT 3
#define P2B_EVENT 4


// define constants for transmitter conditions  IE. myData.status
# define OK 0
# define RETRY 1
# define LOW_BAT 2
# define PERIODIC 4



/**** alternate message Structure def example to send data
**  if we have the handheld keep its own score and send the total along to receiver periodically
*/
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

// Create the struct_message called test to store variables to be sent
struct_message myData;  // the working data structure

// Create the variables to store status flags and status times
unsigned long debounceTime = 0;  // holds when the switch closed for debounce time
unsigned long blinkTime = 0;  // holds when the message lite was turned on 
unsigned long SettleTime = 20;   // debounce time limit
unsigned long HeartBeat = 2000;  // max number of millisec for mess so receiver knows xmitter is alive 
unsigned long ResponseTime = 200; // max number of millisec before message fault fault declared
unsigned long BlinkON = 100; // max number of millisec the message lite is on


unsigned long messSentTime = 0;  // holds time since last message
bool messSentFlag = false;
bool messRetryFlag = false;
uint8_t messRetries = 0;
uint8_t RetriesLimit = 3;  // max mess retries before messFails recorded

bool oneShot1 = false;
bool oneShot2 = false;
bool oneShot3 = false;
bool oneShot4 = false;
bool blockShot = false;

// create program variables for button locations
int p1h_button = D6;   // d6 pin 
int p1b_button = D7;   // d7 pin 
int p2h_button = D3;   // d3 pin 
int p2b_button = D4;   // d4 pin 
int messLite = D1;     // d1 pin

// create program variables for button status
bool p1h_stat = false;
bool p1b_stat = false;
bool p2h_stat = false;
bool p2b_stat = false;
bool messLiteStat = false;

bool debugTest = false;
bool debugLock = false;
int debugCount = 0;

/**********************
**
**   Callback when data is sent and the receiver resp4onds
**
**
*/

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) 
{
	Serial.print("\r\nLast Packet Send Status: ");
	if (sendStatus == 0)
	{
		Serial.println("Delivery success"); // if the reveiver responds sucessfully
		messSentFlag = false;				// clear all mess onfo when completed
		messRetryFlag = false;
		messRetries = 0;
	}
	else
	{
		Serial.println("Delivery fail");   
		messRetryFlag = true;			// messRetryFlag should initiate a resend???
		messRetries++; 			 		// messRetries counts number of retries before failure
	}
}


/**********************
**
**   Setup starts here 
**
**
*/

void setup() 
{
  // Init Serial Monitor
	Serial.begin(115200);
  
  // configute IO to input button info
	pinMode(p1h_button, INPUT_PULLUP);
	pinMode(p1b_button, INPUT_PULLUP);
	pinMode(p2h_button, INPUT_PULLUP);
	pinMode(p2b_button, INPUT_PULLUP);
	pinMode(messLite, OUTPUT);
  
  // Initialize message data
	myData.id = BOARD_ID;
	myData.event = Px_RELEASED;
	myData.status = OK;
	myData.e1Score = 0;
	myData.e2Score = 0;
	myData.e3Score = 0;
	myData.e4Score = 0;
	myData.messFails = 0;

  Serial.println("prelim1 Setup complete");  
  
  // Set device as a Wi-Fi Station
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();

  // Init ESP-NOW
	if (esp_now_init() != 0) 
	{
		Serial.println("Error initializing ESP-NOW");
		return;
	} 
    Serial.println("prelim2 Setup complete");
  // Set ESP-NOW role
	esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

  // Once ESPNow is successfully init, we will register for Send CB to
  // get the status of Trasnmitted packet
	esp_now_register_send_cb(OnDataSent);
  
  // Register peer
	esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  Serial.println("Setup complete");

}

/**********************
**
**   main Loop starts here
**
**
*/

void loop() 
{

	myData.id = BOARD_ID;  // set the board ID

  //  the callback says the message has failed more time than limit, so count as failed
	if( messRetries >= RetriesLimit)
	{ 
		messSentFlag = false;
		messRetryFlag = false;
		messRetries = 0;
		myData.messFails++;  // can't really fix it but can count it.
	}
		
 /*  ***********   rountine to send one message to let receiver know p1h_button was depressed
 ** 
 ** oneShot1 identified button1 and 
 ** debounceTime is the variable to prevent multi trwansmits due to the switch bounce
 ** blockShot forbids other button press overlap
 **
 */
 
// Read Buttons

  p1h_stat = !digitalRead(p1h_button);
	if( !blockShot && p1h_stat )  // one shot when player 1 head button pressed (event 1)
	{    
		oneShot1 = true; 			// one shot latch
		blockShot = true; 		// prevent overlap
		debounceTime = millis();	// get time for switch debounce
	
      // Set messages values to send
		myData.event = P1H_EVENT;
		myData.status = OK;
		myData.e1Score++;          //  data struct keeps track of all button presses used
    
    Serial.print ("p1h is pressed  ");
    Serial.println(debugCount); 

      // Send message via ESP-NOW
		sendMessage();
    }


  p1b_stat =  !digitalRead(p1b_button);
	if( !blockShot && p1b_stat )   // one shot when player 1 body button pressed (event 2)
	{ 
		oneShot2 = true; 			// one shot latch
		blockShot = true; 			//  prevent overlap
		debounceTime = millis();	// get time for switch debounce
	
      // Set messages values to send
		myData.event = P1B_EVENT;
		myData.status = OK;
		myData.e2Score++;          //  data struct keeps track of all button presses used

      // Send message via ESP-NOW
		sendMessage();
    }

 
  p2h_stat = !digitalRead(p2h_button);
	if( !blockShot && p2h_stat )   // one shot when player 2 head button pressed (event 3)
	{ 
		oneShot3 = true; 			// one shot latch
		blockShot = true; 			// prevent overlap
		debounceTime = millis();	// get time for switch debounce
	
      // Set messages values to send
		myData.event = P2H_EVENT;
		myData.status = OK;
		myData.e3Score++;          //  data struct keeps track of all button presses used

      // Send message via ESP-NOW
		sendMessage();
    }

  p2b_stat = !digitalRead(p2b_button);
	if( !blockShot && p2b_stat )   // one shot when player 2 body button pressed (event 4)
	{ 
		oneShot4 = true; 			// one shot latch
		blockShot = true; 			// prevent overlap
		debounceTime = millis();	// get time for switch debounce
	
      // Set messages values to send
		myData.event = P2B_EVENT;
		myData.status = OK;
		myData.e4Score++;          //  data struct keeps track of all button presses used

      // Send message via ESP-NOW
		sendMessage();
    }  // end of button pressed


 /******* clear affected flags when button is released 
 **  
 */
 
  if( blockShot && ((millis() - debounceTime) > SettleTime)) //  debounce time expired on after button pressed
  {    
		if( oneShot1 && !p1h_stat)     //  player 1 head button released
		{ 
			oneShot1 = false;
			blockShot = false;
 			myData.event = Px_RELEASED;  // clear data in message buffer but doesn't send to recvr  (event 0)
		}

		if( oneShot2 && !p1b_stat )   //  player 1 body button released
		{  
			oneShot2 = false;
			blockShot = false;
			myData.event = Px_RELEASED;  // clear data in message buffer but doesn't send to recvr
		}

		if( oneShot3 && !p2h_stat )   //  player 2 head button released
		{ 
			oneShot3 = false;
			blockShot = false;
			myData.event = Px_RELEASED;  // clear data in message buffer but doesn't send to recvr
		}
    
		if( oneShot4 && !p2b_stat )    //  player 2 body button released
		{  
			oneShot4 = false;
			blockShot = false;
			myData.event = Px_RELEASED;  // clear data in message buffer but doesn't send to recvr
		}
	} // end of buttons released
 
/*  ***********   resend a lost message, if  mess sent, but no reply or failure within time send a message again   */

	if ((messSentFlag || messRetryFlag) && ((millis() - messSentTime) > ResponseTime)) 
	{ 
		// Set values to send	    
		myData.status = RETRY;
	
		// Send message via ESP-NOW
		sendMessage();
	}   // end of resend


 /*  ***********  send a heartbeat message, if no other mess sent, to let receiver know you're alive */
	if ((millis() - messSentTime) > HeartBeat) 
	{
		// Set values to send	    
		myData.id = BOARD_ID;
		myData.event = Px_RELEASED;
		myData.status = PERIODIC;

	  Serial.print ("Heart beat from node: ");
    Serial.println (myData.id);
    
		// Send message via ESP-NOW
		sendMessage();
	}  // end of heartbeat


/*  ***********  turn off the lite  after message sent */
	if ((millis() - blinkTime) > BlinkON )
	{
		messLiteStat = false;
		digitalWrite(messLite,LOW);
	}

}

/**********************
**
**   Send Message routine starts here 
**
*/

void sendMessage()
{
    // Send message via ESP-NOW
  esp_now_send(0, (uint8_t *) &myData, sizeof(myData));
	
	messSentFlag = true;
	messSentTime = millis();    // record when messages sent
	blinkLite();
}

/**********************
**
**   Blink the light with every message routine starts here
** 
*/

void blinkLite()
{
	messLiteStat = true;
	blinkTime = millis();
	digitalWrite(messLite,HIGH);
}
