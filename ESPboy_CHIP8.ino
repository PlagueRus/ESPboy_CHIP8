/*
ESPboy chip8/schip emulator by RomanS
Special thanks to Alvaro Alea Fernandez for his Chip-8 emulator, Igor (corax69), DmitryL (Plague), John Earnest (https://github.com/JohnEarnest/Octo/tree/gh-pages/docs) for help

ESPboy project page:
https://hackaday.io/project/164830-espboy-beyond-the-games-platform-with-wifi
*/

#include <FS.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_MCP4725.h>
#include <ESP8266WiFi.h>
#include "ESPboyLogo.h"
#include "Chip8_core.h"
#include <pgmspace.h>
#include <Ticker.h>

#define BIT_RENDERER
//#define SIMPLE_RENDERER

//system
#define NLINEFILES            14 //no of files in menu
#define csTFTMCP23017pin      8
#define LEDquantity           1
#define MCP23017address       0 // actually it's 0x20 but in <Adafruit_MCP23017.h> lib there is (x|0x20) :)
#define MCP4725dacresolution  8
#define MCP4725address        0

//buttons
#define LEFT_BUTTON   (buttonspressed & 1)
#define UP_BUTTON     (buttonspressed & 2)
#define DOWN_BUTTON   (buttonspressed & 4)
#define RIGHT_BUTTON  (buttonspressed & 8)
#define ACT_BUTTON    (buttonspressed & 16)
#define ESC_BUTTON    (buttonspressed & 32)
#define LFT_BUTTON    (buttonspressed & 64)
#define RGT_BUTTON    (buttonspressed & 128)
#define LEFT_BUTTONn  0
#define UP_BUTTONn    1
#define DOWN_BUTTONn  2
#define RIGHT_BUTTONn 3
#define ACT_BUTTONn   4
#define ESC_BUTTONn   5
#define LFT_BUTTONn   6
#define RGT_BUTTONn   7

//pins
#define LEDPIN    D4
#define SOUNDPIN  D3

//lib colors
uint16_t colors[] = { TFT_BLACK, TFT_NAVY, TFT_DARKGREEN, TFT_DARKCYAN, TFT_MAROON,
					 TFT_PURPLE, TFT_OLIVE, TFT_LIGHTGREY, TFT_DARKGREY, TFT_BLUE, TFT_GREEN, TFT_CYAN,
					 TFT_RED, TFT_MAGENTA, TFT_YELLOW, TFT_WHITE, TFT_ORANGE, TFT_GREENYELLOW, TFT_PINK };


static uint8_t   keys[8]={0};

static uint16_t buttonspressed;

Adafruit_MCP23017 mcp;
TFT_eSPI tft;
Adafruit_NeoPixel pixels(LEDquantity, LEDPIN);
Adafruit_MCP4725 dac;
Ticker timers;

String		description_emu;     // file description

//keymapping 0-LEFT, 1-UP, 2-DOWN, 3-RIGHT, 4-ACT, 5-ESC, 6-LFT side button, 7-RGT side button
static uint8_t default_buttons[8] = { 4, 2, 8, 6, 5, 11, 4, 6 };
//1 2 3 C
//4 5 6 D
//7 8 9 E
//A 0 B F

