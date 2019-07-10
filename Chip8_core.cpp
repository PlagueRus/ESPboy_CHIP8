#include "Chip8_core.h"
#include <stdint.h>

// some c++ magic ///////////////////////////////////////////////////////////////////////
struct str2Int {
	const char* const p_, sz_;
	template<int N>	constexpr str2Int(const char(&a)[N]) :p_(a), sz_(N - 1) {}
	constexpr char operator[](int n) const {return n < sz_ ? p_[n] : 0;}
	constexpr char size() const { return sz_; }
	constexpr uint16_t bitline() const { return calc(0, 0); }
	constexpr uint16_t calc(uint16_t res, uint16_t n) const 
	{ return n < sz_ ? (p_[n] == ' ' ? calc(res, n + 1) : calc(res * 2 + (p_[n] == '.' ? 0 : 1), n + 1)) : res; }
};
template<typename ___A, ___A T> constexpr ___A force_compile_time() { return T; }
#define B(___STR) (force_compile_time<int, str2Int(#___STR).bitline()>())
/////////////////////////////////////////////////////////////////////////////////////////

const uint8_t PROGMEM fontchip16x5[16 * 5] = {
	B( . H H . . . . . ),
	B( H . . H . . . . ), 
	B( H . . H . . . . ), 
	B( H . . H . . . . ), 
	B( . H H . . . . . ), // 0

	B( . . H . . . . . ), 
	B( . H H . . . . . ), 
	B( . . H . . . . . ), 
	B( . . H . . . . . ), 
	B( . H H H . . . . ), // 1

	B( H H H . . . . . ), 
	B( . . . H . . . . ), 
	B( . H H . . . . . ), 
	B( H . . . . . . . ), 
	B( H H H H . . . . ), // 2

	B( H H H . . . . . ), 
	B( . . . H . . . . ), 
	B( . H H H . . . . ), 
	B( . . . H . . . . ), 
	B( H H H . . . . . ), // 3

	B( H . . H . . . . ), 
	B( H . . H . . . . ), 
	B( H H H H . . . . ), 
	B( . . . H . . . . ), 
	B( . . . H . . . . ), // 4

	B( H H H H . . . . ), 
	B( H . . . . . . . ), 
	B( H H H . . . . . ), 
	B( . . . H . . . . ), 
	B( H H H . . . . . ), // 5

	B( . H H . . . . . ), 
	B( H . . . . . . . ), 
	B( H H H . . . . . ),
	B( H . . H . . . . ), 
	B( . H H . . . . . ), // 6

	B( H H H H . . . . ), 
	B( . . . H . . . . ), 
	B( . . H . . . . . ),
	B( . H . . . . . . ),
	B( . H . . . . . . ), // 7

	B( . H H . . . . . ), 
	B( H . . H . . . . ), 
	B( . H H . . . . . ), 
	B( H . . H . . . . ), 
	B( . H H . . . . . ), // 8

	B( . H H . . . . . ),
	B( H . . H . . . . ),
	B( . H H H . . . . ), 
	B( . . . H . . . . ),
	B( . H H . . . . . ), // 9

	B( . H H . . . . . ), 
	B( H . . H . . . . ), 
	B( H H H H . . . . ), 
	B( H . . H . . . . ), 
	B( H . . H . . . . ), // A

	B( H H H . . . . . ), 
	B( H . . H . . . . ), 
	B( H H H . . . . . ),
	B( H . . H . . . . ), 
	B( H H H . . . . . ), // B

	B( . H H H . . . . ), 
	B( H . . . . . . . ), 
	B( H . . . . . . . ), 
	B( H . . . . . . . ), 
	B( . H H H . . . . ), // C

	B( H H H . . . . . ), 
	B( H . . H . . . . ), 
	B( H . . H . . . . ), 
	B( H . . H . . . . ), 
	B( H H H . . . . . ), // D

	B( H H H H . . . . ), 
	B( H . . . . . . . ), 
	B( H H H . . . . . ), 
	B( H . . . . . . . ), 
	B( H H H H . . . . ), // E

	B( H H H H . . . . ), 
	B( H . . . . . . . ), 
	B( H H H . . . . . ), 
	B( H . . . . . . . ), 
	B( H . . . . . . . ), // F 

};

