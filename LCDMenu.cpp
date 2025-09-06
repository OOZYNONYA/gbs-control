#include <ESP8266WiFi.h>
#include "options.h"
#include "tv5725.h"
#include "slot.h" //only here because of SLOTS_TOTAL, which we can make instead
#include "src/WebSockets.h"
#include "src/WebSocketsServer.h"

#include "LCDMenu.h"
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
hd44780_I2Cexp lcd;       //LCD this is the member we are using through out the program to print to lcd
//LCD geometry
const int LCD_COLS = 16;  //LCD
const int LCD_ROWS = 2;   //LCD
int	status = lcd.begin(LCD_COLS, LCD_ROWS); //LCD
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
SlotMetaArray LCDslotsObject; // contains all of our saved preset names
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
GlobalSlotOptions globalOptions[MAX_SLOTS]; //stores all GLOBAL_SETTINGS_PER_USER_DATA
/////////////////////////////////////////////////////////////////////////


//LCD menu navigation strings//////////////////////////////////////////////////////////
String LCD_menu[4] =        {"RESOLUTION", "PRESET", "OPTION", "SIGNAL"};
String LCD_Resolutions[7] = {"1280x960", "1280x1024", "1280x720", "1920x1080", "480/576", "DOWNSCALE", "PASS-THROUGH"}; 
//String LCD_Presets[8] =   {"1", "2", "3", "4", "5", "6", "7", "BACK"}; //no longer in use, we display actual preset names
String LCD_Option[8] =      {"FILTER SETTINGS","GLOBAL SETTINGS","LCD SETTINGS","CREATE SLOT","REMOVE SLOT","RESET GBS", "FACTORY RESET", "BACK"}; 
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
int USE_GLOBALSET_PER_SLOT = 0;
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
 LCD_PresetLoadSelection( LCD_pointer_count,printSlotNameToLCD(LCD_pointer_count) ); //load preset 
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
        lcd_OPTIONS_pointerfunction(); //if you add a new option to this section need to update this function to handle new out of bound values
        
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
        // LCD_pointer_count = 0; //needed because the menuing system is different in LCDSET
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
//FACTORY RESET SETTINGS////////////////////////////////
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
        
        // reset all custom settings saved here.
            resetLCDSettingsToDefault(); //reset all new LCD_settings saved by user
            SPIFFS.remove(GLOBAL_OPTIONS_FILE); //remove globalsettings saved per slot file, it will create a new one on next boot
            if(FACTORY_RESET_ALL == 1) {SPIFFS.remove("/slots.bin"); }      //remove slots file for a true factory reset experience, creates a new one on next boot
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
        if (LCD_pointer_count >= 7  && LCD_selectOption == 2)  //change pointer count logic if increasing amount of Option items; >=8 etc
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
    String slotName =  getSlotName();

    // we check for powered signal coming to the GBS here, if powered signal not found we set some values to let the user know
    if(GBS::STATUS_SYNC_PROC_HSACT::read()) {rto->presetID = GBS::GBS_PRESET_ID::read();} // if sync detected
    else { rto->presetID = 0;       slotName = "NO SIGNAL";        ofr = 0; }//presetiD = 0 makes it say bypass        ofr = 0 resets HZ's values


    // Resolution
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

    // Line 2: Input Type + Hz + Sync 
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
   
    delay(300); //400 IS STABLE: this delay prevents system crashing when loading an active signal from bypass. //if it crashes, increase delay
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
              
                 delay(200);
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


void initDefault_GLOBAL_PER_SLOT(uint8_t slotIndex) {
    globalOptions[slotIndex].matchPresetSource = 1;
    globalOptions[slotIndex].wantFullHeight = 1;
    globalOptions[slotIndex].preferScalingRgbhv = 1;
    globalOptions[slotIndex].wantOutputComponent = 0;
    globalOptions[slotIndex].PalForce60 = 0;
    globalOptions[slotIndex].disableExternalClockGenerator = 0;
    globalOptions[slotIndex].enableCalibrationADC = 1;
    globalOptions[slotIndex].enableFrameTimeLock = 0;
    globalOptions[slotIndex].frameTimeLockMethod = 0;
    globalOptions[slotIndex].deintMode = 0;
    memset(globalOptions[slotIndex].reserved, 0, sizeof(globalOptions[slotIndex].reserved));
}

void load_GLOBAL_PER_SLOT() {
    File f = SPIFFS.open(GLOBAL_OPTIONS_FILE, "r");
    if (!f) {
        // first boot -> init all slots
        for (uint8_t i = 0; i < MAX_SLOTS; i++) {
            initDefault_GLOBAL_PER_SLOT(i);
        }
        save_GLOBAL_PER_SLOT();
        return;
    }

    if (f.size() != sizeof(globalOptions)) {
        // mismatch -> reset
        for (uint8_t i = 0; i < MAX_SLOTS; i++) {
            initDefault_GLOBAL_PER_SLOT(i);
        }
        save_GLOBAL_PER_SLOT();
        f.close();
        return;
    }

    f.readBytes((char*)globalOptions, sizeof(globalOptions));
    f.close();
}

void save_GLOBAL_PER_SLOT() {
    //saveUserPrefs();
    File f = SPIFFS.open(GLOBAL_OPTIONS_FILE, "w");
    if (!f) return;
    f.write((const char*)globalOptions, sizeof(globalOptions));
    f.close();
}


