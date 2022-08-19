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

//---

LiquidCrystal_I2C lcd(0x27, 16, 2);

// --------regarding rfid card
int blockAddr = 2;
byte readByte[18];
byte writeByte[16];
int authAddr = 3;
byte byteSize = sizeof(readByte);

//---
byte arrow[8] = {
    0b00000,
    0b11100,
    0b10010,
    0b01001,
    0b01001,
    0b10010,
    0b11100,
    0b00000};


void setup(){
Serial.begin(9600);
    lcd.init(); // initialize the lcd
    lcd.backlight();
    lcd.createChar(0, arrow);
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

      for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }
    startMessage();

}
void startMessage()
{
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Powered By");
    lcd.setCursor(2, 1);
    lcd.print("Kaicho Group");
    delay(3000);
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("RED");
    lcd.setCursor(5, 1);
    lcd.print("INT'L");
    delay(5000);
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Pad Vending");    
    lcd.setCursor(2, 1);
    lcd.print("RFID Recharger");    
    delay(2000);
}
void loop(){

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

void dumpToWriteVar(byte *buffer, byte bufferSize)
{
    for (byte i = 0; i < bufferSize; i++)
    {
        Serial.print(buffer[i]);
        writeByte[i] = buffer[i];
    }
    writeByte[15]--;
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