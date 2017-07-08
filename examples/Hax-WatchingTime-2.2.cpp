/***********************************
 * 
 * Hax/WatchingTime Firmware v2.2
 * 
 * Modified by Kayson to meet GCCVerify spec 
 * (https://github.com/kaysond/GCCVerify)
 * 
 * Instructions:
 * Once you have installed the mod, upload the code and go to live analog inputs display
 * hold dpad down for 3 seconds to turn off the mod then record the notch values
 * doing the steps in this order ensures the most accurate calibration
 * 
 * To return to a stock controller hold dpad down for 3 seconds
 * 
 * To set to dolphin mode hold dpad right for 3 seconds before launching the game
 * 
 * To customize the mods, connect the Arduino to your computer and go to
 * the serial monitor in the Arduino IDE (Tools->Serial Monitor / Ctrl+Shift+M)
 * 
 * Input anything and press enter to activate the configuration menu
 * 
 */

#include "Nintendo.h"
#include <EEPROM.h>

#define FW_NAME "Hax-WatchingTime"
#define MAJOR_VERSION "2"
#define MINOR_VERSION "2"

//Notch value addresses in EEPROM
//Floats are 4 bytes long in arduino nano
//Should really be using a byte for these
//But its convenient because Dolphin reports fractions
#define n_x_addr 0
#define n_y_addr 4
#define e_x_addr 8
#define e_y_addr 16
#define s_x_addr 20
#define s_y_addr 24
#define w_x_addr 28
#define w_y_addr 32
#define sw_x_addr 36
#define sw_y_addr 40
#define se_x_addr 44
#define se_y_addr 48

//One byte that gets bit masked for mod disabling
#define enables_addr 52

#define mask_max_vectors 1
#define mask_perfect_angles 2
#define mask_shield_drop_expand 4
#define mask_dash_back 8
#define mask_dolphin_fix 16

#define MAIN_MENU 0
#define MOD_MENU 1
#define NOTCH_MENU 2
 
CGamecubeController controller(0); //sets RX0 on arduino to read data from controller
CGamecubeConsole console(1);       //sets TX1 on arduino to write data to console
Gamecube_Report_t gcc;             //structure for controller state
Gamecube_Data_t data;        
float r, deg, nang, eang, sang, wang, seang, swang, n__notch_x_value, n__notch_y_value, e__notch_x_value, e__notch_y_value,
s__notch_x_value, s__notch_y_value, w__notch_x_value, w__notch_y_value, se_notch_x_value, se_notch_y_value, sw_notch_x_value, sw_notch_y_value;
char ax, ay, cx, cy, buf, snp, oldx, nx, ny;
bool shield, tilt, dolphin = 0, off = 0;
byte axm, aym, cxm, cym, cycles = 3;
unsigned long n;
word mode;

String serialBuffer = "";
String GCCVerifyJSON;
bool recvdGCCVerify = false;
byte enables = 255;
bool max_vectors_enabled = false;
bool perfect_angles_enabled = false;
bool shield_drop_expand_enabled = false;
bool dash_back_enabled = false;
bool dolphin_fix_enabled = false;

char menu_byte;
byte menu_state = MAIN_MENU;
bool config = false;
 
