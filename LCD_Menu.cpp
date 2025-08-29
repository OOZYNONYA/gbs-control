#include <ESP8266WiFi.h>
#include "options.h"
#include "tv5725.h"
#include "slot.h" //only here because of SLOTS_TOTAL, which we can make instead
#include "src/WebSockets.h"
#include "src/WebSocketsServer.h"

#include "LCD_Menu.h"
#include <hd44780.h>                        //16x2 LCD I2C header
#include <hd44780ioClass/hd44780_I2Cexp.h>  //16x2 LCD I2C header



//////////////////////////////////////////////////
typedef TV5725<GBS_ADDR> GBS;
extern void applyPresets(uint8_t videoMode);
extern void setOutModeHdBypass(bool bypass);
extern void saveUserPrefs();
extern void savePresetToSPIFFS();
extern float getOutputFrameRate();
extern void loadDefaultUserOptions();
extern uint8_t getVideoMode();
extern runTimeOptions *rto;
extern userOptions *uopt;
extern WebSocketsServer webSocket;
///////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
hd44780_I2Cexp lcd;       //LCD this is the class member we are using through the program to print to lcd
//LCD geometry
const int LCD_COLS = 16;  //LCD
const int LCD_ROWS = 2;   //LCD
int	status = lcd.begin(LCD_COLS, LCD_ROWS); //int status;  //LCD
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
SlotMetaArray LCDslotsObject; // contains all of our saved preset names
/////////////////////////////////////////////////////////////////////////



//LCD menu navigation strings//////////////////////////////////////////////////////////
String LCD_menu[4] =        {"RESOLUTION", "PRESET", "OPTION", "SIGNAL"};
String LCD_Resolutions[7] = {"1280x960", "1280x1024", "1280x720", "1920x1080", "480/576", "DOWNSCALE", "PASS-THROUGH"}; 
String LCD_Presets[8] =     {"1", "2", "3", "4", "5", "6", "7", "BACK"};
String LCD_Option[8] =      {"FILTER","GLOBAL SETTINGS","LCD SETTINGS","CREATE SLOT","REMOVE SLOT","RESET GBS", "FACTORY RESET", "BACK"}; 
//////////////////////////////////////////////////////////////////////////////////////


////////////ENCODER AND MENU RELATED VARIABLES///////////////////////////////////////////
//default values
int LCD_menuItem = 1;               //default is 1 (displays main menu)
int LCD_subsetFrame = 0;            //0 default
int LCD_selectOption = 0;           //0 default
int LCD_page = 0;                   //0 default
//default values
int LCD_lastCount = 0;
volatile int LCD_encoder_pos = 0;   //0 default
volatile int LCD_main_pointer = 0;  //0 default // volatile vars change done with ISR
volatile int LCD_pointer_count = 0; //0 default
volatile int LCD_sub_pointer = 0;   //0 default
////////////////////////////////////////////////////////////////////////////////////////////


////////////////MENU SAVE STATE VARIABLES////////////////////////////////
int     SAVESTATE_menuItem          =     0;
int     SAVESTATE_subsetFrame       =     0;
int     SAVESTATE_page              =     0;
int     SAVESTATE_main_pointer      =     0;
int     SAVESTATE_selectOption      =     0;
int     SAVESTATE_pointer_count     =     0; 
int     SAVESTATE_sub_pointer       =     0;

LCDMenuState SAVESTATE_currentMenu;
/////////////////////////////////////////////////////////////////////////



//enum variable that handles being in main LCD menu or OPTIONS menu to clean variables up/////
LCDMenuState currentMenu = MAIN_MENU;
//////////////////////////////////////////////////////////////////////////////////////////////


//LCD screen saver variables//////////////////////////////////////////////////////////
bool screenSaverActive = false; //lcd screensaver
//unsigned long lastActivityTime = 0;
unsigned long lastActivityTime = millis();
int lastClkState = HIGH;
bool HALT_LCD_MENU = false;
//////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int LCD_pin_clk = 14;            //D5 = GPIO14 (input of one direction for encoder)
const int LCD_pin_data = 13;           //D7 = GPIO13	(input of one direction for encoder)
const int LCD_pin_switch = 0;          //D3 = GPIO0 pulled HIGH, else boot fail (middle push button for encoder)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




//OPTIONS SUBMENU SYSTEM STRINGS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
String LCD_SETTINGS[6]               = {     "LCD_ON/OFF","SCREENSAVER","LCD_REFRESH_RATE","STARTUP_SUBMENU","STARTUP_CURSOR", "BACK "                 }; //this back is different on purpose to differentiate from suboption "BACK"
String LCD_SubOptions_ONOFF[3]       = {     "ON","OFF","BACK"                                                                                         };
String LCD_SubOptions_SCREENSAVER[8] = {     "NONE","SPLASHSCREEN","BACKLIGHTOFF","SIGNAL","TIMEOUT:30SECS","TIMEOUT:1MIN","TIMEOUT:5MIN","BACK"       };
String LCD_SubOptions_REFRESHRATE[7] = {     "100","200","300","400","500","1000","BACK"                                                               };
String LCD_SubOptions_SUBMENU[6]     = {     "MAIN","RESOLUTIONS","PRESETS","SETTINGS","SIGNAL","BACK"                                                 };
String LCD_SubOptions_CURSOR[6]      = {     "MAIN","RESOLUTIONS","PRESETS","SETTINGS","SIGNAL","BACK"                                                 };
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//ENCODER SELECTABLE OPTIONS//////////////////////////// 
//These are edited and saved via the EC11 encoder in Options menu, also through LCD_settings.txt with loading function.
int LCD_BACKLIGHT_ALWAYS_OFF = 0;  
int USE_SCREENSAVER = 0;
int USE_SPLASH_SCREENSAVER = 0;
int USE_TURNOFF_LCD_SCREENSAVER = 0; 
int LCD_REFRESHRATE = 200;            
int LOAD_DIRECTLY_INTO_SUB_MENU = 0;  
int CURSOR_ON_MENU_AT_STARTUP = 0;
int SCREEN_TIMEOUT = 60000; 
int USE_SIGNAL_SCREENSAVER = 0;
//ENCODER SELECTABLE OPTIONS/////////////////////////////