void apply_GLOBAL_PER_SLOT(int slotIndex) {
    

    uopt->matchPresetSource             =  globalOptions[slotIndex].matchPresetSource; 
    uopt->wantFullHeight                =  globalOptions[slotIndex].wantFullHeight;
    uopt->preferScalingRgbhv            =  globalOptions[slotIndex].preferScalingRgbhv;
    uopt->wantOutputComponent           =  globalOptions[slotIndex].wantOutputComponent;
    uopt->PalForce60                    =  globalOptions[slotIndex].PalForce60;
    uopt->disableExternalClockGenerator =  globalOptions[slotIndex].disableExternalClockGenerator;
    uopt->enableCalibrationADC          =  globalOptions[slotIndex].enableCalibrationADC;
    uopt->enableFrameTimeLock           =  globalOptions[slotIndex].enableFrameTimeLock;
    uopt->deintMode                     =  globalOptions[slotIndex].deintMode;

}




String printSlotNameToLCD(int index) //index = current lcd_pointer_count
{
               delay(LCD_REFRESHRATE);
               static int last_known_saved_slot;


                  if (index < 0) {index = 0;}
                  if (index > SLOTS_TOTAL) {index = SLOTS_TOTAL;}
               

                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("PRESET SLOT: ");

                int current_index = index; //current lcd_pointer_count sent to function
                String valueOfEmptySlotID = "Empty                   " ;  //value stored in bin for empty slots
       
                //slot is *NOT* empty or uninitialized
                if (String(LCDslotsObject.slot[current_index].name) != valueOfEmptySlotID && String(LCDslotsObject.slot[current_index].name) != "")
                {
                  last_known_saved_slot = current_index;

                  delay(10);
                  lcd.setCursor(0,1);
                  lcd.print(String(LCDslotsObject.slot[current_index].name));
                  return String(LCDslotsObject.slot[current_index].name);
               
                } 
                else //slot is empty or uninitialized //BACK
                {
                         if (digitalRead(LCD_pin_clk) == LOW) {
                         delay(10);
                         if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
                         else LCD_encoder_pos--;
                         }


                        // delay(10); 
                        lcd.setCursor(0,1);
                        lcd.print("BACK");
                        LCD_pointer_count = (last_known_saved_slot + 1); // block user from going into the vast void of 72 empty slots by keeping pointer on first non-saved-slot
                        return ("BACK");
                }
               

}




void LCD_PresetLoadSelection(int index, String slotName)
{
    delay(LCD_REFRESHRATE); 

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// how presetIDS are stored sequentially   0-26 is A-Z capital letters     27-52 is a-z lowercase letters.     53-62 is numeral 0-9     63-72 is special characters -._~()!*:,; */
const char slotCharset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcedefghijklmnopqrstuvwxyz0123456789-._~()!*:,;" ;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


     //back button functionality
    if (  slotName == "BACK" && LCD_selectOption == 2  ) //if currently on empty slot and user pressed button returns to main menu
    {
        
            LCD_page = 0;
            LCD_subsetFrame = 0;
            LCD_main_pointer = 15; //15 cursor back on presets when arriving back into main menu
            LCD_sub_pointer = 0;
            LCD_selectOption = 0;

            LCD_menuItem = 2;      //2 cursor back on presets when arriving back into main menu
            LCD_pointer_count = 0;
          //  currentMenu = MAIN_MENU;
            return;
            
    }
    //selected preset loading functionality
    else if ( LCD_selectOption == 2)
    {
            
        
            //LCD_subsetFrame = 3; 
            uopt->presetSlot =  slotCharset[index];  // we feed it the letter its looking for which correlates to the PRESET slot identifier a...b...c etc
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();

            /////////apply global_per_slot_settings///////////////////////////////////////
            int slotIndexGlobal = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
            apply_GLOBAL_PER_SLOT(slotIndexGlobal);
            //////////////////////////////////////////////////////////////////////////////

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
           // LCD_subsetFrame = 1;  //switch back to prev frame (prev screen) 
           

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
            //currentMenu = MAIN_MENU;
       /////////////////////////////////////
       }  
        else if (RESET_TO_MAIN_MENU != 1) { LCD_resetToXMenu(RETURN_TO_PRESETS);}
     
    }

}



