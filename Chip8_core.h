#ifndef __CHIP8_CORE__
#define __CHIP8_CORE__

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef PROGMEM
#define PROGMEM
#define memcpy_P memcpy
#endif

#define BIT_RENDERER

#define fontchip_OFFSET       0x38

/*compatibility_emu var
8XY6/8XYE opcode
Bit shifts a register by 1, VIP: shifts rY by one and places in rX, SCHIP: ignores rY field, shifts existing value in rX.
bit1 = 1    <<= and >>= takes vx, shifts, puts to vx, ignore vy
bit1 = 0    <<= and >>= takes vy, shifts, puts to vx

FX55/FX65 opcode
Saves/Loads registers up to X at I pointer - VIP: increases I by X, SCHIP: I remains static.
bit2 = 1    load and store operations leave i unchaged
bit2 = 0    I is set to I + X + 1 after operation

8XY4/8XY5/8XY7/ ??8XY6??and??8XYE??
bit3 = 1    arithmetic results write to vf after status flag
bit3 = 0    vf write only status flag

DXYN
bit4 = 1    wrapping sprites
bit4 = 0    clip sprites at screen edges instead of wrapping

BNNN (aka jump0)
Sets PC to address NNN + v0 - VIP: correctly jumps based on value in v0. SCHIP: also uses highest nibble of address to select register, instead of v0 (high nibble pulls double duty). Effectively, making it jumpN where target memory address is N##. Very awkward quirk.
bit5 = 1    Jump to CurrentAddress+NN ;4 high bits of target address determines the offset register of jump0 instead of v0.
bit5 = 0    Jump to address NNN+V0

DXYN checkbit8
bit6 = 1    drawsprite returns number of collised rows of the sprite + rows out of the screen lines of the sprite (check for bit8)
bit6 = 0    drawsprite returns 1 if collision/s and 0 if no collision/s

EMULATOR TFT DRAW
bit7 = 1    draw to TFT just after changing pixel by drawsprite() not on timer
bit7 = 0    redraw all TFT from display on timer

DXYN OUT OF SCREEN checkbit6
bit8 = 1    drawsprite does not add "number of out of the screen lines of the sprite" in returned value
bit8 = 0    drawsprite add "number of out of the screen lines of the sprite" in returned value
*/

#define BIT1CTL (compatibility_emu & 1)
#define BIT2CTL (compatibility_emu & 2)
#define BIT3CTL (compatibility_emu & 4)
#define BIT4CTL (compatibility_emu & 8)
#define BIT5CTL (compatibility_emu & 16)
#define BIT6CTL (compatibility_emu & 32)
#define BIT7CTL (compatibility_emu & 64)
#define BIT8CTL (compatibility_emu & 128)

#define DEFAULTCOMPATIBILITY    0b01000011 //bit bit8,bit7...bit1;
#define DEFAULTOPCODEPERFRAME   40
#define DEFAULTTIMERSFREQ       60 // freq herz
#define DEFAULTBACKGROUND       0  // check colors []
#define DEFAULTDELAY            1
#define DEFAULTSOUNDTONE        300
#define LORESDISPLAY            0
#define HIRESDISPLAY            1

constexpr auto VF = 0xF;

extern const uint8_t PROGMEM fontchip[16 * 5]; // 16 symbols * 5 lines

class Chip8
{
protected:

	uint8_t   mem[0x1000];
	uint8_t   reg[0x10];    // ram 0x0 - 0xF
	int16_t   stack[0x10];  // ram 0x16 - 0x36???  EA0h-EFFh
	uint8_t   sp;
	volatile uint8_t stimer, dtimer;
	uint16_t  pc, I;

	unsigned long m_time;
	uint8_t flagbuzz;

	uint8_t		foreground_emu;
	uint8_t		background_emu;
	uint8_t		compatibility_emu;   // look above
	uint8_t		delay_emu;           // delay in microseconds before next opcode done
	uint8_t		opcodesperframe_emu; // how many opcodes should be done before screen updates
	uint8_t		timers_emu;          // freq of timers. standart 60hz
	uint16_t	soundtone_emu;

public:
	Chip8() :m_time(0), flagbuzz(0)
	{
#ifdef  BIT_RENDERER
		display = NULL;
		dbuffer = NULL;
#endif//BIT_RENDERER
		set_resolution(64, 32);
	}
	virtual ~Chip8() {}
	virtual void clear_screen() = 0;
	virtual void draw_pixel(uint_fast16_t x, uint_fast16_t y, uint_least32_t color) = 0;
	virtual void draw_hline(int x, int y, int w, int color) { for (int i = 0; i < w; i++) draw_pixel(x + i, y, color); }
	virtual void draw_vline(int x, int y, int h, int color) { for (int i = 0; i < h; i++) draw_pixel(x, y + i, color); }

