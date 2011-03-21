/***********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002  Gary Henderson (gary.henderson@ntlworld.com),
                             Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2004  Matthew Kendora

  (c) Copyright 2002 - 2005  Peter Bortas (peter@bortas.org)

  (c) Copyright 2004 - 2005  Joel Yliluoma (http://iki.fi/bisqwit/)

  (c) Copyright 2001 - 2006  John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2006  funkyass (funkyass@spam.shaw.ca),
                             Kris Bleakley (codeviolation@hotmail.com)

  (c) Copyright 2002 - 2010  Brad Jorsch (anomie@users.sourceforge.net),
                             Nach (n-a-c-h@users.sourceforge.net),
                             zones (kasumitokoduck@yahoo.com)

  (c) Copyright 2006 - 2007  nitsuja

  (c) Copyright 2009 - 2010  BearOso,
                             OV2


  BS-X C emulator code
  (c) Copyright 2005 - 2006  Dreamer Nom,
                             zones

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003  _Demo_ (_demo_@zsnes.com),
                             Nach,
                             zsKnight (zsknight@zsnes.com)

  C4 C++ code
  (c) Copyright 2003 - 2006  Brad Jorsch,
                             Nach

  DSP-1 emulator code
  (c) Copyright 1998 - 2006  _Demo_,
                             Andreas Naive (andreasnaive@gmail.com),
                             Gary Henderson,
                             Ivar (ivar@snes9x.com),
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora,
                             Nach,
                             neviksti (neviksti@hotmail.com)

  DSP-2 emulator code
  (c) Copyright 2003         John Weidman,
                             Kris Bleakley,
                             Lord Nightmare (lord_nightmare@users.sourceforge.net),
                             Matthew Kendora,
                             neviksti

  DSP-3 emulator code
  (c) Copyright 2003 - 2006  John Weidman,
                             Kris Bleakley,
                             Lancer,
                             z80 gaiden

  DSP-4 emulator code
  (c) Copyright 2004 - 2006  Dreamer Nom,
                             John Weidman,
                             Kris Bleakley,
                             Nach,
                             z80 gaiden

  OBC1 emulator code
  (c) Copyright 2001 - 2004  zsKnight,
                             pagefault (pagefault@zsnes.com),
                             Kris Bleakley
                             Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code used in 1.39-1.51
  (c) Copyright 2002         Matthew Kendora with research by
                             zsKnight,
                             John Weidman,
                             Dark Force

  SPC7110 and RTC C++ emulator code used in 1.52+
  (c) Copyright 2009         byuu,
                             neviksti

  S-DD1 C emulator code
  (c) Copyright 2003         Brad Jorsch with research by
                             Andreas Naive,
                             John Weidman

  S-RTC C emulator code
  (c) Copyright 2001 - 2006  byuu,
                             John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003         Feather,
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003  _Demo_,
                             pagefault,
                             zsKnight

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound emulator code used in 1.5-1.51
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  Sound emulator code used in 1.52+
  (c) Copyright 2004 - 2007  Shay Green (gblargg@gmail.com)

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  2xSaI filter
  (c) Copyright 1999 - 2001  Derek Liauw Kie Fa

  HQ2x, HQ3x, HQ4x filters
  (c) Copyright 2003         Maxim Stepin (maxim@hiend3d.com)

  NTSC filter
  (c) Copyright 2006 - 2007  Shay Green

  GTK+ GUI code
  (c) Copyright 2004 - 2010  BearOso

  Win32 GUI code
  (c) Copyright 2003 - 2006  blip,
                             funkyass,
                             Matthew Kendora,
                             Nach,
                             nitsuja
  (c) Copyright 2009 - 2010  OV2

  Mac OS GUI code
  (c) Copyright 1998 - 2001  John Stiles
  (c) Copyright 2001 - 2010  zones


  Specific ports contains the works of other authors. See headers in
  individual files.


  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 ***********************************************************************************/




#include "../snes9x.h"
#include "../memmap.h"
#include "../debug.h"
#include "../cpuexec.h"
#include "../ppu.h"
#include "../snapshot.h"
#include "../apu/apu.h"
#include "../display.h"
#include "../gfx.h"
#include "../movie.h"
#include "../netplay.h"

#include "wsnes9x.h"
#include "win32_sound.h"

#include "render.h"
#include "AVIOutput.h"
#include "wlanguage.h"

#include <direct.h>

#include <io.h>
//#define DEBUGGER

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

BYTE *ScreenBuf1 = NULL;
BYTE *ScreenBuffer = NULL;

struct SJoyState Joystick [16];
uint32 joypads [8];
bool8 do_frame_adjust=false;

// avi variables
static uint8* avi_buffer = NULL;
static uint8* avi_sound_buffer = NULL;
static int avi_sound_bytes_per_sample = 0;
static int avi_sound_samples_per_update = 0;
static int avi_width = 0;
static int avi_height = 0;
static uint32 avi_skip_frames = 0;
static bool pre_avi_soundsync = true;
//void Convert8To24 (SSurface *src, SSurface *dst, RECT *srect);
void Convert16To24 (SSurface *src, SSurface *dst, RECT *srect);
void DoAVIOpen(const char* filename);
void DoAVIClose(int reason);

void S9xWinScanJoypads ();

typedef struct
{
    uint8 red;
    uint8 green;
    uint8 blue;
} Colour;

void ConvertDepth (SSurface *src, SSurface *dst, RECT *);
static Colour FixedColours [256];
static uint8 palette [0x10000];

FILE *trace_fs = NULL;

int __fastcall Normalize (int cur, int min, int max)
{
    int Result = 0;

    if ((max - min) == 0)
        return (Result);

    Result = cur - min;
    Result = (Result * 200) / (max - min);
    Result -= 100;

    return (Result);
}

void S9xTextMode( void)
{
}

void S9xGraphicsMode ()
{
}