const uint8_t PROGMEM fontchip16x10[16 * 10] = {
	B(. . . . . . . .),
	B(. . H H H H . .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. . H H H H . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . . . H . . .),
	B(. . H H H . . .),
	B(. . . . H . . .),
	B(. . . . H . . .),
	B(. . . . H . . .),
	B(. . . . H . . .),
	B(. . . . H . . .),
	B(. . H H H H H .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . H H H . . .),
	B(. H . . . H . .),
	B(. . . . . H . .),
	B(. . . . H . . .),
	B(. . . H . . . .),
	B(. . H . . . . .),
	B(. H . . . H . .),
	B(. H H H H H . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . H H H . . .),
	B(. H . . . H . .),
	B(. . . . . H . .),
	B(. . . H H . . .),
	B(. . . . . H . .),
	B(. . . . . H . .),
	B(. H . . . H . .),
	B(. . H H H . . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . . . H H . .),
	B(. . . H . H . .),
	B(. . H . . H . .),
	B(. . H . . H . .),
	B(. H H H H H H .),
	B(. . . . . H . .),
	B(. . . . . H . .),
	B(. . . . H H H .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . H H H H H .),
	B(. . H . . . . .),
	B(. . H . . . . .),
	B(. . H H H H . .),
	B(. . . . . . H .),
	B(. . . . . . H .),
	B(. H . . . . H .),
	B(. . H H H H . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . . . H H H .),
	B(. . . H . . . .),
	B(. . H . . . . .),
	B(. . H H H H . .),
	B(. . H . . . H .),
	B(. . H . . . H .),
	B(. . H . . . H .),
	B(. . . H H H . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. H H H H H H .),
	B(. H . . . . H .),
	B(. . . . . . H .),
	B(. . . . . H . .),
	B(. . . . . H . .),
	B(. . . . H . . .),
	B(. . . . H . . .),
	B(. . . . H . . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . H H H H . .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. . H H H H . .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. . H H H H . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . H H H H . .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. H . . . . H .),
	B(. . H H H H H .),
	B(. . . . . . H .),
	B(. . . . . H . .),
	B(. H H H H . . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . . H H . . .),
	B(. . . . H . . .),
	B(. . . H . H . .),
	B(. . . H . H . .),
	B(. . . H . H . .),
	B(. . . H H H . .),
	B(. . H . . . H .),
	B(. H H H . H H H),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. H H H H H . .),
	B(. . H . . . H .),
	B(. . H . . . H .),
	B(. . H H H H . .),
	B(. . H . . . H .),
	B(. . H . . . H .),
	B(. . H . . . H .),
	B(. H H H H H . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. . . H H H H .),
	B(. . H . . . H .),
	B(. H . . . . . .),
	B(. H . . . . . .),
	B(. H . . . . . .),
	B(. H . . . . . .),
	B(. . H . . . H .),
	B(. . . H H H . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. H H H H . . .),
	B(. . H . . H . .),
	B(. . H . . . H .),
	B(. . H . . . H .),
	B(. . H . . . H .),
	B(. . H . . . H .),
	B(. . H . . H . .),
	B(. H H H H . . .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. H H H H H H .),
	B(. . H . . . H .),
	B(. . H . H . . .),
	B(. . H H H . . .),
	B(. . H . H . . .),
	B(. . H . . . . .),
	B(. . H . . . H .),
	B(. H H H H H H .),
	B(. . . . . . . .),

	B(. . . . . . . .),
	B(. H H H H H H .),
	B(. . H . . . H .),
	B(. . H . H . . .),
	B(. . H H H . . .),
	B(. . H . H . . .),
	B(. . H . . . . .),
	B(. . H . . . . .),
	B(. H H H . . . .),
	B(. . . . . . . .),
};