void LCD_printSplash(int logo_select) // 0 = custom   //1 = Animated GBS[C] logo  //2 = non-animated GBS[C] logo        //logo code generated with https://chareditor.com/
{
     


if (logo_select == 0){
  //you can insert your custom splashscreen here
  //lcd.clear();
  //lcd.setCursor(0,0);
  //lcd.print("SPLASHSCREEN TEXT");
  //change LCD_SPLASH_SCREEN_TYPE  to 0 in LCDmenu.h
  //or make one with custom characters:  //https://chareditor.com/
}

else if (logo_select == 1) //animated GBS[C] logo
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

else if (logo_select == 2){

//lcd.clear();
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
else if (logo_select == 3){
}


///////////////////////////////////////////////
}//end of splash function


void LCD_LOAD_MENU_OPTIONS()
{//used at void setup() to setup LCD options and MENU selection preferences.

if(LCD_BACKLIGHT_ALWAYS_OFF == 1) {lcd.noBacklight(); }  //not sure this still belongs here...find a new home


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
       LCD_printSplash(LCD_SPLASH_SCREEN_TYPE); 
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

void LCD_USER_LCDSET()
{
    const char* LCDOption[] = {"LCD_ON/OFF","SCREENSAVER","LCD_REFRESH_RATE","STARTUP_SUBMENU","STARTUP_CURSOR", "BACK"};
    const int LCDOptionCount = 6; //option count above
    int lcd_selection = 0;
    LCD_encoder_pos = 0;


  lcd.clear();
  while(true)
{
    delay(LCD_REFRESHRATE);


    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= LCDOptionCount) LCD_encoder_pos = 5;  


    lcd.setCursor(0,0);
    lcd.print("LCD SETTINGS:   ");
    lcd.setCursor(0,1);
    lcd.print(LCDOption[LCD_encoder_pos]);
    lcd.print("                ");

    if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

   
      
      if      (strcmp(LCDOption[LCD_encoder_pos],"BACK") == 0)                {lcd_selection = 0;    LCD_resetToXMenu(RETURN_TO_OPTIONS);        lastActivityTime = millis(); return;}
      else if (strcmp(LCDOption[LCD_encoder_pos],"LCD_ON/OFF") == 0)          {lcd_selection = 1;                                                break;}
      else if (strcmp(LCDOption[LCD_encoder_pos],"SCREENSAVER") == 0)         {lcd_selection = 2;                                                break;}
      else if (strcmp(LCDOption[LCD_encoder_pos],"LCD_REFRESH_RATE") == 0)    {lcd_selection = 3;                                                break;}
      else if (strcmp(LCDOption[LCD_encoder_pos],"STARTUP_SUBMENU") == 0)     {lcd_selection = 4;                                                break;}
      else if (strcmp(LCDOption[LCD_encoder_pos],"STARTUP_CURSOR") == 0)      {lcd_selection = 5;                                                break;}
     



    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }


  }
}


   if (lcd_selection == 1){
  // ----------------------------------------------------------------
  // LCD BACKLIGHT OPTION
  // ----------------------------------------------------------------
     const char* LCDLIGHTOptions[] = { "OFF", "ON", "BACK"};
     const int optionCountLIGHT = 3; 
     
    lcd.clear();
    LCD_encoder_pos = 0;


       while (true)
   {
    delay(LCD_REFRESHRATE);


    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountLIGHT) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("LCD_ON/OFF:     ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(LCDLIGHTOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

       if      (strcmp(LCDLIGHTOptions[LCD_encoder_pos], "BACK") == 0)   {  lcd_selection = 0;            lastActivityTime = millis();                 return;}
       else if (strcmp(LCDLIGHTOptions[LCD_encoder_pos], "OFF" ) == 0)   {  LCD_BACKLIGHT_ALWAYS_OFF = 1; lcd.noBacklight(); saveLCDSettingsFile();    break;}                                                        
       else if (strcmp(LCDLIGHTOptions[LCD_encoder_pos], "ON"  ) == 0)   {  LCD_BACKLIGHT_ALWAYS_OFF = 0; lcd.backlight();   saveLCDSettingsFile();    break;}  


      

      //break;
    }


    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }

}

   else if (lcd_selection  == 2){
  // ----------------------------------------------------------------
  // SCREEN SAVER OPTION
  // ----------------------------------------------------------------
     const char* screenSaverOptions[] = {"NONE","SPLASHSCREEN","BACKLIGHTOFF","SIGNAL","TIMEOUT:30SECS","TIMEOUT:1MIN","TIMEOUT:5MIN","BACK"};
     const int optionCountScreensaver = 8; 

  
    lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);

    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountScreensaver) LCD_encoder_pos = 7;  

    lcd.setCursor(0, 0);
    lcd.print("SCREENSAVER:    ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(screenSaverOptions[LCD_encoder_pos]);
    lcd.print("                ");

 
      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(screenSaverOptions[LCD_encoder_pos], "BACK") == 0)                   {  lcd_selection == 0;                                                                                                               lastActivityTime = millis();           return;}
      else if (strcmp(screenSaverOptions[LCD_encoder_pos], "NONE" ) == 0)                  {  USE_SCREENSAVER = 0;      USE_SPLASH_SCREENSAVER      = 0;    USE_TURNOFF_LCD_SCREENSAVER = 0;   USE_SIGNAL_SCREENSAVER = 0;      saveLCDSettingsFile();    delay(100);  break;}
      else if (strcmp(screenSaverOptions[LCD_encoder_pos], "SPLASHSCREEN"  ) == 0)         {  USE_SCREENSAVER = 1;      USE_SPLASH_SCREENSAVER      = 1;    USE_SIGNAL_SCREENSAVER      = 0;                                    saveLCDSettingsFile();    delay(100);  break;}
      else if (strcmp(screenSaverOptions[LCD_encoder_pos], "BACKLIGHTOFF" ) == 0)          {  USE_SCREENSAVER = 1;      USE_TURNOFF_LCD_SCREENSAVER = 1;                                                                        saveLCDSettingsFile();    delay(100);  break;}
      else if (strcmp(screenSaverOptions[LCD_encoder_pos], "SIGNAL"  ) == 0)               {  USE_SCREENSAVER = 1;      USE_SIGNAL_SCREENSAVER      = 1;    USE_SPLASH_SCREENSAVER      = 0;                                    saveLCDSettingsFile();    delay(100);  break;}
      else if (strcmp(screenSaverOptions[LCD_encoder_pos], "TIMEOUT:30SECS" ) == 0)        {  SCREEN_TIMEOUT = 30000;                                                                                                           saveLCDSettingsFile();    delay(100);  break;}
      else if (strcmp(screenSaverOptions[LCD_encoder_pos], "TIMEOUT:1MIN"  ) == 0)         {  SCREEN_TIMEOUT = 60000;                                                                                                           saveLCDSettingsFile();    delay(100);  break;}
      else if (strcmp(screenSaverOptions[LCD_encoder_pos], "TIMEOUT:5MIN" ) == 0)          {  SCREEN_TIMEOUT = 300000;                                                                                                          saveLCDSettingsFile();    delay(100);  break;}
    
     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
   }
  else  if (lcd_selection  == 3){
  // ----------------------------------------------------------------
  // LCD REFRESHRATE OPTION
  // ----------------------------------------------------------------
     const char* refreshRateOptions[] = {"100","200","300","400","500","BACK"};
     const int optionCountRefresh = 6; 
  
   lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);


    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountRefresh) LCD_encoder_pos = 5;  

    lcd.setCursor(0, 0);
    lcd.print("LCD_REFRESHRATE:");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(refreshRateOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

      if      (strcmp(refreshRateOptions[LCD_encoder_pos], "BACK") == 0)   {  lcd_selection ==  0;   lastActivityTime = millis();                                       return;}
      else if (strcmp(refreshRateOptions[LCD_encoder_pos], "100" ) == 0)   {  LCD_REFRESHRATE = 100; saveLCDSettingsFile();                                 delay(100);  break;}
      else if (strcmp(refreshRateOptions[LCD_encoder_pos], "200" ) == 0)   {  LCD_REFRESHRATE = 200; saveLCDSettingsFile();                                 delay(100);  break;}
      else if (strcmp(refreshRateOptions[LCD_encoder_pos], "300" ) == 0)   {  LCD_REFRESHRATE = 300; saveLCDSettingsFile();                                 delay(100);  break;}
      else if (strcmp(refreshRateOptions[LCD_encoder_pos], "400" ) == 0)   {  LCD_REFRESHRATE = 400; saveLCDSettingsFile();                                 delay(100);  break;}
      else if (strcmp(refreshRateOptions[LCD_encoder_pos], "500" ) == 0)   {  LCD_REFRESHRATE = 500; saveLCDSettingsFile();                                 delay(100);  break;}

      //cant just put  saveLCDSettingsFile(); here instead of 5 times above  because break; exits the while loop scope we're in
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}
    else if (lcd_selection  == 4){
 // ----------------------------------------------------------------
  // STARTUP_SUBMENU OPTION
  // ----------------------------------------------------------------
     const char* subMenuOptions[] = { "MAIN","RESOLUTIONS","PRESETS","OPTIONS","SIGNAL","BACK"};
     const int optionCountSubMenu = 6; 
  
   lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);

    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountSubMenu) LCD_encoder_pos = 5;  

    lcd.setCursor(0, 0);
    lcd.print("STARTUP_SUBMENU:");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(subMenuOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(subMenuOptions[LCD_encoder_pos],"BACK") == 0)           { lcd_selection == 0;              lastActivityTime = millis();                                                                                     return;}
      else if (strcmp(subMenuOptions[LCD_encoder_pos], "MAIN" ) == 0)         { LOAD_DIRECTLY_INTO_SUB_MENU = 0; CURSOR_ON_MENU_AT_STARTUP = 0; saveLCDSettingsFile();                                                delay(100);  break;}
      else if (strcmp(subMenuOptions[LCD_encoder_pos], "RESOLUTIONS"  ) == 0) { LOAD_DIRECTLY_INTO_SUB_MENU = 1; CURSOR_ON_MENU_AT_STARTUP = 0; saveLCDSettingsFile();                                                delay(100);  break;}
      else if (strcmp(subMenuOptions[LCD_encoder_pos], "PRESETS" ) == 0)      { LOAD_DIRECTLY_INTO_SUB_MENU = 2; CURSOR_ON_MENU_AT_STARTUP = 0; saveLCDSettingsFile();                                                delay(100);  break;}
      else if (strcmp(subMenuOptions[LCD_encoder_pos], "OPTIONS"  ) == 0)     { LOAD_DIRECTLY_INTO_SUB_MENU = 3; CURSOR_ON_MENU_AT_STARTUP = 0; saveLCDSettingsFile();                                                delay(100);  break;}
      else if (strcmp(subMenuOptions[LCD_encoder_pos], "SIGNAL" ) == 0)       { LOAD_DIRECTLY_INTO_SUB_MENU = 4; CURSOR_ON_MENU_AT_STARTUP = 0; saveLCDSettingsFile();                                                delay(100);  break;}

                   // CURSOR_ON_MENU_AT_STARTUP = 0; //make sure we dont have both enabled, just in case that causes issues;
                    

     
      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}

    else if (lcd_selection  == 5){
 // ----------------------------------------------------------------
  // STARTUP_CURSOR OPTION
  // ----------------------------------------------------------------
     const char* cursorMenuOptions[] = {"MAIN","RESOLUTIONS","PRESETS","OPTIONS","SIGNAL","BACK"};
     const int optionCountCursor = 6; 
  
   lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);

    if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
    if (LCD_encoder_pos >= optionCountCursor) LCD_encoder_pos = 5;  

    lcd.setCursor(0, 0);
    lcd.print("STARTUP_CURSOR: ");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print( cursorMenuOptions[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp( cursorMenuOptions[LCD_encoder_pos], "BACK") == 0)          { lcd_selection == 0;            lastActivityTime = millis();                                                                         return;}
      else if (strcmp( cursorMenuOptions[LCD_encoder_pos], "MAIN" ) == 0)         { CURSOR_ON_MENU_AT_STARTUP = 0; LOAD_DIRECTLY_INTO_SUB_MENU = 0; saveLCDSettingsFile();                                  delay(100);  break;}
      else if (strcmp( cursorMenuOptions[LCD_encoder_pos], "RESOLUTIONS"  ) == 0) { CURSOR_ON_MENU_AT_STARTUP = 1; LOAD_DIRECTLY_INTO_SUB_MENU = 0; saveLCDSettingsFile();                                  delay(100);  break;}
      else if (strcmp( cursorMenuOptions[LCD_encoder_pos], "PRESETS" ) == 0)      { CURSOR_ON_MENU_AT_STARTUP = 2; LOAD_DIRECTLY_INTO_SUB_MENU = 0; saveLCDSettingsFile();                                  delay(100);  break;}
      else if (strcmp( cursorMenuOptions[LCD_encoder_pos], "OPTIONS"  ) == 0)     { CURSOR_ON_MENU_AT_STARTUP = 3; LOAD_DIRECTLY_INTO_SUB_MENU = 0; saveLCDSettingsFile();                                  delay(100);  break;}
      else if (strcmp( cursorMenuOptions[LCD_encoder_pos], "SIGNAL" ) == 0)       { CURSOR_ON_MENU_AT_STARTUP = 4; LOAD_DIRECTLY_INTO_SUB_MENU = 0; saveLCDSettingsFile();                                  delay(100);  break;}

     
                   //LOAD_DIRECTLY_INTO_SUB_MENU = 0; //make sure we dont have both enabled, just in case that causes issues;
                  


      //break;
    }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
}
  

    if(lcd_selection != 0) //if we press back don't say saved.
    {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SAVED!");
    delay(200);
    }
  
   LCD_encoder_pos = 0;
   LCD_resetToXMenu(RETURN_TO_OPTIONS);
 

    ////////////////////////////////////////////////////////////////////////////////
    //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
    lastActivityTime = millis();
    ////////////////////////////////////////////////////////////////////////////////
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
    USE_GLOBALSET_PER_SLOT = file.readStringUntil('\n').toInt();

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
    file.println(USE_GLOBALSET_PER_SLOT);
     
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
     LCD_REFRESHRATE             = 200;             //200 recommended
     LOAD_DIRECTLY_INTO_SUB_MENU = 0;
     CURSOR_ON_MENU_AT_STARTUP   = 0;
     SCREEN_TIMEOUT              = 60000;           //default 60000 (1MIN), 10 seconds is 10000
     USE_GLOBALSET_PER_SLOT      = 0;

    // Save defaults to SPIFFS
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
    file.println(USE_GLOBALSET_PER_SLOT);


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
    file.println("0");    // USE_GLOBALSET_PER_SLOT

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