//////////////LCD start of LCD MENU FUNCTION////////////////////////////////////////////
//LCD Functionality
void LCD_USER_MAIN_MENU() 
{//main menu
    uint8_t videoMode = getVideoMode();
    byte button_nav = digitalRead(LCD_pin_switch);
    if (button_nav == LOW) {
        delay(350);         //TODO
        LCD_subsetFrame++; //this button counter for navigating menu
        LCD_selectOption++;
    }
    
///////////////////////////////////////////////////////////
//main menu //LCD
///////////////////////////////////////////////////////////
    if (LCD_page == 0)
     {
       
        delay(LCD_REFRESHRATE);
        lcd_pointerfunction();
    }
    //cursor location inside  main menu
    if (LCD_main_pointer == 0 && LCD_subsetFrame == 0) {
        LCD_pointer_count = 0;
        LCD_menuItem = 1;

        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("> " + LCD_menu[0]); //-> resolutions 
        
        lcd.setCursor(0,1); 
        lcd.print(LCD_menu[1]);        //presets         
       
    }
    if (LCD_main_pointer == 15 && LCD_subsetFrame == 0) {
        LCD_pointer_count = 0;
        LCD_menuItem = 2;

        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print(LCD_menu[0]);        //resolutions
        
        lcd.setCursor(0,1); 
        lcd.print("> " + LCD_menu[1]); //->presets 

    }
    if (LCD_main_pointer == 30 && LCD_subsetFrame == 0) {
        LCD_pointer_count = 0;
        LCD_sub_pointer = 0;
        LCD_menuItem = 3;

        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("> " + LCD_menu[2]); //->option
        
        lcd.setCursor(0,1); 
        lcd.print(LCD_menu[3]);        //current settings 

    }
    if (LCD_main_pointer == 45 && LCD_subsetFrame == 0) {
        LCD_pointer_count = 0;
        LCD_menuItem = 4;

        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print(LCD_menu[2]);        //option 
        
        lcd.setCursor(0,1); 
        lcd.print("> " + LCD_menu[3]); //-> current settings 
    }

 
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //resolution pages //LCD  
    ///////////////////////////////////////////////////////////
    if (LCD_menuItem == 1 && LCD_subsetFrame == 1)
     {
        delay(LCD_REFRESHRATE);
        LCD_page = 1;
        LCD_main_pointer = 0;
        
    
    if (  (LCD_pointer_count >= 0 && LCD_pointer_count <= 7)  ) //7 being LCD__reso array item "back"
    {

        if(LCD_pointer_count < 7) //7 being the LCD_Resolutions array number for going "back"
        {
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("RESOLUTION: ");  
        lcd.setCursor(0,1); 
        lcd.print(String(LCD_Resolutions[LCD_pointer_count]));  
        }
        else if(LCD_pointer_count >= 7)  //7 being LCD__reso array item "back"
        {
        LCD_pointer_count = 7; //7 being LCD_reso array item "back"
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("RESOLUTION: ");  
        lcd.setCursor(0,1); 
        lcd.print(String("BACK")); 
        }
     } 
     else if (LCD_pointer_count < 0)  { LCD_pointer_count = 0; } //do not want it going below 0 as it causes issues
     else if (LCD_pointer_count > 7)  { LCD_pointer_count = 7; } //7 being lcd_resolutions array item "back"

    }  
 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////
    //resolutions selections //LCD
    ///////////////////////////////////////////////////////////
    //1280x960
    if (LCD_menuItem == 1) {
        if (LCD_pointer_count == 0 && LCD_selectOption == 2) {
            LCD_subsetFrame = 3;
                 
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("1280x960"); 
        lcd.setCursor(0,1); 
        lcd.print("LOADED!"); 
       
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output960P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            delay(1000); //catchup
            if(RESET_TO_MAIN_MENU == 1) {LCD_resetToXMenu(RETURN_TO_MAIN);}    //reset back into main starting menu
            else{ LCD_selectOption = 1; LCD_subsetFrame = 1;} //reset back into resolutions
                
        }
        //1280x1024
        if (LCD_pointer_count == 1 && LCD_selectOption == 2) {
            LCD_subsetFrame = 3;
                    
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("1280x1024"); 
        lcd.setCursor(0,1); 
        lcd.print("LOADED!");  
        
           // }
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output1024P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            delay(1000); //catchup
            if(RESET_TO_MAIN_MENU == 1) {LCD_resetToXMenu(RETURN_TO_MAIN);}
            else{ LCD_selectOption = 1; LCD_subsetFrame = 1;}
        }
        //1280x720
        if (LCD_pointer_count == 2 && LCD_selectOption == 2) {
            LCD_subsetFrame = 3;
           
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("1280x720");  
        lcd.setCursor(0,1); 
        lcd.print("LOADED!");  
        
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output720P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            delay(1000); //catchup
            if(RESET_TO_MAIN_MENU == 1) {LCD_resetToXMenu(RETURN_TO_MAIN);}
            else{ LCD_selectOption = 1; LCD_subsetFrame = 1;}
        }
        //1920x1080
        if (LCD_pointer_count == 3 && LCD_selectOption == 2) {
            LCD_subsetFrame = 3;
          
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("1920x1080"); 
        lcd.setCursor(0,1); 
        lcd.print("LOADED!"); 
        
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output1080P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            delay(1000); //catchup
            if(RESET_TO_MAIN_MENU == 1) {LCD_resetToXMenu(RETURN_TO_MAIN);}
            else{ LCD_selectOption = 1; LCD_subsetFrame = 1;}
        }
        //720x480
        if (LCD_pointer_count == 4 && LCD_selectOption == 2) {
            LCD_subsetFrame = 3;
           
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("480/576"); 
        lcd.setCursor(0,1); 
        lcd.print("LOADED!"); 
      
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output480P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            delay(1000); //catchup
            if(RESET_TO_MAIN_MENU == 1) {LCD_resetToXMenu(RETURN_TO_MAIN);}
            else{ LCD_selectOption = 1; LCD_subsetFrame = 1;}
        }
        //downscale
        if (LCD_pointer_count == 5 && LCD_selectOption == 2) {
            LCD_subsetFrame = 3;
          
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("DOWNSCALE");  
        lcd.setCursor(0,1); 
        lcd.print("LOADED!");  
        
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = OutputDownscale;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            delay(1000); //catchup
            if(RESET_TO_MAIN_MENU == 1) {LCD_resetToXMenu(RETURN_TO_MAIN);}
            else{ LCD_selectOption = 1; LCD_subsetFrame = 1;}
        }
        //passthrough/bypass
        if (LCD_pointer_count == 6 && LCD_selectOption == 2) {
            LCD_subsetFrame = 3;
           
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("PASSTHROUGH");  
        lcd.setCursor(0,1); 
        lcd.print("LOADED!");  
       
            setOutModeHdBypass(false);
            uopt->presetPreference = OutputBypass;
            if (uopt->presetPreference == 10 && rto->videoStandardInput != 15) {
                rto->autoBestHtotalEnabled = 0;
                if (rto->applyPresetDoneStage == 11) {
                    rto->applyPresetDoneStage = 1;
                } else {
                    rto->applyPresetDoneStage = 10;
                }
            } else {
                rto->applyPresetDoneStage = 1;
            }
            saveUserPrefs();
            delay(1000); //catchup
            if(RESET_TO_MAIN_MENU == 1) {LCD_resetToXMenu(RETURN_TO_MAIN);}
            else{ LCD_selectOption = 1; LCD_subsetFrame = 1;}
        }
        //go back
        if (LCD_pointer_count == 7 && LCD_selectOption == 2) {
            LCD_page = 0;
            LCD_subsetFrame = 0;
            LCD_main_pointer = 0;
            LCD_sub_pointer = 0;
            LCD_selectOption = 0;

            LCD_menuItem = 1;
            LCD_pointer_count = 0;
            delay(50); //catchup
        }
    }
//////////////////////////////////////////////////////////////////////////////////////////////
//Presets pages //preset slot listings //LCD
//////////////////////////////////////////////////////////////////////////////////////////////
    if (LCD_menuItem == 2 && LCD_subsetFrame == 1)
        {
        LCD_page = 1;
        LCD_main_pointer = 0;
        printSlotNameToLCD(LCD_pointer_count);
        }  
/////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////
//preset selection //LCD
//////////////////////////////////////////////////////////////////////////////////////////////
if (LCD_menuItem == 2 && LCD_selectOption == 2)
{ 
 LCD_PresetLoadSelection(LCD_pointer_count,  printSlotNameToLCD(LCD_pointer_count) ); //all functionality inside this function
}
///////////////////////////////////////////////////////////////////////////////////////////


 //////////////////////////////////////////////////////////////////////////////////////////////////////////
//Option pages //LCD 
//////////////////////////////////////////////////////////////////////////////////////////////
    if (LCD_menuItem == 3 && LCD_subsetFrame == 1)
     { 
        delay(LCD_REFRESHRATE); 
        LCD_page = 1;
        LCD_main_pointer = 0;
       // lcd_pointerfunction(); 
        lcd_OPTIONS_pointerfunction();
        
       //might be able to remove this now
        if (LCD_pointer_count >= 7) 
          { LCD_pointer_count = 7; } 

        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("OPTION: ");  
        lcd.setCursor(0,1); 
        lcd.print(LCD_Option[LCD_pointer_count]);  
     }
    
   
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Option selection //LCD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//FILTER SETTINGS MENU//////////////////////////////////////////
    if (LCD_menuItem == 3) {
        
       if (LCD_menuItem == 3 && LCD_pointer_count == 0 && LCD_selectOption == 2)
        {//ENTER SETTINGS/OPTIONS SUBMENU
         currentMenu = FILTER_MENU;
         return; // exit main menu early since we're switching menu state
        }
    

//GLOBAL SETTINGS MENU//////////////////////////////////////////
   
        
       if (LCD_menuItem == 3 && LCD_pointer_count == 1 && LCD_selectOption == 2)
        {//ENTER SETTINGS/OPTIONS SUBMENU
         currentMenu = GLOBAL_MENU;
         return; // exit main menu early since we're switching menu state
        }
    

//LCD SETTINGS MENU//////////////////////////////////////////
  
        
       if (LCD_menuItem == 3 && LCD_pointer_count == 2 && LCD_selectOption == 2)
        {//ENTER SETTINGS/OPTIONS SUBMENU
         LCD_pointer_count = 0; //needed because the menuing system is different in LCDSET
         currentMenu = LCDSET_MENU;
         return; // exit main menu early since we're switching menu state
        }
    


//CREATE SLOT MENU//////////////////////////////////////////
   

       if (LCD_menuItem == 3 && LCD_pointer_count == 3 && LCD_selectOption == 2)
        {//ENTER SETTINGS/OPTIONS SUBMENU
         currentMenu = CREATESLOT_MENU;
         return; // exit main menu early since we're switching menu state
        }
    

//REMOVE SLOT MENU//////////////////////////////////////////
    
        
       if (LCD_menuItem == 3 && LCD_pointer_count == 4 && LCD_selectOption == 2)
        {//ENTER SETTINGS/OPTIONS SUBMENU
         currentMenu = REMOVESLOT_MENU;
         return; // exit main menu early since we're switching menu state
        }
    

//RESET GBS//////////////////////////////////////////
        if (LCD_pointer_count == 5 && LCD_selectOption == 2) {
        LCD_subsetFrame = 3;
            
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("RESETTING GBS");  
        lcd.setCursor(0,1); 
        lcd.print("PLEASE WAIT...");  
        delay(1000);

    
            webSocket.close();
            delay(60);
            ESP.reset();
            LCD_selectOption = 0;
            LCD_subsetFrame = 0;

        }
//FACTORY RESTORE SETTINGS////////////////////////////////
        if (LCD_pointer_count == 6 && LCD_selectOption == 2)
        { 
            LCD_subsetFrame = 3;

        //ABORT CODE//////////////////////////////////////////////////////////////////
            LCD_selectOption = 0;
        for (int time = 30; time != 0; time--)
        {
            
        delay(1000);//delay 1 second per loop tick
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("FACTORY RESET");  
        lcd.setCursor(0,1); 
        lcd.print("IN: ");  
        lcd.print(String(time) + " SECONDS" ); 
       
        
          if(digitalRead(LCD_pin_switch) == LOW) //if push-button: abort factory reset and return to main menu
           {
              LCD_selectOption == 0;
              lcd.clear();
              lcd.setCursor(0,0); 
              lcd.print("FACTORY RESET:");  
              lcd.setCursor(0,1);
              lcd.print("ABORTED!"); 
              delay(2500); //give user a chance to see they aborted

            LCD_page = 0;
            LCD_subsetFrame = 0;
            LCD_main_pointer = 30;
            LCD_sub_pointer = 0;
            LCD_selectOption = 0;

            LCD_menuItem = 3;
            LCD_pointer_count = 0;
            return;
           }
        } ////////////////////////////////////////////////////////////////////
        //DIDNT ABORT
        lcd.clear();
        lcd.setCursor(0,0); 
        lcd.print("DEFAULTS LOADING");  
        lcd.setCursor(0,1); 
        lcd.print("PLEASE WAIT..."); 
        delay(1000);
        
        //TO DO: reset all LCD_SETTINGS saved options here.
            resetLCDSettingsToDefault(); //reset all new LCD_settings saved by user
            webSocket.close();
            loadDefaultUserOptions();
            saveUserPrefs();
            delay(60);
            ESP.reset(); 
            LCD_selectOption = 1;
            LCD_subsetFrame = 1;
        }
/////////////////////////////////////////////////////////////////////////
//BACK BUTTON FUNCTIONALITY
        if (LCD_pointer_count >= 7  && LCD_selectOption == 2)  //change pointer count logic if increasing amount of Option items; >=4 etc
         { 
            LCD_page = 0;
            LCD_subsetFrame = 0;
            LCD_main_pointer = 30;
            LCD_sub_pointer = 0;
            LCD_selectOption = 0;

            LCD_menuItem = 3;
            LCD_pointer_count = 0;
        }
    }
    
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//SIGNAL  pages //LCD
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (LCD_menuItem == 4 && LCD_subsetFrame == 1) {
    boolean vsyncActive = 0;
    boolean hsyncActive = 0;
    float ofr = getOutputFrameRate();  // Output frame rate
    uint8_t currentInput = GBS::ADC_INPUT_SEL::read();  // 1 = RGB, else YPbPr
    //rto->presetID = GBS::GBS_PRESET_ID::read(); //moved to the if/else directly below
    String slotName =  getSlotName();

    // we check for powered signal coming to the GBS here, if powered signal not found we set some values to let the user know
    if(GBS::STATUS_SYNC_PROC_HSACT::read()) {rto->presetID = GBS::GBS_PRESET_ID::read();} // if sync detected
    else { rto->presetID = 0;       slotName = "NO SIGNAL";        ofr = 0; }//presetiD = 0 makes it say bypass        ofr = 0 resets HZ's values


    // --- Line 1: Resolution ---
    String resText;
    switch (rto->presetID) {
    case 0x01: case 0x11: resText = "1280x960"; break;
    case 0x02: case 0x12: resText = "1280x1024"; break;
    case 0x03: case 0x13: resText = "1280x720"; break;
    case 0x05: case 0x15: resText = "1920x1080"; break;
    case 0x06: case 0x16: resText = "DOWNSCALE"; break;
    case 0x04: resText = "720x480"; break;
    case 0x14: resText = "768x576"; break;
    default:   resText = "BYPASS"; break;
}

                 
LCD_scrollResolutionAndSlot(resText, slotName);

    // --- Line 2: Input Type + Hz + Sync ---
    lcd.setCursor(0, 1);

    if (currentInput == 1) {
        lcd.print("RGB ");
    } else {
        lcd.print("YPbpR ");
    }

    // Limit Hz to 2 decimal places
    lcd.print(String(ofr, 2));
    lcd.print("Hz");

    // Add sync indicators (e.g. H V)
    if (currentInput == 1) {
        vsyncActive = GBS::STATUS_SYNC_PROC_VSACT::read();
        if (vsyncActive) {
            lcd.print(" V");
            hsyncActive = GBS::STATUS_SYNC_PROC_HSACT::read();
            if (hsyncActive) {
                lcd.print("H");
            }
        }
    }
   
    delay(300); //400 IS STABLE: this delay prevents system crashing when loading an active signal from bypass. //if it crashes on signal increase delay
}
    
//////////////////////////////////////////////////////////////////////////////////////////////
//SIGNAL Selection //LCD
//////////////////////////////////////////////////////////////////////////////////////////////

            //exit SIGNAL DISPLAY if button pressed
            if (LCD_selectOption == 2)
            {
                {
                if (LCD_pointer_count >= 3) {LCD_pointer_count = 3;}
                if (LCD_pointer_count <= 0) {LCD_pointer_count = 0;}
              
                 delay(200);//debounce??
                 LCD_resetToXMenu(RETURN_TO_MAIN);
                 return;
                }
             
            }
        
}//end of LCD_USER_MAIN_MENU(); 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////