void S9xExit( void)
{
    SendMessage (GUI.hWnd, WM_COMMAND, ID_FILE_EXIT, 0);
}

const char *S9xGetFilename (const char *ex, enum s9x_getdirtype dirtype)
{
    static char filename [PATH_MAX + 1];
    char dir [_MAX_DIR + 1];
    char drive [_MAX_DRIVE + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
   _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
   _snprintf(filename, sizeof(filename), "%s" SLASH_STR "%s%s",
             S9xGetDirectory(dirtype), fname, ex);
    return (filename);
}

const char *S9xGetFilenameRel (const char *ex)
{
	static char filename [PATH_MAX + 1];
	char dir [_MAX_DIR + 1];
	char drive [_MAX_DRIVE + 1];
	char fname [_MAX_FNAME + 1];
	char ext [_MAX_EXT + 1];

	if (Memory.ROMFilename[0] == '\0')
		strcpy(filename, "");
	else {
		_splitpath (Memory.ROMFilename, drive, dir, fname, ext);
		_makepath (filename, "", "", fname, ex);
	}
	return filename;
}

const void S9xGetLastDirectory (char* buffer, int buf_len)
{
	if(buf_len <= 0)
		return;

	GetCurrentDirectory(buf_len, buffer);
}

#define IS_SLASH(x) ((x) == '\\' || (x) == '/')
static char startDirectory [PATH_MAX];
static bool startDirectoryValid = false;

const char *S9xGetDirectory (enum s9x_getdirtype dirtype)
{
//	_fullpath
	if(!startDirectoryValid)
	{
		// directory from which the executable was launched
//		GetCurrentDirectory(PATH_MAX, startDirectory);

		// directory of the executable's location:
		GetModuleFileName(NULL, startDirectory, PATH_MAX);
        for(int i=strlen(startDirectory); i>=0; i--){
            if(IS_SLASH(startDirectory[i])){
                startDirectory[i]='\0';
                break;
            }
        }

		startDirectoryValid = true;
	}

	SetCurrentDirectory(startDirectory); // makes sure relative paths are relative to the application's location

	const char* rv = startDirectory;

    switch(dirtype){
	  default:
      case DEFAULT_DIR:
	  case HOME_DIR:
		  break;

	  case SCREENSHOT_DIR:
		  rv = GUI.ScreensDir;
		  break;

      case ROM_DIR:
		  rv = GUI.RomDir;
		  break;

      case SRAM_DIR:
		  rv = GUI.SRAMFileDir;
		  break;

	  case BIOS_DIR:
		  rv = GUI.BiosDir;
		  break;

      case SPC_DIR:
		  rv = GUI.SPCDir;
		  break;

	  case IPS_DIR:
	  case CHEAT_DIR:
		  rv = GUI.PatchDir;
		  break;

	  case SNAPSHOT_DIR:
		  rv = GUI.FreezeFileDir;
		  break;

	  case ROMFILENAME_DIR: {
			static char filename [PATH_MAX];
			strcpy(filename, Memory.ROMFilename);
			if(!filename[0])
				rv = GUI.RomDir;
			for(int i=strlen(filename); i>=0; i--){
				if(IS_SLASH(filename[i])){
					filename[i]='\0';
					break;
				}
			}
			rv = filename;
		}
		break;
    }

	mkdir(rv);

	return rv;
}

///*extern "C"*/ const char *S9xGetFilename (const char *e)
//{
//    static char filename [_MAX_PATH + 1];
//    char drive [_MAX_DRIVE + 1];
//    char dir [_MAX_DIR + 1];
//    char fname [_MAX_FNAME + 1];
//    char ext [_MAX_EXT + 1];
//
//    if (strlen (GUI.FreezeFileDir))
//    {
//        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
//        strcpy (filename, GUI.FreezeFileDir);
//        strcat (filename, TEXT("\\"));
//        strcat (filename, fname);
//        strcat (filename, e);
//    }
//    else
//    {
//        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
//        _makepath (filename, drive, dir, fname, e);
//    }
//
//    return (filename);
//}

const char *S9xGetFilenameInc (const char *e, enum s9x_getdirtype dirtype)
{
    static char filename [PATH_MAX + 1];
    char dir [_MAX_DIR + 1];
    char drive [_MAX_DRIVE + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    unsigned int i=0;
    const char *d;

    _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
    d=S9xGetDirectory(dirtype);
    do {
        _snprintf(filename, sizeof(filename), "%s\\%s%03d%s", d, fname, i, e);
        i++;
    } while(_access (filename, 0) == 0 && i!=0);

    return (filename);
}

bool8 S9xOpenSnapshotFile( const char *fname, bool8 read_only, STREAM *file)
{
    char filename [_MAX_PATH + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fn [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];

    _splitpath( fname, drive, dir, fn, ext);
    _makepath( filename, drive, dir, fn, ext[0] == '\0' ? ".000" : ext);

    if (read_only)
    {
	if ((*file = OPEN_STREAM (filename, "rb")))
	    return (TRUE);
    }
    else
    {
	if ((*file = OPEN_STREAM (filename, "wb")))
	    return (TRUE);
        FILE *fs = fopen (filename, "rb");
        if (fs)
        {
            sprintf (String, "Freeze file \"%s\" exists but is read only",
                     filename);
            fclose (fs);
            S9xMessage (S9X_ERROR, S9X_FREEZE_FILE_NOT_FOUND, String);
        }
        else
        {
            sprintf (String, "Cannot create freeze file \"%s\". Directory is read-only or does not exist.", filename);

            S9xMessage (S9X_ERROR, S9X_FREEZE_FILE_NOT_FOUND, String);
        }
    }
    return (FALSE);
}

void S9xCloseSnapshotFile( STREAM file)
{
    CLOSE_STREAM (file);
}

void S9xMessage (int type, int, const char *str)
{
#ifdef DEBUGGER
    static FILE *out = NULL;

    if (out == NULL)
        out = fopen ("out.txt", "w");

    fprintf (out, "%s\n", str);
#endif

    S9xSetInfoString (str);

	// if we can't draw on the screen, messagebox it
	// also send to stderr/stdout depending on message type
	switch(type)
	{
		case S9X_INFO:
			if(Settings.StopEmulation)
				fprintf(stdout, "%s\n", str);
			break;
		case S9X_WARNING:
			fprintf(stdout, "%s\n", str);
			if(Settings.StopEmulation)
				MessageBox(GUI.hWnd, str, "Warning",     MB_OK | MB_ICONWARNING);
			break;
		case S9X_ERROR:
			fprintf(stderr, "%s\n", str);
			if(Settings.StopEmulation)
				MessageBox(GUI.hWnd, str, "Error",       MB_OK | MB_ICONERROR);
			break;
		case S9X_FATAL_ERROR:
			fprintf(stderr, "%s\n", str);
			if(Settings.StopEmulation)
				MessageBox(GUI.hWnd, str, "Fatal Error", MB_OK | MB_ICONERROR);
			break;
		default:
				fprintf(stdout, "%s\n", str);
			break;
	}
}

/*unsigned long _interval = 10;
long _buffernos = 4;
long _blocksize = 4400;
long _samplecount = 440;
long _maxsamplecount = 0;
long _buffersize = 0;

bool StartPlaying = false;
DWORD _lastblock = 0;
bool8 pending_setup = false;
long pending_rate = 0;
bool8 pending_16bit = false;
bool8 pending_stereo = false;*/
extern uint8 *syncSoundBuffer;

//static bool8 block_signal = FALSE;
//static volatile bool8 pending_signal = FALSE;

extern unsigned long START;

void S9xSyncSpeed( void)
{
#ifdef NETPLAY_SUPPORT
    if (Settings.NetPlay)
    {
#if defined (NP_DEBUG) && NP_DEBUG == 2
        printf ("CLIENT: SyncSpeed @%d\n", timeGetTime () - START);
#endif
        S9xWinScanJoypads ();

		LONG prev;
        BOOL success;

	// Wait for heart beat from server
        if ((success = ReleaseSemaphore (GUI.ClientSemaphore, 1, &prev)) &&
            prev == 0)
        {
            // No heartbeats already arrived, have to wait for one.
            // Mop up the ReleaseSemaphore test above...
            WaitForSingleObject (GUI.ClientSemaphore, 0);

            // ... and then wait for the real sync-signal from the
            // client loop thread.
            NetPlay.PendingWait4Sync = WaitForSingleObject (GUI.ClientSemaphore, 100) != WAIT_OBJECT_0;
#if defined (NP_DEBUG) && NP_DEBUG == 2
            if (NetPlay.PendingWait4Sync)
                printf ("CLIENT: PendingWait4Sync1 @%d\n", timeGetTime () - START);
#endif
            IPPU.RenderThisFrame = TRUE;
            IPPU.SkippedFrames = 0;
        }
        else
        {
            if (success)
            {
                // Once for the ReleaseSemaphore above...
                WaitForSingleObject (GUI.ClientSemaphore, 0);
                if (prev == 4 && NetPlay.Waiting4EmulationThread)
                {
                    // Reached the lower behind count threshold - tell the
                    // server its safe to start sending sync pulses again.
                    NetPlay.Waiting4EmulationThread = FALSE;
                    S9xNPSendPause (FALSE);
                }

#if defined (NP_DEBUG) && NP_DEBUG == 2
                if (prev > 1)
                {
                    printf ("CLIENT: SyncSpeed prev: %d @%d\n", prev, timeGetTime () - START);
                }
#endif
            }
            else
            {
#ifdef NP_DEBUG
                printf ("*** CLIENT: SyncSpeed: Release failed @ %d\n", timeGetTime () - START);
#endif
            }

            // ... and again to mop up the already-waiting sync-signal
            NetPlay.PendingWait4Sync = WaitForSingleObject (GUI.ClientSemaphore, 200) != WAIT_OBJECT_0;
#if defined (NP_DEBUG) && NP_DEBUG == 2
            if (NetPlay.PendingWait4Sync)
                printf ("CLIENT: PendingWait4Sync2 @%d\n", timeGetTime () - START);
#endif

	    if (IPPU.SkippedFrames < NetPlay.MaxFrameSkip)
	    {
		IPPU.SkippedFrames++;
		IPPU.RenderThisFrame = FALSE;
	    }
	    else
	    {
		IPPU.RenderThisFrame = TRUE;
		IPPU.SkippedFrames = 0;
	    }
        }
        // Give up remainder of time-slice to any other waiting threads,
        // if they need any time, that is.
        Sleep (0);
        if (!NetPlay.PendingWait4Sync)
        {
            NetPlay.FrameCount++;
            S9xNPStepJoypadHistory ();
        }
    }
    else
#endif

    if (!Settings.TurboMode && Settings.SkipFrames == AUTO_FRAMERATE &&
		!GUI.AVIOut)
    {
		if (!do_frame_adjust)
		{
			IPPU.RenderThisFrame = TRUE;
			IPPU.SkippedFrames = 0;
		}
		else
		{
			if (IPPU.SkippedFrames < Settings.AutoMaxSkipFrames)
			{
				IPPU.SkippedFrames++;
				IPPU.RenderThisFrame = FALSE;
			}
			else
			{
				IPPU.RenderThisFrame = TRUE;
				IPPU.SkippedFrames = 0;
			}
		}
	}
    else
    {
	uint32 SkipFrames;
	if(Settings.TurboMode && !GUI.AVIOut)
		SkipFrames = Settings.TurboSkipFrames;
	else
		SkipFrames = (Settings.SkipFrames == AUTO_FRAMERATE) ? 0 : Settings.SkipFrames;
	if (++IPPU.FrameSkip >= SkipFrames)
	{
	    IPPU.FrameSkip = 0;
	    IPPU.SkippedFrames = 0;
	    IPPU.RenderThisFrame = TRUE;
	}
	else
	{
	    IPPU.SkippedFrames++;
		IPPU.RenderThisFrame = GUI.AVIOut!=0;
	}
    }
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
	return (p + 1);

#ifdef __DJGPP
    if (p = strrchr (f, SLASH_CHAR))
	return (p + 1);
#endif

    return (f);
}

bool8 S9xReadMousePosition (int which, int &x, int &y, uint32 &buttons)
{
    if (which == 0)
    {
        x = GUI.MouseX;
        y = GUI.MouseY;
        buttons = GUI.MouseButtons;
        return (TRUE);
    }

    return (FALSE);
}

bool S9xGetState (WORD KeyIdent)
{
	if(KeyIdent == 0 || KeyIdent == VK_ESCAPE) // if it's the 'disabled' key, it's never pressed
		return true;

	if(!GUI.BackgroundInput && GUI.hWnd != GetActiveWindow())
		return true;

    if (KeyIdent & 0x8000) // if it's a joystick 'key':
    {
        int j = (KeyIdent >> 8) & 15;

        switch (KeyIdent & 0xff)
        {
            case 0: return !Joystick [j].Left;
            case 1: return !Joystick [j].Right;
            case 2: return !Joystick [j].Up;
            case 3: return !Joystick [j].Down;
            case 4: return !Joystick [j].PovLeft;
            case 5: return !Joystick [j].PovRight;
            case 6: return !Joystick [j].PovUp;
            case 7: return !Joystick [j].PovDown;
			case 49:return !Joystick [j].PovDnLeft;
			case 50:return !Joystick [j].PovDnRight;
			case 51:return !Joystick [j].PovUpLeft;
			case 52:return !Joystick [j].PovUpRight;
            case 41:return !Joystick [j].ZUp;
            case 42:return !Joystick [j].ZDown;
            case 43:return !Joystick [j].RUp;
            case 44:return !Joystick [j].RDown;
            case 45:return !Joystick [j].UUp;
            case 46:return !Joystick [j].UDown;
            case 47:return !Joystick [j].VUp;
            case 48:return !Joystick [j].VDown;

            default:
                if ((KeyIdent & 0xff) > 40)
                    return true; // not pressed

                return !Joystick [j].Button [(KeyIdent & 0xff) - 8];
        }
    }

	// the pause key is special, need this to catch all presses of it
	if(KeyIdent == VK_PAUSE)
	{
		if(GetAsyncKeyState(VK_PAUSE)) // not &'ing this with 0x8000 is intentional and necessary
			return false;
	}

    return ((GetKeyState (KeyIdent) & 0x80) == 0);
}

void CheckAxis (int val, int min, int max, bool &first, bool &second)
{
    if (Normalize (val, min, max) < -S9X_JOY_NEUTRAL)
    {
        second = false;
        first = true;
    }
    else
        first = false;

    if (Normalize (val, min, max) > S9X_JOY_NEUTRAL)
    {
        first = false;
        second = true;
    }
    else
        second = false;
}

void S9xWinScanJoypads ()
{
    uint8 PadState[2];
    JOYINFOEX jie;

    for (int C = 0; C != 16; C ++)
    {
        if (Joystick[C].Attached)
        {
            jie.dwSize = sizeof (jie);
            jie.dwFlags = JOY_RETURNALL;

            if (joyGetPosEx (JOYSTICKID1+C, &jie) != JOYERR_NOERROR)
            {
                Joystick[C].Attached = false;
                continue;
            }

            CheckAxis (jie.dwXpos,
                       Joystick[C].Caps.wXmin, Joystick[C].Caps.wXmax,
                       Joystick[C].Left, Joystick[C].Right);
            CheckAxis (jie.dwYpos,
                       Joystick[C].Caps.wYmin, Joystick[C].Caps.wYmax,
                       Joystick[C].Up, Joystick[C].Down);
            CheckAxis (jie.dwZpos,
                       Joystick[C].Caps.wZmin, Joystick[C].Caps.wZmax,
                       Joystick[C].ZUp, Joystick[C].ZDown);
            CheckAxis (jie.dwRpos,
                       Joystick[C].Caps.wRmin, Joystick[C].Caps.wRmax,
                       Joystick[C].RUp, Joystick[C].RDown);
            CheckAxis (jie.dwUpos,
                       Joystick[C].Caps.wUmin, Joystick[C].Caps.wUmax,
                       Joystick[C].UUp, Joystick[C].UDown);
            CheckAxis (jie.dwVpos,
                       Joystick[C].Caps.wVmin, Joystick[C].Caps.wVmax,
                       Joystick[C].VUp, Joystick[C].VDown);

            switch (jie.dwPOV)
            {
                case JOY_POVBACKWARD:
                    Joystick[C].PovDown = true;
                    Joystick[C].PovUp = false;
                    Joystick[C].PovLeft = false;
                    Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
					 break;
				case 4500:
					Joystick[C].PovDown = false;
					Joystick[C].PovUp = false;
					Joystick[C].PovLeft = false;
					Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = true;
					break;
				case 13500:
					Joystick[C].PovDown = false;
					Joystick[C].PovUp = false;
					Joystick[C].PovLeft = false;
					Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = true;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
					break;
				case 22500:
					Joystick[C].PovDown = false;
					Joystick[C].PovUp = false;
					Joystick[C].PovLeft = false;
					Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = true;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
					break;
				case 31500:
					Joystick[C].PovDown = false;
					Joystick[C].PovUp = false;
					Joystick[C].PovLeft = false;
					Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = true;
					Joystick[C].PovUpRight = false;
					break;


                case JOY_POVFORWARD:
                    Joystick[C].PovDown = false;
                    Joystick[C].PovUp = true;
                    Joystick[C].PovLeft = false;
                    Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
                    break;

                case JOY_POVLEFT:
                    Joystick[C].PovDown = false;
                    Joystick[C].PovUp = false;
                    Joystick[C].PovLeft = true;
                    Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
                    break;

                case JOY_POVRIGHT:
                    Joystick[C].PovDown = false;
                    Joystick[C].PovUp = false;
                    Joystick[C].PovLeft = false;
                    Joystick[C].PovRight = true;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
                    break;

                default:
                    Joystick[C].PovDown = false;
                    Joystick[C].PovUp = false;
                    Joystick[C].PovLeft = false;
                    Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
                    break;
            }

            for (int B = 0; B < 32; B ++)
                Joystick[C].Button[B] = (jie.dwButtons & (1 << B)) != 0;
        }
    }

    for (int J = 0; J < 8; J++)
    {
        if (Joypad [J].Enabled)
        {
			// toggle checks
			{
       	     	PadState[0]  = 0;
				PadState[0] |= ToggleJoypadStorage[J].R||TurboToggleJoypadStorage[J].R      ?  16 : 0;
				PadState[0] |= ToggleJoypadStorage[J].L||TurboToggleJoypadStorage[J].L      ?  32 : 0;
				PadState[0] |= ToggleJoypadStorage[J].X||TurboToggleJoypadStorage[J].X      ?  64 : 0;
				PadState[0] |= ToggleJoypadStorage[J].A||TurboToggleJoypadStorage[J].A      ? 128 : 0;

	            PadState[1]  = 0;
				PadState[1] |= ToggleJoypadStorage[J].Right||TurboToggleJoypadStorage[J].Right   ?   1 : 0;
				PadState[1] |= ToggleJoypadStorage[J].Left||TurboToggleJoypadStorage[J].Left     ?   2 : 0;
				PadState[1] |= ToggleJoypadStorage[J].Down||TurboToggleJoypadStorage[J].Down     ?   4 : 0;
				PadState[1] |= ToggleJoypadStorage[J].Up||TurboToggleJoypadStorage[J].Up         ?   8 : 0;
				PadState[1] |= ToggleJoypadStorage[J].Start||TurboToggleJoypadStorage[J].Start   ?  16 : 0;
				PadState[1] |= ToggleJoypadStorage[J].Select||TurboToggleJoypadStorage[J].Select ?  32 : 0;
				PadState[1] |= ToggleJoypadStorage[J].Y||TurboToggleJoypadStorage[J].Y           ?  64 : 0;
				PadState[1] |= ToggleJoypadStorage[J].B||TurboToggleJoypadStorage[J].B           ? 128 : 0;
			}
			// auto-hold AND regular key/joystick presses
			if(S9xGetState(Joypad[J+8].Left))
			{
				PadState[0] ^= (!S9xGetState(Joypad[J].R)||!S9xGetState(Joypad[J+8].R))      ?  16 : 0;
				PadState[0] ^= (!S9xGetState(Joypad[J].L)||!S9xGetState(Joypad[J+8].L))      ?  32 : 0;
				PadState[0] ^= (!S9xGetState(Joypad[J].X)||!S9xGetState(Joypad[J+8].X))      ?  64 : 0;
				PadState[0] ^= (!S9xGetState(Joypad[J].A)||!S9xGetState(Joypad[J+8].A))      ? 128 : 0;

				PadState[1] ^= (!S9xGetState(Joypad[J].Right))  ?   1 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Right_Up))  ? 1 + 8 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Right_Down)) ? 1 + 4 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Left))   ?   2 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Left_Up)) ?   2 + 8 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Left_Down)) ?  2 + 4 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Down))   ?   4 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Up))     ?   8 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Start)||!S9xGetState(Joypad[J+8].Start))  ?  16 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Select)||!S9xGetState(Joypad[J+8].Select)) ?  32 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].Y)||!S9xGetState(Joypad[J+8].Y))      ?  64 : 0;
				PadState[1] ^= (!S9xGetState(Joypad[J].B)||!S9xGetState(Joypad[J+8].B))      ? 128 : 0;
			}

			bool turbofy = !S9xGetState(Joypad[J+8].Up); // All Mod for turbo

			//handle turbo case! (autofire / auto-fire)
			if(turbofy || ((GUI.TurboMask&TURBO_A_MASK))&&(PadState[0]&128) || !S9xGetState(Joypad[J+8].A      )) PadState[0]^=(joypads[J]&128);
			if(turbofy || ((GUI.TurboMask&TURBO_B_MASK))&&(PadState[1]&128) || !S9xGetState(Joypad[J+8].B      )) PadState[1]^=((joypads[J]&(128<<8))>>8);
			if(turbofy || ((GUI.TurboMask&TURBO_Y_MASK))&&(PadState[1]&64) || !S9xGetState(Joypad[J+8].Y       )) PadState[1]^=((joypads[J]&(64<<8))>>8);
			if(turbofy || ((GUI.TurboMask&TURBO_X_MASK))&&(PadState[0]&64) || !S9xGetState(Joypad[J+8].X       )) PadState[0]^=(joypads[J]&64);
			if(turbofy || ((GUI.TurboMask&TURBO_L_MASK))&&(PadState[0]&32) || !S9xGetState(Joypad[J+8].L       )) PadState[0]^=(joypads[J]&32);
			if(turbofy || ((GUI.TurboMask&TURBO_R_MASK))&&(PadState[0]&16) || !S9xGetState(Joypad[J+8].R       )) PadState[0]^=(joypads[J]&16);
			if(turbofy || ((GUI.TurboMask&TURBO_STA_MASK))&&(PadState[1]&16) || !S9xGetState(Joypad[J+8].Start )) PadState[1]^=((joypads[J]&(16<<8))>>8);
			if(turbofy || ((GUI.TurboMask&TURBO_SEL_MASK))&&(PadState[1]&32) || !S9xGetState(Joypad[J+8].Select)) PadState[1]^=((joypads[J]&(32<<8))>>8);
			if(           ((GUI.TurboMask&TURBO_LEFT_MASK))&&(PadState[1]&2)                                    ) PadState[1]^=((joypads[J]&(2<<8))>>8);
			if(           ((GUI.TurboMask&TURBO_UP_MASK))&&(PadState[1]&8)                                      ) PadState[1]^=((joypads[J]&(8<<8))>>8);
			if(           ((GUI.TurboMask&TURBO_RIGHT_MASK))&&(PadState[1]&1)                                   ) PadState[1]^=((joypads[J]&(1<<8))>>8);
			if(           ((GUI.TurboMask&TURBO_DOWN_MASK))&&(PadState[1]&4)                                    ) PadState[1]^=((joypads[J]&(4<<8))>>8);

			if(TurboToggleJoypadStorage[J].A     ) PadState[0]^=(joypads[J]&128);
			if(TurboToggleJoypadStorage[J].B     ) PadState[1]^=((joypads[J]&(128<<8))>>8);
			if(TurboToggleJoypadStorage[J].Y     ) PadState[1]^=((joypads[J]&(64<<8))>>8);
			if(TurboToggleJoypadStorage[J].X     ) PadState[0]^=(joypads[J]&64);
			if(TurboToggleJoypadStorage[J].L     ) PadState[0]^=(joypads[J]&32);
			if(TurboToggleJoypadStorage[J].R     ) PadState[0]^=(joypads[J]&16);
			if(TurboToggleJoypadStorage[J].Start ) PadState[1]^=((joypads[J]&(16<<8))>>8);
			if(TurboToggleJoypadStorage[J].Select) PadState[1]^=((joypads[J]&(32<<8))>>8);
			if(TurboToggleJoypadStorage[J].Left  ) PadState[1] ^= ((joypads[J]&(2<<8))>>8);
			if(TurboToggleJoypadStorage[J].Up    ) PadState[1] ^= ((joypads[J]&(8<<8))>>8);
			if(TurboToggleJoypadStorage[J].Right ) PadState[1] ^= ((joypads[J]&(1<<8))>>8);
			if(TurboToggleJoypadStorage[J].Down  ) PadState[1] ^= ((joypads[J]&(4<<8))>>8);
			//end turbo case...


			// enforce left+right/up+down disallowance here to
			// avoid recording unused l+r/u+d that will cause desyncs
			// when played back with l+r/u+d is allowed
			if(!Settings.UpAndDown)
			{
				if((PadState[1] & 2) != 0)
					PadState[1] &= ~(1);
				if((PadState[1] & 8) != 0)
					PadState[1] &= ~(4);
			}

            joypads [J] = PadState [0] | (PadState [1] << 8) | 0x80000000;
        }
        else
            joypads [J] = 0;
    }
