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
#include <Keyboard.h>

// nRF24L01 radio: SPI bus and pins 9, 10 
RF24 radio(9,10);
// Radio addresses
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// How many times should we try to fetch next character before giving up?
// This should probably be at least one as sender may not be able to keep up with reading. 
int RETRIES=5;
// How long to wait between retries. This should be long enough time so that sender can send something new. 
int RETRY_DELAY=5;
// How long to keep pressing combi buttons
int buttonCombiDelay = 50;


void setup(void)
{
  // Wait for inits
  delay(5000);

  // Init radio
  radio.begin();
  radio.setRetries(15,15);
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();
}

void loop(void)
{
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);

    // if there is data ready
    if ( radio.available() )
    {
      // Read amount of data that is to be sent
      int dataSize;
      radio.read( &dataSize, sizeof(int) );

      // Read data
      char test[dataSize];
      for(int i = 0; i < dataSize; i++)
      {
        /*
         * It may be that next byte is not yet received. If so we should wait a bit. (max belt * delay)
         */
        int belt = RETRIES;
        test[i] = 0;
        while(test[i] == 0 && belt > 0)
        {
          radio.read( &test[i], sizeof(char));

          if(test[i] == 0)
          {
            delay(RETRY_DELAY);
            belt--;
          }
        }
      }

      // Wait for sender state transfer
      delay(20);
      radio.stopListening();

      // Send confirmation
      radio.write( &dataSize, sizeof(int) );

      // Resume listening for next round
      radio.startListening();

      // Output what we got
      Keyboard.begin();
      bool skipSpecial = false;
      for(int i = 0; i < dataSize; i++)
      {
        char nextChar = (char)test[i];
        if (skipSpecial)
        {
          Keyboard.print(nextChar);
          skipSpecial = false;
        }
        else if (nextChar == '\\')
        {
          skipSpecial = true;
        }
        else if (nextChar == '[')
        {
          i = handleSpecialSequence(test, i+1) - 1;
        }
        else
        {
          Keyboard.print(nextChar);
        }
      }
      Keyboard.print("\n");
    }
}

int handleSpecialSequence(char *test, int currentIndex)
{
  String seq = "";
  seq += (char)test[currentIndex++];
  seq += (char)test[currentIndex++];
  seq += (char)test[currentIndex++];

  // Handle delay sequence
  if (seq.equals("DEL"))
  {
    String collect = "";
    while (((char)test[currentIndex]) != ']')
    {
      collect += (char)test[currentIndex++];
    }
    currentIndex++;// Skip ]
    int del = collect.toInt();

    delay(del);
  }
  // Handle key press sequence
  else
  {
    int charCode = seq.toInt();
    char character = (char)charCode;
    Keyboard.press(character);

    char testChar = test[currentIndex++];
    if (testChar == '+')
    {
      return handleSpecialSequence(test, currentIndex);
    }
    else
    {
      delay(buttonCombiDelay);
      Keyboard.releaseAll();

      return currentIndex;
    }
  }
  return currentIndex;
}