	virtual void reset()
	{
		base_clear_screen();

		stimer = 0;
		dtimer = 0;
		buzz();

		pc = 0x200;
		sp = 0;
	}

	virtual void set_resolution(uint_fast16_t w, uint_fast16_t h)
	{
		SCREEN_WIDTH = w;
		SCREEN_HEIGHT = h;
		LINE_SIZE = (SCREEN_WIDTH / 8 / sizeof(displaybase_t));
		BITS_PER_BLOCK = 8 * sizeof(displaybase_t);

		delete[] display; display = NULL;
		delete[] dbuffer; dbuffer = NULL;

		display = new displaybase_t[LINE_SIZE * SCREEN_HEIGHT]; // 64x32 bit == 8*8x32 bit (+2 for memcpy simplification)
		dbuffer = new displaybase_t[LINE_SIZE * SCREEN_HEIGHT]; // (8 + 2 for edges)*8x32 == 32 rows 10 cols
	}

	virtual bool check_buttons()
	{
		return false;
	}

	virtual void run()
	{
		uint16_t c = 0;
		memcpy_P(&mem[fontchip_OFFSET], fontchip, 16 * 5);
		while (true)
		{
			if (c < opcodesperframe_emu)
			{
				do_cpu();
				buzz();
				delay(delay_emu);
				c++;
			}
			else
			{
				if (!BIT7CTL)
					update_display();
				c = 0;
			}

			if (check_buttons()) break;
			update_timers();
		}
		//timers.detach();
	}
protected:

	virtual void start_buzz() {}
	virtual void stop_buzz() {}

	uint8_t* getROM(uint_fast16_t size) { return &mem[0x200]; }

	virtual uint8_t iskeypressed(uint8_t key) { return 0; }
	virtual uint8_t waitanykey() { return 0; }

	virtual void delay(uint_fast16_t ms) {}

	virtual bool getTimer(uint_fast32_t ms) = 0;

private:
	void base_clear_screen()
	{
		clear_screen();
		memset(display, 0, LINE_SIZE * SCREEN_HEIGHT * sizeof(displaybase_t));
		memset(dbuffer, 0, LINE_SIZE * SCREEN_HEIGHT * sizeof(displaybase_t));
	}

	void buzz()
	{
		if (stimer > 1 && !flagbuzz)
		{
			flagbuzz++;
			start_buzz();
		}
		if (stimer < 1 && flagbuzz)
		{
			flagbuzz = 0;
			stop_buzz();
		}
	}

	void update_timers()
	{
		if (getTimer(1000 / timers_emu))
		{
			if (dtimer) dtimer--;
			if (stimer) stimer--;
		}
	}


#ifdef BIT_RENDERER

	typedef uint_fast8_t displaybase_t;

	uint_fast16_t SCREEN_WIDTH;
	uint_fast16_t SCREEN_HEIGHT;
	uint_fast16_t LINE_SIZE;
	uint_fast8_t BITS_PER_BLOCK;

	displaybase_t *display; // 64x32 bit == 8*8x32 bit (+2 for memcpy simplification)
	displaybase_t *dbuffer; // (8 + 2 for edges)*8x32 == 32 rows 10 cols

	// function declaration before definition to avoid compiler bug
	//void drawblock(int x, int y, displaybase_t block, displaybase_t diff);

	void draw_block(int_fast8_t x, int_fast8_t y, displaybase_t block, displaybase_t diff)
	{
		for (int k = BITS_PER_BLOCK - 1; k >= 0; k--)
		{
			if (diff & 1)
				draw_pixel(x + k, y, block & 1 );
			diff >>= 1;
			block >>= 1;
		}
	}

	void update_display()
	{
		//static unsigned long tme;
		//tme=millis();
		for (auto i = 0u; i < SCREEN_HEIGHT; i++)
			for (auto j = 0u; j < LINE_SIZE; j++)
			{
				auto blockindex = i * LINE_SIZE + j;
				if (display[blockindex] != dbuffer[blockindex])
				{
					draw_block(j * BITS_PER_BLOCK, i, dbuffer[blockindex], display[blockindex] ^ dbuffer[blockindex]);
					display[blockindex] = dbuffer[blockindex];
				}
			}
		//Serial.println(millis()-tme);
	}