#ifdef NETPLAY_SUPPORT
    if (Settings.NetPlay)
	{
		// Send joypad position update to server
		S9xNPSendJoypadUpdate (joypads [GUI.NetplayUseJoypad1 ? 0 : NetPlay.Player-1]);

		// set input from network
		for (int J = 0; J < NP_MAX_CLIENTS; J++)
			joypads[J] = S9xNPGetJoypad (J);
	}
#endif
}

void InitSnes9X( void)
{
#ifdef DEBUGGER
//    extern FILE *trace;

//    trace = fopen( "SNES9X.TRC", "wt");
//    freopen( "SNES9X.OUT", "wt", stdout);
//    freopen( "SNES9X.ERR", "wt", stderr);

//    CPU.Flags |= TRACE_FLAG;
//    APU.Flags |= TRACE_FLAG;
#endif

//#ifdef GENERATE_OFFSETS_H
//    offsets_h = fopen ("offsets.h", "wt");
//    generate_offsets_h (0, NULL);
//    fclose (offsets_h);
//#endif

    Memory.Init();

	extern void S9xPostRomInit();
	Memory.PostRomInitFunc = S9xPostRomInit;

    ScreenBuf1 = new BYTE [EXT_PITCH * EXT_HEIGHT];

    ScreenBuffer = ScreenBuf1 + EXT_OFFSET;
    memset (ScreenBuf1, 0, EXT_PITCH * EXT_HEIGHT);

    GFX.Pitch = EXT_PITCH;
    GFX.RealPPL = EXT_PITCH;
    GFX.Screen = (uint16*)(ScreenBuf1 + EXT_OFFSET);

    S9xSetWinPixelFormat ();
    S9xGraphicsInit();

	InitializeCriticalSection(&GUI.SoundCritSect);
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

    S9xInitAPU();
	
	ReInitSound();

	S9xMovieInit ();

    for (int C = 0; C != 16; C ++)
        Joystick[C].Attached = joyGetDevCaps (JOYSTICKID1+C, &Joystick[C].Caps,
                                              sizeof( JOYCAPS)) == JOYERR_NOERROR;
}
void DeinitS9x()
{
	if(ScreenBuf1)
		delete [] ScreenBuf1;
	DeleteCriticalSection(&GUI.SoundCritSect);
	CoUninitialize();
	if(GUI.GunSight)
		DestroyCursor(GUI.GunSight);//= LoadCursor (hInstance, MAKEINTRESOURCE (IDC_CURSOR_SCOPE));
    if(GUI.Arrow)
		DestroyCursor(GUI.Arrow);// = LoadCursor (NULL, IDC_ARROW);
	if(GUI.Accelerators)
		DestroyAcceleratorTable(GUI.Accelerators);// = LoadAccelerators (hInstance, MAKEINTRESOURCE (IDR_SNES9X_ACCELERATORS));
}