void setup(){
  //Get the neutral analog stick values immediately
  gcc.origin=0; gcc.errlatch=0; gcc.high1=0; gcc.errstat=0;  //init values
  controller.read(); gcc = controller.getReport();
  nx = gcc.xAxis-128; ny = gcc.yAxis-128;

  //Load EEPROM values

  //Open serial port, wait up to 1 seconds from boot for "GCCVerify"
  Serial.begin(9600);
  while ( millis() < 1000 ) {
    while( Serial.available() > 0 )
      serialBuffer.concat((char) Serial.read());
     
    if ( serialBuffer.indexOf("GCCVerify") > -1 ) {
      recvdGCCVerify = true;
      break;
    }
  }

  //Get enables from EEPROM
  //This needs to happen before a verification attempt
  enables = EEPROM.read(enables_addr);
  if ( enables & mask_max_vectors )
    max_vectors_enabled = true;
  if ( enables & mask_perfect_angles )
    perfect_angles_enabled = true;
  if ( enables & mask_shield_drop_expand )
    shield_drop_expand_enabled = true;
  if ( enables & mask_dash_back )
    dash_back_enabled = true;
  if ( enables & mask_dolphin_fix )
    dolphin_fix_enabled = true;
  
  if( recvdGCCVerify ) {
    //Build the response string
    //This is a little slow and wastes some memory
    //A better method would be to put all constants in progmem and use Serial.print() on the biggest sections possible
    //Not as readable, though
    GCCVerifyJSON = F("{\"name\":\"");
    GCCVerifyJSON += String(FW_NAME) + "\",\"major_version\":" + String(MAJOR_VERSION) + ",\"minor_version\":" + String(MINOR_VERSION) + ",\"mods\":[";
      GCCVerifyJSON += "{\"name\":\"max_vectors\",\"enabled\":" + String(max_vectors_enabled ? "true" : "false") + ",\"values\":[";
        GCCVerifyJSON += F("{\"name\":\"analog_radius\",\"value\":75},");
        GCCVerifyJSON += F("{\"name\":\"analog_angle\",\"value\":6},");
        
        //These two values are not exactly correct because of the way the c stick snapping is calculated
        //It is the minimum radius and angle that gets snapped, but the mod is a box in the cartesian plane rather than polar based
        GCCVerifyJSON += F("{\"name\":\"c_radius\",\"value\":79},"); 
        GCCVerifyJSON += F("{\"name\":\"c_angle\",\"value\":17}");
        GCCVerifyJSON += F("]");
      GCCVerifyJSON += F("},");
      GCCVerifyJSON += "{\"name\":\"perfect_angles\",\"enabled\":" + String(perfect_angles_enabled ? "true" : "false") + ",\"values\":[";
        GCCVerifyJSON += F("{\"name\":\"radius\",\"value\":75},");
        GCCVerifyJSON += F("{\"name\":\"angle_min\",\"value\":6},");
        GCCVerifyJSON += F("{\"name\":\"angle_max\",\"value\":19}");
        GCCVerifyJSON += F("]");
      GCCVerifyJSON += F("},");
      GCCVerifyJSON += "{\"name\":\"shield_drop_expand\",\"enabled\":" + String(shield_drop_expand_enabled ? "true" : "false") + ",\"values\":[";
        GCCVerifyJSON += F("{\"name\":\"radius\",\"value\":72},");
        GCCVerifyJSON += F("{\"name\":\"angle\",\"value\":4}");
        GCCVerifyJSON += F("]");
      GCCVerifyJSON += F("},");
      //Send partially before string gets too large as there seemed to be a problem
      Serial.print(GCCVerifyJSON);
      
      GCCVerifyJSON = "{\"name\":\"dash_back\",\"enabled\":" + String(dash_back_enabled ? "true" : "false") + ",\"values\":[";
        GCCVerifyJSON += F("{\"name\":\"frames\",\"value\":1}");
        GCCVerifyJSON += F("]");
      GCCVerifyJSON += F("},");
      GCCVerifyJSON += "{\"name\":\"dolphin_fix\",\"enabled\":" + String(dolphin_fix_enabled ? "true" : "false") + ",\"values\":[";
        GCCVerifyJSON += F("{\"name\":\"analog_radius\",\"value\":8},");
        GCCVerifyJSON += F("{\"name\":\"c_radius\",\"value\":8},");
        GCCVerifyJSON += F("{\"name\":\"dash_back_frames\",\"value\":6}");
        GCCVerifyJSON += F("]");
      GCCVerifyJSON += F("}");
    GCCVerifyJSON += F("]}\r\n");

    Serial.print(GCCVerifyJSON);
    Serial.flush();
  }

  //Read notch values from EEPROM
  EEPROM.get(n_x_addr, n__notch_x_value);
  EEPROM.get(n_y_addr, n__notch_y_value);
  EEPROM.get(e_x_addr, e__notch_x_value);
  EEPROM.get(e_y_addr, e__notch_y_value);
  EEPROM.get(s_x_addr, s__notch_x_value);
  EEPROM.get(s_y_addr, s__notch_y_value);
  EEPROM.get(w_x_addr, w__notch_x_value);
  EEPROM.get(w_y_addr, w__notch_y_value);
  EEPROM.get(se_x_addr, se_notch_x_value);
  EEPROM.get(se_y_addr, se_notch_y_value);
  EEPROM.get(sw_x_addr, sw_notch_x_value);
  EEPROM.get(sw_y_addr, sw_notch_y_value);
  
  nang  = ang(n__notch_x_value, n__notch_y_value);
  eang  = ang(e__notch_x_value, e__notch_y_value);
  sang  = ang(s__notch_x_value, s__notch_y_value);
  wang  = ang(w__notch_x_value, w__notch_y_value);
  seang = ang(se_notch_x_value, se_notch_y_value);
  swang = ang(sw_notch_x_value, sw_notch_y_value);

  Serial.println("Enter anything to access config menu");
}
 