class TFT_Chip8 : public Chip8
{
	TFT_eSPI *m_tft;
	uint_fast16_t m_shift = 16; // shift from upper edge of the screen

public:
	uint16_t		foreground_emu;
	uint16_t		background_emu;

public:
	TFT_Chip8(TFT_eSPI *tft): m_tft(tft) {}
	virtual ~TFT_Chip8() {}
	virtual void clear_screen() override
	{
		m_tft->fillScreen(colors[background_emu]);
	}
	virtual void draw_pixel(uint_fast16_t x, uint_fast16_t y, uint_least32_t color) override
	{
		auto pxwidth = is_hires() ? 1 : 2;
		m_tft->fillRect(x * pxwidth, y * pxwidth + m_shift, pxwidth, pxwidth, colors[color ? foreground_emu : background_emu]);
	}
	virtual void draw_hline(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_least32_t color) override
	{
		auto pxwidth = is_hires() ? 1 : 2;
		m_tft->fillRect(x * pxwidth, y * pxwidth + m_shift, w * pxwidth, pxwidth, colors[color ? foreground_emu : background_emu]);
	}
	virtual bool check_buttons() override
	{
		checkbuttons();
		if (LFT_BUTTON && RGT_BUTTON)
		{
			this->reset();
			if (waitkeyunpressed() > 300) return true;
		}
		return false;
	}
public:
	virtual void start_buzz() override
	{
		tone(SOUNDPIN, soundtone_emu);
		pixels.setPixelColor(0, pixels.Color(30, 0, 0));
		pixels.show();
	}
	virtual void stop_buzz() override
	{
		noTone(SOUNDPIN);
		pixels.setPixelColor(0, pixels.Color(0, 0, 0));
		pixels.show();
	}
	virtual uint8_t iskeypressed(uint8_t key) override
	{
		uint_fast8_t ret = 0;
		checkbuttons(); 
		if (LEFT_BUTTON && keys[LEFT_BUTTONn] == key)
			ret++;
		if (UP_BUTTON && keys[UP_BUTTONn] == key)
			ret++;
		if (DOWN_BUTTON && keys[DOWN_BUTTONn] == key)
			ret++;
		if (RIGHT_BUTTON && keys[RIGHT_BUTTONn] == key)
			ret++;
		if (ACT_BUTTON && keys[ACT_BUTTONn] == key)
			ret++;
		if (ESC_BUTTON && keys[ESC_BUTTONn] == key)
			ret++;
		if (LFT_BUTTON && keys[LFT_BUTTONn] == key)
			ret++;
		if (RGT_BUTTON && keys[RGT_BUTTONn] == key)
			ret++;
		return (ret ? 1 : 0);
	}
	virtual uint8_t waitanykey() override
	{
		uint8_t ret = 0;
		while (!checkbuttons())
			delay(5);
		if (LEFT_BUTTON)
			ret = keys[LEFT_BUTTONn];
		if (UP_BUTTON)
			ret = keys[UP_BUTTONn];
		if (DOWN_BUTTON)
			ret = keys[DOWN_BUTTONn];
		if (RIGHT_BUTTON)
			ret = keys[RIGHT_BUTTONn];
		if (ACT_BUTTON)
			ret = keys[ACT_BUTTONn];
		if (ESC_BUTTON)
			ret = keys[ESC_BUTTONn];
		if (RGT_BUTTON)
			ret = keys[RGT_BUTTONn];
		if (LFT_BUTTON)
			ret = keys[LFT_BUTTONn];
		return ret;
	}

	unsigned long m_millis = 0;

	virtual bool getTimer(uint_fast32_t ms) 
	{
		unsigned long curmillis = ::millis();
		if (curmillis - m_millis > ms)
		{
			m_millis = curmillis - ((curmillis - m_millis) % ms);
			return true;
		}
		return false;
	}

	void delay(uint_fast16_t ms) { ::delay(ms); }

	void loadrom(String filename)
	{
		File f;
		String data;
		uint16_t c;
		f = SPIFFS.open(filename, "r");
		f.read(this->getROM(f.size()), f.size());
		f.close();
		filename.setCharAt(filename.length() - 3, 'k');
		filename = filename.substring(0, filename.length() - 2);
		if (SPIFFS.exists(filename))
		{
			f = SPIFFS.open(filename, "r");
			for (c = 0; c < 7; c++)
			{
				data = f.readStringUntil(' ');
				keys[c] = data.toInt();
			}
			data = f.readStringUntil('\n');
			keys[7] = data.toInt();
			data = f.readStringUntil('\n');
			foreground_emu = data.toInt();
			data = f.readStringUntil('\n');
			background_emu = data.toInt();
			data = f.readStringUntil('\n');
			delay_emu = data.toInt();
			data = f.readStringUntil('\n');
			compatibility_emu = data.toInt();
			data = f.readStringUntil('\n');
			opcodesperframe_emu = data.toInt();
			data = f.readStringUntil('\n');
			timers_emu = data.toInt();
			data = f.readStringUntil('\n');
			soundtone_emu = data.toInt();
			description_emu = f.readStringUntil('\n').substring(0, 314);
			f.close();
			if (foreground_emu == 255)
				foreground_emu = random(17) + 1;
		}
		else
		{
			for (c = 0; c < 8; c++)
				keys[c] = default_buttons[c];
			foreground_emu = random(10) + 9;
			background_emu = DEFAULTBACKGROUND;
			compatibility_emu = DEFAULTCOMPATIBILITY;
			delay_emu = DEFAULTDELAY;
			opcodesperframe_emu = DEFAULTOPCODEPERFRAME;
			timers_emu = DEFAULTTIMERSFREQ;
			soundtone_emu = DEFAULTSOUNDTONE;
			description_emu = F("No configuration\nfile found\n\nUsing unoptimal\ndefault parameters");
		}
		this->reset();
	}
};


