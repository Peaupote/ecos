#ifndef _HWD_H
#define _HWD_H

// --Control Registers--

//Paging
#define CR0_PG 31

//Page Size Extensions
#define CR4_PSE 5
//Physical Address Extension
#define CR4_PAE 5
//Page Global Enable
#define CR4_PGE 7

// --Model Specific Registers--

//Extended Feature Enables
#define MSR_EFER     0xC0000080
//Long Mode
#define MSR_EFER_LME 8


#define VGA_BUFFER 0xB8000

#endif