//void Convert8To24 (SSurface *src, SSurface *dst, RECT *srect)
//{
//    uint32 brightness = IPPU.MaxBrightness >> 1;
//    uint8 levels [32];
//    int height = srect->bottom - srect->top;
//    int width = srect->right - srect->left;
//    int offset1 = srect->top * src->Pitch + srect->left;
//    int offset2 = ((dst->Height - height) >> 1) * dst->Pitch +
//        ((dst->Width - width) >> 1) * 3;
//
//    for (int l = 0; l < 32; l++)
//	levels [l] = l * brightness;
//
//    for (register int y = 0; y < height; y++)
//    {
//        register uint8 *s = ((uint8 *) src->Surface + y * src->Pitch + offset1);
//        register uint8 *d = ((uint8 *) dst->Surface + y * dst->Pitch + offset2);
//
//#ifdef LSB_FIRST
//        if (GUI.RedShift < GUI.BlueShift)
//#else
//	if (GUI.RedShift > GUI.BlueShift)
//#endif
//        {
//            // Order is RGB
//            for (register int x = 0; x < width; x++)
//            {
//                uint16 pixel = PPU.CGDATA [*s++];
//                *(d + 0) = levels [(pixel & 0x1f)];
//                *(d + 1) = levels [((pixel >> 5) & 0x1f)];
//                *(d + 2) = levels [((pixel >> 10) & 0x1f)];
//                d += 3;
//            }
//        }
//        else
//        {
//            // Order is BGR
//            for (register int x = 0; x < width; x++)
//            {
//                uint16 pixel = PPU.CGDATA [*s++];
//                *(d + 0) = levels [((pixel >> 10) & 0x1f)];
//                *(d + 1) = levels [((pixel >> 5) & 0x1f)];
//                *(d + 2) = levels [(pixel & 0x1f)];
//                d += 3;
//            }
//        }
//    }
//}


