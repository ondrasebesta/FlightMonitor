/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */
#include "SDcard.h"


void SDcard::listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void SDcard::createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void SDcard::removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void SDcard::readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void SDcard::writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}



void SDcard::appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

bool SDcard::existFile(fs::FS &fs, const char * path){
    Serial.printf("Exist file: %s?\n", path);

    File file = fs.open(path, FILE_READ);
    if(!file){
        Serial.println("File not exist");
        return false;
    }
    file.close();
    return true;
}


void SDcard::renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void SDcard::deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void SDcard::testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

void SDcard::setup(){
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    //listDir(SD, "/", 0);
    //createDir(SD, "/mydir");
    //listDir(SD, "/", 0);
    //removeDir(SD, "/mydir");
    //listDir(SD, "/", 2);
    //writeFile(SD, "/hello.txt", "Hello ");
    //appendFile(SD, "/hello.txt", "World!\n");
    //appendFile(SD, "/hello.txt", "123 456 789\n");
    //appendFile(SD, "/hello.txt", "123 456 789\n");
   // readFile(SD, "/hello.txt");
    //deleteFile(SD, "/foo.txt");
    //renameFile(SD, "/hello.txt", "/foo.txt");
    //readFile(SD, "/foo.txt");
    //testFileIO(SD, "/test.txt");
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void SDcard::writetemp(float Jtemp, float deltaT, uint32_t RPMactual, String ActualDate, String ActualTime, uint32_t flytime, float kmh){
  char messageTempAndDates[128];
  char filename[32];
  const char * firstlinemessage="Datum,Cas UTC,Teplota hlavy C,deltaT C,RPM,Doba letu s,Rychlost km/h\n";
  sprintf(filename, "/%s%s",ActualDate.c_str(),".csv" );
  sprintf(messageTempAndDates, "%s,%s,%4.2f,%4.2f,%d,%d,%4.2f\n", ActualDate.c_str(), ActualTime.c_str(), Jtemp, deltaT, RPMactual, flytime, kmh);
  //* char filename=ActualDate;
   const char * path="/hello789.txt";
   const char * message="OK";
    //Serial.printf("Writing file: %s\n", path);
   if (!existFile(SD, filename))
   {
    appendFile(SD, filename,firstlinemessage);
   }
   // File file = fs.open(filename, FILE_READ);//Zkouška čtení souboru
    /*if(!file){            
        file.close()                        ;//Pokud soubor neexistuje, vytvoří se*/
        appendFile(SD, filename,messageTempAndDates);
        //appendFile(SD, path,firstlinemessage);
   /* }
    appendFile(SD, filename, messageTempAndDates);*/
    //else
    /*if(file.read(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }*/
    //file.close();
  
    //sprintf(TempAndDates, "Jtemp = %03d NTCtemp = %03d %s\n", Jtemp, NTCtemp, ActualDate.c_str() );
    //writeTempAndDateToFile(SD);
   // appendFile(SD, "/hello.txt", TempAndDates);

}



void SDcard::writeerrorlog(float Jtemp, float deltaT, uint32_t RPMactual, String ActualDate, String ActualTime, uint32_t flytime, float kmh, String Error){
  char messageTempAndDates[128];
  //char filename[32];
  const char * firstlinemessage="Datum,Cas UTC,Teplota hlavy C,deltaT C,RPM,Doba letu s,Rychlost km/h, ERROR\n";
  //sprintf(filename, "/%s%s",ActualDate.c_str(),".txt" );
  sprintf(messageTempAndDates, "%s,%s,%4.2f,%4.2f,%d,%d,%4.2f,%s\n", ActualDate.c_str(), ActualTime.c_str(), Jtemp, deltaT, RPMactual, flytime, kmh, Error.c_str());
  const char * filename="/errorlog.csv";
  //const char * path="/errorlog.txt";
   if (!existFile(SD, filename))
   {
    appendFile(SD, filename,firstlinemessage);
   }
    appendFile(SD, filename,messageTempAndDates);
}

//void SDcard::writeTempAndDateToFile(fs::FS &fs, const char * path, const char * message){
uint16_t SDcard::errorlogcount(fs::FS &fs){
    const char * path="/errorlog.csv";
    Serial.printf("File buffer: %s\n", path);

    File file = fs.open(path, FILE_READ);
    if(!file){
        Serial.println("Failed to open file for reading");
        return 8888;
    }
    int errorcount=0;
    while (file.available())
    {
      String buffer = file.readStringUntil('\n');
      //Serial.println(buffer); 
      errorcount++;
    }
    Serial.print("Errorlog count: ");
    Serial.println(errorcount-1);
    /*if(file.read(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }*/
    file.close();
    if(errorcount>=1)
    {
      errorcount-=1;
    }
    return errorcount;
}


uint16_t SDcard::THminread(fs::FS &fs){
    const char * path="/THmin.txt";
    File file = fs.open(path, FILE_READ);
    if(!file){
        Serial.println("Failed to open file for reading");
        return 8888;
    }
      String buffer = file.readStringUntil('\n');
      Serial.println(buffer); 
      uint16_t THmin = buffer.toInt();   
    file.close();
    return THmin;
}

void SDcard::THminwrite(int THmin)
{
char THminSD[32];
  sprintf(THminSD,"%d\n", THmin);
    const char * path="/THmin.txt";
    //Serial.printf("File buffer: %s\n", path);
    deleteFile(SD, path);
    appendFile(SD, path,THminSD);
}
uint16_t SDcard::THdeltamaxread(fs::FS &fs){
    const char * path="/deltaTmax.txt";
    File file = fs.open(path, FILE_READ);
    if(!file){
        Serial.println("Failed to open file for reading");
        return 8888;
    }
      String buffer = file.readStringUntil('\n');
      Serial.println(buffer); 
      uint16_t deltaTmax = buffer.toInt();   
    file.close();
    return deltaTmax;
}

void SDcard::THdeltamaxwrite(int deltaTmax)
{
char deltaTmaxSD[32];
  sprintf(deltaTmaxSD,"%d\n", deltaTmax);
    const char * path="/deltaTmax.txt";
    //Serial.printf("File buffer: %s\n", path);
    deleteFile(SD, path);
    appendFile(SD, path,deltaTmaxSD);
}
uint16_t SDcard::THcoolingread(fs::FS &fs){
    const char * path="/THcooling.txt";
    File file = fs.open(path, FILE_READ);
    if(!file){
        Serial.println("Failed to open file for reading");
        return 8888;
    }
      String buffer = file.readStringUntil('\n');
      Serial.println(buffer); 
      uint16_t THcooling = buffer.toInt();   
    file.close();
    return THcooling;
}

void SDcard::THcoolingwrite(int THcooling)
{
char THcoolingSD[32];
  sprintf(THcoolingSD,"%d\n", THcooling);
    const char * path="/THcooling.txt";
    //Serial.printf("File buffer: %s\n", path);
    deleteFile(SD, path);
    appendFile(SD, path,THcoolingSD);
}
uint16_t SDcard::THmaxread(fs::FS &fs){
    const char * path="/THmax.txt";
    File file = fs.open(path, FILE_READ);
    if(!file){
        Serial.println("Failed to open file for reading");
        return 8888;
    }
      String buffer = file.readStringUntil('\n');
      Serial.println(buffer); 
      uint16_t THmax = buffer.toInt();   
    file.close();
    return THmax;
}

void SDcard::THmaxwrite(int THmax)
{
char THmaxSD[32];
  sprintf(THmaxSD,"%d\n", THmax);
    const char * path="/THmax.txt";
    //Serial.printf("File buffer: %s\n", path);
    deleteFile(SD, path);
    appendFile(SD, path,THmaxSD);
}
