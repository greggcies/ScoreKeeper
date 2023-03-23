// https://discourse.processing.org/t/use-the-arduino-processing-//interface/9409/3

import processing.serial.*;      // import the Processing serial library
import processing.sound.*;

Serial myPort;                   // The serial port
Sound s;
Pulse pulse;

// TaeKwonDo variables
int id=0;
int event=0;
int status=0;
int  e1Count=0;
int  e2Count=0;
int  e3Count=0;
int  e4Count=0;
int  mfails=0;

int player1Score = 0;
int player2Score = 0;

char keyValue = '0';

float centBox1x, centBox2x;
float centBox1y, centBox2y;
float dataLinex, dataLiney;

int rectWidth = 400;  //was 250
int rectHeight = 400;  //was 250
int scoreSize = 260;  // was 150
int clockSize = 200; // was 120
int tSize = 32;
int dataSize = 16;
int indent = 20;

int secs = 38;
int clockSecs = 0;
int runningSecs = 0;
int mins = 1;
int commCode = 0;

boolean beeped = false;

//int value1[] = new int[8];    // 1-d arrat

int value1[][] = new int[3][8];   //2-d array


void setup(){
//  size(1000,800);
  fullScreen();
  // Open serial port
//  printArray(Serial.list());
  myPort = new Serial(this, Serial.list()[0], 115200);
  myPort.bufferUntil('\n');          /* Read bytes into a buffer until you get a linefeed (ASCII 10):*/

  centBox1x = width/2 - width/4; // was width/2 - width/3
  centBox2x = width/2 + width/4; // was width/2 + width/4
  centBox1y = height/2 - height/5 + 35;  // was height/2 - height/4
  centBox2y = height/2 - height/5 + 35; // was height/2 - height/4
  
  dataLinex = width/2 - width/4;
  dataLiney = height/2 + height/3; // was  height/2 + height/4
  
  for (int i = 0; i<3; i++)     // for initialiing 2d array of ints
  {  
    for (int j = 0; j<8; j++)
    {
      value1[i][j] = 0;
    }
  }
  value1[0][0] = 1;
  value1[1][0] = 2;
  value1[2][0] = 3;

  
  
//  SinOsc sin = new SinOsc(this);
//  sin.play(660, 0.4);
  
   pulse = new Pulse(this);

}
 
void draw() {
  background(0);
  textAlign(CENTER,TOP);
  textSize(tSize);
  fill(255,0,0);  // red
  strokeWeight(4);

  if( mins == 0 && secs == 0 && !beeped )
  {
     pulse.play();
     delay(500);
     pulse.stop();  
     beeped = true;
  }
  else if( mins != 0 )
  {
    beeped = false;
  }
    
  
  text("Tae Kwon Do Scoring", width/2, 2);
  
  fill(0,0,255);  // Blue
  rect(centBox1x - rectWidth/2, centBox1y - rectHeight/2, rectWidth, rectHeight);
  fill(255,0,0);  // Blue
  rect(centBox2x - rectWidth/2, centBox2y - rectHeight/2, rectWidth, rectHeight);
  

  fill(255,0,255);  // Magenta
  
//  text("#1", centBox1x, centBox1y + rectHeight/2);
//  text("#2", centBox2x, centBox2y + rectHeight/2);
  
  textSize(scoreSize);
  fill(255,255,255);  // White

  text(player1Score, centBox1x, centBox1y - (2 * scoreSize / 3));
  text(player2Score, centBox2x, centBox2y - (2 * scoreSize / 3));

  textSize(clockSize-120);
  text("Time", width/4-10, height/2 + 175); // was "Time", width/2, height/2
  textSize(clockSize);
  text(mins, width/2 - 75, height/2 + 50);  // was mins, width/2-50, height/2+120
  text(":", width/2 - 10, height/2 + 50);
  
  if (secs <= 9)
  {
    text("0", width/2 + 60, height/2 + 50);
    text(secs, width/2 + 150, height/2 + 50);
  }
  else 
  {
    text(secs, width/2 + 105, height/2 + 50 );
  }
  textSize(dataSize);

  stroke(255,255,0);
  strokeWeight(4);
  noFill();
  
 // The rest of the variables
  textAlign(LEFT,TOP);
  fill(0,255,255);  // cyan

//  text("id", indent, dataLiney + 85);
  text("id", dataLinex - 60, dataLiney + 85);
  text("#1", dataLinex , dataLiney + 85);
  text("#2", dataLinex + width/4, dataLiney + 85);
  text("#3", dataLinex + (2 * width/4), dataLiney + 85);
  
   fill(255,0,0);  // red  
   fill(0,255,0);  // green 
   strokeWeight(1); // thin line around comm boxes

//  line(dataLinex-10 , dataLiney + 75, dataLinex + 20, dataLiney +75);
//  line(dataLinex + width/4 - 10 , dataLiney + 75,  dataLinex + width/4  + 20 , dataLiney + 75);
//  line(dataLinex + 2*width/4 - 10, dataLiney + 75, dataLinex + 2 * width/4 + 20, dataLiney + 75);

// Comm Status boxes for each controller

  if(commCode == 1 || commCode == 3 || commCode == 5 || commCode == 7)
  {
     fill(255,0,0);  // red
  }
  rect(dataLinex-2 , dataLiney +65, 18, 18);
  
  fill(0,255,0);  // green 
  if(commCode == 2 || commCode == 3 || commCode == 6 || commCode == 7)
  {
     fill(255,0,0);  // red
  }
  rect(dataLinex + width/4 - 2 , dataLiney + 65,  18, 18);
  
  fill(0,255,0);  // green 
  if(commCode == 4 || commCode == 5 || commCode == 6 || commCode == 7)
  {
     fill(255,0,0);  // red
  }
  rect(dataLinex + 2*width/4 - 2, dataLiney + 65, 18, 18);


  textAlign(CENTER,TOP);
  
}
 
/* serialEvent  method is run automatically by the Processing applet 
** whenever the buffer reaches the byte value set in the bufferUntil() method in the setup():
*/

void serialEvent(Serial myPort) {                     // read the serial buffer:
  String myString = myPort.readStringUntil('\n');
  myString = trim(myString);                          // if you got any bytes other than the linefeed:
  println(myString);

  int values[] = int(split(myString, ','));           // split the string at the commas// and convert the sections into integers:
  if (values.length > 0) 
  { 
    if (values.length < 6) 
    {
      player1Score = values[0];
      player2Score = values[1];
      mins = values[2];
      secs = values[3];
      commCode = values[4];
    }
    else
    {
      for(int i = 0; i < 8; i++) 
      {
        value1[values[0]-1][i] = values[i];
      }
    }
  }
}

/*
  void mousePressed() {
      secs = 0;
      mins = 0;   
      
      for (int i = 0; i<3; i++)     // for initialiing 2d array of ints
      {  
        for (int j = 0; j<8; j++)
        {
            value1[i][j] = 0;
        }
      }
      value1[0][0] = 1;
      value1[1][0] = 2;
      value1[2][0] = 3;

  }
*/  
  /*
  **  routine that automatically runs whenever a key is pressed
  ** pre-defined variable key willl hole the input value
  */
  void keyPressed() {
      keyValue = key;
      
      myPort.write(keyValue);
      println("keyPressed");
      
  }