void S9xAutoSaveSRAM ()
{
    Memory.SaveSRAM (S9xGetFilename (".srm", SRAM_DIR));
}

void S9xSetPause (uint32 mask)
{
    Settings.ForcedPause |= mask;
	S9xSetSoundMute(TRUE);
}

void S9xClearPause (uint32 mask)
{
    Settings.ForcedPause &= ~mask;
    if (!Settings.ForcedPause)
    {
        // Wake up the main loop thread just if its blocked in a GetMessage call.
        PostMessage (GUI.hWnd, WM_NULL, 0, 0);
    }
}

bool JustifierOffscreen()
{
	return (bool)((GUI.MouseButtons&2)!=0);
}

//void JustifierButtons(uint32& justifiers)
//{
//	if(IPPU.Controller==SNES_JUSTIFIER_2)
//	{
//		if((GUI.MouseButtons&1)||(GUI.MouseButtons&2))
//		{
//			justifiers|=0x00200;
//		}
//		if(GUI.MouseButtons&4)
//		{
//			justifiers|=0x00800;
//		}
//	}
//	else
//	{
//		if((GUI.MouseButtons&1)||(GUI.MouseButtons&2))
//		{
//			justifiers|=0x00100;
//		}
//		if(GUI.MouseButtons&4)
//		{
//			justifiers|=0x00400;
//		}
//	}
//}