uint16_t waitkeyunpressed()
{
	unsigned long starttime = millis();
	while (checkbuttons())
		delay(5);
	return (millis() - starttime);
}

uint16_t checkbuttons()
{
	buttonspressed = ~mcp.readGPIOAB() & 255;
	return (buttonspressed);
}

uint8_t readkey()
{
	checkbuttons();
	if (LEFT_BUTTON)
		return LEFT_BUTTONn;
	if (UP_BUTTON)
		return UP_BUTTONn;
	if (DOWN_BUTTON)
		return DOWN_BUTTONn;
	if (RIGHT_BUTTON)
		return RIGHT_BUTTONn;
	if (ACT_BUTTON)
		return ACT_BUTTONn;
	if (ESC_BUTTON)
		return ESC_BUTTONn;
	if (LFT_BUTTON)
		return LFT_BUTTONn;
	if (RGT_BUTTON)
		return RGT_BUTTONn;
	if (!buttonspressed)
		return 255;
	return 0;
}


void draw_loading(bool reset = false)
{
	static bool firstdraw = true;
	static uint8_t index = 0;
	int32_t pos, next_pos;

	constexpr auto x = 29, y = 96, w = 69, h = 14, cnt = 5, padding = 3;

	if (reset)
	{
		firstdraw = true;
		index = 0;
	}

	if (firstdraw)
	{
		tft.fillRect(x+1, y+1, w-2, h-2, TFT_BLACK);
		tft.drawRect(x, y, w, h, TFT_YELLOW);
		firstdraw = false;
	}

	pos = ((index * (w - padding - 1)) / cnt);
	index++;
	next_pos = ((index * (w - padding - 1)) / cnt);

	tft.fillRect(x + padding + pos, y + padding, next_pos - pos - padding + 1, h - padding*2, TFT_YELLOW);
}

void setup()
{
	// DISPLAY FIRST
	//buttons on mcp23017 init
	mcp.begin(MCP23017address);
	//delay(10);
	for (int i = 0; i < 8; i++)
	{
		mcp.pinMode(i, INPUT);
		mcp.pullUp(i, HIGH);
	}

	// INIT SCREEN FIRST
	//TFT init
	mcp.pinMode(csTFTMCP23017pin, OUTPUT);
	mcp.digitalWrite(csTFTMCP23017pin, LOW);
	tft.init();
	tft.setRotation(0);
	tft.fillScreen(TFT_BLACK);

	draw_loading();

	//draw ESPboylogo
	tft.drawXBitmap(30, 24, ESPboyLogo, 68, 64, TFT_YELLOW);
	tft.setTextSize(1);
	tft.setTextColor(TFT_YELLOW);
	tft.setCursor(4, 118);
	tft.print(F("Chip8/Schip emulator"));

	Serial.begin(115200); //serial init
	if (!SPIFFS.begin())
	{
		SPIFFS.format();
		SPIFFS.begin();
	}
	delay(100);
	WiFi.mode(WIFI_OFF); // to safe some battery power
	draw_loading();

	//LED init
	pinMode(LEDPIN, OUTPUT);
	pixels.begin();
	delay(100);
	pixels.setPixelColor(0, pixels.Color(0, 0, 0));
	pixels.show();
	draw_loading();

	//sound init and test
	pinMode(SOUNDPIN, OUTPUT);
	tone(SOUNDPIN, 200, 100);
	delay(100);
	tone(SOUNDPIN, 100, 100);
	delay(100);
	noTone(SOUNDPIN);
	draw_loading();

	//DAC init
	dac.begin(0x60);
	delay(100);
	dac.setVoltage(4095, true);
	draw_loading();

	//clear TFT
	delay(1500);
	tft.fillScreen(TFT_BLACK);
}

TFT_Chip8 chip8(&tft);

enum EMUSTATE {
	APP_HELP,
	APP_SHOW_DIR,
	APP_CHECK_KEY,
	APP_EMULATE
};
EMUSTATE emustate = APP_HELP;

