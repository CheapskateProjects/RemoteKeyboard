# RemoteKeyboard
  This code transfers serial input from transmitter through radio link (nRF24L01) to receiver. Receiver
  then uses that input to perform matching keyboard actions. 

  Receiver should use 32u4 based Arduino to have keyboard interface support.

  Unless escape sequences are used each line of serial input is
  used as input that is inserted through key presses by receiver. Escape sequences may be used
  to press special keys or key combinations.

  Sequences:
  - To simulate pressing left Crtl, left Alt and t -buttons simultaneously use sequence [128+130+116]
  - To add 1000ms delay use sequence [DEL1000]
  - To actually use [ or \ character as input escape it with backslash.
  - Key codes may be found from Arduino reference: <https://www.arduino.cc/en/Reference/KeyboardModifiers> or from any ascii table (e.g. <http://www.asciitable.com/> )

  Dependencies: <https://github.com/maniacbug/RF24>
  
  