/******************************************************************************
 * M5Stack Pressure measurement software
 * A simple software to measure the environmental pressure under two 
 * different conditions with the M5Stack and the ENVIII Unit.
 * 
 * Hague Nusseck @ electricidea
 * v1.0 | 12.December.2021
 * https://github.com/electricidea/M5Stack_Pressure_measurement
 * 
 * 
 * Check the complete project at Hackster.io:
 * https://www.hackster.io/hague/altitude-sickness-due-to-cooker-hoods-721664
 * 
 * Distributed as-is; no warranty is given.
 ******************************************************************************/
#include <Arduino.h>

#include <M5Stack.h>
// install the library:
// pio lib install "M5Stack"

// Free Fonts for nice looking fonts on the screen
#include "Free_Fonts.h"

// logo with 150x150 pixel size in XBM format
// check the file header for more information
#include "electric-idea_logo.h"

#include "UNIT_ENV.h"
// ENVIII:
// SHT30:   temperature and humidity sensor  I2C: 0x44
// QMP6988: absolute air pressure sensor     I2C: 0x70
SHT3X sht30;
QMP6988 qmp6988;


// logfile and addressfile definition
File log_data;

unsigned long nextMillis = 0;
int state = 0;
int state_cnt = 0;
// Buffer for snprintf calls
char String_buffer[256];
// counter for screen captures
int screen_cnt = 0;

// forward declarations
void I2Cscan();
void clear_screen(){
    M5.Lcd.clear();   
    M5.Lcd.setCursor(0,0);
    M5.Lcd.println(""); 
}
void print_menu(int menu_index);
uint16_t RGB2Color(uint8_t r, uint8_t g, uint8_t b);
bool M5Screen2bmp(fs::FS &fs, const char * path);


void setup() {
  // start M5Stack without I2C
  M5.begin(true, true, true, false);
  M5.Power.begin();  

  // bool TwoWire::begin(int sda = -1, int scl = -1, uint32_t frequency = 0U)
  Wire.begin();

  M5.Lcd.setBrightness(100); //Brightness (0: Off - 255: Full)
  // electric-idea logo
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.drawXBitmap((int)(320-logoWidth)/2, (int)(240-logoHeight)/2, logo, logoWidth, logoHeight, TFT_WHITE);
  M5Screen2bmp(SD, "/cpt_a.bmp");
  delay(1500);

  M5.Lcd.setTextSize(1);
  // configure Top-Left oriented String output
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setFreeFont(FF2);
  M5.Lcd.setBrightness(100); //Brightness (0: Off - 255: Full)
  
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE , BLACK);
  M5.Lcd.println("\n\n  ENV III Measure\n");
  M5Screen2bmp(SD, "/cpt_b.bmp");
  delay(1500);
  clear_screen(); 

  M5.Lcd.setFreeFont(FF1);
  // scan for I2C devices
  I2Cscan();
  M5Screen2bmp(SD, "/cpt_c.bmp");
  delay(2500);
  clear_screen(); 

  
  char filename1[15];
  // creat new filename that does not exist at SD
  // Format:
  // log_000.txt = first file
  // log_001.txt = second file
  // ...
  // log_999.txt = last file!
  strcpy(filename1, "/log_000.txt");
  for (int i = 0; i < 1000; i++) {
    filename1[5] = '0' + i/100;
    filename1[6] = '0' + (i/10)%10;
    filename1[7] = '0' + i%10;
    // exit if filename does not exist
    // !do not open existing, write, sync after write!
    if (!SD.exists(filename1)) {
      break;
    }
  }
  // try to open the files for writing
  log_data = SD.open(filename1, FILE_WRITE);
  if(!log_data) {
    M5.Lcd.setTextColor(TFT_RED,TFT_BLACK);  
    M5.Lcd.println("\nCould not create\n");
    M5.Lcd.println(filename1);
    while(true){
      delay(100);
    }
  }
  M5.Lcd.println("\nWriting data to ");
  M5.Lcd.println(filename1);

  M5.Lcd.println("\n\nLogger Ready!");
  M5Screen2bmp(SD, "/cpt_d.bmp");

  // header for data output
  log_data.print("ms\tstate\tPressure\tTemperature\taltitute\n");
  // flush files to save data onto SD
  log_data.flush();


  
  if(qmp6988.init()==1){
    Serial.println("[OK] QMP6988 ready");
  }

  // high precision mode
  qmp6988.setFilter(QMP6988_FILTERCOEFF_32);
  qmp6988.setOversamplingP(QMP6988_OVERSAMPLING_32X);
  qmp6988.setOversamplingT(QMP6988_OVERSAMPLING_4X);
  
  print_menu(-1);
  M5.Lcd.setTextColor(TFT_WHITE);

  state = 0;
  state_cnt = 0;
  // start in 10 Seconds
  nextMillis = millis() + 10000;
}


