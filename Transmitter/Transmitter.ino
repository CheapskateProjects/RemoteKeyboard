/*
  RemoteKeyboard

  This code transfers serial input from transmitter through radio link (nRF24L01) to receiver. Receiver
  then uses that input to perform matching keyboard actions. 

  Receiver should use 32u4 based Arduino to have keyboard interface support.

  Unless escape sequences are used each line of serial input is
  used as input that is inserted through key presses by receiver. Escape sequences may be used
  to press special keys or key combinations.

  Sequences:
  - To simulate pressing left Crtl, left Alt and t -buttons simultaneously use sequence [128+130+116]
  - To add 1000ms delay use sequence [DEL1000]
  - To actually use [ or \ character as input escape it with backslash. \[
  - Key codes may be found from Arduino reference: https://www.arduino.cc/en/Reference/KeyboardModifiers or from any ascii table (e.g. http://www.asciitable.com/ )

  created   Nov 2016
  by CheapskateProjects
  ---------------------------
  Dependencies: https://github.com/maniacbug/RF24
  ---------------------------
  The MIT License (MIT)

  Copyright (c) 2016 CheapskateProjects

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// nRF24L01 radio: SPI bus and pins 9, 10 
RF24 radio(9,10);
// Radio addresses
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// How long to wait before checking next line
int BETWEEN_LINES_MILLIS = 30;
// How long to wait response after sending one line
int RESPONSE_TIMEOUT_MILLIS = 500;

// Global buffer and size
char lineBuffer[512];
int lineLength = 0;

void setup(void)
{
  // Wait for inits
  delay(5000);

  // DEBUG
  Serial.begin(57600);

  // Init radio
  radio.begin();
  radio.setRetries(15,15);
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();
}

bool info = true;
void loop(void)
{
  // Collect serial into buffer
  if (Serial.available() > 0)
  {
    lineBuffer[lineLength] = (char)Serial.read();
    lineLength++;
    info = true;
  }
  else
  {
    // Send buffer if we have something in it
    if(lineLength > 0)
    {
      writeLine(lineBuffer, lineLength);
      lineLength = 0;
      delay(BETWEEN_LINES_MILLIS);
    }
    else if(info)
    {
      // Instructions if not already printed
      Serial.print("Give line to send!\n");    
      info = false;
    }
  }
}

void writeLine(char *buffer, int size)
{
  // Open pipes
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  Serial.print("Sending line with ");
  Serial.print(size);
  Serial.print(" characters\n");

  // Write size of data and actual data
  radio.stopListening();
  radio.write( &size, sizeof(int) );
  for(int i = 0; i < size; i++)
  {
      radio.write( &buffer[i], sizeof(char));
  }
    
  Serial.print("Line sent. Waiting for confirmation...\n");

  // Wait for confirmation to become available
  radio.startListening();
  unsigned long start_time = millis();
  bool timeout = false;
  while ( !radio.available())
  {
    if (millis() - start_time > RESPONSE_TIMEOUT_MILLIS )
    {
      timeout = true;
      break;
    }
  }

  // Describe the results
  if ( timeout )
  {
      Serial.print("ERROR: Confirmation timed out!\n");
  }
  else
  {
      // Read confirmation
      int confirmation;
      radio.read( &confirmation, sizeof(int) );
      Serial.print("Got confirmation for line\n\n");
  }
}
