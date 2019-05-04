// Prototypes
const unsigned int _html5_func[] __attribute__((address(0x9D006080)));

// Macro to call HTML5 function from C file
#define html5function(x) \
	asm volatile("ori $v0,$zero,%0"::"i"(x));\
	asm volatile("j _html5_func");
#define HTML5_ps2readkey 0