void loop() {
  M5.update();

  if(M5.BtnA.wasPressed()){
    state = 0;
    state_cnt = 0;
  } 

  if(M5.BtnB.wasPressed()){
    char FileName[100];
    sprintf(FileName, "/cpt_%i.bmp", screen_cnt);
    M5Screen2bmp(SD, FileName);
    screen_cnt++;
  } 

  if(M5.BtnC.wasPressed()){
    state = 1;
    state_cnt = 0;
  } 
  
  // get actual time in miliseconds
  unsigned long currentMillis = millis();
  // next interval
  if(currentMillis > nextMillis) {
    print_menu(state);
    state_cnt += 1;
    if (sht30.get() != 0) {
      return;
    }
    float qmp_Pressure = qmp6988.calcPressure();
    float sht30_Temperature = sht30.cTemp;
    M5.Lcd.fillRect(0,0, M5.Lcd.width(), 185, TFT_BLACK);
    M5.Lcd.setFreeFont(FF3);
    M5.Lcd.setTextDatum(TC_DATUM);
    char TextBuffer[100];
    sprintf(TextBuffer, "%3.2fPa", qmp_Pressure);
    M5.Lcd.drawString(TextBuffer, (int)(M5.Lcd.width()/2), 20, 1);
    sprintf(TextBuffer, "%3.2f*C", sht30_Temperature);
    M5.Lcd.drawString(TextBuffer, (int)(M5.Lcd.width()/2), 50, 1);

    sprintf(TextBuffer, "State: %i (%i)", state, state_cnt);
    M5.Lcd.drawString(TextBuffer, (int)(M5.Lcd.width()/2), 90, 1);
    
    //Serial.printf("Temperatura: %2.2f*C  Humedad: %0.2f%%  Pressure: %0.2fPa\r\n", sht30.cTemp, sht30.humidity, qmp6988.calcPressure());
    Serial.printf("ENV III: %2.2f*C  %0.2fPa  %0.2fm\n\n", sht30_Temperature, qmp_Pressure, qmp6988.calcAltitude(qmp_Pressure, sht30_Temperature));
  
    snprintf(String_buffer, sizeof(String_buffer), "%lu\t%i\t%7.3f\t%7.3f\t%7.3f\n", 
              millis(), state,
              qmp_Pressure, sht30_Temperature, qmp6988.calcAltitude(qmp_Pressure, sht30_Temperature));
    log_data.print(String_buffer);
    // flush files to save data onto SD
    log_data.flush();

    M5.Lcd.setTextDatum(TL_DATUM);
    // every 250 milliseconds
    nextMillis = millis() + 250; 
  }
  delay(100);
}


//==============================================================
void I2Cscan(){
  // scan for i2c devices
  byte error, address;
  int nDevices;

  M5.Lcd.println("Scanning I2C bus...\n");
  Serial.println("Scanning I2C bus...\n");

  nDevices = 0;
  for(address = 1; address < 127; address++ ){
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    // Errors:
    //  0 : Success
    //  1 : Data too long
    //  2 : NACK on transmit of address
    //  3 : NACK on transmit of data
    //  4 : Other error
    if (error == 0){
      nDevices++;
      M5.Lcd.printf("[OK] %i 0x%.2X\n", nDevices, address);
      Serial.printf("[OK] %i 0x%.2X\n", nDevices, address);
    } else{
      if(error == 4)
        M5.Lcd.printf("[ERR] %i 0x%.2X\n", nDevices, address);
    }
  }
  M5.Lcd.printf("\n%i devices found\n\n", nDevices);
  Serial.printf("\n%i devices found\n\n", nDevices);
 }