#ifdef MK_APU_RESAMPLE
void ResampleTo16000HzM16(uint16* input, uint16*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i++)
	{
		output[i]=(input[i*2]+input[(2*i)+1])>>1;
	}
}

void ResampleTo16000HzS16(uint16* input, uint16*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i+=2)
	{
		output[i]=(input[i*2]+input[(2*(i+1))])>>1;
		output[i+1]=(input[(i*2)+1]+input[(2*(i+1))+1])>>1;
	}
}
void ResampleTo8000HzM16(uint16* input, uint16*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i++)
	{
		output[i]=(input[i*4]+input[(4*i)+1]+input[(4*i)+2]+input[(4*i)+3])>>2;
	}
}

void ResampleTo8000HzS16(uint16* input, uint16*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i+=2)
	{
		output[i]=(input[i*4]+input[(4*i)+2]+input[(4*(i+1))]+input[(4*(i+1))+2])>>2;
		output[i+1]=(input[(i*4)+1]+input[(4*i)+3]+input[(4*(i+1))+1]+input[(4*(i+1))+3])>>2;
	}
}

void ResampleTo16000HzM8(uint8* input, uint8*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i++)
	{
		output[i]=(input[i*2]+input[(2*i)+1])>>1;
	}
}

void ResampleTo16000HzS8(uint8* input, uint8*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i+=2)
	{
		output[i]=(input[i*2]+input[(2*(i+1))])>>1;
		output[i+1]=(input[(i*2)+1]+input[(2*(i+1))+1])>>1;
	}
}
void ResampleTo8000HzM8(uint8* input, uint8*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i++)
	{
		output[i]=(input[i*4]+input[(4*i)+1]+input[(4*i)+2]+input[(4*i)+3])>>2;
	}
}

