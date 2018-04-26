/************************************************************************************
 *  
 *  Name    : SST25VF.cpp                        
 *  Author  : Noah Shibley                         
 *  Date    : Aug 17th, 2013                                   
 *  Version : 0.1                                              
 *  Notes   : Based on SST code from: www.Beat707.com design. (Rugged Circuits and Wusik)      
 * 
 *  
 * 
 ***********************************************************************************/

#include "SST25VF.h"
 

SST25VF::SST25VF(){
 
}
 

void SST25VF::begin(int chipSelect,int writeProtect,int hold){
  
  //set pin #s
  FLASH_SSn = chipSelect;
  FLASH_Wp = writeProtect;
  FLASH_Hold = hold; 
  
  pinMode(FLASH_Wp, OUTPUT); 
  digitalWrite(FLASH_Wp, HIGH); //write protect off

  pinMode(FLASH_Hold, OUTPUT); 
  digitalWrite(FLASH_Hold, HIGH); //mem hold off

  pinMode(FLASH_SSn, OUTPUT); //chip select 
  digitalWrite(FLASH_SSn, HIGH);
  digitalWrite(FLASH_SSn, LOW);
  
  sstSPISettings = SPISettings(SST25VF_SPI_CLOCK, SST25VF_SPI_BIT_ORDER, SST25VF_SPI_MODE);
  SPI.begin();
  init(); 
  readID();
}
 
void SST25VF::setInterrupt(uint8_t pin) {
   SPI.usingInterrupt(pin);
}

void SST25VF::update(){
  

}

// ======================================================================================= //

void SST25VF::waitUntilDone()
{
  uint8_t data = 0;
  while (1)
  {
    
    digitalWrite(FLASH_SSn,LOW);
    (void) SPI.transfer(0x05);
    data = SPI.transfer(0);
    digitalWrite(FLASH_SSn,HIGH);
    if (!bitRead(data,0)) break;
    nop();
  }

}

// ======================================================================================= //

void SST25VF::init()
{
  SPI.beginTransaction(sstSPISettings);
  
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x50); //enable write status register instruction
  digitalWrite(FLASH_SSn,HIGH);
  delay(50);
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x01); //write the status register instruction
  SPI.transfer(0x00);//value to write to register - xx0000xx will remove all block protection
  digitalWrite(FLASH_SSn,HIGH);
  delay(50);
  SPI.endTransaction();  
}

// ======================================================================================= //

void SST25VF::readID()
{
  uint8_t id, mtype, dev;
  SPI.beginTransaction(sstSPISettings);
  digitalWrite(FLASH_SSn,LOW);
  (void) SPI.transfer(0x9F); // Read ID command
  id = SPI.transfer(0);
  mtype = SPI.transfer(0);
  dev = SPI.transfer(0);
  char buf[16] = {0};
  sprintf(buf, "%02X %02X %02X", id, mtype, dev);
  Serial.print("SPI ID ");
  Serial.println(buf);
  digitalWrite(FLASH_SSn,HIGH);
  SPI.endTransaction();
}

// ======================================================================================= //

void SST25VF::totalErase()
{
  SPI.beginTransaction(sstSPISettings);
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x06);//write enable instruction
  digitalWrite(FLASH_SSn,HIGH);
  nop();
  digitalWrite(FLASH_SSn, LOW); 
  (void) SPI.transfer(0x60); // Erase Chip //
  digitalWrite(FLASH_SSn, HIGH);
  waitUntilDone();
  SPI.endTransaction();
}

// ======================================================================================= //

void SST25VF::setAddress(uint32_t addr)
{
  (void) SPI.transfer(addr >> 16);
  (void) SPI.transfer(addr >> 8);  
  (void) SPI.transfer(addr);
}

// ======================================================================================= //

void SST25VF::readInit(uint32_t address)
{
  SPI.beginTransaction(sstSPISettings);
  digitalWrite(FLASH_SSn,LOW);
  (void) SPI.transfer(0x03); // Read Memory - 25/33 Mhz //
  setAddress(address);
}

// ======================================================================================= //

uint8_t SST25VF::readNext() { 
  return SPI.transfer(0); 
}

// ======================================================================================= //

