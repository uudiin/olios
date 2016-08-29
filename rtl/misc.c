

#include	"proto.h"


void
_stdcall
spin(char* func_name)
{
	DisplayReturn();
	DisplayColorString ("spinning in ", 15);
	DisplayColorString (func_name, 15);
	DisplayReturn();
	while (1)
	{
	    __asm__ __volatile__("pause");
	}
}


void
_stdcall
assertion_failure(
    char*   exp,
    char*   file,
    char*   base_file,
    int     line
    )
{
	DisplayColorString("\n assert: ", 9);
	DisplayColorString(exp, 15);
	DisplayColorString("\n file: ", 15);
	DisplayColorString(file, 15);
	DisplayColorString("\n basefile: ", 15);
	DisplayColorString(base_file, 15);
	DisplayColorString("\n line: ", 15);
	DisplayInt(line);
	spin("assertion_failure()");
	__asm__ __volatile__("ud2");
}


void
_stdcall
DelayBreak()
{
    int     i;
    
    i = 0x900000;
    
    while (i)
    {
        i--;
        __asm__("nop");
    }
}



void
_stdcall
KiBugCheck(
    pchar   ErrorInfo
    )
{
    DisplayReturn();
	DisplayColorString ("KiBugCheck -> ", 15);
	DisplayColorString (ErrorInfo, 15);
	DisplayReturn();
	while (1)
	{
	    __asm__ __volatile__("pause");
	}
}

