// we use this incase the user starts up the LCD menu before ever starting the webapp (which makes the empty slots/initializes them on first startup of the webapp)???.
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

   
      
      if      (strcmp(Filter[LCD_encoder_pos],"BACK") == 0)               {filter_selection = 0;    LCD_resetToXMenu(RETURN_TO_OPTIONS);        lastActivityTime = millis();      return;} 
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

       if      (strcmp(ScanlinesOptions[LCD_encoder_pos], "BACK") == 0)   {  filter_selection = 0;        lastActivityTime = millis();        return;}
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

       if      (strcmp(LineFilterOptions[LCD_encoder_pos], "BACK") == 0)   {  filter_selection = 0;        lastActivityTime = millis();                                   return;}
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

       if      (strcmp(PeakingOptions[LCD_encoder_pos], "BACK") == 0)   {  filter_selection  = 0;  lastActivityTime = millis();                                  return;}
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

       if      (strcmp(StepResponseOptions[LCD_encoder_pos], "BACK") == 0)   { filter_selection = 0;        lastActivityTime = millis();                                  return;}
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
   LCD_resetToXMenu(RETURN_TO_OPTIONS); 
 

    ////////////////////////////////////////////////////////////////////////////////
    //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
    lastActivityTime = millis();
    ////////////////////////////////////////////////////////////////////////////////
}