void SST25VF::readFinish()
{
  digitalWrite(FLASH_SSn,HIGH);
  SPI.endTransaction();;
}

// ======================================================================================= //

void SST25VF::writeByte(uint32_t address, uint8_t data)
{
  SPI.beginTransaction(sstSPISettings);
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x06);//write enable instruction
  digitalWrite(FLASH_SSn,HIGH);
  nop();
  digitalWrite(FLASH_SSn,LOW);
  (void) SPI.transfer(0x02); // Write Byte //
  setAddress(address);
  (void) SPI.transfer(data);
  digitalWrite(FLASH_SSn,HIGH);
  waitUntilDone();
  SPI.endTransaction();;
}

void SST25VF::writeArray(uint32_t address,const uint8_t dataBuffer[],uint16_t dataLength)
{
  SPIDBG("writeArray");
  //get the sector block where we are writing
  uint16_t sectorBlock = floor(address/FLASH_SECTOR_BYTES);
  //and the adress
  uint32_t sectorAddress = FLASH_SECTOR_BYTES * sectorBlock;

  //read the sector data
  uint8_t sectorData[FLASH_SECTOR_BYTES] PROGMEM;
  readSector(address,sectorData);

#ifdef SPI_DEBUG
      SPIDBG("Sector Data");
      for(uint16_t i=0;i<FLASH_SECTOR_BYTES;i++) {
        if (sectorData[i] != 0XFF){
          SPIDBG("data: " + String(sectorData[i]) + " at: " + String(i));
        }
    }
#endif

  //now replace the sectorData bytes with the data...
  memcpy(sectorData+address, dataBuffer, dataLength);

#ifdef SPI_DEBUG
    SPIDBG("Writing Data");
    for(uint16_t i=0;i<FLASH_SECTOR_BYTES;i++) {
      if (sectorData[i] != 0XFF){
        SPIDBG("data: " + String(sectorData[i]) + " at: " + String(i));
      }
  }
#endif

  //erase sector
  sectorErase(sectorBlock);

  //write the bytes
  for(uint16_t i=0;i<FLASH_SECTOR_BYTES;i++) {
      writeByte((uint32_t)sectorAddress+i,sectorData[i]);
  }
}

void SST25VF::readArray(uint32_t address,uint8_t dataBuffer[],uint16_t dataLength)
{
  readInit((address));

    for (uint16_t i=0; i<dataLength; ++i)
    {
      uint8_t result = readNext();
      SPIDBG("reading byte:");
      SPIDBG(result);
      if (result == 0xFF) {
        break;
      }
      dataBuffer[i] = result;
    }

    readFinish();
}

void SST25VF::readSector(uint32_t address,uint8_t dataBuffer[])
{
  //get the sector 
  uint16_t sectorAddress = floor(address/FLASH_SECTOR_BYTES);
  readInit((sectorAddress));

    for (uint16_t i=0; i<FLASH_SECTOR_BYTES; ++i)
    {
      uint8_t result = readNext();
      dataBuffer[i] = result;
    }

    readFinish();
}

void SST25VF::writeString(uint32_t addr, char* string) {
  int numBytes = strlen(string)+1;
  writeArray(addr, (const byte*)string, numBytes);
}

void SST25VF::readString(uint32_t addr, char* string, int bufSize) {
  readArray(addr, (byte*)string, bufSize);
}

void SST25VF::writeInt(uint32_t addr, int value) {
  byte *ptr = (byte*)&value;
  writeArray(addr, ptr, sizeof(value));
}

void SST25VF::readInt(uint32_t addr, int* value) {
  readArray(addr, (byte*)value, sizeof(int));
}

// ======================================================================================= //

void SST25VF::sectorErase(uint8_t sectorAddress)
{
  SPI.beginTransaction(sstSPISettings);
  digitalWrite(FLASH_SSn,LOW);
  SPI.transfer(0x06);//write enable instruction
  digitalWrite(FLASH_SSn,HIGH);
  nop();
  digitalWrite(FLASH_SSn,LOW);
  (void) SPI.transfer(0x20); // Erase 4KB Sector //
  setAddress(4096UL*long(sectorAddress));
  digitalWrite(FLASH_SSn,HIGH);
  waitUntilDone();
  SPI.endTransaction();;
}