void ResampleTo8000HzS8(uint8* input, uint8*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i+=2)
	{
		output[i]=(input[i*4]+input[(4*i)+2]+input[(4*(i+1))]+input[(4*(i+1))+2])>>2;
		output[i+1]=(input[(i*4)+1]+input[(4*i)+3]+input[(4*(i+1))+1]+input[(4*(i+1))+3])>>2;
	}
}

#endif

void DoAVIOpen(const char* filename)
{
	// close current instance
	if(GUI.AVIOut)
	{
		AVIClose(&GUI.AVIOut);
		GUI.AVIOut = NULL;
	}

	CloseSoundDevice();
	pre_avi_soundsync = Settings.SoundSync;
	Settings.SoundSync = false;

	// create new writer
	AVICreate(&GUI.AVIOut);

	int framerate = Memory.ROMFramesPerSecond;
	int frameskip = Settings.SkipFrames;
	if(frameskip == AUTO_FRAMERATE)
		frameskip = 1;
	else
		frameskip++;

	AVISetFramerate(framerate, frameskip, GUI.AVIOut);

	avi_width = IPPU.RenderedScreenWidth;
	avi_height = IPPU.RenderedScreenHeight;
	avi_skip_frames = Settings.SkipFrames;

	if(GUI.HeightExtend && avi_height < SNES_HEIGHT_EXTENDED)
		avi_height = SNES_HEIGHT_EXTENDED;

	if(avi_height % 2 != 0) // most codecs can't handle odd-height images
		avi_height++;

	BITMAPINFOHEADER bi;
	memset(&bi, 0, sizeof(bi));
	bi.biSize = 0x28;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biWidth = avi_width;
	bi.biHeight = avi_height;
	bi.biSizeImage = 3*bi.biWidth*bi.biHeight;

	AVISetVideoFormat(&bi, GUI.AVIOut);

	WAVEFORMATEX wfx;

	wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = Settings.Stereo ? 2 : 1;
    wfx.nSamplesPerSec = Settings.SoundPlaybackRate;
    wfx.nBlockAlign = (Settings.SixteenBitSound ? 2 : 1) * (Settings.Stereo ? 2 : 1);
    wfx.wBitsPerSample = Settings.SixteenBitSound ? 16 : 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

	if(!GUI.Mute)
	{
		AVISetSoundFormat(&wfx, GUI.AVIOut);
	}

	if(!AVIBegin(filename, GUI.AVIOut))
	{
		DoAVIClose(2);
		GUI.AVIOut = NULL;
		return;
	}

	avi_sound_samples_per_update = (wfx.nSamplesPerSec * frameskip) / framerate;
	avi_sound_bytes_per_sample = wfx.nBlockAlign;

	// init buffers
	avi_buffer = new uint8[3*avi_width*avi_height];
	avi_sound_buffer = new uint8[avi_sound_samples_per_update * avi_sound_bytes_per_sample];
}