void LCD_USER_GlobalSET() {

  const int optionCount = 3; //OFF ON BACK count //used in most sections
  const int globalCount = 11;//count of globalsettings array strings
  const char* Global[] = {"FULL HEIGHT","LOWRES UPSCALING","FRAMETIME LOCK","DEINTERLACE MODE","OUTPUT RGBHV","ADC CALIBRATION","MATCHED PRESET","DISABLE GENLOCK","FORCE PAL60HZ","SAVE_PER_SLOT?","BACK"};
  int global_selection = 0; //global selection = whichever Global[] string we press-in rotary button on
  LCD_encoder_pos = 0;


  lcd.clear();
  while(true)
{
    delay(LCD_REFRESHRATE);

       if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= globalCount) LCD_encoder_pos = 10;


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
      else if (strcmp(Global[LCD_encoder_pos],"SAVE_PER_SLOT?") == 0)     {global_selection = 10;                                               break;}



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

          if(USE_GLOBALSET_PER_SLOT == 0)
          {
                     if  (strcmp(matchedPresetsOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
                else if (strcmp(matchedPresetsOptions[LCD_encoder_pos], "OFF") == 0)   { uopt->matchPresetSource = 0;  saveUserPrefs();  delay(100);   break;}
                else if (strcmp(matchedPresetsOptions[LCD_encoder_pos], "ON") == 0)    { uopt->matchPresetSource = 1;  saveUserPrefs();  delay(100);   break;}
          }
          else if (USE_GLOBALSET_PER_SLOT == 1)
           {

                    if  (strcmp(matchedPresetsOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
               else if (strcmp(matchedPresetsOptions[LCD_encoder_pos], "OFF") == 0)
                {
                  uopt->matchPresetSource = 0;
                  int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                  globalOptions[slotIndex].matchPresetSource = 0; 
                  save_GLOBAL_PER_SLOT(); 
                  delay(100); 
                  break;
                }
               else if (strcmp(matchedPresetsOptions[LCD_encoder_pos], "ON") == 0)
                {
                uopt->matchPresetSource = 1;
                int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                globalOptions[slotIndex].matchPresetSource = 1; 
                save_GLOBAL_PER_SLOT(); 
                delay(100); 
                break;
                }
           }
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

          if(USE_GLOBALSET_PER_SLOT == 0)
          {
                    if (strcmp(ADC_CAL_Options[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                               break;}
               else if (strcmp(ADC_CAL_Options[LCD_encoder_pos], "OFF") == 0) { uopt->enableCalibrationADC = 0;  saveUserPrefs();       delay(100); break;}
               else if (strcmp(ADC_CAL_Options[LCD_encoder_pos], "ON") == 0)  { uopt->enableCalibrationADC = 1;  saveUserPrefs();       delay(100); break;}
          }
          else if (USE_GLOBALSET_PER_SLOT == 1)
          {
                    if  (strcmp(ADC_CAL_Options[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
               else if (strcmp(ADC_CAL_Options[LCD_encoder_pos], "OFF") == 0)
                {
                  uopt->enableCalibrationADC = 0;
                  int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                  globalOptions[slotIndex].enableCalibrationADC = 0; 
                  save_GLOBAL_PER_SLOT(); 
                  delay(100); 
                  break;

                }
               else if (strcmp(ADC_CAL_Options[LCD_encoder_pos], "ON") == 0)
               {
                 uopt->enableCalibrationADC = 1;
                 int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                 globalOptions[slotIndex].enableCalibrationADC = 1; 
                 save_GLOBAL_PER_SLOT(); 
                // saveUserPrefs(); // see if these can work together or not
                 delay(100); 
                 break;
               }
        }
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

     if(USE_GLOBALSET_PER_SLOT == 0)
          {
                      if (strcmp(fullHeightOptions[LCD_encoder_pos], "BACK") == 0){  global_selection = 0;                                    break;}
                 else if (strcmp(fullHeightOptions[LCD_encoder_pos], "OFF") == 0) {  uopt->wantFullHeight = 0;  saveUserPrefs();  delay(100); break;}
                 else if (strcmp(fullHeightOptions[LCD_encoder_pos], "ON") == 0)  {  uopt->wantFullHeight = 1;  saveUserPrefs();  delay(100); break;}
          }
     else if (USE_GLOBALSET_PER_SLOT == 1)
          {
                     if  (strcmp(fullHeightOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                     break;}
                else if (strcmp(fullHeightOptions[LCD_encoder_pos], "OFF") == 0)
                 {
                    uopt->wantFullHeight = 0;
                    int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                    globalOptions[slotIndex].wantFullHeight = 0; 
                    save_GLOBAL_PER_SLOT(); 
                    delay(100); 
                    break;
                   }
                else if (strcmp(fullHeightOptions[LCD_encoder_pos], "ON") == 0)
                {
                   uopt->wantFullHeight = 1;
                   int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                   globalOptions[slotIndex].wantFullHeight = 1; 
                   save_GLOBAL_PER_SLOT(); 
                   delay(100); 
                   break;
                 }
          }
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

          if(USE_GLOBALSET_PER_SLOT == 0)
          {
                     if (strcmp( upscalingOptions[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                       break;}
                else if (strcmp( upscalingOptions[LCD_encoder_pos], "OFF") == 0) { uopt->preferScalingRgbhv = 0;  saveUserPrefs(); delay(100); break;}
                else if (strcmp( upscalingOptions[LCD_encoder_pos], "ON") == 0)  { uopt->preferScalingRgbhv = 1;  saveUserPrefs(); delay(100); break;}
          }
          else if (USE_GLOBALSET_PER_SLOT == 1)
          {
                     if  (strcmp(upscalingOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
                else if (strcmp(upscalingOptions[LCD_encoder_pos], "OFF") == 0)
                {
                   uopt->preferScalingRgbhv = 0;
                   int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                   globalOptions[slotIndex].preferScalingRgbhv = 0; 
                   save_GLOBAL_PER_SLOT(); 
                   delay(100); 
                  break;
                }
                else if (strcmp(upscalingOptions[LCD_encoder_pos], "ON") == 0)
               {
                  uopt->preferScalingRgbhv = 1;
                  int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                  globalOptions[slotIndex].preferScalingRgbhv = 1; 
                  save_GLOBAL_PER_SLOT(); 
                  break;
                }
          }
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

          if(USE_GLOBALSET_PER_SLOT == 0)
          {
                      if (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                        break;}
                 else if (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "OFF") == 0) { uopt->wantOutputComponent = 0;  saveUserPrefs(); delay(100); break;}
                 else if (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "ON") == 0)  { uopt->wantOutputComponent = 1;  saveUserPrefs(); delay(100); break;}
          }
          else if (USE_GLOBALSET_PER_SLOT == 1)
          {
                      if  (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
                 else if (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "OFF") == 0)
                 {
                     uopt->wantOutputComponent = 0;
                    int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                     globalOptions[slotIndex].wantOutputComponent = 0; 
                     save_GLOBAL_PER_SLOT(); 
                     delay(100); 
                    break;

                 }
                 else if (strcmp(RGBHVCOMPOptions[LCD_encoder_pos], "ON") == 0)
                  {
                     uopt->wantOutputComponent = 1;
                     int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                     globalOptions[slotIndex].wantOutputComponent = 1; 
                     save_GLOBAL_PER_SLOT(); 
                     break;
                  }
          }
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

          if(USE_GLOBALSET_PER_SLOT == 0)
          {
                      if (strcmp(forcePALOptions[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                break;}
                 else if (strcmp(forcePALOptions[LCD_encoder_pos], "OFF") == 0) {  uopt->PalForce60 = 0;  saveUserPrefs(); delay(100); break;}
                 else if (strcmp(forcePALOptions[LCD_encoder_pos], "ON") == 0)  {  uopt->PalForce60 = 1;  saveUserPrefs(); delay(100); break;}
          }
          else if (USE_GLOBALSET_PER_SLOT == 1)
          {
                      if  (strcmp(forcePALOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
                 else if (strcmp(forcePALOptions[LCD_encoder_pos], "OFF") == 0)
                 {
                  uopt->PalForce60 = 0;
                  int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                  globalOptions[slotIndex].PalForce60 = 0; 
                  save_GLOBAL_PER_SLOT(); 
                  delay(100); 
                  break;

                 }
                else if (strcmp(forcePALOptions[LCD_encoder_pos], "ON") == 0)
                 {
                  uopt->PalForce60 = 1;
                  int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                  globalOptions[slotIndex].PalForce60 = 1; 
                  save_GLOBAL_PER_SLOT(); 
                 break;
                }
          }
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

           if(USE_GLOBALSET_PER_SLOT == 0)
          {
                     if (strcmp( DisableClockgenOptions[LCD_encoder_pos], "BACK") == 0){global_selection = 0;                                                  break;}
                else if (strcmp( DisableClockgenOptions[LCD_encoder_pos], "OFF") == 0) {uopt->disableExternalClockGenerator = 0;  saveUserPrefs(); delay(100); break;}
                else if (strcmp( DisableClockgenOptions[LCD_encoder_pos], "ON") == 0)  {uopt->disableExternalClockGenerator = 1;  saveUserPrefs(); delay(100); break;}
          }
          else if (USE_GLOBALSET_PER_SLOT == 1)
          {
                     if  (strcmp(DisableClockgenOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
                else if (strcmp(DisableClockgenOptions[LCD_encoder_pos], "OFF") == 0)
               {
                   uopt->disableExternalClockGenerator = 0;
                   int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                   globalOptions[slotIndex].disableExternalClockGenerator = 0; 
                   save_GLOBAL_PER_SLOT(); 
                   delay(100); 
                   break;
                   }
                else if (strcmp(DisableClockgenOptions[LCD_encoder_pos], "ON") == 0)
               {
                   uopt->disableExternalClockGenerator = 1;
                   int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                   globalOptions[slotIndex].disableExternalClockGenerator = 1; 
                   save_GLOBAL_PER_SLOT(); 
                   break;
               }
          }
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

           if(USE_GLOBALSET_PER_SLOT == 0)
          {
                     if (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                        break;}
                else if (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "OFF") == 0) { uopt->enableFrameTimeLock = 0;  saveUserPrefs(); delay(100); break;}
                else if (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "ON") == 0)  { uopt->enableFrameTimeLock = 1;  saveUserPrefs(); delay(100); break;}
          }
          else if (USE_GLOBALSET_PER_SLOT == 1)
          {
                    if  (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
               else if (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "OFF") == 0)
          {
                   uopt->enableFrameTimeLock = 0;
                   int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                   globalOptions[slotIndex].enableFrameTimeLock = 0; 
                   save_GLOBAL_PER_SLOT(); 
                   delay(100); 
                 break;

          }
               else if (strcmp(FrameTimeLockOptions[LCD_encoder_pos], "ON") == 0)
               {
                uopt->enableFrameTimeLock = 1;
                 int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                 globalOptions[slotIndex].enableFrameTimeLock = 1; 
                 save_GLOBAL_PER_SLOT(); 
                 delay(100); 
                break;
               }
          }
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

           if(USE_GLOBALSET_PER_SLOT == 0)
          {
                    if (strcmp(deinterlaceOptions[LCD_encoder_pos], "BACK") == 0)            { global_selection = 0;                              break;}
               else if (strcmp(deinterlaceOptions[LCD_encoder_pos], "MOTION ADAPTIVE") == 0) { uopt->deintMode = 0;  saveUserPrefs(); delay(100); break;}
               else if (strcmp(deinterlaceOptions[LCD_encoder_pos], "BOB") == 0)             { uopt->deintMode = 1;  saveUserPrefs(); delay(100); break;}
          }
          else if (USE_GLOBALSET_PER_SLOT == 1)
          {
                    if (strcmp(deinterlaceOptions[LCD_encoder_pos], "BACK") == 0) { global_selection = 0;                                         break;}
               else if (strcmp(deinterlaceOptions[LCD_encoder_pos], "MOTION ADAPTIVE") == 0)
                {
                  uopt->deintMode = 0;
                  int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot); 
                  globalOptions[slotIndex].deintMode = 0; 
                  save_GLOBAL_PER_SLOT(); 
                  delay(100); 
                  break;

                 }
               else if (strcmp(deinterlaceOptions[LCD_encoder_pos], "BOB") == 0)
               {
                 uopt->deintMode = 1;
                  int slotIndex = getCurrentSlotIndexFromPresetSlotChar(uopt->presetSlot);
                  globalOptions[slotIndex].deintMode = 1; 
                  save_GLOBAL_PER_SLOT(); 
                  delay(100); 
                  break;
                }
          }
      //break;
       }

    if (digitalRead(LCD_pin_clk) == LOW) {
      delay(50);
      if (digitalRead(LCD_pin_data) == HIGH) LCD_encoder_pos++;
      else LCD_encoder_pos--;
    }

   }
  }


else if(global_selection == 10){
  // ----------------------------------------------------------------
  // SAVE GLOBAL SETTINGS PER USER SLOT OPTION
  // ----------------------------------------------------------------
    const char* GLOBALSAVEPERSLOT_Options[] = { "OFF", "ON", "BACK"}; //add reset defaults?

    lcd.clear();
    LCD_encoder_pos = 0;

       while (true)
   {
    delay(LCD_REFRESHRATE);

       if (LCD_encoder_pos <= 0) LCD_encoder_pos = 0; 
       if (LCD_encoder_pos >= optionCount) LCD_encoder_pos = 2;  

    lcd.setCursor(0, 0);
    lcd.print("SAVE_PER_SLOT?:");

    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(GLOBALSAVEPERSLOT_Options[LCD_encoder_pos]);
    lcd.print("                ");


      if (digitalRead(LCD_pin_switch) == LOW) {
      while (digitalRead(LCD_pin_switch) == LOW) { delay(10); }

           if (strcmp(GLOBALSAVEPERSLOT_Options[LCD_encoder_pos], "BACK") == 0){ global_selection = 0;                                                    break;}
      else if (strcmp(GLOBALSAVEPERSLOT_Options[LCD_encoder_pos], "OFF") == 0) { USE_GLOBALSET_PER_SLOT  = 0; saveLCDSettingsFile();          delay(100); break;}
      else if (strcmp(GLOBALSAVEPERSLOT_Options[LCD_encoder_pos], "ON") == 0)  { USE_GLOBALSET_PER_SLOT  = 1; saveLCDSettingsFile();          delay(100); break;}

     
     
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

   ////////////////////////////////////////////////////////////////////////
   //apply video settings instantly
          uint8_t videoMode = getVideoMode();
          if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read()) {
           videoMode = rto->videoStandardInput;
          }
          applyPresets(videoMode);
   ////////////////////////////////////////////////////////////////////////


    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SAVED!");
    delay(200);
    }
  

    ////////////////////////////////////////////////////////////////////////////////
    //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
    lastActivityTime = millis();
    ////////////////////////////////////////////////////////////////////////////////




}






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

      const char* selected_character = alphabet[LCD_encoder_pos];

      if (strcmp(selected_character, "BACKSPACE") == 0) { // == 0 means compared strings are equal
        // Remove last character
        if (namePos > 0) {
           
             newSlotName[namePos] = '\0'; //deletes current char in the array and makes it the end of line
            --namePos; //instantly go to previous position
             newSlotName[namePos] = ' '; //change it to blank so we see a change
          
         
        }
      }
      else if (strcmp(selected_character, "EXIT") == 0) {

       //abort name creation and return to previous menu (options)
       LCD_resetToXMenu(RETURN_TO_OPTIONS);
       lastActivityTime = millis();
        return;
      }
      else if (strcmp(selected_character, "CONFIRM") == 0) { //name confirmed, create slot
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

                    initDefault_GLOBAL_PER_SLOT(i);

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
          newSlotName[namePos++] = selected_character[0];
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
// LOAD NEWLY CREATED PRESET SLOT 
    for (int i = 0; i < SLOTS_TOTAL; i++)
    {
         if(strcmp(LCDslotsObject.slot[i].name, newSlotName) == 0)
         {
            uopt->presetSlot = LCDslotsObject.slot[i].presetID;
     
             //uopt->presetPreference = OutputCustomized;
             saveUserPrefs();
             delay(100);
             savePresetToSPIFFS();
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

  // If no slots saved/exist, go straight BACK
  if (lastValidIndex < 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NO SLOTS!");
    delay(1500);
      
    LCD_resetToXMenu(RETURN_TO_OPTIONS);
    lastActivityTime = millis();
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
         lastActivityTime = millis(); //screensaver reset
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



       //globalsettingsperslot changes//
      for (int j = selectionIndex; j < SLOTS_TOTAL - 1; j++) {
        globalOptions[j] = globalOptions[j + 1];
      }
      initDefault_GLOBAL_PER_SLOT(SLOTS_TOTAL - 1);
      save_GLOBAL_PER_SLOT();
      //////////////////////////////////

      // Confirm
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SLOT REMOVED!");
      delay(1500);


      LCD_resetToXMenu(RETURN_TO_OPTIONS);
      
    ////////////////////////////////////////////////////////////////////////////////
    //TEMP FIX FOR SCREEN SAVER INSTANTLY STARTING ONCE WE LEAVE X_MENU FUNCTIONS 
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



