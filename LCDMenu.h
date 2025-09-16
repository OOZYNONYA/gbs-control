#ifndef LCD_MENU_H
#define LCD_MENU_H



/////////////////////////////////////////////////////////MANUAL SETTINGS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define NO_ENCODER_MODE        0       // useful for people with no rotary dial to see what resolution/signal/slot they are currently working with (instantly updates when loading preset using webapp/turning power off of console  (if realtime updating is enabled))
                                       // 0 = NO ENCODER MODE SET TO OFF
                                       // 1 = NO ENCODER MODE SET TO ON
                                       // 2 = NO ENCODER MODE SET TO ON AND LCD BACKLIGHT WILL BE TURNED OFF     

#define USE_REALTIME_UPDATING  1       // 0 = you will need to restart the GBS to see changes with saved slot names and such.  
                                       // 1 = when you make or change a preset name your changes will instantly appear in the menu 
                                       
    
#define RESET_TO_MAIN_MENU     1       // 0 = do not reset to main menu from resolutions/presets/ after selecting an option  (stay inside menu after until back is selected)
                                       // 1 = reset to main menu        from resolutions/presets/ after selecting an option
                                       // USE CASE EXAMPLE: if you want to continuously cycle through resolutions/presets without having to manually re-enter the menu every single time.


                                       //WARNING: do not enable without backing up your current slots file in the web application                                  
#define FACTORY_RESET_ALL      0       //if enabled: all saved PRESET SLOTS will be deleted with "FACTORY RESET" of system. by default: factory reset just resets global settings,LCD settings,filter settings
                                       // 0 = disabled
                                       // 1 = enabled


#define SIGNAL_SCROLL_SPEED    5000    //control the speed of the scrolling text in SIGNAL mode. displays slotname for x_amount_of_time, scrolls, then displays resolution for x_amount_of_time
                                       // 1000  = 1 second,
                                       // 10000 = 10 SECONDS 

#define SIGNAL_DISPLAY_ONLY_SLOTNAME 0 //only display current loaded SLOT NAME on TOP-TEXT of SIGNAL, NO scrolling text or resolution displayed. BOTTOM-TEXT displays as normal.
                                       // 0 = disabled                                                                        
                                       // 1 = enabled

//if you want to make your own screensaver splash go to " void LCD_printSplash(int logo_select) " in the LCDMenu.cpp file.
//and use the link listed there to generate your own splash screen to use with this! just replace whats there and add nothing more. 
//or just make your own custom version however you want with lcd.print("example");
#define LCD_SPLASH_SCREEN_TYPE 1       // 0 = add in your own custom text in LCD_printSplash() function (located in LCDMenu.cpp)
                                       // 1 = display ANIMATED "GBS[C]" splashscreen
                                       // 2 = display NON-animated "GBS[C]" splashscreen


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//EXPERIMENTAL/TESTING: 
#define SAVE_RESOLUTION_PER_SLOT   1      //currently LCDMENU >RESOLUTIONS only loads a resolution temporarily, with this enabled, it will save resolution and load resolution PER SLOT but with some caveats:                                
                                          //gbsc must already be connected to any powered signal before saving new resolution to file,
                                          // it also will not update in WEB APPLICATION what current saved resolution is under preset slot name (will just say last webapp saved resolution/"custom")
                                          // 0 = disabled
                                          // 1 = enabled         
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//EXPERIMENTAL/TESTING:                   //TEMPORARY FIX
#define RESET_ON_BYPASS            1      //currently when loading a bypass resolution preset, it takes a long time to load, and it gets stuck on passthrough/bypass when switching off to a normal resolution preset
                                          //enabling this makes it so gbscontrol restarts when you load a bypass preset, or try to switch to a normal preset while currently on a bypass preset
                                          //after you switch to a normal preset, it will no longer restart each time until you once again load a bypass preset
                                          // 0 = disabled
                                          // 1 = enabled
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////USER SELECT OPTIONS INFO////////////////////////////////////////////////////    
//***************** A BRIEF DESCRIPTION OF WHAT EACH ITEM DOES IN THE MENU:**************************************************************************************************************************************************************

////////////////////////////////////////////////////////////////////////////LCD SETTINGS////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//LCD_REFRESHRATE                         //choose best suited value (100-200) recommended // do not set lower than 100 or you might start having system crashes. if you have system crashes with 100, try increasing the value higher
                                          //there are refreshrate delays in every menu that use this variable
                                          //refreshrate also affects encoder press-button response. higher refresh rates you may need to hold the button-press longer.. 100 = almost instant press,  200 = half a second press etc...
                                          //if the values given in menu don't work for you, you can simply change the values in this array in LCDMenu.cpp. No other changes needed in code.   // String LCD_SubOptions_REFRESHRATE[7] = {"100","200","300","400","500","1000","BACK"};  

//LCD_BACKLIGHT_ALWAYS_OFF                //lcd backlight is always off, otherwise its always ON unless turned off with screensaver mode


//Screensaver options menu                   (meant to work only inside of mainmenu/resolutions/presets/signal/)
//SCREENSAVER  "NONE"                     // No screensavers will be used and all screensaver settings reset to 0 
//USE_SPLASH_SCREENSAVER                  // when this screensaver is enabled, a splash screen will display on the LCD and halt main menu loop while user inactive on menu  //current disabled in SIGNAL because the whole point of signal is to see a live feed of current loaded signal
//USE_SIGNAL_SCREENSAVER                  // when this screensaver is enabled, the SIGNAL category will be loaded on screen and have realtime updating (if enabled) until you wake it up, at which point you will resume your current state. 
//USE_LCD_TURNOFF_SCREENSAVER             // when this screensaver is enabled, the backlight of your LCD will turn off and halt main menu loop while user inactive on menu 
                                          // its possible to select both NO BACKLIGHT SCREENSAVER AND ANY OTHER SCREEN SAVER; it will turn off LCD backlight and display screen in the dark and halt menu