void DoAVIClose(int reason)
{
	if(!GUI.AVIOut)
	{
		return;
	}

	AVIClose(&GUI.AVIOut);
	GUI.AVIOut = NULL;

	delete [] avi_buffer;
	delete [] avi_sound_buffer;

	avi_buffer = NULL;
	avi_sound_buffer = NULL;

	Settings.SoundSync = pre_avi_soundsync;
	ReInitSound();

	switch(reason)
	{
	case 1:
		// emu settings changed
		S9xMessage(S9X_INFO, S9X_AVI_INFO, AVI_CONFIGURATION_CHANGED);
		break;
	case 2:
		// create AVI failed
		S9xMessage(S9X_INFO, S9X_AVI_INFO, AVI_CREATION_FAILED);
		break;
	default:
		// print nothing
		break;
	}
}

void DoAVIVideoFrame(SSurface* source_surface, int Width, int Height/*, bool8 sixteen_bit*/)
{
	static uint32 lastFrameCount=0;
	if(!GUI.AVIOut || !avi_buffer || (IPPU.FrameCount==lastFrameCount))
	{
		return;
	}
	lastFrameCount=IPPU.FrameCount;

	// check configuration
	const WAVEFORMATEX* pwfex = NULL;
	WAVEFORMATEX wfx;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = Settings.Stereo ? 2 : 1;
    wfx.nSamplesPerSec = Settings.SoundPlaybackRate;
    wfx.nBlockAlign = (Settings.SixteenBitSound ? 2 : 1) * (Settings.Stereo ? 2 : 1);
    wfx.wBitsPerSample = Settings.SixteenBitSound ? 16 : 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
	if(//avi_width != Width ||
		//avi_height != Height ||
		avi_skip_frames != Settings.SkipFrames ||
		(AVIGetSoundFormat(GUI.AVIOut, &pwfex) && memcmp(pwfex, &wfx, sizeof(WAVEFORMATEX))))
	{
		DoAVIClose(1);
		return;
	}

	// convert to bitdepth 24
	SSurface avi_dest_surface;
	RECT full_rect;
	avi_dest_surface.Surface = avi_buffer;
	avi_dest_surface.Pitch = avi_width * 3;
	avi_dest_surface.Width = avi_width;
	avi_dest_surface.Height = avi_height;
	full_rect.top = 0;
	full_rect.left = 0;
	full_rect.bottom = avi_height;
	full_rect.right = avi_width;
	//if(sixteen_bit)
	//{
		Convert16To24(source_surface, &avi_dest_surface, &full_rect);
	//}
	//else
	//{
	//	Convert8To24(source_surface, &avi_dest_surface, &full_rect);
	//}

	// flip the image vertically
	const int pitch = 3*avi_width;
	int y;
	for(y=0; y<avi_height>>1; ++y)
	{
		uint8* lo_8 = avi_buffer+y*pitch;
		uint8* hi_8 = avi_buffer+(avi_height-1-y)*pitch;
		uint32* lo_32=(uint32*)lo_8;
		uint32* hi_32=(uint32*)hi_8;

		int q;
		{
			register uint32 a, b;
			for(q=pitch>>4; q>0; --q)
			{
				a=*lo_32;  b=*hi_32;  *lo_32=b;  *hi_32=a;  ++lo_32;  ++hi_32;
				a=*lo_32;  b=*hi_32;  *lo_32=b;  *hi_32=a;  ++lo_32;  ++hi_32;
				a=*lo_32;  b=*hi_32;  *lo_32=b;  *hi_32=a;  ++lo_32;  ++hi_32;
				a=*lo_32;  b=*hi_32;  *lo_32=b;  *hi_32=a;  ++lo_32;  ++hi_32;
			}
		}

		{
			register uint8 c, d;
			for(q=(pitch&0x0f); q>0; --q)
			{
				c=*lo_8;  d=*hi_8;  *lo_8=d;  *hi_8=c;
			}
		}
	}

	// write to AVI
	AVIAddVideoFrame(avi_buffer, GUI.AVIOut);

	// generate sound
	if(pwfex)
	{
		const int stereo_multiplier = (Settings.Stereo) ? 2 : 1;
		
		S9xMixSamples(avi_sound_buffer, avi_sound_samples_per_update*stereo_multiplier);

		AVIAddSoundSamples(avi_sound_buffer, avi_sound_samples_per_update, GUI.AVIOut);
	}
}