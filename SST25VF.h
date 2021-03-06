/************************************************************************************
 * 	
 * 	Name    	: SST25VF.h                        
 * 	Author  	: Noah Shibley
 *	Contributors: Ubi de Feo [SPI Transactions implementation]                   
 * 	Date    	: Aug 17th, 2013                                    
 * 	Version 	: 0.1                                              
 * 	Notes   	: Nor Serial Flash 16Mbit (2Mbytes) - S71295 (SST) 2097152 bytes.
 *				  Based on SST code from: www.Beat707.com design. (Rugged Circuits and Wusik)
 *				  Should work for SST25VF004, SST25VF016, SST25VF064, etc.                 
 * 
 * 	
 * 
 ***********************************************************************************/

//include guard
#ifndef SST25VF_H
#define SST25VF_H

#include <SPI.h>

#if defined(ARDUINO)
  #if ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#define SPI_DEBUG Serial

#ifdef SPI_DEBUG
namespace {
  template<typename T>
  static void SPIDBG(T last) {
    SPI_DEBUG.println(last);
  }

  template<typename T, typename... Args>
  static void SPIDBG(T head, Args... tail) {
    SPI_DEBUG.print(head);
    SPI_DEBUG.print(' ');
    SPIDBG(tail...);
  }
}
#else
  #define SPIDBG(...)
#endif

#define FLASH_MAX_BYTES 2097152 //this chip contains this much storage
#define FLASH_MAX_SECTOR 512 //this chip has tha many sectors
#define FLASH_SECTOR_BYTES 4096 //each sector is 4096 bytes

#define SST25VF_SPI_MODE SPI_MODE0
#define SST25VF_SPI_CLOCK F_CPU
#define SST25VF_SPI_BIT_ORDER MSBFIRST

class SST25VF {
 
public: 

	SST25VF();
    void begin(int chipSelect,int writeProtect,int hold);
    void setInterrupt(uint8_t pin);
    void update();
	void readID();
	void totalErase();
	void sectorErase(uint8_t sectorAddress);
	
	void readInit(uint32_t address);
	uint8_t readNext();
	void readFinish();
	void readArray(uint32_t address,uint8_t dataBuffer[],uint16_t dataLength);
	void readString(uint32_t addr, char* string, int bufSize);
  	uint16_t readInt(uint32_t addr);
  	unsigned long readLong(uint32_t addr);
  	float readFloat(uint32_t addr);
	
	void writeByte(uint32_t address, uint8_t data);
	void writeArray(uint32_t address,const uint8_t dataBuffer[],uint16_t dataLength);
	void writeString(uint32_t addr, char* string);
	void writeInt(uint32_t addr, uint16_t value);
	void writeLong(uint32_t addr, unsigned long value);
	void writeFloat(uint32_t addr, float value);

private: 
	
	void init();
	char buf[16];
	uint8_t FLASH_SSn; //chip select pin
	uint8_t FLASH_Wp;  //Write protection pin
	uint8_t FLASH_Hold; //read/write hold pin (pause)

	void readSector(uint32_t address,uint8_t dataBuffer[]);
	void waitUntilDone();
	void setAddress(uint32_t addr);
	inline void volatile nop(void) { asm __volatile__ ("nop"); }
	SPISettings sstSPISettings;
}; 

#endif