void loop() {
  //If any serial data is available enter config mode
  if ( Serial.available() > 0 )
  {
    printMainMenu();
    config = true;
  }
  while ( config ) {
    if ( Serial.available() > 0 ) {
      menu_byte = Serial.read();
      switch ( menu_state ) {
        case MAIN_MENU:
          switch ( menu_byte ) {
            case '0':
              printModMenu();
              menu_state = MOD_MENU;
              break;
            case '1':
              printNotchMenu();
              menu_state = NOTCH_MENU;
              break;
            default:
              break;
          }
          break;
        case MOD_MENU:
          switch ( menu_byte ) {
            case '0':
              max_vectors_enabled = !max_vectors_enabled;
              writeEnablesToEEPROM();
              printModMenu();
              break;
            case '1':
              perfect_angles_enabled = !perfect_angles_enabled;
              writeEnablesToEEPROM();
              printModMenu();
              break;
            case '2':
              shield_drop_expand_enabled = !shield_drop_expand_enabled;
              writeEnablesToEEPROM();
              printModMenu();
              break;
            case '3':
              dash_back_enabled = !dash_back_enabled;
              writeEnablesToEEPROM();
              printModMenu();
              break;
            case '4':
              dolphin_fix_enabled = !dolphin_fix_enabled;
              writeEnablesToEEPROM();
              printModMenu();
              break;
            case '5':
              menu_state = MAIN_MENU;
              printMainMenu();
              break;
            default:
              break;
          }
          break;
        case NOTCH_MENU:
          switch ( menu_byte ) {
            case '0':
              getNotchValue(n__notch_x_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '1':
              getNotchValue(n__notch_y_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '2':
              getNotchValue(s__notch_x_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '3':
              getNotchValue(s__notch_y_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '4':
              getNotchValue(w__notch_x_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '5':
              getNotchValue(w__notch_y_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '6':
              getNotchValue(e__notch_x_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '7':
              getNotchValue(e__notch_y_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '8':
              getNotchValue(sw_notch_x_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case '9':
              getNotchValue(sw_notch_y_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case 'a':
            case 'A':
              getNotchValue(se_notch_x_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case 'b':
            case 'B':
              getNotchValue(se_notch_y_value);
              writeNotchValuesToEEPROM();
              printNotchMenu();
              break;
            case 'c':
            case 'C':
              menu_state = MAIN_MENU;
              printMainMenu();
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
    } //if Serial.available
  } //while
  controller.read();
  data = defaultGamecubeData;
  gcc = controller.getReport();
  calibration();
  if(!off) convertinputs();                 //implements all the fixes (remove this line to unmod the controller)
  data.report = gcc;
  console.write(data);                      //sends controller data to the console
  controller.setRumble(data.status.rumble); //fixes rumble
}

 
void convertinputs(){
  if (max_vectors_enabled)
    maxvectors();    //snaps sufficiently high cardinal inputs to vectors of 1.0 magnitude of analog stick and c stick
  if (perfect_angles_enabled)
    perfectangles(); //reduces deadzone of cardinals and gives steepest/shallowest angles when on or near the gate
  if (shield_drop_expand_enabled)
    shielddrops();   //gives an 8 degree range of shield dropping centered on SW and SE gates
  if (dash_back_enabled)
    backdash();      //fixes dashback by imposing a 1 frame buffer upon tilt turn values
  if (dolphin_fix_enabled)
    dolphinfix();    //ensures close to 0 values are reported as 0 on the sticks to fix dolphin calibration and fixes poll speed issues
  nocode();        //function to disable all code if dpad down is held for 3 seconds (unplug controller to reactivate)
} //more mods to come!
 
void maxvectors(){
  if(r>75.0){
    if(arc(nang)<6.0){gcc.xAxis = 128; gcc.yAxis = 255;}
    if(arc(eang)<6.0){gcc.xAxis = 255; gcc.yAxis = 128;}
    if(arc(sang)<6.0){gcc.xAxis = 128; gcc.yAxis =   1;}
    if(arc(wang)<6.0){gcc.xAxis =   1; gcc.yAxis = 128;}
  }
  if(cxm>75&&cym<23){gcc.cxAxis = (cx>0)?255:1; gcc.cyAxis = 128;}
  if(cym>75&&cxm<23){gcc.cyAxis = (cy>0)?255:1; gcc.cxAxis = 128;}
}
 
void perfectangles(){
  if(r>75.0){
    if(mid(arc(nang),6,19)){gcc.xAxis = (ax>0)?151:105; gcc.yAxis = 204;}
    if(mid(arc(eang),6,19)){gcc.yAxis = (ay>0)?151:105; gcc.xAxis = 204;}
    if(mid(arc(sang),6,19)){gcc.xAxis = (ax>0)?151:105; gcc.yAxis =  52;}
    if(mid(arc(wang),6,19)){gcc.yAxis = (ay>0)?151:105; gcc.xAxis =  52;}
  }
}
 
void shielddrops(){
  shield = gcc.l||gcc.r||gcc.left>74||gcc.right>74||gcc.z;
  if(shield){
    if(ay<0&&r>72){
      if(arc(swang)<4.0){gcc.yAxis = 73; gcc.xAxis =  73;}
      if(arc(seang)<4.0){gcc.yAxis = 73; gcc.xAxis = 183;}
    }
  }
}
 
void backdash(){
  if(aym<23){
    if(axm<23)buf = cycles;
    if(buf>0){buf--; if(axm<64) gcc.xAxis = 128+ax*(axm<23);}
  }else buf = 0;
}
 
void dolphinfix(){
  if(r<8)         {gcc.xAxis  = 128; gcc.yAxis  = 128;}
  if(mag(cx,cy)<8){gcc.cxAxis = 128; gcc.cyAxis = 128;}
  if(gcc.dright&&mode<2000) dolphin = dolphin||(mode++>2000);
  else mode = 0;
  cycles = 3 + (14*dolphin);
}
 
void nocode(){
  if(gcc.ddown){
    if(n == 0) n = millis();
    off = off||(millis()-n>500);
  }else n = 0;
}
 
void calibration(){
  ax = gcc.xAxis-128-nx; ay = gcc.yAxis-128-ny; //offsets from nuetral position of analog stick
  cx = gcc.cxAxis - 128; cy = gcc.cyAxis - 128; //offsets from nuetral position of c stick
  axm = abs(ax); aym = abs(ay);                 //magnitude of analog stick offsets
  cxm = abs(cx); cym = abs(cy);                 //magnitude of c stick offsets
  r   = mag(ax, ay); deg = ang(ax, ay);         //obtains polar coordinates
  if(r>1.0){
    gcc.xAxis = 128+r*cos(deg/57.3);
    gcc.yAxis = 128+r*sin(deg/57.3);
  }else{
    gcc.xAxis = 128;
    gcc.yAxis = 128;
  }
}
 
float ang(float x, float y){return atan(y/x)*57.3+180*(x<0)+360*(y<0&&x>0);} //returns angle in degrees when given x and y components
float mag(char  x, char  y){return sqrt(sq(x)+sq(y));}                       //returns vector magnitude when given x and y components
bool mid(float val, float n1, float n2){return val>n1&&val<n2;}              //returns whether val is between n1 and n2
float arc(float val){return abs(180-abs(abs(deg-val)-180));}                  //returns length of arc between deg and val

void printMainMenu() {
  Serial.println("Config menu:");
  Serial.println("0: Enable/Disable Mods");
  Serial.println("1: Update notch values");
}

void printModMenu() {
  Serial.println("Toggle mods:");
  Serial.println("0: max_vectors : " + String((max_vectors_enabled)?"Enabled":"Disabled"));
  Serial.println("1: perfect_angles : " + String((perfect_angles_enabled)?"Enabled":"Disabled"));
  Serial.println("2: shield_drop_expand : " + String((shield_drop_expand_enabled)?"Enabled":"Disabled"));
  Serial.println("3: dash_back : " + String((dash_back_enabled)?"Enabled":"Disabled"));
  Serial.println("4: dolphin_fix : " + String((dolphin_fix_enabled)?"Enabled":"Disabled"));
  Serial.println("5: Exit");
}

void printNotchMenu() {
  Serial.println("Update notch values:");
  Serial.print("0: North X : "); Serial.println(n__notch_x_value);
  Serial.print("1: North Y : "); Serial.println(n__notch_y_value);
  Serial.print("2: South X : "); Serial.println(s__notch_x_value);
  Serial.print("3: South Y : "); Serial.println(s__notch_y_value);
  Serial.print("4: West X : "); Serial.println(w__notch_x_value);
  Serial.print("5: West Y : "); Serial.println(w__notch_y_value);
  Serial.print("6: East X : "); Serial.println(e__notch_x_value);
  Serial.print("7: East Y : "); Serial.println(e__notch_y_value);
  Serial.print("8: Southwest X : "); Serial.println(sw_notch_x_value);
  Serial.print("9: Southwest Y : "); Serial.println(sw_notch_y_value);
  Serial.print("A: Southeast X : "); Serial.println(se_notch_x_value);
  Serial.print("B: Southeast Y : "); Serial.println(se_notch_y_value);
  Serial.println("C: Exit");
}

void writeNotchValuesToEEPROM() {
  EEPROM.put(n_x_addr, n__notch_x_value);
  EEPROM.put(n_y_addr, n__notch_y_value);
  EEPROM.put(e_x_addr, e__notch_x_value);
  EEPROM.put(e_y_addr, e__notch_y_value);
  EEPROM.put(s_x_addr, s__notch_x_value);
  EEPROM.put(s_y_addr, s__notch_y_value);
  EEPROM.put(w_x_addr, w__notch_x_value);
  EEPROM.put(w_y_addr, w__notch_y_value);
  EEPROM.put(se_x_addr, se_notch_x_value);
  EEPROM.put(se_y_addr, se_notch_y_value);
  EEPROM.put(sw_x_addr, sw_notch_x_value);
  EEPROM.put(sw_y_addr, sw_notch_y_value);
}

void writeEnablesToEEPROM() {
  enables = 0;
  enables = (mask_max_vectors & max_vectors_enabled) | (mask_perfect_angles & perfect_angles_enabled)
            | (mask_shield_drop_expand & shield_drop_expand_enabled) | (mask_dash_back & dash_back_enabled)
            | (mask_dolphin_fix & dolphin_fix_enabled);
  EEPROM.put(enables_addr, enables);
}

void getNotchValue(float &notch_value) {
  Serial.println("Please enter the notch value.");
  while ( Serial.available() <= 0 ) {}; //Wait for the first serial bit before parsing
  Serial.setTimeout(250); //Speed up the timeout
  notch_value = Serial.parseFloat();
  Serial.setTimeout(1000);
}