	uint_fast8_t draw_sprite(uint_fast16_t x, uint_fast16_t y, uint_fast16_t size)
	{
		uint_fast8_t ret = 0;
		bool isOnlyClear = true;
		uint_fast16_t vline, xline;
		displaybase_t data, datal, datah;
		uint_fast8_t shift = (x % BITS_PER_BLOCK);
		uint_fast8_t freebits = (BITS_PER_BLOCK - 8);
		if (!size) size = 16;

		x = x % SCREEN_WIDTH;
		y = y % SCREEN_HEIGHT;

		for (auto line = 0u; line < size; line++)
		{
			data = mem[I + line];
			data <<= freebits;

			vline = ((y + line) % SCREEN_HEIGHT) * LINE_SIZE;

			datal = data >> shift;
			if (datal)
			{
				xline = (x / BITS_PER_BLOCK) % LINE_SIZE;
				displaybase_t* scr1 = &dbuffer[vline + xline];
				if (*scr1 & datal) ret++;
				if (isOnlyClear && (*scr1 & datal) != datal) isOnlyClear = false;
				*scr1 ^= datal;
			}

			// In normal situations condition is not necessary. But there is a bug, when the shift is more than 
			// the width of the variable then the variable does not change, appear only with the type uint32_t
			if (shift > freebits)
			{
				datah = data << (BITS_PER_BLOCK - shift);
				if (datah)
				{
					xline = (x / BITS_PER_BLOCK + 1) % LINE_SIZE;
					displaybase_t* scr2 = &dbuffer[vline + xline];
					if (*scr2 & datah) ret++;
					if (isOnlyClear && (*scr2 & datah) != datah) isOnlyClear = false;
					*scr2 ^= datah;
				}
			}
		}

		if (!isOnlyClear)
			update_display();

		if (BIT6CTL) return ret;
		return !!ret;
	}
#endif

#ifdef SIMPLE_RENDERER // old version

	uint8_t   display[64 * 32];// ram F00h-FFFh

	void chip8_cls()
	{
		tft.fillScreen(TFT_BLACK);
		memset(display, 0, sizeof(display));
	}

	void chip8_reset()
	{
		chip8_cls();
		stimer = 0;
		dtimer = 0;
		buzz();
		pc = 0x200;
		sp = 0;
	}

	void updatedisplay()
	{
		static uint8_t drawcolor;
		static uint16_t i, j;
		//static unsigned long tme;
		//  tme=millis();
		for (i = 0; i < 32; i++)
			for (j = 0; j < 64; j++) {
				if (display[(i << 6) + j])
					drawcolor = foreground_emu;
				else
					drawcolor = background_emu;
				tft.fillRect(j << 1, (i << 1) + 16, 2, 2, colors[drawcolor]);
			}
		//   Serial.println(millis()-tme);
	}


	//SIMPLE VERSION without compatibility and realtime drawing
	uint8_t drawsprite(uint8_t x, uint8_t y, uint8_t size)
	{
		uint8_t data, mask, c, d, masked, xdisp, ydisp, ret;
		uint16_t addrdisplay;
		ret = 0;
		for (c = 0; c < size; c++) {
			data = mem[I + c];
			mask = 0b10000000;
			ydisp = y + c;
			for (d = 0; d < 8; d++) {
				xdisp = x + d;
				masked = !!(data & mask);
				addrdisplay = xdisp + (ydisp << 6);
				if (xdisp < 64 && ydisp < 32) {
					if (display[addrdisplay] && masked) ret++;

					display[addrdisplay] ^= masked;
				}
				mask >>= 1;
			}
		}

		if (BIT7CTL) updatedisplay();

		return (ret ? 1 : 0);
	}

#endif

	enum
	{
		CHIP8_JMP = 0x1,
		CHIP8_JSR = 0x2,
		CHIP8_SKEQx = 0x3,
		CHIP8_SKNEx = 0x4,
		CHIP8_SKEQr = 0x5,
		CHIP8_MOVx = 0x6,
		CHIP8_ADDx = 0x7,
		CHIP8_MOVr = 0x8,
		CHIP8_SKNEr = 0x9,
		CHIP8_MVI = 0xa,
		CHIP8_JMI = 0xb,
		CHIP8_RAND = 0xc,
		CHIP8_DRYS = 0xd,