//LCD FUNCTIONS
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
void lcd_OPTIONS_pointerfunction() //LCD 
{
    if (LCD_main_pointer <= 0)
     {
        LCD_main_pointer = 0;
     }
    if (LCD_main_pointer >= 105) //limits
    { 
        LCD_main_pointer = 105;
    }

    if (LCD_pointer_count <= 0)
     {
        LCD_pointer_count = 0;
     }
     else if (LCD_pointer_count >= 7)
     {
        LCD_pointer_count = 7;
     }
}



void lcd_pointerfunction() //LCD 
{
    if (LCD_main_pointer <= 0) {
        LCD_main_pointer = 0;
    }
    if (LCD_main_pointer >= 45) { //limits
        LCD_main_pointer = 45;
    }

    if (LCD_pointer_count <= 0) {
        LCD_pointer_count = 0;
    } else if (LCD_pointer_count >= 3) {
        LCD_pointer_count = 3;
    }
}

void lcd_subpointerfunction()
{
    if (LCD_sub_pointer < 0)
     {
        LCD_sub_pointer = 0;
        LCD_subsetFrame = 1;
        LCD_page = 1;
     }
    if (LCD_sub_pointer > 45)
     { //limits to switch between the two pages
        LCD_sub_pointer = 0;    //TODO
        LCD_subsetFrame = 2;
        LCD_page = 2;
     }
   
    if (LCD_pointer_count < 0) 
      { LCD_pointer_count = 0; }
    else if (LCD_pointer_count >= SLOTS_TOTAL) //originally '7' //slots total is now the max amount of preset slots indicated in slot.h header file which is around 72
     {
        LCD_pointer_count = SLOTS_TOTAL;
     }
}




String printSlotNameToLCD(int index) //index = current lcd_pointer_count
{
               delay(LCD_REFRESHRATE);

               // if index (value of LCD_pointer_count) out of bounds
                if (index < 0 || index >= SLOTS_TOTAL)
                { 
                    if (index <= 0) {index = 0;}
                    if (index >= SLOTS_TOTAL) {index = SLOTS_TOTAL;}
                   // return "NULL"; 
                } 

///////////////////////////////////////////////////////////////////////////////////////////      
             //   String slotName = LCDslotsObject.slot[index].name; //store saveds preset name of current lcd_pointer_count index into a string
///////////////////////////////////////////////////////////////////////////////////////////
                 

                //Back button functionality!
                //convoluted way to make sure current slot never goes past the first "Empty" slot (and print BACK in place of the first empty slot)
                int current_index = index;                                                      //current lcd_pointer_count sent to function
                String valueOfEmptySlotID = "Empty                   " ;                        //value stored in bin for empty slots
              if (String(LCDslotsObject.slot[current_index].name) == valueOfEmptySlotID)        //if current slot we're on is an empty slot
            {
                int slotBeforeEmpty;
                while (String(LCDslotsObject.slot[current_index].name) == valueOfEmptySlotID  )
                {

                delay(50);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("PRESET SLOT: ");
                lcd.setCursor(0,1);
                lcd.print("BACK");
               

                --current_index; //deincrement until we find an actual preset with an ID which should be the slot right before the empty slot
                 slotBeforeEmpty = current_index;

                    if (  valueOfEmptySlotID != String(LCDslotsObject.slot[current_index].name) ) //convoluted way to make sure current slot never goes past the first "Empty" slot
                    {
                        LCD_pointer_count = (slotBeforeEmpty + 1); //this should put lcd_counter_pointer right on the first empty slot
                         return "BACK"; //pointer is now on first empty slot, so we return "BACK" to pretend it doesnt exist.
                    }

                 
                   
                } 
            }
            
                else 
                {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("PRESET SLOT: ");
                lcd.setCursor(0,1);

                if (index <= 0) {current_index = 0;}    // if index (value of LCD_pointer_count) out of bounds
                lcd.print(   String(LCDslotsObject.slot[current_index].name)   );
                return String(LCDslotsObject.slot[current_index].name);
                }
               
               

}

void LCD_PresetLoadSelection(int index, String slotName)
{
    delay(LCD_REFRESHRATE); 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// how presetIDS are stored sequentially   0-25 is A-Z capital letters     26-51 is a-z lowercase letters.     52-61 is numeral 0-9     62-71 is special characters -._~()!*:,; */
const char slotCharset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcedefghijklmnopqrstuvwxyz0123456789-._~()!*:,;" ;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


     //back button functionality
    String valueOfEmptySlotID = "Empty                   " ;     //slot name value stored in gbs of empty slots [19 spaces]
    if ( ( slotName == "BACK" && LCD_selectOption == 2 ) || (slotName == "NULL" && LCD_selectOption == 2 )) //if currently on empty slot and pressed button returns to main menu
    {
        
            LCD_page = 0;
            LCD_subsetFrame = 0;
            LCD_main_pointer = 15; //15 cursor back on presets when arriving back into main menu
            LCD_sub_pointer = 0;
            LCD_selectOption = 0;

            LCD_menuItem = 2;      //2 cursor back on presets when arriving back into main menu
            LCD_pointer_count = 0;
            return;
            
    }
    //selected preset loading functionality
    else if ( LCD_selectOption == 2)
    {
        
            //LCD_subsetFrame = 3; 
            uopt->presetSlot =  slotCharset[index];  // we feed it the letter its looking for which correlates to the PRESET slot identifier a..b..c etc
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();
            
          
                  lcd.clear();
                  lcd.setCursor(0,0); 
                  lcd.print(slotName);  
                  lcd.setCursor(0,1); 
                  lcd.print("LOADED!");  
                  delay(300);
          
           
            uopt->presetPreference = OutputCustomized;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
            delay(50);             //allowing "catchup"   
            LCD_selectOption = 1; //reset select container 
           // LCD_subsetFrame = 1;  //switch back to prev frame (prev screen) //lcd uncommented to test
           

        if (RESET_TO_MAIN_MENU == 1)//go back to main menu after loading a preset, otherwise wait for back button press.
        {
       /////////////////////////////////////
            LCD_page = 0;
            LCD_subsetFrame = 0;
            LCD_main_pointer = 15; //15 cursor back on presets when arriving back into main menu
            LCD_sub_pointer = 0;
            LCD_selectOption = 0;

            LCD_menuItem = 2;      //2 cursor back on presets when arriving back into main menu
            LCD_pointer_count = 0;
       /////////////////////////////////////
       }
    }

}