//==============================================================
// convert a RGB color into a LCD color value
uint16_t RGB2Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

//==============================================================
// Print a small menu at the bottom of the display above the buttons
void print_menu(int menu_index){
    M5.Lcd.fillRect(0, M5.Lcd.height()-25, M5.Lcd.width(), 25, RGB2Color(50,50,50));
    M5.Lcd.setCursor(0, 230);    
    M5.Lcd.setFreeFont(FF1);
    M5.Lcd.setTextColor(TFT_WHITE);
    switch (menu_index) {
      case 0: { 
        M5.Lcd.print("      -       -       ON ");
        break;       
      }
      case 1: { 
        M5.Lcd.print("     OFF      -       - ");
        break;       
      }
      default: { // should never been called
        M5.Lcd.print("      -       -       - ");
        break;
      }
    }
}


/***************************************************************************************
* Function name:          M5Screen2bmp
* Description:            Dump the screen to a bmp image File
* Image file format:      .bmp
* return value:           true:  succesfully wrote screen to file
*                         false: unabel to open file for writing
* example for screen capture onto SD-Card: 
*                         M5Screen2bmp(SD, "/screen.bmp");
* inspired by: https://stackoverflow.com/a/58395323
***************************************************************************************/
bool M5Screen2bmp(fs::FS &fs, const char * path){
  // Open file for writing
  // The existing image file will be replaced
  File file = fs.open(path, FILE_WRITE);
  if(file){
    // M5Stack:      TFT_WIDTH = 240 / TFT_HEIGHT = 320
    // M5StickC:     TFT_WIDTH =  80 / TFT_HEIGHT = 160
    // M5StickCplus: TFT_WIDTH =  135 / TFT_HEIGHT = 240
    int image_height = M5.Lcd.height();
    int image_width = M5.Lcd.width();
    // horizontal line must be a multiple of 4 bytes long
    // add padding to fill lines with 0
    const uint pad=(4-(3*image_width)%4)%4;
    // header size is 54 bytes:
    //    File header = 14 bytes
    //    Info header = 40 bytes
    uint filesize=54+(3*image_width+pad)*image_height; 
    unsigned char header[54] = { 
      'B','M',  // BMP signature (Windows 3.1x, 95, NT, …)
      0,0,0,0,  // image file size in bytes
      0,0,0,0,  // reserved
      54,0,0,0, // start of pixel array
      40,0,0,0, // info header size
      0,0,0,0,  // image width
      0,0,0,0,  // image height
      1,0,      // number of color planes
      24,0,     // bits per pixel
      0,0,0,0,  // compression
      0,0,0,0,  // image size (can be 0 for uncompressed images)
      0,0,0,0,  // horizontal resolution (dpm)
      0,0,0,0,  // vertical resolution (dpm)
      0,0,0,0,  // colors in color table (0 = none)
      0,0,0,0 };// important color count (0 = all colors are important)
    // fill filesize, width and heigth in the header array
    for(uint i=0; i<4; i++) {
        header[ 2+i] = (char)((filesize>>(8*i))&255);
        header[18+i] = (char)((image_width   >>(8*i))&255);
        header[22+i] = (char)((image_height  >>(8*i))&255);
    }
    // write the header to the file
    file.write(header, 54);
    
    // To keep the required memory low, the image is captured line by line
    unsigned char line_data[image_width*3+pad];
    // initialize padded pixel with 0 
    for(int i=(image_width-1)*3; i<(image_width*3+pad); i++){
      line_data[i]=0;
    }
    // The coordinate origin of a BMP image is at the bottom left.
    // Therefore, the image must be read from bottom to top.
    for(int y=image_height; y>0; y--){
      // get one line of the screen content
      M5.Lcd.readRectRGB(0, y-1, image_width, 1, line_data);
      // BMP color order is: Blue, Green, Red
      // return values from readRectRGB is: Red, Green, Blue
      // therefore: R und B need to be swapped
      for(int x=0; x<image_width; x++){
        unsigned char r_buff = line_data[x*3];
        line_data[x*3] = line_data[x*3+2];
        line_data[x*3+2] = r_buff;
      }
      // write the line to the file
      file.write(line_data, (image_width*3)+pad);
    }
    file.close();
    return true;
  }
  return false;
}