		CHIP8_EXT0 = 0x0,
		CHIP8_EXT0_CLS = 0xE0,
		CHIP8_EXT0_RTS = 0xEE,

		CHIP8_EXTF = 0xF,
		CHIP8_EXTF_GDELAY = 0x07,
		CHIP8_EXTF_KEY = 0x0a,
		CHIP8_EXTF_SDELAY = 0x15,
		CHIP8_EXTF_SSOUND = 0x18,
		CHIP8_EXTF_ADI = 0x1e,
		CHIP8_EXTF_FONT = 0x29,
		CHIP8_EXTF_XFONT = 0x30,
		CHIP8_EXTF_BCD = 0x33,
		CHIP8_EXTF_STR = 0x55,
		CHIP8_EXTF_LDR = 0x65,

		CHIP8_MATH = 0x8,
		CHIP8_MATH_MOV = 0x0,
		CHIP8_MATH_OR = 0x1,
		CHIP8_MATH_AND = 0x2,
		CHIP8_MATH_XOR = 0x3,
		CHIP8_MATH_ADD = 0x4,
		CHIP8_MATH_SUB = 0x5,
		CHIP8_MATH_SHR = 0x6,
		CHIP8_MATH_RSB = 0x7,
		CHIP8_MATH_SHL = 0xe,

		CHIP8_SK = 0xe,
		CHIP8_SK_RP = 0x9e,
		CHIP8_SK_UP = 0xa1,
	};