// SCREENSAVER_TIMEOUT                    // Screensaver enables after 30seconds/1minutes/5minutes until user moves encoder knob around a bit


//LOAD_DIRECTLY_INTO_SUB_MENU    
//load directly into sub menu of choice after splash screen startup (useful if you ever only use preset selection or signal)
//0 default(MAIN MENU)   //1 RESOLUTION //2 PRESET //3 OPTION //4 SIGNAL                        

//CURSOR_ON_MENU_AT_STARTUP     
//have the encoder cursor selector already on the menu you are most likely to use at startup //i.e > Presets
//0 default(resolutions) //1 RESOLUTION //2 PRESET //3 OPTION //4 SIGNAL    


//////////////////////////////////////////////////////////////////////////////OPTIONS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////                        
//FILTER SETTINGS                           //same as webapp descriptions  (scanlines,line filter,step response)                                 
//GLOBAL SETTINGS                           //same as webapp descriptions  (full height,output RGBHV/comp,adc calibration,matched presets, etc.......)  
                                            //save_per_slot?               (save all globalsettings(preferences) per saved slot. loading a preset loads the saved globalsettings for that slot)  

//CREATE SLOT                               //use rotary to select a letter, press rotary button, then choose next letter. press confirm when done selecting name. exit = cancel slot creation.
//REMOVE SLOT                               //choose from an already created slot list to permanently remove a slot
//WARNING:                                   Backup your current GBSCONTROL file in the webapp before using these. they may not be 100% safe.

//RESET GBS                                 //restarts GBS
//FACTORY RESTORE                           //deviation from original code: adds abort ability within 30 seconds (press button), resets LCDSETTINGS/GLOBALSETTINGS (and slots if option is enabled)


///////////////////////////////////////////////////////////////////////////////MENU////////////////////////////////////////////////////////////////////
//SIGNAL                                    //scrolls and displays current loaded preset slot/current loaded resolution.
                                            //if power to signal source is lost, instantly displays on LCD "NO SIGNAL"
                                            //if realtimeupdating is enabled, instantly see new presets data that was loaded

//************************************************************************************************************************************************************************************************************************************************************************************************************************










//////////////////////////////////////////////////////////////
//functions that change settings based on user input with rotary encoder
void LCD_USER_MAIN_MENU();     // main menu function
void LCD_USER_LCDSET();        // lcdsettings menu
void LCD_USER_GlobalSET();     // global settings menu
void LCD_USER_FilterSET();     // filter settings menu
void LCD_USER_createNewSlot(); // create new slot menu
void LCD_USER_removeSlot();    // remove slot menu
/////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//screensaver related functions
void LCD_screenSaver();
bool userActivity();
void wakeUpLCD();
void enterScreenSaver();
void LCD_printSplash(int logo_select);
void LCD_scrollResolutionAndSlot(const String &resolution, const String &slotName);
//////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//menu location and navigation functions
void LCD_resetToXMenu(int location);
void LCD_LOAD_MENU_OPTIONS();

void lcd_subpointerfunction();
void lcd_pointerfunction();
void lcd_OPTIONS_pointerfunction();
void ICACHE_RAM_ATTR isrRotaryEncoder();
//////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////
//slot creation/editing/getting functions
String getSlotName();
String printSlotNameToLCD(int index);
void LCD_PresetLoadSelection(int index, String slotName);
int getCurrentSlotIndexFromPresetSlotChar(char slotChar); 
void LCD_Load_PresetIDs_Into_slotobject();
//////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//file creation and editing functions
void createDefaultLCDSettingsFile();
void loadLCDSettingsFile();
void saveLCDSettingsFile();
void resetLCDSettingsToDefault();
void createDefaultSlotsIfMissing();
//////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////
enum LCDMenuState {
    MAIN_MENU,
    FILTER_MENU,
    GLOBAL_MENU,
    LCDSET_MENU,
    CREATESLOT_MENU,
    REMOVESLOT_MENU
};

enum LOCATIONS { //for use with resettoXmenu
    RETURN_TO_MAIN,
    RETURN_TO_RESOLUTIONS,
    RETURN_TO_PRESETS,
    RETURN_TO_OPTIONS,
    RETURN_TO_SIGNAL
};
//////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////////////
//GLOBAL PER SLOT SAVING RELATED FUNCTIONS/ETC
#define GLOBAL_OPTIONS_FILE "/GLOBALSETSPERSLOT.bin"
#define MAX_SLOTS 72   // same as slots.bin max slots


void load_GLOBAL_PER_SLOT();
void save_GLOBAL_PER_SLOT();
void apply_GLOBAL_PER_SLOT(int slotIndex);
void initDefault_GLOBAL_PER_SLOT(uint8_t slotIndex); 
extern int USE_GLOBALSET_PER_SLOT;


    struct GlobalSlotOptions
{

    uint8_t matchPresetSource;
    uint8_t wantFullHeight;
    uint8_t preferScalingRgbhv;
    uint8_t wantOutputComponent;
    uint8_t PalForce60;
    uint8_t disableExternalClockGenerator;
    uint8_t enableCalibrationADC;
    uint8_t enableFrameTimeLock;
    uint8_t frameTimeLockMethod;
    uint8_t deintMode;

    //uint8_t USE_GLOBALSET_PER_SLOT; //currently saving in LCD_settings.txt file instead
    uint8_t reserved[8]; 
};
extern GlobalSlotOptions globalOptions[MAX_SLOTS];
//////////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////////////
#endif
