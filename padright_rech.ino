#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

//-----
#define RST_PIN 9 // Configurable, see typical pin layout above
#define SS_PIN 10 // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode rfidstatus;

LiquidCrystal_I2C lcd(0x27, 16, 2); // instance for 16*2 i2c lcd

// --------regarding rfid card
int blockAddr = 2;
byte readByte[18];
byte writeByte[16];
int authAddr = 3;
byte byteSize = sizeof(readByte);

//---
byte padQuantity = 1;
int i = 0;                  /// used for rotation detection
int pinAstateCurrent = LOW; // Current state of Pin A
int pinAStateLast = pinAstateCurrent;
int pinA = 17; // Rotary encoder Pin A
int pinB = 16;
int switchPin = 2;
byte buzzer = 8;
int batchState = 0; // 0 for nothing // 1 for single write // 2 for batch write
uint32_t previousTime = 0;
//------------
byte arrow[8] = {
  0b00000,
  0b11100,
  0b10010,
  0b01001,
  0b01001,
  0b10010,
  0b11100,
  0b00000
};
void ICACHE_RAM_ATTR update()
{
  pinAstateCurrent = digitalRead(pinA); // Read the current state of Pin A
  // If there is a minimal movement of 1 step
  if ((pinAStateLast == LOW) && (pinAstateCurrent == HIGH))
  {
    if (digitalRead(pinB) == HIGH)
    { // If Pin B is HIGH
      i = 1; // Print on screen
    }
    else
    {
      i = 2; // Print on screen
    }
  }
  pinAStateLast = pinAstateCurrent; // Store the latest read value in the currect state variable
}
void setup()
{
  Serial.begin(9600);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.createChar(0, arrow);
  SPI.begin();                      // Init SPI bus
  mfrc522.PCD_Init();               // Init MFRC522 card
  pinMode(switchPin, INPUT_PULLUP); // Enable the switchPin as input with a PULLUP resistor
  pinMode(pinA, INPUT);             // Set PinA as input
  pinMode(pinB, INPUT);
  pinMode(buzzer, OUTPUT);

  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  attachInterrupt(pinB, update, CHANGE);
  startMessage();
}
void startMessage()
{
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Welcome");
  lcd.setCursor(1, 1);
  lcd.print("Pad Recharger");
  delay(3000);
  homepage();
}
void loop()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Card Detected!");
    lcd.setCursor(0, 1);
    lcd.print("Please Wait.....");
    success(500);
    if (mfrc522.PICC_ReadCardSerial())
    {
      if (readCard())
      {
        if (readByte[0] == 107)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Quantity left");
          lcd.setCursor(0, 1);
          lcd.print(readByte[15]);
        }
        else
        {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("New Card");
          lcd.setCursor(3, 1);
          lcd.print("Detected!!");
        }
      }
    }
    delay(2000);
    homepage();
    halt();
  }
  previousTime = millis();
  while (!digitalRead(switchPin))
  {
    if (millis() - previousTime >= 5000) {
      batchState = 2;
      success(500);
      delay(500);
//      Serial.println(batchState);
    }
    else if (millis() - previousTime >= 1000) {
      //previousTime = millis();
      batchState = 1;
//      Serial.println(batchState);
    }
  }

  switch (batchState) {
    case 1:
      delay(400);
      Notify("Normal Mode");
      delay(2000);
      menuMessage(padQuantity);

      while (digitalRead(switchPin))
      {
        if (i != 0)
          menuManagement();
      }
      while (digitalRead(switchPin))
        ;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Show your card");
      delay(2000);
      saveData();
      delay(2000);
      homepage();
      batchState = 0;
      break;
    case 2:
      delay(400);
      Notify("Batch Mode: On"); /// message on display
      delay(2000);
      menuMessage(padQuantity); /// select quantity
      while (digitalRead(switchPin))
      {
        if (i != 0)
          menuManagement();
      }
      while (!digitalRead(switchPin));

      while (digitalRead(switchPin) == HIGH)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Show your card");
        delay(2000);
        saveData();
        delay(2000);
      }
      homepage();
      batchState = 0;
      break;
    default:
      break;

  }
}
void homepage()
{
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Pad Vending");
  lcd.setCursor(1, 1);
  lcd.print("RFID Recharger");
}
void menuManagement()
{
  if (readRotate() == 1)
  {
    if (padQuantity < 100)
      padQuantity++;
    menuMessage(padQuantity);
  }
  else if (readRotate() == 2)
  {
    if (padQuantity != 0)
      padQuantity--;
    menuMessage(padQuantity);
  }
  i = 0;
}
void menuMessage(int number)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Quantity:");
  lcd.setCursor(0, 1);
  lcd.print(number);
}
void Notify(String msg)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  Serial.println("inside");
}

bool readCard()
{
  byte buffersize = 18;
  if (auth_A())
  {
    rfidstatus = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, readByte, &buffersize);
    if (rfidstatus != MFRC522::STATUS_OK)
    {
      return false;
    }
  }
  return true;
}
bool writeCard()
{
  if (auth_B())
  {
    rfidstatus = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, writeByte, 16);
    if (rfidstatus != MFRC522::STATUS_OK)
    {
      return false;
    }
  }
  return true;
}

void dumpToWriteVar(byte * buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i]);
    writeByte[i] = buffer[i];
  }
}
void halt()
{
  mfrc522.PICC_HaltA();      // Halt PICC
  mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
}
bool auth_A()
{
  rfidstatus = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, authAddr, &key, &(mfrc522.uid));
  if (rfidstatus != MFRC522::STATUS_OK)
  {
    return false;
  }
  return true;
}
bool auth_B()
{
  rfidstatus = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, authAddr, &key, &(mfrc522.uid));
  if (rfidstatus != MFRC522::STATUS_OK)
  {
    return false;
  }
  return true;
}

int readRotate()
{
  if (i != 0 && i == 1)

    return 1;

  else if (i != 0 && i == 2)
    return 2;
}
void saveData()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Card Detected!");
    lcd.setCursor(0, 1);
    lcd.print("Please Wait.....");
    success(500);
    delay(500);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Writing...");
    lcd.setCursor(0, 1);
    lcd.print("Please Wait...");
    if (mfrc522.PICC_ReadCardSerial())
    {
      if (readCard())
      {
        dumpToWriteVar(readByte, 16);

        if (padQuantity == 0 && readByte[0] != 107)
        {
          writeByte[15] = padQuantity;
          writeByte[0] = 'k';
          writeCard();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Success....");
          lcd.setCursor(0, 1);
          lcd.print("Thank you!!");
          success(800);
        }
        else if (padQuantity != 0 && readByte[0] == 107)
        {
          writeByte[15] = padQuantity;
          writeCard();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Success....");
          lcd.setCursor(0, 1);
          lcd.print("Thank you!!");
          success(800);
        }
        else
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Card not match");
        }
      }
    }
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No card");
    lcd.setCursor(0, 1);
    lcd.print("Detected!");
    error();
  }
  halt();
}

void success(int _time)
{
  digitalWrite(buzzer, HIGH);
  delay(_time);
  digitalWrite(buzzer, LOW);
}
void error()
{
  digitalWrite(buzzer, HIGH);
  delay(400);
  digitalWrite(buzzer, LOW);
  delay(400);
  digitalWrite(buzzer, HIGH);
  delay(400);
  digitalWrite(buzzer, LOW);
}