void loop()
{
	static uint16_t selectedfilech8, countfilesonpage, countfilesch8, maxfilesch8;
	Dir dir;
	static String filename, selectedfilech8name;
	dir = SPIFFS.openDir("/");
	maxfilesch8 = 0;
	while (dir.next())
	{
		filename = String(dir.fileName());
		filename.toLowerCase();

		if (filename.lastIndexOf(String(F(".ch8"))) > 0)
			maxfilesch8++;
	}

	switch (emustate)
	{
	case APP_HELP:
		tft.setTextColor(TFT_YELLOW);
		tft.setTextSize(1);
		tft.setCursor(0, 0);
		tft.print(
			F("upload .ch8 to spiffs\n"
			"\n"
			"add config file for\n"
			" - keys remap\n"
			" - fore/back color\n"
			" - compatibility ops\n"
			" - opcodes per redraw\n"
			" - timers freq hz\n"
			" - sound tone\n"
			" - description\n"
			"\n"
			"during the play press\n"
			"both side buttons for\n"
			" - RESET - shortpress\n"
			" - EXIT  - longpress"));
		waitkeyunpressed();
		chip8.waitanykey();
		waitkeyunpressed();
		selectedfilech8 = 1;
		if (maxfilesch8)
			emustate = APP_SHOW_DIR;
    else{
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 60);
        tft.setTextColor(TFT_MAGENTA);
        tft.print(F(" Chip8 ROMs not found"));
        while (1) 
            delay(5000);
    }
		break;
	case APP_SHOW_DIR:
		dir = SPIFFS.openDir("/");
		countfilesch8 = 0;
		while (countfilesch8 < (selectedfilech8 / (NLINEFILES + 1)) * (NLINEFILES + 1) - 1)
		{
			dir.next();
			filename = String(dir.fileName());
			filename.toLowerCase();

			if (filename.lastIndexOf(String(F(".ch8"))) > 0)
				countfilesch8++;
		}
		tft.fillScreen(TFT_BLACK);
		tft.setCursor(0, 0);
		tft.setTextColor(TFT_YELLOW);
		tft.print(F("SELECT GAME:"));
		tft.setTextColor(TFT_MAGENTA);
		countfilesonpage = 0;
		while (dir.next() && (countfilesonpage < NLINEFILES))
		{
			filename = String(dir.fileName());
			String tfilename = filename;
			tfilename.toLowerCase();

			if (tfilename.lastIndexOf(String(F(".ch8"))) > 0)
			{
				if (filename.indexOf(".") < 17)
					filename = filename.substring(1, filename.indexOf("."));
				else
					filename = filename.substring(1, 16);
				filename = "  " + filename;
				countfilesch8++;
				if (countfilesch8 == selectedfilech8)
				{
					selectedfilech8name = filename;
					filename.trim();
					filename = String("> " + filename);
					tft.setTextColor(TFT_YELLOW);
				}
				else
				{
					tft.setTextColor(TFT_MAGENTA);
				}
				tft.setCursor(0, countfilesonpage * 8 + 12);
				tft.print(filename);
				countfilesonpage++;
			}
		}
		emustate = APP_CHECK_KEY;
		break;
	case APP_CHECK_KEY:
		waitkeyunpressed();
		chip8.waitanykey();
		if (UP_BUTTON && selectedfilech8 > 1)
			selectedfilech8--;
		if (DOWN_BUTTON && (selectedfilech8 < maxfilesch8))
			selectedfilech8++;
		if (ACT_BUTTON)
		{
			selectedfilech8name.trim();
			chip8.loadrom("/" + selectedfilech8name + ".ch8");
			tft.fillScreen(TFT_BLACK);
			tft.setCursor((((21 - selectedfilech8name.length())) / 2) * 6, 0);
			tft.setTextColor(TFT_YELLOW);
			tft.print(selectedfilech8name);
			tft.setCursor(0, 12);
			tft.setTextColor(TFT_MAGENTA);
			tft.print(description_emu);
			waitkeyunpressed();
			chip8.waitanykey();
			waitkeyunpressed();
			emustate = APP_EMULATE;
		}
		else
			emustate = APP_SHOW_DIR;
		waitkeyunpressed();
		break;
	case APP_EMULATE: //chip8 emulation
		tft.fillScreen(colors[chip8.background_emu]);
		chip8.run();
		emustate = APP_SHOW_DIR;
		break;
	}
}
