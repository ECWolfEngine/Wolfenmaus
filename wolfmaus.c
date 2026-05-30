/* Copyright (C) 2026 Braden "Blzut3" Obrzut
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dos.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>

#define ANGLESCALE 20
#define MOUSE_ADJUSTMENT_BASE 13

#define MOUSE_INT 0x33
#define MOUSE_GET_POSITION_AND_BUTTON_STATUS 0x3
#define MOUSE_SET_CURSOR_POSITION 0x4
#define MOUSE_READ_MOTION_COUNTERS 0xB

#define VR_INIT_INT 0x60
#define VR_SHUTDOWN_INT 0x61

// Segment 0040 is the BIOS Data Area and the paragraph at 00F0-00FF is marked
// for "Inter-application Communication Area." Value is expected to be within
// the range of [0, 359].
static volatile int __far* const helmetangle = (volatile int _far*)MK_FP(0x40,0xF0);
static int helmetfrac = 0;

static void (__interrupt __far* old_mouse_interrupt)(void);

static unsigned mouse_button_invert_mask = 0;
static unsigned mouse_button_invert_mask_desired = 0;
static int mouseadjustment = 5;
static int novert = 0;

/**
 * Computes the VR helmet angle using the same formula that's used for scaling
 * mouse inputs in vanilla.
 *
 * Called while intercepting the motion counter read in the ISR. The caller
 * will nullify the horizontal input (and vertical input if desired).
 */
static void __watcall VRL_MouseReadMotionCounters(int ax_unused, int y, int bx_unused, int x)
{
	div_t scaled;

	scaled = div(x*10/(MOUSE_ADJUSTMENT_BASE-mouseadjustment) + helmetfrac, ANGLESCALE);
	helmetfrac = scaled.rem;

	x = *helmetangle - scaled.quot;
	if((x %= 360) < 0)
		x += 360;
	*helmetangle = x;

	// Re-enable button inversion
	mouse_button_invert_mask = mouse_button_invert_mask_desired;
}

/**
 * Mouse interrupt service routine interceptor.
 */
static void __declspec(naked) __interrupt __far VRL_MouseService()
{
	_asm
	{
		push ds
		push ax
		mov ax,seg old_mouse_interrupt
		mov ds,ax
		pop ax
		push ax
		pushf
		call dword ptr old_mouse_interrupt

		pop ax
		cmp ax, MOUSE_READ_MOTION_COUNTERS
		jnz CheckButtons
		pusha
		push es
		call VRL_MouseReadMotionCounters
		pop es
		popa
		xor cx, cx
		cmp novert, 0
		jz Done
		xor dx, dx
		jmp Done

CheckButtons:
		cmp ax, MOUSE_GET_POSITION_AND_BUTTON_STATUS
		jnz CheckReset
		xor bx, mouse_button_invert_mask
		jmp Done

CheckReset:
		cmp ax, MOUSE_SET_CURSOR_POSITION
		jnz Done
		// The Wolf3D menu code calls this function regularly so seeing this
		// function means we should disable inversion so that the menu isn't
		// perpetually waiting for the button to be released.
		mov mouse_button_invert_mask, 0

Done:
		pop ds
		iret
	}
}

/**
 * ISR handler for VR init.
 */
static void __interrupt __far VRL_Init()
{
	*helmetangle = 0;
}

/**
 * ISR handler for VR shutdown.
 */
static void __interrupt __far VRL_Shutdown()
{
}

/**
 * Parse command line and return the index of the start of the Wolf3D commnad.
 */
static int VRL_ParseCmdLine(int argc, const char* argv[])
{
	int i;

	for(i = 1; i < argc; ++i)
	{
		if(argv[i][0] != '-')
			return i;

		switch(argv[i][1])
		{
			case 'i':
				mouse_button_invert_mask_desired = atoi(&argv[i][2]);
				if(mouse_button_invert_mask_desired & (~0x7))
				{
					printf("Invalid button mask %d", mouse_button_invert_mask_desired);
					return -1;
				}
				break;
			case 's':
				mouseadjustment = atoi(&argv[i][2]);
				if(mouseadjustment == MOUSE_ADJUSTMENT_BASE)
				{
					puts("Invalid sensitivity\n");
					return -1;
				}
				break;
			case 'v':
				novert = 1;
				break;
			default:
				printf("Invalid option %s", argv[i]);
				return -1;
		}
	}

	return -1;
}

/**
 * Prints usage information.
 */
static void VRL_PrintHelp()
{
	puts(
		"Wolfenmaus - Modern mouse control for Wolfenstein 3D/Spear of Destiny      v1.0\n"
		"-------------------------------------------------------------------------------\n"
		"Usage: wolfmaus [options] wolf3d ...\n"
		"\n"
		"Options:\n"
		"  -i<mask>  Inversion mask for mouse buttons (1 = left, 2 = right, 4 = middle)\n"
		"  -s<num>   Sensitivity (0 = slow, 5 = def, 9 = fast, 12 = max, 14+ = invert)\n"
		"  -v        No vert\n"
	);
}

int main(int argc, const char* argv[])
{
	void (__interrupt __far* old_vr_init)(void);
	void (__interrupt __far* old_vr_shutdown)(void);
	int cmd_start;
	int ret;
	const char* arg0;

	if((cmd_start = VRL_ParseCmdLine(argc, argv)) <= 0)
	{
		VRL_PrintHelp();
		return 1;
	}

	old_mouse_interrupt = _dos_getvect(MOUSE_INT);
	_dos_setvect(MOUSE_INT, VRL_MouseService);

	old_vr_init = _dos_getvect(VR_INIT_INT);
	old_vr_shutdown = _dos_getvect(VR_SHUTDOWN_INT);
	_dos_setvect(VR_INIT_INT, VRL_Init);
	_dos_setvect(VR_SHUTDOWN_INT, VRL_Shutdown);

	// Add "virtual" to the given command line
	arg0 = argv[cmd_start];
	argv[cmd_start-1] = argv[cmd_start];
	argv[cmd_start] = "virtual";
	ret = spawnvp(P_WAIT, argv[cmd_start-1], &argv[cmd_start-1]);

	_dos_setvect(VR_INIT_INT, old_vr_init);
	_dos_setvect(VR_SHUTDOWN_INT, old_vr_shutdown);
	_dos_setvect(MOUSE_INT, old_mouse_interrupt);
	return ret;
}