void LCD_printSplash(int logo_select) //1 = Animated GBS[C] logo          //logo code generated with https://chareditor.com/
{
     
if (logo_select == 0) //animated GBS[C] logo
{

//frame 1
{
lcd.clear();

byte name0x4[] =  { B11111, B10000, B10111, B10111, B10111, B10111, B10000, B11111 };
byte name0x5[] =  { B11111, B00001, B11111, B11111, B00001, B11101, B00001, B11111 };
byte name0x6[] =  { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x7[] =  { B00000, B11110, B00100, B01000, B01100, B00010, B11110, B00000 };
byte name0x8[] =  { B00000, B01111, B01000, B01111, B00000, B00000, B01111, B00000 };
byte name0x9[] =  { B00000, B11110, B00000, B11110, B00010, B00010, B11110, B00000 };
byte name0x10[] = { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x11[] = { B00000, B11100, B00000, B00000, B00000, B00000, B11100, B00000 };


  
  lcd.createChar(0, name0x4);
  lcd.setCursor(4, 0);
  lcd.write(0);
  
  lcd.createChar(1, name0x5);
  lcd.setCursor(5, 0);
  lcd.write(1);
  
  lcd.createChar(2, name0x6);
  lcd.setCursor(6, 0);
  lcd.write(2);
  
  lcd.createChar(3, name0x7);
  lcd.setCursor(7, 0);
  lcd.write(3);
  
  lcd.createChar(4, name0x8);
  lcd.setCursor(8, 0);
  lcd.write(4);
  
  lcd.createChar(5, name0x9);
  lcd.setCursor(9, 0);
  lcd.write(5);
  
  lcd.createChar(6, name0x10);
  lcd.setCursor(10, 0);
  lcd.write(6);
  
  lcd.createChar(7, name0x11);
  lcd.setCursor(11, 0);
  lcd.write(7);

delay(200);
}
//frame 2
lcd.clear();

{
byte name0x9[] = { B00000, B11110, B00000, B11110, B00010, B00010, B11110, B00000 };
byte name0x4[] = { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x5[] = { B00000, B11110, B00000, B00000, B11110, B00010, B11110, B00000 };
byte name0x6[] = { B11111, B10000, B10111, B10111, B10111, B10111, B10000, B11111 };
byte name0x7[] = { B11111, B00001, B11011, B10111, B10011, B11101, B00001, B11111 };
byte name0x8[] = { B00000, B01111, B01000, B01111, B00000, B00000, B01111, B00000 };
byte name0x10[] = { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x11[] = { B00000, B11100, B00000, B00000, B00000, B00000, B11100, B00000 };


  
  lcd.createChar(0, name0x9);
  lcd.setCursor(9, 0);
  lcd.write(0);
  
  lcd.createChar(1, name0x4);
  lcd.setCursor(4, 0);
  lcd.write(1);
  
  lcd.createChar(2, name0x5);
  lcd.setCursor(5, 0);
  lcd.write(2);
  
  lcd.createChar(3, name0x6);
  lcd.setCursor(6, 0);
  lcd.write(3);
  
  lcd.createChar(4, name0x7);
  lcd.setCursor(7, 0);
  lcd.write(4);
  
  lcd.createChar(5, name0x8);
  lcd.setCursor(8, 0);
  lcd.write(5);
  
  lcd.createChar(6, name0x10);
  lcd.setCursor(10, 0);
  lcd.write(6);
  
  lcd.createChar(7, name0x11);
  lcd.setCursor(11, 0);
  lcd.write(7);

delay(200);
}
//frame 3
{
lcd.clear();
byte name0x9[] =  { B11111, B00001, B11111, B00001, B11101, B11101, B00001, B11111 };
byte name0x4[] =  { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x5[] =  { B00000, B11110, B00000, B00000, B11110, B00010, B11110, B00000 };
byte name0x6[] =  { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x7[] =  { B00000, B11110, B00100, B01000, B01100, B00010, B11110, B00000 };
byte name0x8[] =  { B11111, B10000, B10111, B10000, B11111, B11111, B10000, B11111 };
byte name0x10[] = { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x11[] = { B00000, B11100, B00000, B00000, B00000, B00000, B11100, B00000 };


  
  lcd.createChar(0, name0x9);
  lcd.setCursor(9, 0);
  lcd.write(0);
  
  lcd.createChar(1, name0x4);
  lcd.setCursor(4, 0);
  lcd.write(1);
  
  lcd.createChar(2, name0x5);
  lcd.setCursor(5, 0);
  lcd.write(2);
  
  lcd.createChar(3, name0x6);
  lcd.setCursor(6, 0);
  lcd.write(3);
  
  lcd.createChar(4, name0x7);
  lcd.setCursor(7, 0);
  lcd.write(4);
  
  lcd.createChar(5, name0x8);
  lcd.setCursor(8, 0);
  lcd.write(5);
  
  lcd.createChar(6, name0x10);
  lcd.setCursor(10, 0);
  lcd.write(6);
  
  lcd.createChar(7, name0x11);
  lcd.setCursor(11, 0);
  lcd.write(7);

delay(200);
}
//frame 4
{
lcd.clear();
byte name0x11[] = { B11111, B00011, B11111, B11111, B11111, B11111, B00011, B11111 };
byte name0x4[] = { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x5[] = { B00000, B11110, B00000, B00000, B11110, B00010, B11110, B00000 };
byte name0x6[] = { B00000, B01111, B01000, B01000, B01000, B01000, B01111, B00000 };
byte name0x7[] = { B00000, B11110, B00100, B01000, B01100, B00010, B11110, B00000 };
byte name0x8[] = { B00000, B01111, B01000, B01111, B00000, B00000, B01111, B00000 };
byte name0x9[] = { B00000, B11110, B00000, B11110, B00010, B00010, B11110, B00000 };
byte name0x10[] = { B11111, B10000, B10111, B10111, B10111, B10111, B10000, B11111 };


  lcd.createChar(0, name0x11);
  lcd.setCursor(11, 0);
  lcd.write(0);
  
  lcd.createChar(1, name0x4);
  lcd.setCursor(4, 0);
  lcd.write(1);
  
  lcd.createChar(2, name0x5);
  lcd.setCursor(5, 0);
  lcd.write(2);
  
  lcd.createChar(3, name0x6);
  lcd.setCursor(6, 0);
  lcd.write(3);
  
  lcd.createChar(4, name0x7);
  lcd.setCursor(7, 0);
  lcd.write(4);
  
  lcd.createChar(5, name0x8);
  lcd.setCursor(8, 0);
  lcd.write(5);
  
  lcd.createChar(6, name0x9);
  lcd.setCursor(9, 0);
  lcd.write(6);
  
  lcd.createChar(7, name0x10);
  lcd.setCursor(10, 0);
  lcd.write(7);
}

}

else if (logo_select == 1){
  //insert new splashscreen here
}
else if (logo_select == 2){
}
else if (logo_select == 3){
}


///////////////////////////////////////////////
}//end of splash function


void LCD_LOAD_MENU_OPTIONS()
{//used at void setup() to setup LCD options and MENU selection preferences.

if(LCD_BACKLIGHT_ALWAYS_OFF == 1) {lcd.noBacklight(); }  //not sure this still belongs here...


if (LOAD_DIRECTLY_INTO_SUB_MENU != 0)
{
    switch(LOAD_DIRECTLY_INTO_SUB_MENU)
    {
    case 0: LCD_menuItem = 1; LCD_subsetFrame = 0; LCD_page = 0; LCD_main_pointer = 0; LCD_selectOption = 0; LCD_pointer_count = 0;  break; //load into main menu     //stay on defaults
    case 1: LCD_menuItem = 1; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0;  break; //load into resolutions
    case 2: LCD_menuItem = 2; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0;  break; //load into presets
    case 3: LCD_menuItem = 3; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0;  break; //load into Option
    case 4: LCD_menuItem = 4; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0;  break; //load into signal display
    default: break;
    }
    return;
}
else if (CURSOR_ON_MENU_AT_STARTUP != 0)
{
    switch(CURSOR_ON_MENU_AT_STARTUP)
    {
    case 0: LCD_menuItem = 1; LCD_main_pointer = 0;  LCD_subsetFrame = 0;  LCD_pointer_count = 0; break;                      //stay on defaults                
    case 1: LCD_menuItem = 1; LCD_main_pointer = 0;  LCD_subsetFrame = 0;  LCD_pointer_count = 0; break;                      //selector cursor on resolutions
    case 2: LCD_menuItem = 2; LCD_main_pointer = 15; LCD_subsetFrame = 0;  LCD_pointer_count = 0; break;                      //selector cursor on presets
    case 3: LCD_menuItem = 3; LCD_main_pointer = 30; LCD_subsetFrame = 0;  LCD_pointer_count = 0; LCD_sub_pointer = 0; break; //selector cursor on Option
    case 4: LCD_menuItem = 4; LCD_main_pointer = 45; LCD_subsetFrame = 0;  LCD_pointer_count = 0; break;                      //selector cursor on signal display
    default: break;
    }
    return;
    
}

       
   
}

void LCD_Load_PresetIDs_Into_slotobject()
{
    // initialize and load saved slot names into LCDslotsobject
    // we do it here ONCE, looping it in main LCD loop crashes the system //place somewhere where it will not be looped
                LCDslotsObject;
                File slotsBinaryFileRead = SPIFFS.open(SLOTS_FILE, "r");
                slotsBinaryFileRead.read((byte *)&LCDslotsObject, sizeof(LCDslotsObject));
                slotsBinaryFileRead.close();
             
}


void enterScreenSaver() //LCD SCREENSAVER
 {
   //possible to use both screensavers at once! (this is intended)
    if (USE_SCREENSAVER != 0)
    {
     screenSaverActive = true;


      if(USE_TURNOFF_LCD_SCREENSAVER == 1) 
      {
                                       
       if (USE_SIGNAL_SCREENSAVER != 1) {HALT_LCD_MENU = true;} //dont halt if SIGNAL screensaver is on
       //lcd.noDisplay();   //uncomment this to turn off all text as well //wrap it in if statement and make global variable to handle it by user choice
       lcd.noBacklight(); // turn off just backlight (text still appears)
      }

     if (USE_SPLASH_SCREENSAVER == 1)
      {  
       if (USE_TURNOFF_LCD_SCREENSAVER == 1) {lcd.noBacklight();}
       //halt main menu loop so its not constantly running when user is not using menu system
       HALT_LCD_MENU = true; //halt main loop and only display splash screen
       lcd.clear();
       LCD_printSplash(LCD_SPLASH_SCREEN_TYPE); //show the startup logo as screensaver splash screen
      }

      if (USE_SIGNAL_SCREENSAVER == 1)
      {
        lcd.clear(); 

        //save where user is currently at 
        SAVESTATE_menuItem      =     LCD_menuItem ;
        SAVESTATE_subsetFrame   =     LCD_subsetFrame;
        SAVESTATE_page          =     LCD_page;
        SAVESTATE_main_pointer  =     LCD_main_pointer;
        SAVESTATE_selectOption  =     LCD_selectOption;
        SAVESTATE_pointer_count =     LCD_pointer_count;
        SAVESTATE_sub_pointer   =     LCD_sub_pointer;
        
        SAVESTATE_currentMenu = currentMenu;  


        //change location of user to the SIGNAL section while in screensaver
        if (currentMenu != MAIN_MENU) {currentMenu = MAIN_MENU;}
        LCD_menuItem = 4; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0; 

        delay(100);
      }
 }

}

void wakeUpLCD() //LCD SCREENSAVER WAKE UP FUNCTIONALITY
{

  screenSaverActive = false;
  HALT_LCD_MENU = false; // stops the LCD_USER_MAIN_MENU();  main loop from running during screensavers

  lcd.begin(LCD_COLS, LCD_ROWS);
  if (LCD_BACKLIGHT_ALWAYS_OFF == 1) {lcd.noBacklight();}
  if (USE_SIGNAL_SCREENSAVER == 1)
  {    //return to last known menu state
       LCD_menuItem =       SAVESTATE_menuItem;
       LCD_subsetFrame =    SAVESTATE_subsetFrame;
       LCD_page =           SAVESTATE_page;
       LCD_main_pointer =   SAVESTATE_main_pointer;
       LCD_selectOption =   SAVESTATE_selectOption;
       LCD_pointer_count =  SAVESTATE_pointer_count; 
       LCD_sub_pointer =    SAVESTATE_sub_pointer;
       currentMenu =        SAVESTATE_currentMenu;

       //blank all original save states
       SAVESTATE_menuItem =        0;
       SAVESTATE_subsetFrame =     0;
       SAVESTATE_page =            0;
       SAVESTATE_sub_pointer =     0;
       SAVESTATE_main_pointer =    0;
       SAVESTATE_selectOption =    0;
       SAVESTATE_pointer_count =   0;
       SAVESTATE_currentMenu = MAIN_MENU; 
       delay(100);
  }

}
  
bool userActivity() { //LCD SCREENSAVER ACTIVITY DETECTION
  
  static int lastButtonState = HIGH;
  bool activity = false;

  // Encoder rotation
  int clkState = digitalRead(LCD_pin_clk);
  if (clkState != lastClkState) {
    activity = true;
  }
  lastClkState = clkState;

  // Encoder push button
  int buttonState = digitalRead(LCD_pin_switch);
  if (buttonState == LOW && lastButtonState == HIGH) {
    activity = true;
  }
  lastButtonState = buttonState;
  
  return activity;

}

void LCD_screenSaver()
{//placed in gbs_control.ino void loop();
    

    if (USE_SCREENSAVER == 0) //leave function if screensaver not enabled in option
    { return;}
    else
    {
      //Check for activity
       if (userActivity()) 
       {
             lastActivityTime = millis();
             if (screenSaverActive){   wakeUpLCD(); } //resume main menu, turn on LCD if its set to on 
       }

      //Check for inactivity
       if (millis() - lastActivityTime > SCREEN_TIMEOUT && !screenSaverActive)
       {    enterScreenSaver();   }
   
    }
}



void LCD_USER_LCDSET() {                 // LCD MAIN FUNCTIONALITY FOR SETTINGS/OPTIONS SUB MENU SYSTEM
    static int categoryIndex = 0;        // LCD_SETTINGS[] index
    static int subOptionIndex = 0;       // Sub-option index for selected category
    static bool insideSubOptions = false;
  
    const int totalCategories = 6;
    const int maxSubOptions[] = {2, 7, 6, 5, 5, 0};  // Items before "BACK" //example: if array has 10 items, list it here as -1 (9);

   
    lcd.clear();
    lcd.setCursor(0, 0);
    if (!insideSubOptions) {
      
        lcd.print("LCD SETTINGS:");
        lcd.setCursor(0, 1);
        lcd.print("> " + LCD_SETTINGS[categoryIndex]);
    } else {
       if(categoryIndex != 5) {lcd.print(LCD_SETTINGS[categoryIndex]);} //if category == (LCD_SETTINGS["BACK"] //change this if we add a new category to LCD_SETTINGS
        lcd.setCursor(0, 1);
        switch (categoryIndex) {
            case 0: lcd.print(LCD_SubOptions_ONOFF[subOptionIndex]); break;
            case 1: lcd.print(LCD_SubOptions_SCREENSAVER[subOptionIndex]); break;
            case 2: lcd.print(LCD_SubOptions_REFRESHRATE[subOptionIndex]); break;
            case 3: lcd.print(LCD_SubOptions_SUBMENU[subOptionIndex]); break;
            case 4: lcd.print(LCD_SubOptions_CURSOR[subOptionIndex]); break;
            case 5: //IF WE'RE IN LCD_SETTINGS[BACK] SUBOPTION MODE, WE NEED TO EXIT AS THERE ARE NO OPTIONS 
            { //put functionality of LCD_SETTINGS[BACK] here to exit properly. 
             // BACK BUTTON FROM LCD_SETTINGS HAS BEEN PRESSED TO ENTER THIS CASE

                insideSubOptions = false;
                LCD_pointer_count = 0; //categoryIndex
                categoryIndex = 0;  subOptionIndex = 0; //these """fix""" the "back" being displayed instantly once we re-enter LCD SETTINGS aftering going out of it with BACK
 
                // LCD_resetToXMenu(RETURN_TO_OPTIONS); 
                LCD_page = 1;  LCD_subsetFrame = 1; LCD_main_pointer = 0; LCD_sub_pointer = 0; LCD_selectOption = 1; LCD_menuItem = 3;    LCD_pointer_count = 0;   currentMenu = MAIN_MENU;//return to previous menu (options)
                      
               ////////////////////////////////////////////////////////////////////////////////
                //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
                screenSaverActive = false; 
                lastActivityTime = millis();
               ////////////////////////////////////////////////////////////////////////////////



               // reset options menu state if needed
               categoryIndex = 0;        // LCD_SETTINGS[] index
               subOptionIndex = 0;       // Sub-option index for selected category
               insideSubOptions = false;
               return;

            
            } 
               break;
        }
    }

    delay(LCD_REFRESHRATE);

    // Clamp encoder range
    int maxIndex = insideSubOptions ? maxSubOptions[categoryIndex] : totalCategories - 1;
    if (LCD_pointer_count < 0) LCD_pointer_count = 0;
    if (LCD_pointer_count > maxIndex) LCD_pointer_count = maxIndex;

    if (!insideSubOptions)
        categoryIndex = LCD_pointer_count;
    else
        subOptionIndex = LCD_pointer_count;

    // Handle button press
    if (digitalRead(LCD_pin_switch) == LOW) {
        delay(300);

        if (!insideSubOptions) {
            insideSubOptions = true;
            LCD_pointer_count = 0;
        } else {
            String selected;
            switch (categoryIndex) {
                //case 0: selected = LCD_SubOptions_SCANLINES[subOptionIndex]; break;
                case 0: selected = LCD_SubOptions_ONOFF[subOptionIndex]; break;
                case 1: selected = LCD_SubOptions_SCREENSAVER[subOptionIndex]; break;
                case 2: selected = LCD_SubOptions_REFRESHRATE[subOptionIndex]; break;
                case 3: selected = LCD_SubOptions_SUBMENU[subOptionIndex]; break;
                case 4: selected = LCD_SubOptions_CURSOR[subOptionIndex]; break;
                case 5: selected = LCD_SETTINGS[subOptionIndex]; break;  // Explicitly handle the "BACK" option
            }

            if (selected == "BACK") //submenu option BACK
             { //should return back into LCD_Settings array displaying all of our settings options to choose from
                insideSubOptions = false;
                LCD_pointer_count = categoryIndex; 
                 subOptionIndex = 0;
                 selected = "TEST";

                return;
            }
                          
                if (categoryIndex == 0)
                { // LCD ON/OFF
            
                 if (selected == "OFF")    {LCD_BACKLIGHT_ALWAYS_OFF = 1; lcd.noBacklight(); }  // LCD_BACKLIGHT_ALWAYS_OFF
                 else if(selected == "ON") {LCD_BACKLIGHT_ALWAYS_OFF = 0; lcd.backlight();   }
                saveLCDSettingsFile();
                // RESERVED: Save changed backlight setting
            } else if (categoryIndex == 1) { // SCREENSAVER
                     if (selected == "NONE")          {USE_SCREENSAVER = 0;      USE_SPLASH_SCREENSAVER      = 0;    USE_TURNOFF_LCD_SCREENSAVER = 0;   USE_SIGNAL_SCREENSAVER = 0; }
                else if (selected == "SPLASHSCREEN")  {USE_SCREENSAVER = 1;      USE_SPLASH_SCREENSAVER      = 1;    USE_SIGNAL_SCREENSAVER      = 0;                               }
                else if (selected == "BACKLIGHTOFF")  {USE_SCREENSAVER = 1;      USE_TURNOFF_LCD_SCREENSAVER = 1;                                                                   }
                else if (selected == "SIGNAL"      )  {USE_SCREENSAVER = 1;      USE_SIGNAL_SCREENSAVER      = 1;    USE_SPLASH_SCREENSAVER      = 0;                               }
                else if (selected == "TIMEOUT:30SECS"){SCREEN_TIMEOUT = 30000;                                                                                                      }
                else if (selected == "TIMEOUT:1MIN")  {SCREEN_TIMEOUT = 60000;                                                                                                      }
                else if (selected == "TIMEOUT:5MIN")  {SCREEN_TIMEOUT = 300000;                                                                                                     }
                saveLCDSettingsFile();
                // RESERVED: Save changed screensaver mode
            } else if (categoryIndex == 2) { // REFRESH RATE
               if(subOptionIndex >=0 && subOptionIndex < 6) //don't load "back" value into save file
                LCD_REFRESHRATE = selected.toInt();
                saveLCDSettingsFile();
                // RESERVED: Save changed LCD refresh rate
            } else if (categoryIndex == 3) { // STARTUP SUBMENU
                if(subOptionIndex >=0 && subOptionIndex < 5) //don't load "back" value into save file
                  {
                    LOAD_DIRECTLY_INTO_SUB_MENU = subOptionIndex;
                    CURSOR_ON_MENU_AT_STARTUP = 0;
                    saveLCDSettingsFile();
                  }
                // RESERVED: Save changed startup submenu setting
            } else if (categoryIndex == 4) { // STARTUP CURSOR
                if(subOptionIndex >=0 && subOptionIndex < 5)  //don't load "back" value into save file
                {
                   CURSOR_ON_MENU_AT_STARTUP = subOptionIndex;
                   LOAD_DIRECTLY_INTO_SUB_MENU = 0; //make sure we dont have both enabled, just in case that causes issues;
                   saveLCDSettingsFile();
                }
                // RESERVED: Save changed cursor setting
            } else if (categoryIndex == 5)
             { 
                 // RESERVED: currently for LCD_SETTINGS[BACK] all though it seems to never get here. but still you cannot use this category number.
                 // //LCD_SETTINGS[BACK] should always be the last category, change as needed. if you add a new category, change this to 7

             }

            // Confirmation message
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("SAVED:");
            lcd.setCursor(0, 1);
            lcd.print(selected);
            delay(1000);
           

            insideSubOptions = false;
            LCD_pointer_count = categoryIndex;
            ////////////////////////////////////////////////////////////////////////////////
            //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
            screenSaverActive = false; 
            lastActivityTime = millis();
    ////////////////////////////////////////////////////////////////////////////////

        }
    }
}


int getCurrentSlotIndexFromPresetSlotChar(char slotChar) {
    const char slotCharset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~()!*:,;";
    for (int i = 0; i < strlen(slotCharset); i++) {
        if (slotCharset[i] == slotChar)
            return i;
    }
    return -1; // Not found
}


void loadLCDSettingsFile() {
    File file = SPIFFS.open("/LCD_settings.txt", "r");  // Open file for reading
    if (!file) {
       // Serial.println("No saved settings, using defaults.");
        return;
    }

    // Read each setting from the file and assign values to variables
    LCD_BACKLIGHT_ALWAYS_OFF = file.readStringUntil('\n').toInt();
    USE_SCREENSAVER = file.readStringUntil('\n').toInt();
    USE_SPLASH_SCREENSAVER = file.readStringUntil('\n').toInt();
    USE_TURNOFF_LCD_SCREENSAVER = file.readStringUntil('\n').toInt();  // Read new setting
    LCD_REFRESHRATE = file.readStringUntil('\n').toInt();
    LOAD_DIRECTLY_INTO_SUB_MENU = file.readStringUntil('\n').toInt();
    CURSOR_ON_MENU_AT_STARTUP = file.readStringUntil('\n').toInt();
    SCREEN_TIMEOUT = file.readStringUntil('\n').toInt();
    USE_SIGNAL_SCREENSAVER = file.readStringUntil('\n').toInt();

    delay(50);
    file.close();
    delay(50);
    //Serial.println("Settings loaded.");
}

void saveLCDSettingsFile() {
    File file = SPIFFS.open("/LCD_settings.txt", "w");  // Open file for writing
    if (!file) {
        //Serial.println("Failed to open file for writing.");
        return;
    }

    // Save settings to the file
    file.println(LCD_BACKLIGHT_ALWAYS_OFF);
    file.println(USE_SCREENSAVER);
    file.println(USE_SPLASH_SCREENSAVER);
    file.println(USE_TURNOFF_LCD_SCREENSAVER); 
    file.println(LCD_REFRESHRATE);
    file.println(LOAD_DIRECTLY_INTO_SUB_MENU);
    file.println(CURSOR_ON_MENU_AT_STARTUP);
    file.println(SCREEN_TIMEOUT);
    file.println(USE_SIGNAL_SCREENSAVER);
     
    delay(50);
    file.close();
    delay(50);
   // Serial.println("Settings saved.");
}


void resetLCDSettingsToDefault() {
    // DELETE SETTINGS.TXT  THEN LOAD IN DEFAULT VALUES (ONLY USE WITH FACTORY RESTORE OPTION)
    // Delete the existing file if it exists
    if (SPIFFS.exists("/LCD_settings.txt")) {
        SPIFFS.remove("/LCD_settings.txt");
    }

    //  Set defaults in RAM
     LCD_BACKLIGHT_ALWAYS_OFF    = 0;              // Default value
     USE_SCREENSAVER             = 0;
     USE_SPLASH_SCREENSAVER      = 0;
     USE_TURNOFF_LCD_SCREENSAVER = 0;              // Default value (disabled)
     USE_SIGNAL_SCREENSAVER      = 0;
     LCD_REFRESHRATE             = 200;            //200 recommended
     LOAD_DIRECTLY_INTO_SUB_MENU = 0;
     CURSOR_ON_MENU_AT_STARTUP   = 0;
     SCREEN_TIMEOUT              = 60000;           //default 60000 (1MIN), 10 seconds is 10000
    

    //Step 3: Save defaults to SPIFFS
    File file = SPIFFS.open("/LCD_settings.txt", "w");
    if (file) {

    file.println(LCD_BACKLIGHT_ALWAYS_OFF);
    file.println(USE_SCREENSAVER);
    file.println(USE_SPLASH_SCREENSAVER);
    file.println(USE_TURNOFF_LCD_SCREENSAVER);
    file.println(LCD_REFRESHRATE);
    file.println(LOAD_DIRECTLY_INTO_SUB_MENU);
    file.println(CURSOR_ON_MENU_AT_STARTUP);
    file.println(SCREEN_TIMEOUT);
    file.println(USE_SIGNAL_SCREENSAVER);


        file.close();
    }
   // Serial.println("Settings reset to defaults.");

}


void createDefaultLCDSettingsFile() {
  if (!SPIFFS.begin()) {
   // Serial.println("Failed to mount SPIFFS");
    return;
  }

  if (!SPIFFS.exists("/LCD_settings.txt")) {
   // Serial.println("Settings file not found, creating default...");

    File file = SPIFFS.open("/LCD_settings.txt", "w");
    if (!file) {
      //Serial.println("Failed to create settings file");
      return;
    }

    // Write default settings line-by-line 
    file.println("0");    // LCD_BACKLIGHT_ALWAYS_OFF
    file.println("0");    // USE_SCREENSAVER
    file.println("0");    // USE_SPLASH_SCREENSAVER
    file.println("0");    // USE_TURNOFF_LCD_SCREENSAVER
    file.println("200");  // LCD_REFRESHRATE
    file.println("0");    // LOAD_DIRECTLY_INTO_SUB_MENU
    file.println("0");    // CURSOR_ON_MENU_AT_STARTUP
    file.println("60000");// SCREEN_TIMEOUT
    file.println("0");    // USE_SIGNAL_SCREENSAVER

    delay(100);

    file.close();
   // Serial.println("Default settings file created.");
  } else {
   // Serial.println("Settings file exists.");
  }
 
}




String getSlotName()
{
//get current selected slot name

int currentSlotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
char id = LCDslotsObject.slot[currentSlotIndex].presetID;


       String slotName = LCDslotsObject.slot[currentSlotIndex].name; 
       return slotName;
}



void LCD_scrollResolutionAndSlot(const String &resolution, const String &slotName) {
    static unsigned long phaseTimer = 0;
    static unsigned long scrollTimer = 0;
    static int scrollStep = 0;
    static int phase = 0; 
    // 0 = hold resolution, 1 = scroll to slot, 2 = hold slot, 3 = scroll to resolution

    const unsigned long scrollInterval = 200; // ms between scroll steps
    const unsigned long holdTime = SIGNAL_SCROLL_SPEED;     // ms hold for each phase

    auto pad = [](String text) {
        while (text.length() < LCD_COLS) text += " ";
        return text.substring(0, LCD_COLS);
    };

    static bool firstRun = true;
    if (firstRun) {
        phase = 0;
        phaseTimer = millis();
        scrollStep = 0;
        firstRun = false;
    }

    if (phase == 0) { 
        // Hold resolution (top line)
        lcd.setCursor(0, 0);
        lcd.print(pad(resolution));
        // Clear and pad bottom line with spaces to avoid leftover text
        lcd.setCursor(0, 1);
        lcd.print(pad("")); 

        if (millis() - phaseTimer >= holdTime) {
            phase = 1;
            scrollStep = 0;
            scrollTimer = millis();
        }
    }
    else if (phase == 1) {
        // Scroll from resolution to slot (top line)
        if (millis() - scrollTimer >= scrollInterval) {
            scrollTimer = millis();
            String displayText = resolution + "   " + slotName;
            lcd.setCursor(0, 0);
            lcd.print(pad(displayText.substring(scrollStep, scrollStep + LCD_COLS)));
            // Clear and pad bottom line again
            lcd.setCursor(0, 1);
            lcd.print(pad(""));
            scrollStep++;
            if (scrollStep > resolution.length()) {
                phase = 2;
                phaseTimer = millis();
            }
        }
    }
    else if (phase == 2) {
        // Hold slot name (top line)
        lcd.setCursor(0, 0);
        lcd.print(pad(slotName));
        // Clear and pad bottom line
        lcd.setCursor(0, 1);
        lcd.print(pad(""));

        if (millis() - phaseTimer >= holdTime) {
            phase = 3;
            scrollStep = 0;
            scrollTimer = millis();
        }
    }
    else if (phase == 3) {
        // Scroll from slot back to resolution (top line)
        if (millis() - scrollTimer >= scrollInterval) {
            scrollTimer = millis();
            String displayText = slotName + "   " + resolution;
            lcd.setCursor(0, 0);
            lcd.print(pad(displayText.substring(scrollStep, scrollStep + LCD_COLS)));
            // Clear and pad bottom line
            lcd.setCursor(0, 1);
            lcd.print(pad(""));

            scrollStep++;
            if (scrollStep > slotName.length()) {
                phase = 0;
                phaseTimer = millis();
            }
        }
    }
}

// we use this incase the user starts up the LCD menu before ever starting the webapp (which makes the empty slots on first startup of the app)?.
// Otherwise it would show a blank slot name and be confusing. With this, it just shows "BACK" so user can deduce they need saved slots to work with in app.
void createDefaultSlotsIfMissing() { 
    SlotMetaArray slotsObject;
    File slotsBinaryFileRead = SPIFFS.open(SLOTS_FILE, "r");

    if (!slotsBinaryFileRead) {
        File slotsBinaryFileWrite = SPIFFS.open(SLOTS_FILE, "w");
        for (int i = 0; i < SLOTS_TOTAL; i++) {
            slotsObject.slot[i].slot = i;
            slotsObject.slot[i].presetID = 0;
            slotsObject.slot[i].scanlines = 0;
            slotsObject.slot[i].scanlinesStrength = 0;
            slotsObject.slot[i].wantVdsLineFilter = false;
            slotsObject.slot[i].wantStepResponse = true;
            slotsObject.slot[i].wantPeaking = true;
            char emptySlotName[25] = "Empty                   ";
            strncpy(slotsObject.slot[i].name, emptySlotName, 25);
        }
        slotsBinaryFileWrite.write((byte *)&slotsObject, sizeof(slotsObject));
        slotsBinaryFileWrite.close();
    } else {
        slotsBinaryFileRead.close();
    }
}


void LCD_USER_FilterSET()
{
    const char* Filter[] = {"SCANLINES","LINE FILTER","PEAKING","STEP RESPONSE","BACK"};
    const int filterCount = 5; //option count above
    int filter_selection = 0;
    LCD_encoder_pos = 0;
    
    int currentSlotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); //moved to above cluster to handle "ON" issue


  lcd.clear();
  while(true)
{
    delay(LCD_REFRESHRATE);


    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= filterCount) LCD_encoder_pos = 4;  


    lcd.setCursor(0,0);
    lcd.print("FILTER:");
    lcd.setCursor(0,1);
    lcd.print(Filter[LCD_encoder_pos]);
     lcd.print("                ");

    if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

   
      
      if      (strcmp(Filter[LCD_encoder_pos],"BACK") == 0)               {filter_selection = 0;    LCD_resetToXMenu(RETURN_TO_OPTIONS);        break;}//return to previous menu (options)                return;}
      else if (strcmp(Filter[LCD_encoder_pos],"SCANLINES") == 0)          {filter_selection = 1;                                                break;}
      else if (strcmp(Filter[LCD_encoder_pos],"LINE FILTER") == 0)        {filter_selection = 2;                                                break;}
      else if (strcmp(Filter[LCD_encoder_pos],"PEAKING") == 0)            {filter_selection = 3;                                                break;}
      else if (strcmp(Filter[LCD_encoder_pos],"STEP RESPONSE") == 0)      {filter_selection = 4;                                                break;}
 
     



    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }


  }
}


   if (filter_selection == 1){
  // ----------------------------------------------------------------
  // SCANLINES OPTION
  // ----------------------------------------------------------------
     const char* ScanlinesOptions[] = { "OFF", "ON", "10%", "20%", "30%", "40%", "50%", "BACK"};
     const int optionCountSCAN = 8; 
     
    lcd.clear();
    LCD_encoder_pos = 0;


       while (true)
   {
    delay(LCD_REFRESHRATE);


    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountSCAN) LCD_encoder_pos = 7;  

    lcd.setCursor(0, 0);
    lcd.print("SCANLINES:      ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(ScanlinesOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

       if      (strcmp(ScanlinesOptions[LCD_encoder_pos], "BACK") == 0)   {  filter_selection = 0;                                             break;}
       else if (strcmp(ScanlinesOptions[LCD_encoder_pos], "OFF" ) == 0)   {  uopt->wantScanlines = false; uopt->scanlineStrength = 0x00;       break;}                                                         //... basically we want the user to instantly have visual confirmation when turning on scanlines.
       else if (strcmp(ScanlinesOptions[LCD_encoder_pos], "ON"  ) == 0)   {  uopt->wantScanlines = true;  if (LCDslotsObject.slot[currentSlotIndex].scanlinesStrength == 0x00) {uopt->scanlineStrength = 0x20;} else {uopt->scanlineStrength = LCDslotsObject.slot[currentSlotIndex].scanlinesStrength;}    break;}  
       else if (strcmp(ScanlinesOptions[LCD_encoder_pos], "10%" ) == 0)   {  uopt->wantScanlines = true;  uopt->scanlineStrength = 0x10;       break;}
       else if (strcmp(ScanlinesOptions[LCD_encoder_pos], "20%" ) == 0)   {  uopt->wantScanlines = true;  uopt->scanlineStrength = 0x20;       break;}
       else if (strcmp(ScanlinesOptions[LCD_encoder_pos], "30%" ) == 0)   {  uopt->wantScanlines = true;  uopt->scanlineStrength = 0x30;       break;}
       else if (strcmp(ScanlinesOptions[LCD_encoder_pos], "40%" ) == 0)   {  uopt->wantScanlines = true;  uopt->scanlineStrength = 0x40;       break;}
       else if (strcmp(ScanlinesOptions[LCD_encoder_pos], "50%" ) == 0)   {  uopt->wantScanlines = true;  uopt->scanlineStrength = 0x50;       break;}
     
      //break;
    }


    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }

}

   else if (filter_selection == 2){
  // ----------------------------------------------------------------
  // LINE FILTER OPTION
  // ----------------------------------------------------------------
     const char* LineFilterOptions[] = { "OFF", "ON", "BACK"};
     const int optionCountLINE = 3; 
  
    lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);

    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountLINE) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("LINE FILTER:    ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(LineFilterOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

       if      (strcmp(LineFilterOptions[LCD_encoder_pos], "BACK") == 0)   {  filter_selection == 0;                                                                       break;}
       else if (strcmp(LineFilterOptions[LCD_encoder_pos], "OFF" ) == 0)   {  uopt->wantVdsLineFilter = 0;  saveUserPrefs();                                  delay(100);  break;}
       else if (strcmp(LineFilterOptions[LCD_encoder_pos], "ON"  ) == 0)   {  uopt->wantVdsLineFilter = 1;  saveUserPrefs();                                  delay(100);  break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
   }
  else  if (filter_selection == 3){
  // ----------------------------------------------------------------
  // PEAKING OPTION
  // ----------------------------------------------------------------
     const char* PeakingOptions[] = { "OFF", "ON", "BACK"};
     const int optionCountPEAK = 3; 
  
   lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);


    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountPEAK) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("PEAKING:        ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(PeakingOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

       if      (strcmp(PeakingOptions[LCD_encoder_pos], "BACK") == 0)   {  filter_selection == 0;                                                                 break;}
       else if (strcmp(PeakingOptions[LCD_encoder_pos], "OFF" ) == 0)   {  uopt->wantPeaking = 0;  saveUserPrefs();                                  delay(100);  break;}
       else if (strcmp(PeakingOptions[LCD_encoder_pos], "ON"  ) == 0)   {  uopt->wantPeaking = 1;  saveUserPrefs();                                  delay(100);  break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}
    else if (filter_selection == 4){
 // ----------------------------------------------------------------
  // STEP RESPONSE OPTION
  // ----------------------------------------------------------------
     const char* StepResponseOptions[] = { "OFF", "ON", "BACK"};
     const int optionCountSTEP = 3; 
  
   lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);

    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountSTEP) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("STEP RESPONSE:  ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(StepResponseOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

       if      (strcmp(StepResponseOptions[LCD_encoder_pos], "BACK") == 0)   { filter_selection == 0;                                                                      break;}
       else if (strcmp(StepResponseOptions[LCD_encoder_pos], "OFF" ) == 0)   { uopt->wantStepResponse = 0;  saveUserPrefs();                                  delay(100);  break;}
       else if (strcmp(StepResponseOptions[LCD_encoder_pos], "ON"  ) == 0)   { uopt->wantStepResponse = 1;  saveUserPrefs();                                  delay(100);  break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}

    


   // int currentSlotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); //moved to above cluster to handle "ON" issue
    if (currentSlotIndex >= 0 && currentSlotIndex < SLOTS_TOTAL) {
          LCDslotsObject.slot[currentSlotIndex].scanlines         = uopt->wantScanlines ? 1 : 0;
          LCDslotsObject.slot[currentSlotIndex].scanlinesStrength = uopt->scanlineStrength;

        //new
          LCDslotsObject.slot[currentSlotIndex].wantVdsLineFilter = uopt->wantVdsLineFilter;
          LCDslotsObject.slot[currentSlotIndex].wantStepResponse  = uopt->wantStepResponse;
          LCDslotsObject.slot[currentSlotIndex].wantPeaking       = uopt->wantPeaking;
        

          File f = SPIFFS.open(SLOTS_FILE, "w");
          f.write((byte*)&LCDslotsObject, sizeof(LCDslotsObject));
          f.close();
        }

          saveUserPrefs();

          uint8_t videoMode = getVideoMode();
          if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read()) {
           videoMode = rto->videoStandardInput;
          }
          applyPresets(videoMode);


    if(filter_selection != 0) //if we press back don't say saved.
    {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SAVED!");
    delay(200);
    }
  
   LCD_encoder_pos = 0;
   LCD_resetToXMenu(RETURN_TO_OPTIONS); //this is a compromise because scanlines and such were not instantly updating until I left the FILTERmenu function (Unsolved)
 

    ////////////////////////////////////////////////////////////////////////////////
    //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
    screenSaverActive = false; 
    lastActivityTime = millis();
    ////////////////////////////////////////////////////////////////////////////////
}




void LCD_USER_GlobalSET() {

  const int optionCount = 3; 
  const int globalCount = 10;
  const char* Global[] = {"FULL HEIGHT","LOWRES UPSCALING","FRAMETIME LOCK","DEINTERLACE MODE","OUTPUT RGBHV","ADC CALIBRATION","MATCHED PRESET","DISABLE GENLOCK","FORCE PAL60HZ","BACK"};
  int global_selection = 0;
  LCD_encoder_pos = 0;


  lcd.clear();
  while(true)
{
    delay(LCD_REFRESHRATE);

       if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= globalCount) LCD_encoder_pos = 9;


    lcd.setCursor(0,0);
    lcd.print("GLOBAL SETTING: ");
    lcd.setCursor(0,1);
    lcd.print(Global[LCD_encoder_pos]);
     lcd.print("                ");

    if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }


      if      (strcmp(Global[LCD_encoder_pos],"BACK") == 0)               {global_selection = 0;          LCD_resetToXMenu(RETURN_TO_OPTIONS);  break;}//return to previous menu (options)                return;}
      else if (strcmp(Global[LCD_encoder_pos],"FULL HEIGHT") == 0)        {global_selection = 1;                                                break;}
      else if (strcmp(Global[LCD_encoder_pos],"LOWRES UPSCALING") == 0)   {global_selection = 2;                                                break;}
      else if (strcmp(Global[LCD_encoder_pos],"FRAMETIME LOCK") == 0)     {global_selection = 3;                                                break;}
      else if (strcmp(Global[LCD_encoder_pos],"DEINTERLACE MODE") == 0)   {global_selection = 4;                                                break;}
      else if (strcmp(Global[LCD_encoder_pos],"OUTPUT RGBHV") == 0)       {global_selection = 5;                                                break;}
      else if (strcmp(Global[LCD_encoder_pos],"ADC CALIBRATION") == 0)    {global_selection = 6;                                                break;}
      else if (strcmp(Global[LCD_encoder_pos],"MATCHED PRESET") == 0)     {global_selection = 7;                                                break;}
      else if (strcmp(Global[LCD_encoder_pos],"DISABLE GENLOCK") == 0)    {global_selection = 8;                                                break;}
      else if (strcmp(Global[LCD_encoder_pos],"FORCE PAL60HZ") == 0)      {global_selection = 9;                                                break;}



    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }


  }
}
  
  if(global_selection == 7){
  // ----------------------------------------------------------------
  // MATCHED PRESETS OPTION
  // ----------------------------------------------------------------
    const char* matchedPresetsOptions[] = { "OFF", "ON", "BACK"};
    
     lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);

       if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("MATCHED PRESETS:");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(matchedPresetsOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

     
           if  (strcmp(matchedPresetsOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
      else if (strcmp(matchedPresetsOptions[LCD_encoder_pos], "OFF") == 0)   { uopt->matchPresetSource = 0;  saveUserPrefs();  delay(100);   break;}
      else if (strcmp(matchedPresetsOptions[LCD_encoder_pos], "ON") == 0)    { uopt->matchPresetSource = 1;  saveUserPrefs();  delay(100);   break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}

  else if(global_selection == 6){
  // ----------------------------------------------------------------
  // ADC CALIBRATION OPTION
  // ----------------------------------------------------------------
    const char* ADC_CAL_Options[] = { "OFF", "ON", "BACK"};

    lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);

       if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("ADC CALIBRATION:");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(ADC_CAL_Options[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(ADC_CAL_Options[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                               break;}
      else if (strcmp(ADC_CAL_Options[LCD_encoder_pos], "OFF") == 0) { uopt->enableCalibrationADC = 0;  saveUserPrefs();       delay(100); break;}
      else if (strcmp(ADC_CAL_Options[LCD_encoder_pos], "ON") == 0)  { uopt->enableCalibrationADC = 1;  saveUserPrefs();       delay(100); break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}
  else if(global_selection == 1){
  // ----------------------------------------------------------------
  // FULL HEIGHT OPTION
  // ----------------------------------------------------------------
  const char* fullHeightOptions[] = { "OFF", "ON", "BACK"};
   lcd.clear();
  LCD_encoder_pos = 0;

  while (true) {
    delay(LCD_REFRESHRATE);
  
       if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("FULL HEIGHT:    ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(fullHeightOptions[LCD_encoder_pos]);
    lcd.print("                ");

      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(fullHeightOptions[LCD_encoder_pos], "BACK") == 0){  global_selection = 0;                                    break;}
      else if (strcmp(fullHeightOptions[LCD_encoder_pos], "OFF") == 0) {  uopt->wantFullHeight = 0;  saveUserPrefs();  delay(100); break;}
      else if (strcmp(fullHeightOptions[LCD_encoder_pos], "ON") == 0)  {  uopt->wantFullHeight = 1;  saveUserPrefs();  delay(100); break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

  }
}

  else if(global_selection == 2){
  // ----------------------------------------------------------------
  // LOW RES: USE UPSCALING OPTION
  // ----------------------------------------------------------------
    const char* upscalingOptions[] = { "OFF", "ON", "BACK"};
    lcd.clear();
    LCD_encoder_pos = 0;

  while (true)
   {
    delay(LCD_REFRESHRATE);
   
       if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("LOWRES UPSCALING");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(upscalingOptions[LCD_encoder_pos]);
     lcd.print("                ");

      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp( upscalingOptions[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                       break;}
      else if (strcmp( upscalingOptions[LCD_encoder_pos], "OFF") == 0) { uopt->preferScalingRgbhv = 0;  saveUserPrefs(); delay(100); break;}
      else if (strcmp( upscalingOptions[LCD_encoder_pos], "ON") == 0)  { uopt->preferScalingRgbhv = 1;  saveUserPrefs(); delay(100); break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }


   }
}
 else if(global_selection == 5){
  // ----------------------------------------------------------------
  // OUTPUT RGBHV/Componnent  OPTION
  // ----------------------------------------------------------------
    const char* RGBHVCOMPOptions[] = { "OFF", "ON", "BACK"};
   
    lcd.clear();
    LCD_encoder_pos = 0;

    while (true)
   {
    delay(LCD_REFRESHRATE);
   
      if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("OUTPUT RBGHV:   ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(RGBHVCOMPOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                        break;}
      else if (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "OFF") == 0) { uopt->wantOutputComponent = 0;  saveUserPrefs(); delay(100); break;}
      else if (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "ON") == 0)  { uopt->wantOutputComponent = 1;  saveUserPrefs(); delay(100); break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}
  else if(global_selection == 9){
  // ----------------------------------------------------------------
  // FORCE PAL 60HZ OPTION
  // ----------------------------------------------------------------
     const char* forcePALOptions[] = { "OFF", "ON", "BACK"};
    
     lcd.clear();
     LCD_encoder_pos = 0;

    while (true)
   {
    delay(LCD_REFRESHRATE);
   
          if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("FORCE PAL 60HZ: ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(forcePALOptions[LCD_encoder_pos]);
    lcd.print("                ");

      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(forcePALOptions[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                break;}
      else if (strcmp(forcePALOptions[LCD_encoder_pos], "OFF") == 0) {  uopt->PalForce60 = 0;  saveUserPrefs(); delay(100); break;}
      else if (strcmp(forcePALOptions[LCD_encoder_pos], "ON") == 0)  {  uopt->PalForce60 = 1;  saveUserPrefs(); delay(100); break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
 }  
   
    else if(global_selection == 8){ 
  // ----------------------------------------------------------------
  // DISABLE EXTERNAL CLOCKGEN OPTION
  // ----------------------------------------------------------------
     const char* DisableClockgenOptions[] = { "OFF", "ON", "BACK"};
    
     lcd.clear();
     LCD_encoder_pos = 0;

        while (true)
   {
    delay(LCD_REFRESHRATE);

          if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("DISABLE CLOCKGEN");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(DisableClockgenOptions[LCD_encoder_pos]);
    lcd.print("                ");

      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp( DisableClockgenOptions[LCD_encoder_pos], "BACK") == 0){global_selection = 0;                                                 break;}
      else if (strcmp( DisableClockgenOptions[LCD_encoder_pos], "OFF") == 0) {uopt->disableExternalClockGenerator = 0;  saveUserPrefs(); delay(100); break;}
      else if (strcmp( DisableClockgenOptions[LCD_encoder_pos], "ON") == 0)  {uopt->disableExternalClockGenerator = 1;  saveUserPrefs(); delay(100); break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}
     else if(global_selection == 3){
  // ----------------------------------------------------------------
  // FRAMETIME LOCK OPTION
  // ----------------------------------------------------------------
     const char* FrameTimeLockOptions[] = { "OFF", "ON", "BACK"};
   
    lcd.clear();
     LCD_encoder_pos = 0;

        while (true)
   {
    delay(LCD_REFRESHRATE);
    
          if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("FRAMETIME LOCK: ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(FrameTimeLockOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                        break;}
      else if (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "OFF") == 0) { uopt->enableFrameTimeLock = 0;  saveUserPrefs(); delay(100); break;}
      else if (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "ON") == 0)  { uopt->enableFrameTimeLock = 1;  saveUserPrefs(); delay(100); break;}
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
   
}
  else if(global_selection == 4){
  // ----------------------------------------------------------------
  // DEINTERLACE METHOD OPTION
  // ----------------------------------------------------------------
    const char* deinterlaceOptions[] = { "MOTION ADAPTIVE", "BOB", "BACK"};
   //const int optionCount = 3;
    lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);
   
          if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("DEINTERLACE MODE");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(deinterlaceOptions[LCD_encoder_pos]);
    lcd.print("                 ");


      if (digitalRead(LCD_pin_switch) == LOW)
       {
           while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(deinterlaceOptions[LCD_encoder_pos], "BACK") == 0)            { global_selection = 0;                              break;}
      else if (strcmp(deinterlaceOptions[LCD_encoder_pos], "MOTION ADAPTIVE") == 0) { uopt->deintMode = 0;  saveUserPrefs(); delay(100); break;}
      else if (strcmp(deinterlaceOptions[LCD_encoder_pos], "BOB") == 0)             { uopt->deintMode = 1;  saveUserPrefs(); delay(100); break;}
     
      //break;
       }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
    }


   
    if(global_selection != 0) //if we press back don't say saved.
    {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SAVED!");
    delay(200);
    }
  


    ////////////////////////////////////////////////////////////////////////////////
    //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
    screenSaverActive = false; 
    lastActivityTime = millis();
    ////////////////////////////////////////////////////////////////////////////////


}





 //original working version 
// ----------------------------------------------------------
// Create a new slot via LCD + rotary encoder
// ----------------------------------------------------------
void LCD_USER_createNewSlot() {
  // Alphabet characters and special commands
  const char* alphabet[] = { "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"," ","BACKSPACE", "CONFIRM","EXIT" };
  const int alphabetSize = sizeof(alphabet) / sizeof(alphabet[0]);

  // Slot name buffer (max 15 chars + null terminator)
  char newSlotName[16];
  memset(newSlotName, 0, sizeof(newSlotName));
  int namePos = 0;
  
  // ----------------------------------------------------------------
  // NAME SELECTION
  // ----------------------------------------------------------------
  lcd.clear();
  LCD_encoder_pos = 0;
  while (true) {
    delay(LCD_REFRESHRATE);
    // Wrap encoder position
    if (LCD_encoder_pos < 0) LCD_encoder_pos = alphabetSize - 1;
    if (LCD_encoder_pos >= alphabetSize) LCD_encoder_pos = 0;

    // Show current name on first row
    lcd.setCursor(0, 0);
    lcd.print("NAME:");
    lcd.setCursor(5, 0);
    lcd.print(newSlotName);

    // Show candidate character on second row
    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(alphabet[LCD_encoder_pos]);
    lcd.print("           "); // clear trailing chars
    
  

    // Handle click (select character / command)
    if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); } // debounce

      const char* token = alphabet[LCD_encoder_pos];

      if (strcmp(token, "BACKSPACE") == 0) { // == 0 means compared strings = equal
        // Remove last character
        if (namePos > 0) {
           
             newSlotName[namePos] = '\0'; //deletes current char in the array and makes it the end of line
            --namePos; //instantly go to previous position
             newSlotName[namePos] = ' '; //change it to blank so we see a change?
          
         
        }
      }
      else if (strcmp(token, "EXIT") == 0) {

       //return to previous menu (options)
       //LCD_page = 1;  LCD_subsetFrame = 1; LCD_main_pointer = 0; LCD_sub_pointer = 0; LCD_selectOption = 1; LCD_menuItem = 3;    LCD_pointer_count = 0;   currentMenu = MAIN_MENU;
       LCD_resetToXMenu(RETURN_TO_OPTIONS);
        return;
      }
      else if (strcmp(token, "CONFIRM") == 0) { //name confirmed
        if (namePos > 0)
        {
        
         for (int i = 0; i < SLOTS_TOTAL; i++)
          {
            delay(100);

              if (String(LCDslotsObject.slot[i].name) == "Empty                   ")
               {
                 // Copy name safely into slot
                    strncpy(LCDslotsObject.slot[i].name, newSlotName, sizeof(LCDslotsObject.slot[i].name) - 1);
                    LCDslotsObject.slot[i].name[sizeof(LCDslotsObject.slot[i].name) - 1] = '\0';
                    LCDslotsObject.slot[i].presetID = 'A' + i;

                    /*
                     // Initialize slot with current runtime settings
                      LCDslotsObject.slot[i].slot              = i;
                      LCDslotsObject.slot[i].scanlines         = uopt->wantScanlines;
                      LCDslotsObject.slot[i].scanlinesStrength = uopt->scanlineStrength;
                      LCDslotsObject.slot[i].wantVdsLineFilter = uopt->wantVdsLineFilter;
                      LCDslotsObject.slot[i].wantStepResponse  = uopt->wantStepResponse;
                      LCDslotsObject.slot[i].wantPeaking       = uopt->wantPeaking;
                    */

                  // Save slot file
                      File f = SPIFFS.open(SLOTS_FILE, "w");

                   if (f) 
                   {
                     f.write((byte*)&LCDslotsObject, sizeof(LCDslotsObject));
                     f.close();
                     delay(200);
                   }
                   
                     break;
               }
         } 
      }
      break; // Done with name entry
    }
      else {
        // Regular character selected
        if (namePos < (int)sizeof(newSlotName) - 1) {
          newSlotName[namePos++] = token[0];
          newSlotName[namePos]   = '\0'; // keep null terminated
        }
      }
    }

    // Handle encoder rotation
    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }
  }
     


//#######################################################################################################
// LOAD NEWLY CREATED PRESET SLOT SO WE CAN SAVE PREFERENCES TO IT
    for (int i = 0; i < SLOTS_TOTAL; i++)
    {
         if(strcmp(LCDslotsObject.slot[i].name, newSlotName) == 0)
         {
            uopt->presetSlot = LCDslotsObject.slot[i].presetID;
     
             //uopt->presetPreference = OutputCustomized;
             saveUserPrefs();
             delay(100);
             savePresetToSPIFFS(); //without this the web app was displaying that no preference file was found for these manual made slots
             delay(100);
             break;
         }
    }


        
         LCD_resetToXMenu(RETURN_TO_OPTIONS);
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("SLOT CREATED!");
         delay(1000);
        

////////////////////////////////////////////////////////////////////////////////
//TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
screenSaverActive = false; 
lastActivityTime = millis();
////////////////////////////////////////////////////////////////////////////////
        return;


}



void LCD_USER_removeSlot() {
  // Load slots object
  File slotsBinaryFileRead = SPIFFS.open(SLOTS_FILE, "r");
  if (!slotsBinaryFileRead) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NO SLOTS FILE!");
    delay(2000);
    return;
  }
  slotsBinaryFileRead.read((byte *)&LCDslotsObject, sizeof(LCDslotsObject));
  slotsBinaryFileRead.close();

  // Find last valid slot
  int lastValidIndex = -1;
  for (int i = 0; i < SLOTS_TOTAL; i++) {
    if (String(LCDslotsObject.slot[i].name) != "Empty                   ") {
      lastValidIndex = i;
    }
  }

  // If no slots exist, go straight to BACK
  if (lastValidIndex < 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NO SLOTS!");
    delay(1500);
      
    LCD_resetToXMenu(RETURN_TO_OPTIONS);
    return;
  }

  int selectionIndex = 0;   // current menu selection: 0..lastValidIndex, or BACK
  int prevEncoder = LCD_encoder_pos;

  while (true) {
    // Handle encoder movement
    if (LCD_encoder_pos != prevEncoder) {
      int delta = LCD_encoder_pos - prevEncoder;
      prevEncoder = LCD_encoder_pos;

      selectionIndex += delta;

      // Clamp range: [0..lastValidIndex] + BACK
      if (selectionIndex < 0) selectionIndex = 0;
      if (selectionIndex > lastValidIndex + 1) selectionIndex = lastValidIndex + 1;
    }

    // Draw screen
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("REMOVE SLOT:");

    if (selectionIndex == lastValidIndex + 1) {
      lcd.setCursor(0, 1);
      lcd.print("BACK");
    } else {
      lcd.setCursor(0, 1);
      lcd.print(String(LCDslotsObject.slot[selectionIndex].name));
    }

    // Handle button press
    if (digitalRead(LCD_pin_switch) == LOW) {
      delay(250); // debounce

      if (selectionIndex == lastValidIndex + 1) {
        // BACK BUTTON pressed
         LCD_resetToXMenu(RETURN_TO_OPTIONS);
        return;
      }

      // --- Remove slot ---
      char slotChar = 'A' + selectionIndex;

      // Delete preset files
      SPIFFS.remove("/preset_ntsc." + String(slotChar));
      SPIFFS.remove("/preset_pal." + String(slotChar));
      SPIFFS.remove("/preset_ntsc_480p." + String(slotChar));
      SPIFFS.remove("/preset_pal_576p." + String(slotChar));
      SPIFFS.remove("/preset_ntsc_720p." + String(slotChar));
      SPIFFS.remove("/preset_ntsc_1080p." + String(slotChar));
      SPIFFS.remove("/preset_medium_res." + String(slotChar));
      SPIFFS.remove("/preset_vga_upscale." + String(slotChar));
      SPIFFS.remove("/preset_unknown." + String(slotChar));

      // Shift slots down
      for (int j = selectionIndex; j < SLOTS_TOTAL - 1; j++) {
        LCDslotsObject.slot[j] = LCDslotsObject.slot[j + 1];
      }

      // Reset the last slot to "Empty"
      LCDslotsObject.slot[SLOTS_TOTAL - 1].slot = SLOTS_TOTAL - 1;
      LCDslotsObject.slot[SLOTS_TOTAL - 1].presetID = 0;
      LCDslotsObject.slot[SLOTS_TOTAL - 1].scanlines = 0;
      LCDslotsObject.slot[SLOTS_TOTAL - 1].scanlinesStrength = 0;
      LCDslotsObject.slot[SLOTS_TOTAL - 1].wantVdsLineFilter = false;
      LCDslotsObject.slot[SLOTS_TOTAL - 1].wantStepResponse = true;
      LCDslotsObject.slot[SLOTS_TOTAL - 1].wantPeaking = true;
      char emptyName[25] = "Empty                   ";
      strncpy(LCDslotsObject.slot[SLOTS_TOTAL - 1].name, emptyName, 25);

      // Save back to file
      File slotsBinaryFileWrite = SPIFFS.open(SLOTS_FILE, "w");
      slotsBinaryFileWrite.write((byte *)&LCDslotsObject, sizeof(LCDslotsObject));
      slotsBinaryFileWrite.close();

      // Confirm
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SLOT REMOVED!");
      delay(1500);



      //return to previous menu (options)

      LCD_resetToXMenu(RETURN_TO_OPTIONS);
      
    ////////////////////////////////////////////////////////////////////////////////
    //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
    screenSaverActive = false; 
    lastActivityTime = millis();
    ////////////////////////////////////////////////////////////////////////////////

      
      return;
    }

    delay(50); // lightweight loop delay
  }
}


void LCD_resetToXMenu(int location) //uses enum LOCATIONS for values
{
 switch(location)
    {
    case 0: LCD_menuItem = 1; LCD_subsetFrame = 0; LCD_page = 0; LCD_main_pointer = 0; LCD_selectOption = 0; LCD_pointer_count = 0; currentMenu = MAIN_MENU;               break; //load into main menu     //stay on defaults
    case 1: LCD_menuItem = 1; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0; currentMenu = MAIN_MENU;               break; //load into resolutions
    case 2: LCD_menuItem = 2; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0; currentMenu = MAIN_MENU;               break; //load into presets
    case 3: LCD_menuItem = 3; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0; currentMenu = MAIN_MENU;               break; //load into Option
    case 4: LCD_menuItem = 4; LCD_subsetFrame = 1; LCD_page = 1; LCD_main_pointer = 0; LCD_selectOption = 1; LCD_pointer_count = 0; currentMenu = MAIN_MENU;               break; //load into signal display 
    default: break;
    }

}