	uint8_t do_cpu()
	{
		uint16_t inst, op2, wr, xxx;
		uint8_t cr;
		uint8_t op, x, y, zz;

		inst = (mem[pc] << 8) + mem[pc + 1];
		pc += 2;
		op = (inst >> 12) & 0xF;
		x = (inst >> 8) & 0xF;
		y = (inst >> 4) & 0xF;
		zz = inst & 0x00FF;
		xxx = inst & 0x0FFF;

		switch (op)
		{
		case CHIP8_JSR: // jsr xyz
			stack[sp] = pc;
			sp = (sp + 1) & 0x0F;
			pc = inst & 0xFFF;
			break;

		case CHIP8_JMP: // jmp xyz
			pc = xxx;
			break;

		case CHIP8_SKEQx: // skeq:  skip next opcode if r(x)=zz
			if (reg[x] == zz)
				pc += 2;
			break;

		case CHIP8_SKNEx: //skne: skip next opcode if r(x)<>zz
			if (reg[x] != zz)
				pc += 2;
			break;

		case CHIP8_SKEQr: //skeq: skip next opcode if r(x)=r(y)
			if (reg[x] == reg[y])
				pc += 2;
			break;

		case CHIP8_MOVx: // mov xxx
			reg[x] = zz;
			break;

		case CHIP8_ADDx: // add xxx
			reg[x] = reg[x] + zz;
			break;

		case CHIP8_SKNEr: //skne: skip next opcode if r(x)<>r(y)
			if (reg[x] != reg[y])
				pc += 2;
			break;

		case CHIP8_MVI: // mvi xxx
			I = xxx;
			break;

		case CHIP8_JMI: // jmi xxx		
			if (BIT5CTL)
				pc = xxx + reg[x];
			else
				pc = xxx + reg[0];
			break;
		case CHIP8_RAND: //rand xxx
			reg[x] = (rand() % 256) & zz;
			break;

		case CHIP8_DRYS: //draw sprite
			reg[VF] = draw_sprite(reg[x], reg[y], inst & 0xF);
			break;

		case CHIP8_EXT0: //extended instruction
			switch (zz)
			{
			case CHIP8_EXT0_CLS:
				base_clear_screen();
				//if (BIT7CTL)
				//	tft.fillRect(0, 16, 128, 64, colors[background_emu]);
				break;
			case CHIP8_EXT0_RTS: // ret
				sp = (sp - 1) & 0xF;
				pc = stack[sp] & 0xFFF;
				break;
			}
			break;

		case CHIP8_MATH: //extended math instruction
			op2 = inst & 0xF;
			switch (op2)
			{
			case CHIP8_MATH_MOV: //mov r(x)=r(y)
				reg[x] = reg[y];
				break;
			case CHIP8_MATH_OR: //or
				reg[x] |= reg[y];
				break;
			case CHIP8_MATH_AND: //and
				reg[x] &= reg[y];
				break;
			case CHIP8_MATH_XOR: //xor
				reg[x] ^= reg[y];
				break;
			case CHIP8_MATH_ADD: //add
				if (BIT3CTL)
				{   // carry first
					wr = reg[x];
					wr += reg[y];
					reg[VF] = wr > 0xFF;
					reg[x] = reg[x] + reg[y];
				}
				else
				{
					wr = reg[x];
					wr += reg[y];
					reg[x] = static_cast<uint8_t>(wr);
					reg[VF] = wr > 0xFF;
				}
				break;
			case CHIP8_MATH_SUB: //sub
				if (BIT3CTL)
				{   // carry first
					reg[VF] = reg[y] < reg[x];
					reg[x] = reg[x] - reg[y];
				}
				else
				{
					cr = reg[y] < reg[x];
					reg[x] = reg[x] - reg[y];
					reg[VF] = cr;
				}
				break;
			case CHIP8_MATH_SHR: //shr
				if (BIT3CTL)
				{   // carry first
					if (BIT1CTL)
					{
						reg[VF] = reg[x] & 0x1;
						reg[x] >>= 1;
					}
					else
					{
						reg[VF] = reg[y] & 0x1;
						reg[x] = reg[y] >> 1;
					}
				}
				else
				{
					if (BIT1CTL)
					{
						cr = reg[x] & 0x1;
						reg[x] >>= 1;
						reg[VF] = cr;
					}
					else
					{
						cr = reg[y] & 0x1;
						reg[x] = reg[y] >> 1;
						reg[VF] = cr;
					}
				}
				break;
			case CHIP8_MATH_RSB: //rsb
				if (BIT3CTL)
				{   // carry first
					reg[VF] = reg[y] > reg[x];
					reg[x] = reg[y] - reg[x];
				}
				else
				{
					cr = reg[y] > reg[x];
					reg[x] = reg[y] - reg[x];
					reg[VF] = cr;
				}
				break;
			case CHIP8_MATH_SHL: //shl
				if (BIT3CTL)
				{   // carry first
					if (BIT1CTL)
					{
						reg[VF] = reg[x] >> 7;
						reg[x] <<= 1;
					}
					else
					{
						reg[VF] = reg[y] >> 7;
						reg[x] = reg[y] << 1;
					}
				}
				else
				{
					if (BIT1CTL)
					{
						cr = reg[x] >> 7;
						reg[x] <<= 1;
						reg[VF] = cr;
					}
					else
					{
						cr = reg[y] >> 7;
						reg[x] = reg[y] << 1;
						reg[VF] = cr;
					}
				}
				break;
			}
			break;

		case CHIP8_SK: //extended instruction
			switch (zz)
			{
			case CHIP8_SK_RP: // skipifkey
				//updatedisplay();
				if (iskeypressed(reg[x]))
					pc += 2;
				break;
			case CHIP8_SK_UP: // skipifnokey
				//updatedisplay();
				if (!iskeypressed(reg[x]))
					pc += 2;
				break;
			}
			break;

		case CHIP8_EXTF: //extended instruction
			switch (zz)
			{
			case CHIP8_EXTF_GDELAY: // getdelay
				reg[x] = dtimer;
				update_display();
				break;
			case CHIP8_EXTF_KEY: //waitkey
				reg[x] = waitanykey();
				break;
			case CHIP8_EXTF_SDELAY: //setdelay
				dtimer = reg[x];
				//updatedisplay();
				break;
			case CHIP8_EXTF_SSOUND: //setsound
				stimer = reg[x];
				break;
			case CHIP8_EXTF_ADI: //add i+r(x)
				I += reg[x];
				break;
			case CHIP8_EXTF_FONT: //fontchip i
				I = fontchip_OFFSET + (reg[x] * 5);
				break;
			case CHIP8_EXTF_BCD: //bcd
				mem[I] = reg[x] / 100;
				mem[I + 1] = (reg[x] / 10) % 10;
				mem[I + 2] = reg[x] % 10;
				break;
			case CHIP8_EXTF_STR: //save
				memcpy(&mem[I], reg, x + 1);
				if (!BIT2CTL)
					I = I + x + 1;
				break;
			case CHIP8_EXTF_LDR: //load
				memcpy(reg, &mem[I], x + 1);
				if (!BIT2CTL)
					I = I + x + 1;
				break;
			}
			break;
		}
		return (0);
	}

};

#endif//__CHIP8_CORE__
