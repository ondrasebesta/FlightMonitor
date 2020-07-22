#ifndef SDcard_h
#define SDcard_h

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "FFat.h"



class SDcard
{
public:
	void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
	void createDir(fs::FS &fs, const char * path);
	void removeDir(fs::FS &fs, const char * path);
	void readFile(fs::FS &fs, const char * path);
  bool existFile(fs::FS &fs, const char * path);
	void writeFile(fs::FS &fs, const char * path, const char * message);
  uint16_t errorlogcount(fs::FS &fs);
  uint16_t THminread(fs::FS &fs);
  void THminwrite(int THmin);
  uint16_t THdeltamaxread(fs::FS &fs);
  void THdeltamaxwrite(int deltaTmax);
  uint16_t THcoolingread(fs::FS &fs);
  void THcoolingwrite(int THcooling);
  uint16_t THmaxread(fs::FS &fs);
  void THmaxwrite(int THmax);
	void appendFile(fs::FS &fs, const char * path, const char * message);
	void renameFile(fs::FS &fs, const char * path1, const char * path2);
	void deleteFile(fs::FS &fs, const char * path);
	void testFileIO(fs::FS &fs, const char * path);
	void setup();
  void writetemp(float Jtemp, float deltaT, uint32_t RPMactual, String ActualDate, String ActualTime, uint32_t flytime, float kmh);
  void writeerrorlog(float Jtemp, float deltaT, uint32_t RPMactual, String ActualDate, String ActualTime, uint32_t flytime, float kmh, String Error);

private:

};

#endif
