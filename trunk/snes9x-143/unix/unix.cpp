/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/
#include <signal.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
/* FIXME: Linux alpha (aristocat) has a broken timercmp. Make testcase */
#if defined(__linux)
# ifndef timercmp
#  define timercmp(tvp, uvp, cmp)\
        ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
        (tvp)->tv_sec == (uvp)->tv_sec &&\
        (tvp)->tv_usec cmp (uvp)->tv_usec)
# endif
#endif
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef USE_THREADS
#include <pthread.h>
#include <sched.h>

pthread_t thread;
pthread_mutex_t mutex;
#endif

#if !defined(NOSOUND) && defined(__linux)
#include <sys/soundcard.h>
#include <sys/mman.h>
#endif

#if !defined(NOSOUND) && defined(__sun)
#ifdef __SVR4
#include <sys/audioio.h>
#else
#include <sun/audioio.h>
#endif
#endif

//Breaks sol9 and probably others.
//#if defined(__sun) && defined(__GNUC__)
//typedef void (*SIG_PF)();
//#endif

#include <sys/select.h>

#include "snes9x.h"
#include "memmap.h"
#include "debug.h"
#include "cpuexec.h"
#include "ppu.h"
#include "snapshot.h"
#include "apu.h"
#include "display.h"
#include "gfx.h"
#include "soundux.h"
#include "spc700.h"
#include "spc7110.h"
#include "logger.h"
#include "version.h"

#include "x11.h"
#include "snapshot.h"

#include "s9xlua.h"

#ifdef SPCTOOL
#include "spctool/spc700.h"
extern "C" void TraceSPC (unsigned char *PC, unsigned short YA, unsigned char X,
			  SPCFlags PS, unsigned char *SP);
#endif

#ifdef _NETPLAY_SUPPORT
#include "netplay.h"
#endif

uint32 joypads [5] = {0};
int NumControllers = 5;

fd_set waitfds;
int maxwaitfds = -1;

#ifdef JOYSTICK_SUPPORT
#if defined(__linux)
#include <linux/joystick.h>
int js_fd [4] = {-1, -1, -1, -1};
int js_map_button [4][16] = {
    {
	SNES_B_MASK, SNES_A_MASK, SNES_A_MASK,
	SNES_Y_MASK, SNES_X_MASK, SNES_B_MASK,
	SNES_TL_MASK, SNES_TR_MASK,
	SNES_START_MASK, SNES_SELECT_MASK, 0, 0, 0, 0, 0, 0
    },
    {
	SNES_B_MASK, SNES_A_MASK, SNES_A_MASK,
	SNES_Y_MASK, SNES_X_MASK, SNES_B_MASK,
	SNES_TL_MASK, SNES_TR_MASK,
	SNES_START_MASK, SNES_SELECT_MASK, 0, 0, 0, 0, 0, 0
    },
    {
	SNES_B_MASK, SNES_A_MASK, SNES_A_MASK,
	SNES_Y_MASK, SNES_X_MASK, SNES_B_MASK,
	SNES_TL_MASK, SNES_TR_MASK,
	SNES_START_MASK, SNES_SELECT_MASK, 0, 0, 0, 0, 0, 0
    },
    {
	SNES_B_MASK, SNES_A_MASK, SNES_A_MASK,
	SNES_Y_MASK, SNES_X_MASK, SNES_B_MASK,
	SNES_TL_MASK, SNES_TR_MASK,
	SNES_START_MASK, SNES_SELECT_MASK, 0, 0, 0, 0, 0, 0
    }
};

#if 0
SNES_A_MASK, SNES_B_MASK, SNES_X_MASK, SNES_Y_MASK,
		            SNES_TL_MASK, SNES_TR_MASK, SNES_START_MASK, SNES_SELECT_MASK,
			    0, 0, 0, 0, 0, 0, 0, 0},
			   {SNES_A_MASK, SNES_B_MASK, SNES_X_MASK, SNES_Y_MASK,
			    SNES_TL_MASK, SNES_TR_MASK, SNES_START_MASK, SNES_SELECT_MASK,
			    0, 0, 0, 0, 0, 0, 0, 0},
			   {SNES_A_MASK, SNES_B_MASK, SNES_X_MASK, SNES_Y_MASK,
			    SNES_TL_MASK, SNES_TR_MASK, SNES_START_MASK, SNES_SELECT_MASK,
			    0, 0, 0, 0, 0, 0, 0, 0},
			   {SNES_A_MASK, SNES_B_MASK, SNES_X_MASK, SNES_Y_MASK,
			    SNES_TL_MASK, SNES_TR_MASK, SNES_START_MASK, SNES_SELECT_MASK,
			    0, 0, 0, 0, 0, 0, 0, 0}};
#endif
char *js_device [4] = {"/dev/js0", "/dev/js1", "/dev/js2", "/dev/js3"};
#endif

int FromPause = 1;

void InitJoysticks ();
void ReadJoysticks ();
#endif

void InitTimer ();
void *S9xProcessSound (void *);
void S9xProcessEvents(bool8 blocking);

char *rom_filename = NULL;
char *snapshot_filename = NULL;
char *SDD1_pack = NULL;

//FIXME: I see no reason not to configureenable this for all Unixen
#if defined(DEBUGGER) && (defined(__linux) || defined(__sun))
static void sigbrkhandler(int)
{
    CPU.Flags |= DEBUG_MODE_FLAG;
    signal(SIGINT, (SIG_PF) sigbrkhandler);
}
#endif

void OutOfMemory ()
{
    fprintf (stderr, "Snes9X: Memory allocation failure -"
             " not enough RAM/virtual memory available.\n"
             "S9xExiting...\n");
    Memory.Deinit ();
    S9xDeinitAPU ();
    
    exit (1);
}

static void S9xInfoMessage(const char *msg)
{
    S9xSetInfoString(msg);
    if(Settings.Paused) puts(msg);
}


void S9xParseArg (char **argv, int &i, int argc)
{
#ifdef JOYSTICK_SUPPORT
    if (strcmp (argv [i], "-j") == 0 ||
	     strcasecmp (argv [i], "-nojoy") == 0)
	Settings.JoystickEnabled = FALSE;
    else if (strcasecmp (argv [i], "-joydev1") == 0)
    {
	if (i + 1 < argc)
	    js_device[0] = argv[++i];
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joydev2") == 0)
    {
	if (i + 1 < argc)
	    js_device[1] = argv[++i];
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joymap1") == 0)
    {
	if (i + 8 < argc)
	{
	    int t;

            for (t=0; t<15; t++)
                js_map_button [0][t] = 0;

	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_A_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_B_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_X_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_Y_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_TL_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_TR_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_START_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_SELECT_MASK;
	}
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joymap2") == 0)
    {
	if (i + 8 < argc)
	{
	    int t;

            for (t=0; t<15; t++)
                js_map_button [1][t] = 0;

	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_A_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_B_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_X_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_Y_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_TL_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_TR_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_START_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_SELECT_MASK;
	}
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joymap3") == 0)
    {
	if (i + 8 < argc)
	{
	    int t;

            for (t=0; t<15; t++)
                js_map_button [2][t] = 0;

	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_A_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_B_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_X_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_Y_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_TL_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_TR_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_START_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_SELECT_MASK;
	}
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joymap4") == 0)
    {
	if (i + 8 < argc)
	{
	    int t;

            for (t=0; t<15; t++)
                js_map_button [3][t] = 0;

	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_A_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_B_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_X_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_Y_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_TL_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_TR_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_START_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_SELECT_MASK;
	}
	else
	    S9xUsage ();
    }
    else 
#endif
    if (strcasecmp (argv [i], "-b") == 0 ||
	     strcasecmp (argv [i], "-bs") == 0 ||
	     strcasecmp (argv [i], "-buffersize") == 0)
    {
	if (i + 1 < argc)
	    Settings.SoundBufferSize = atoi (argv [++i]);
	else
	    S9xUsage ();
    }
    else if (strcmp (argv [i], "-l") == 0 ||
	     strcasecmp (argv [i], "-loadsnapshot") == 0)
    {
	if (i + 1 < argc)
	    snapshot_filename = argv [++i];
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-sdd1-pack") == 0)
    {
	if (i + 1 < argc)
	    SDD1_pack = argv [++i];
	else
	    S9xUsage ();
    }
    else
	S9xParseDisplayArg (argv, i, argc);
}

#include "cheats.h"

void sigcont(int number)
{
	printf("Sigcont\n");
	::FromPause = 1;
}

int main (int argc, char **argv)
{
    signal(SIGCONT, sigcont);
    if (argc < S9xMinCommandLineArgs ())
	S9xUsage ();
	
    ZeroMemory (&Settings, sizeof (Settings));
    Settings.HighSpeedSeek = 0;
#ifdef JOYSTICK_SUPPORT
    Settings.JoystickEnabled = TRUE;
#else
    Settings.JoystickEnabled = FALSE;
#endif
    Settings.UpAndDown = 0;
    Settings.SoundPlaybackRate = 4;
    Settings.Stereo = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.CyclesPercentage = 100;
    Settings.DisableSoundEcho = FALSE;
#ifndef NOSOUND
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
    Settings.InterpolatedSound = TRUE;
#else
    Settings.APUEnabled = Settings.NextAPUEnabled = FALSE;
#endif
    Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
    Settings.SkipFrames = AUTO_FRAMERATE;
    Settings.ShutdownMaster = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.DisableSampleCaching = FALSE;
    Settings.DisableMasterVolume = FALSE;
    Settings.Mouse = TRUE;
    Settings.SuperScope = TRUE;
    Settings.MultiPlayer5 = TRUE;
    Settings.ControllerOption = SNES_JOYPAD;
    Settings.Transparency = FALSE;
//    Settings.SixteenBit = FALSE;
    Settings.SupportHiRes = FALSE;
    Settings.NetPlay = FALSE;
    Settings.ServerName [0] = 0;
    Settings.ThreadSound = FALSE;
    Settings.AutoSaveDelay = 30;
#ifdef _NETPLAY_SUPPORT
    Settings.Port = NP_DEFAULT_PORT;
#endif
    Settings.ApplyCheats = TRUE;
    Settings.TurboMode = FALSE;
    Settings.TurboSkipFrames = 40;
    Settings.StretchScreenshots = 1;
    rom_filename = S9xParseArgs (argv, argc);

    Settings.Transparency = Settings.ForceTransparency;
    if (Settings.ForceNoTransparency)
	Settings.Transparency = FALSE;

//    if (Settings.Transparency)
//	Settings.SixteenBit = TRUE;

    Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;

    if (!Memory.Init () || !S9xInitAPU())
	OutOfMemory ();

    S9xInitSound (Settings.SoundPlaybackRate, Settings.Stereo,
                  Settings.SoundBufferSize);

    if (!Settings.APUEnabled)
	S9xSetSoundMute (TRUE);

    uint32 saved_flags = CPU.Flags;

#ifdef GFX_MULTI_FORMAT
    S9xSetRenderPixelFormat (RGB565);
#endif

    if (rom_filename)
    {
	if (!Memory.LoadROM (rom_filename))
	{
	    char dir [_MAX_DIR + 1];
	    char drive [_MAX_DRIVE + 1];
	    char name [_MAX_FNAME + 1];
	    char ext [_MAX_EXT + 1];
	    char fname [_MAX_PATH + 1];

	    _splitpath (rom_filename, drive, dir, name, ext);
	    _makepath (fname, drive, dir, name, ext);

	    strcpy (fname, S9xGetDirectory (ROM_DIR));
	    strcat (fname, SLASH_STR);
	    strcat (fname, name);
	    if (ext [0])
	    {
		strcat (fname, ".");
		strcat (fname, ext);
	    }
	    _splitpath (fname, drive, dir, name, ext);
	    _makepath (fname, drive, dir, name, ext);
	    if (!Memory.LoadROM (fname))
	    {
		fprintf (stderr, "Error opening: %s\n", rom_filename);
		exit (1);
	    }
	}
	Memory.LoadSRAM (S9xGetFilename (".srm", SRAM_DIR));
	S9xLoadCheatFile (S9xGetFilename (".cht", CHEAT_DIR));
    }
    else
    {
	S9xReset ();
	Settings.Paused |= 2;
    }
    CPU.Flags = saved_flags;

#if !defined(__MSDOS) && defined(DEBUGGER)
#if defined(__unix) && !defined(__NeXT__)
    struct sigaction sa;
#if defined(__linux)
    sa.sa_handler = sigbrkhandler;
#else
    sa.sa_handler = (SIG_PF) sigbrkhandler;
#endif

#ifdef SA_RESTART
    sa.sa_flags = SA_RESTART;
#else
    sa.sa_flags = 0;
#endif

    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
#else
    signal(SIGINT, (SIG_PF) sigbrkhandler);
#endif
#endif

    S9xInitInputDevices ();

    S9xInitDisplay (argc, argv);
    if (!S9xGraphicsInit ())
	OutOfMemory ();
	
    S9xTextMode ();

#ifdef _NETPLAY_SUPPORT
    if (strlen (Settings.ServerName) == 0)
    {
	char *server = getenv ("S9XSERVER");
	if (server)
	{
	    strncpy (Settings.ServerName, server, 127);
	    Settings.ServerName [127] = 0;
	}
    }
    char *port = getenv ("S9XPORT");
    if (Settings.Port >= 0 && port)
	Settings.Port = atoi (port);
    else
    if (Settings.Port < 0)
	Settings.Port = -Settings.Port;

    if (Settings.NetPlay)
    {
	int player;

	if (!S9xNetPlayConnectToServer (Settings.ServerName, Settings.Port,
					Memory.ROMName, player))
	{
	    fprintf (stderr, "Failed to connected to Snes9x netplay"
                     " server \"%s\" on port %d.\n",
                     Settings.ServerName, Settings.Port);
	    S9xExit ();
	}
	fprintf (stderr, "Connected to \"%s\" on port %d as"
                 " player #%d playing \"%s\"\n",
		 Settings.ServerName, Settings.Port, player, Memory.ROMName);
    }
    
#endif

    if (snapshot_filename)
    {
	int Flags = CPU.Flags & (DEBUG_MODE_FLAG | TRACE_FLAG);
	if (!S9xLoadSnapshot (snapshot_filename))
	    exit (1);
	CPU.Flags |= Flags;
    }

    S9xGraphicsMode ();

    sprintf (String, "\"%s\" %s: %s", Memory.ROMName, SNES9X_NAME, SNES9X_VERSION_STRING);
    S9xSetTitle (String);
    
#ifdef JOYSTICK_SUPPORT
    uint32 JoypadSkip = 0;
#endif

    InitTimer ();
    if (!Settings.APUEnabled)
	S9xSetSoundMute (FALSE);

    /* FIXME: Is someone using this dead code, or should it go? */
#if 0
    {
	FILE *fs = fopen ("test.bin", "r");
	if (fs)
	{
	    memset (IAPU.RAM, 0, 1024 * 64);
	    int bytes = fread (IAPU.RAM + 1024, 1, 13, fs);
	    bytes = fread (IAPU.RAM + 1024, 1, 1024 * 63, fs);
	    fclose (fs);
#ifdef SPCTOOL
	    _FixSPC (1024, 0, 0, 0, 0, 0xff);
#else
	    IAPU.PC = IAPU.RAM + 1024;
#endif
	    APU.Flags ^= TRACE_FLAG;
	    extern FILE *apu_trace;
	    if (APU.Flags & TRACE_FLAG)
	    {
#ifdef SPCTOOL
		printf ("ENABLED\n");
		_SetSPCDbg (TraceSPC);                   //Install debug handler
#endif
		if (apu_trace == NULL)
		    apu_trace = fopen ("aputrace.log", "wb");
	    }
	    CPU.Cycles = 1024 * 10;
	    APU_EXECUTE ();
	    exit (0);
	}
    }
#endif

    while (1)
    {
	if ((!Settings.Paused) || Settings.FrameAdvance || Settings.HighSpeedSeek > 0 || S9xLuaSpeed() > 0
#ifdef DEBUGGER
	    || (CPU.Flags & (DEBUG_MODE_FLAG | SINGLE_STEP_FLAG))
#endif
           )
           {
            Settings.FrameAdvance = 0;
	    S9xMainLoop ();
	    S9xLuaFrameBoundary();

           }
	if (Settings.Paused
#ifdef DEBUGGER
	    || (CPU.Flags & DEBUG_MODE_FLAG)
#endif
           )
	{
	    S9xSetSoundMute (TRUE);
	}

#ifdef DEBUGGER
	if (CPU.Flags & DEBUG_MODE_FLAG)
	{
	    S9xDoDebug ();
	}
	else
#endif
	if (Settings.Paused && !Settings.HighSpeedSeek)
	{
	    S9xProcessEvents (FALSE);
            fd_set copy = waitfds;
	    select(maxwaitfds+1, &copy, NULL, NULL, NULL);
	}

#ifdef JOYSTICK_SUPPORT
	if (Settings.JoystickEnabled && (JoypadSkip++ & 1) == 0)
	    ReadJoysticks ();
#endif
	S9xProcessEvents (FALSE);
	
	if (!Settings.Paused
#ifdef DEBUGGER
	    && !(CPU.Flags & DEBUG_MODE_FLAG)
#endif	    
           )
	{
	    S9xSetSoundMute (FALSE);
	}
    }
    return (0);
}

void S9xAutoSaveSRAM ()
{
    Memory.SaveSRAM (S9xGetFilename (".srm", SRAM_DIR));
}

void S9xExit ()
{
  if(Settings.SPC7110)
    (*CleanUp7110)();

   S9xSetSoundMute (TRUE);
    S9xDeinitDisplay ();
    Memory.SaveSRAM (S9xGetFilename (".srm", SRAM_DIR));
    S9xSaveCheatFile (S9xGetFilename (".cht", CHEAT_DIR));
    Memory.Deinit ();
    S9xDeinitAPU ();

#ifdef _NETPLAY_SUPPORT
    if (Settings.NetPlay)
	S9xNetPlayDisconnect ();
#endif
    
    exit (0);
}

void S9xInitInputDevices ()
{
#ifdef JOYSTICK_SUPPORT
    InitJoysticks ();
#endif
}

#ifdef JOYSTICK_SUPPORT
void InitJoysticks ()
{
#ifdef JSIOCGVERSION
    int version;
    unsigned char axes, buttons;

    if ((js_fd [0] = open (js_device [0], O_RDONLY | O_NONBLOCK)) < 0)
    {
#ifdef DEBUGGER
        perror (js_device [0]);
#endif
        fprintf(stderr, "joystick: No joystick found.\n");
	return;
    }

/*
    if (ioctl (js_fd [0], JSIOCGVERSION, &version))
    {
        fprintf(stderr, "joystick: You need at least driver version 1.0"
                " for joystick support.\n");
	close (js_fd [0]);
	return;
    }
*/
    FD_SET(js_fd[0], &waitfds);
    js_fd [1] = open (js_device [1], O_RDONLY | O_NONBLOCK);
    if (js_fd[1] >= 0) FD_SET(js_fd[1], &waitfds);
    js_fd [2] = open (js_device [2], O_RDONLY | O_NONBLOCK);
    if (js_fd[2] >= 0) FD_SET(js_fd[2], &waitfds); 

    js_fd [3] = open (js_device [3], O_RDONLY | O_NONBLOCK);
    if (js_fd[3] >= 0) FD_SET(js_fd[3], &waitfds); 

#define MAX(a,b) ((a) > (b) ? (a) : (b))

    maxwaitfds = MAX(MAX(maxwaitfds, js_fd[0]),MAX(js_fd[1], MAX(js_fd[2], js_fd[3])));

#undef MAX
/*
#ifdef JSIOCGNAME
    char name [130];
    bzero (name, 128);
    if (ioctl (js_fd [0], JSIOCGNAME(128), name) > 0) 
    {
        printf ("Using %s (%s) as pad1", name, js_device [0]);
        if (js_fd [1] > 0)
	{
	    ioctl (js_fd [1], JSIOCGNAME(128), name);
	    printf ("and %s (%s) as pad2", name, js_device [1]);
	}
    } 
    else
#endif
    {
	ioctl (js_fd [0], JSIOCGAXES, &axes);
	ioctl (js_fd [0], JSIOCGBUTTONS, &buttons);
	printf ("Using %d-axis %d-button joystick (%s) as pad1", axes, buttons, js_device [0]);
	if (js_fd [1] > 0)
	{
	    ioctl (js_fd [0], JSIOCGAXES, &axes);
	    ioctl (js_fd [0], JSIOCGBUTTONS, &buttons);
	    printf (" and %d-axis %d-button (%s) as pad2", axes, buttons, js_device [1]);
	}
    }
*/
    puts (".");
#endif
}


#ifdef DEHACKED_JOYPAD

int js_ss_slot = 100;
int js_ss_press = 0;


void ReadJoysticks ()
{

#ifdef JSIOCGVERSION
    struct js_event js_ev;
    int i;

    for (i = 0; i < 4 && js_fd [i] >= 0; i++)
    {
	while (read (js_fd[i], &js_ev, sizeof (struct js_event)) == sizeof (struct js_event) )
	{
	    switch (js_ev.type & ~JS_EVENT_INIT)
	    {
	    case JS_EVENT_AXIS:
		if (js_ev.number == 4)
		{
		    if(js_ev.value < -16384)
		    {
			joypads [i] |= SNES_LEFT_MASK; 
			joypads [i] &= ~SNES_RIGHT_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			joypads [i] &= ~SNES_LEFT_MASK;
			joypads [i] |= SNES_RIGHT_MASK; 
			break;
		    }
		    joypads [i] &= ~SNES_LEFT_MASK;
		    joypads [i] &= ~SNES_RIGHT_MASK;
		    break;	
		}

		if (js_ev.number == 5)
		{
		    if (js_ev.value < -16384)
		    {
			joypads [i] |= SNES_UP_MASK; 
			joypads [i] &= ~SNES_DOWN_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			joypads [i] &= ~SNES_UP_MASK;
			joypads [i] |= SNES_DOWN_MASK; 
			break;
		    }
		    joypads [i] &= ~SNES_UP_MASK;
		    joypads [i] &= ~SNES_DOWN_MASK;
		    break;	
		}
		
		if (js_ev.number == 0)
		{
		
		    if (js_ev.value < -16384 && !js_ss_press)
		    {
		    	js_ss_slot--;
		    	fprintf(stderr, "\nNew savestate slot: %d\n", js_ss_slot);
			js_ss_press=1;
		    	break;
		    }
		    if (js_ev.value > 16384 && !js_ss_press)
		    {
		    	js_ss_slot++;
		    	fprintf(stderr, "\nNew savestate slot: %d\n", js_ss_slot);
		    	js_ss_press = 1;
		    	break;
		    }
		    if (js_ev.value == 0)
			    js_ss_press = 0;
		    break;
		}
		// if (js_ev.number == 3) // axis not used...
		
		if (js_ev.number == 2)
		{
		    if (js_ev.value > 16384 || js_ev.value < -16384)
		    {
			joypads [i] &= ~(SNES_TL_MASK | SNES_TR_MASK);
			joypads [i] |= SNES_TL_MASK | SNES_TR_MASK; 
			break;
		    }
		    joypads [i] &= ~SNES_TR_MASK;
		    joypads [i] &= ~SNES_TR_MASK;
		    break;
		}
		if (js_ev.number == 3)
		{
		    if (js_ev.value < -16384)
		    {
			joypads [i] |= SNES_TL_MASK; 
			joypads [i] &= ~SNES_TR_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			joypads [i] &= ~SNES_TL_MASK;
			joypads [i] |= SNES_TR_MASK; 
			break;
		    }
		    joypads [i] &= ~SNES_TL_MASK;
		    joypads [i] &= ~SNES_TR_MASK;
		    break;	
		}

		break;

	    case JS_EVENT_BUTTON:
		if (js_ev.number > 15)
		    break;

		if (js_ev.value)
                {
                    if (js_ev.number == 6 || js_ev.number == 7)
                        Settings.FrameAdvance++;
                    else if (js_ev.number == 4)
                    {
	                    char def [PATH_MAX];
	                    char filename [PATH_MAX];
	                    char drive [_MAX_DRIVE];
	                    char dir [_MAX_DIR];
	                    char ext [_MAX_EXT];
	
	                    _splitpath (Memory.ROMFilename, drive, dir, def, ext);
	                    sprintf (filename, "%s%s%s.%03d",
	                             S9xGetSnapshotDirectory (), SLASH_STR, def,
	                             js_ss_slot);
			    S9xFreezeGame (filename);
                            S9xInfoMessage ("Joypad savestate OK");
                    }
                    else if (js_ev.number == 5)
                    {
	                    char def [PATH_MAX];
	                    char filename [PATH_MAX];
	                    char drive [_MAX_DRIVE];
	                    char dir [_MAX_DIR];
	                    char ext [_MAX_EXT];
	
	                    _splitpath (Memory.ROMFilename, drive, dir, def, ext);
	                    sprintf (filename, "%s%s%s.%03d",
	                             S9xGetSnapshotDirectory (), SLASH_STR, def,
	                             js_ss_slot);
	                    if (S9xUnfreezeGame (filename))
	                    {
	                        S9xInfoMessage ("Joypad loadstate OK");
	                    }

                    }
                    else
		        joypads [i] |= js_map_button [i][js_ev.number];
                }
		else
		    joypads [i] &= ~js_map_button [i][js_ev.number];

		break;
	    }
	}
    }
#endif
}

#else // DEHACKED_JOYPAD
void ReadJoysticks ()
{
#ifdef JSIOCGVERSION
    struct js_event js_ev;
    int i;

    for (i = 0; i < 4 && js_fd [i] >= 0; i++)
    {
	while (read (js_fd[i], &js_ev, sizeof (struct js_event)) == sizeof (struct js_event) )
	{
	    switch (js_ev.type & ~JS_EVENT_INIT)
	    {
	    case JS_EVENT_AXIS:
		if (js_ev.number == 0)
		{
		    if(js_ev.value < -16384)
		    {
			joypads [i] |= SNES_LEFT_MASK; 
			joypads [i] &= ~SNES_RIGHT_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			joypads [i] &= ~SNES_LEFT_MASK;
			joypads [i] |= SNES_RIGHT_MASK; 
			break;
		    }
		    joypads [i] &= ~SNES_LEFT_MASK;
		    joypads [i] &= ~SNES_RIGHT_MASK;
		    break;	
		}

		else if (js_ev.number == 1)
		{
		    if (js_ev.value < -16384)
		    {
			joypads [i] |= SNES_UP_MASK; 
			joypads [i] &= ~SNES_DOWN_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			joypads [i] &= ~SNES_UP_MASK;
			joypads [i] |= SNES_DOWN_MASK; 
			break;
		    }
		    joypads [i] &= ~SNES_UP_MASK;
		    joypads [i] &= ~SNES_DOWN_MASK;
		    break;	
		}
//		else printf("Movement on unknown joystick axis %d\n", js_ev.number);
		break;

	    case JS_EVENT_BUTTON:
		if (js_ev.number > 15)
		    break;

		if (js_ev.value)
		{
/*                    if (js_ev.number == 6 || js_ev.number == 7)
                        Settings.FrameAdvance = 1;
                    else if (js_ev.number == 10)
			Settings.Paused = !Settings.Paused, ::FromPause=1;
		    else*/
		        joypads [i] |= js_map_button [i][js_ev.number];
		}
		else
		    joypads [i] &= ~js_map_button [i][js_ev.number];

		break;
	    }
	}
    }
#endif
}

#endif // DEHACKED_JOYPAD


#endif // defined (JOYSTICK_SUPPORT)

const char *GetHomeDirectory ()
{
    return (getenv ("HOME"));
}

START_EXTERN_C
char* osd_GetPackDir()
{
  static char filename[_MAX_PATH];
  memset(filename, 0, _MAX_PATH);
  
  if(strlen(S9xGetSnapshotDirectory())!=0)
    strcpy (filename, S9xGetSnapshotDirectory());
  else
  {
    char dir [_MAX_DIR + 1];
    char drive [_MAX_DRIVE + 1];
    char name [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    _splitpath(Memory.ROMFilename, drive, dir, name, ext);
    _makepath(filename, drive, dir, NULL, NULL);
  }
  
  if(!strncmp((char*)&Memory.ROM [0xffc0], "SUPER POWER LEAG 4   ", 21))
  {
    if (getenv("SPL4PACK"))
      return getenv("SPL4PACK");
    else 
      strcat(filename, "/SPL4-SP7");
  }
  else if(!strncmp((char*)&Memory.ROM [0xffc0], "MOMOTETSU HAPPY      ",21))
  {
    if (getenv("MDHPACK"))
      return getenv("MDHPACK");
    else 
      strcat(filename, "/SMHT-SP7");
  }
  else if(!strncmp((char*)&Memory.ROM [0xffc0], "HU TENGAI MAKYO ZERO ", 21))
  {
    if (getenv("FEOEZPACK"))
      return getenv("FEOEZPACK");
    else 
      strcat(filename, "/FEOEZSP7");
  }
  else if(!strncmp((char*)&Memory.ROM [0xffc0], "JUMP TENGAIMAKYO ZERO",21))
  {
    if (getenv("SJNSPACK"))
      return getenv("SJNSPACK");
    else 
      strcat(filename, "/SJUMPSP7");
  } else strcat(filename, "/MISC-SP7");
  return filename;
}
END_EXTERN_C

const char *S9xGetSnapshotDirectory ()
{
    static char filename [PATH_MAX];
    const char *snapshot;
    
    if (!(snapshot = getenv ("SNES9X_SNAPSHOT_DIR")) &&
	!(snapshot = getenv ("SNES96_SNAPSHOT_DIR")))
    {
	const char *home = GetHomeDirectory ();
	strcpy (filename, home);
	strcat (filename, SLASH_STR);
	strcat (filename, ".snes96_snapshots");
	mkdir (filename, 0777);
	chown (filename, getuid (), getgid ());
    }
    else
	return (snapshot);

    return (filename);
}

const char *S9xGetFilename (const char *ex)
{
    static char filename [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
    strcpy (filename, S9xGetSnapshotDirectory ());
    strcat (filename, SLASH_STR);
    strcat (filename, fname);
    strcat (filename, ex);

    return (filename);
}

const char *S9xGetFilenameInc (const char *e)
{
    static char filename [_MAX_PATH + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    char *ptr;
    struct stat buf;

    if (strlen (S9xGetSnapshotDirectory()))
    {
        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
        strcpy (filename, S9xGetSnapshotDirectory());
        strcat (filename, "/");
        strcat (filename, fname);
        ptr = filename + strlen (filename);
        strcat (filename, "00/");
        strcat (filename, e);
    }
    else
    {
        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
        strcat (fname, "00/");
        _makepath (filename, drive, dir, fname, e);
        ptr = strstr (filename, "00/");
    }

    do
    {
        if (++*(ptr + 2) > '9')
        {
            *(ptr + 2) = '0';
            if (++*(ptr + 1) > '9')
            {
                *(ptr + 1) = '0';
                if (++*ptr > '9')
                    break;
            }
        }
    } while( stat(filename, &buf) == 0 );

    return (filename);
}

const char *S9xGetROMDirectory ()
{
    const char *roms;
    
    if (!(roms = getenv ("SNES9X_ROM_DIR")) &&
	!(roms = getenv ("SNES96_ROM_DIR")))
	return ("." SLASH_STR "roms");
    else
	return (roms);
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
	return (p + 1);

    return (f);
}

const char *S9xChooseFilename (bool8 read_only)
{
    char def [PATH_MAX + 1];
    char title [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (Memory.ROMFilename, drive, dir, def, ext);
    strcat (def, ".s96");
    sprintf (title, "%s snapshot filename",
	    read_only ? "Select load" : "Choose save");
    const char *filename;

    S9xSetSoundMute (TRUE);
    filename = S9xSelectFilename (def, S9xGetSnapshotDirectory (), "s96", title);
    S9xSetSoundMute (FALSE);
    return (filename);
}

bool8 S9xOpenSnapshotFile (const char *fname, bool8 read_only, STREAM *file)
{
    char filename [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (fname, drive, dir, filename, ext);

    if (*drive || *dir == '/' ||
	(*dir == '.' && (*(dir + 1) == '/'
        )))
    {
	strcpy (filename, fname);
	if (!*ext)
	    strcat (filename, ".s96");
    }
    else
    {
	strcpy (filename, S9xGetSnapshotDirectory ());
	strcat (filename, SLASH_STR);
	strcat (filename, fname);
	if (!*ext)
	    strcat (filename, ".s96");
    }
    
#ifdef ZLIB
    if (read_only)
    {
	if ((*file = OPEN_STREAM (filename, "rb")))
	    return (TRUE);
    }
    else
    {
	if ((*file = OPEN_STREAM (filename, "wb")))
	{
	    chown (filename, getuid (), getgid ());
	    return (TRUE);
	}
    }
#else
    char command [PATH_MAX];
    
    if (read_only)
    {
	sprintf (command, "gzip -d <\"%s\"", filename);
	if (*file = popen (command, "r"))
	    return (TRUE);
    }
    else
    {
	sprintf (command, "gzip --best >\"%s\"", filename);
	if (*file = popen (command, "wb"))
	    return (TRUE);
    }
#endif
    return (FALSE);
}

void S9xCloseSnapshotFile (STREAM file)
{
#ifdef ZLIB
    CLOSE_STREAM (file);
#else
    pclose (file);
#endif
}

bool8 S9xInitUpdate ()
{
    return (TRUE);
}

bool8 S9xDeinitUpdate (int Width, int Height)
{
    //S9xLuaGui((uint16*)GFX.Screen,GFX.Pitch/2, Width, Height);
    S9xLuaGui((void*)GFX.Screen,GFX.Pitch/2, Width, Height, 32);
    S9xPutImage (Width, Height);
    if (!Settings.SoundSync)
    	S9xGenerateSound();
    return (TRUE);
}

static unsigned long now ()
{
    static unsigned long seconds_base = 0;
    struct timeval tp;
    gettimeofday (&tp, NULL);
    if (!seconds_base)
	seconds_base = tp.tv_sec;

    return ((tp.tv_sec - seconds_base) * 1000 + tp.tv_usec / 1000);
}

void OutputFrameRate ()
{
    static int FrameCount = 0;
    static unsigned long then = now ();

    if (++FrameCount % 60 == 0)
    {
	unsigned long here = now ();
//	printf ("\rFrame rate: %.2lfms", (double) (here - then) / 60);
//	fflush (stdout);
	then = here;
    }
}

void _makepath (char *path, const char *, const char *dir,
		const char *fname, const char *ext)
{
    if (dir && *dir)
    {
	strcpy (path, dir);
	strcat (path, "/");
    }
    else
	*path = 0;
    strcat (path, fname);
    if (ext && *ext)
    {
        strcat (path, ".");
        strcat (path, ext);
    }
}

void _splitpath (const char *path, char *drive, char *dir, char *fname,
		 char *ext)
{
    *drive = 0;

    char *slash = strrchr (path, '/');
    if (!slash)
	slash = strrchr (path, '\\');

    char *dot = strrchr (path, '.');

    if (dot && slash && dot < slash)
	dot = NULL;

    if (!slash)
    {
	strcpy (dir, "");
	strcpy (fname, path);
        if (dot)
        {
	    *(fname + (dot - path)) = 0;
	    strcpy (ext, dot + 1);
        }
	else
	    strcpy (ext, "");
    }
    else
    {
	strcpy (dir, path);
	*(dir + (slash - path)) = 0;
	strcpy (fname, slash + 1);
        if (dot)
	{
	    *(fname + (dot - slash) - 1) = 0;
    	    strcpy (ext, dot + 1);
	}
	else
	    strcpy (ext, "");
    }
}

void S9xToggleSoundChannel (int c)
{
    if (c == 8)
	so.sound_switch = 255;
    else
	so.sound_switch ^= 1 << c;
    S9xSetSoundControl (so.sound_switch);
}

static void SoundTrigger ()
{
    if (Settings.APUEnabled && !so.mute_sound)
	S9xProcessSound (NULL);
}

void StopTimer ()
{
    struct itimerval timeout;

    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_usec = 0;
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_usec = 0;
//    if (setitimer (ITIMER_REAL, &timeout, NULL) < 0)
//	perror ("setitimer");
}

void InitTimer ()
{
    return;
    struct itimerval timeout;
    struct sigaction sa;
    
#ifdef USE_THREADS
    if (Settings.ThreadSound)
    {
	pthread_mutex_init (&mutex, NULL);
	pthread_create (&thread, NULL, S9xProcessSound, NULL);
	return;
    }
#endif

    sa.sa_handler = (SIG_PF) SoundTrigger;

#if defined (SA_RESTART)
    sa.sa_flags = SA_RESTART;
#else
    sa.sa_flags = 0;
#endif

    
#ifndef NOSOUND //FIXME: Kludge to get calltree running. Remove later.
    sigemptyset (&sa.sa_mask);
    sigaction (SIGALRM, &sa, NULL);
    
    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_usec = 10000;
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_usec = 10000;
    if (setitimer (ITIMER_REAL, &timeout, NULL) < 0)
	perror ("setitimer");
#endif
}

void S9xSyncSpeed ()
{
    IPPU.RenderThisFrame = TRUE;
    if (dumpstreams == 1 && !Settings.HighSpeedSeek)
    	return;
#ifdef _NETPLAY_SUPPORT
    if (Settings.NetPlay)
    {
	// XXX: Send joypad position update to server
	// XXX: Wait for heart beat from server
	S9xNetPlaySendJoypadUpdate (joypads [0]);
	if (!S9xNetPlayCheckForHeartBeat ())
	{
	    do
	    {
		CHECK_SOUND ();
		S9xProcessEvents (FALSE);
	    } while (!S9xNetPlayCheckForHeartBeat ());
	    IPPU.RenderThisFrame = TRUE;
	    IPPU.SkippedFrames = 0;
	}
	// Don't skip frames. EVER.
	/*
	else
	{
	    if (IPPU.SkippedFrames < 10)
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
	*/
    }
    else
#endif

#if 0
    if (Settings.SoundSync == 2)
    {
	IPPU.RenderThisFrame = TRUE;
	IPPU.SkippedFrames = 0;
	return;
    }
#endif

    if (Settings.TurboMode || Settings.HighSpeedSeek)
    {
//        if(++IPPU.FrameSkip >= Settings.TurboSkipFrames || !Settings.HighSpeedSeek)
       if (IPPU.SkippedFrames >= Settings.TurboSkipFrames && !Settings.HighSpeedSeek) 
       {
            IPPU.FrameSkip = 0;
            IPPU.SkippedFrames = 0;
            IPPU.RenderThisFrame = TRUE;
        }
        else
        {
            ++IPPU.SkippedFrames;
            IPPU.RenderThisFrame = FALSE;
            Logger_NextFrame();
            FromPause = 1;
        }
        if (Settings.HighSpeedSeek > 0)
        	Settings.HighSpeedSeek--;
        return;
    }
    
#ifdef __sgi
    /* BS: saves on CPU usage */
    sginap(1);
#endif

    /* Check events */
    
    static struct timeval next1 = {0, 0};
    struct timeval now;

    CHECK_SOUND(); S9xProcessEvents(FALSE);

    if (S9xLuaSpeed() > 0)
    	return;

    while (gettimeofday (&now, NULL) < 0) ;
    
    /* If there is no known "next" frame, initialize it now */
    if (next1.tv_sec == 0) { next1 = now; ++next1.tv_usec; }

    /* If we're on AUTO_FRAMERATE, we'll display frames always
     * only if there's excess time.
     * Otherwise we'll display the defined amount of frames.
     */
    unsigned limit = Settings.SkipFrames == AUTO_FRAMERATE
                     ? (timercmp(&next1, &now, <) ? 10 : 1)
                     : Settings.SkipFrames;




      if (FromPause)
      {
      	next1 = now;
      	FromPause = 0;
      }
      else
      {
            // Unforseen events, such as SIGSTOP, severe system lag, etc
            unsigned lag = (now.tv_sec - next1.tv_sec) * 1000000 + now.tv_usec - next1.tv_usec;

            // I'm using 0.25 of a second instead of 1.00
//            if (lag >= 250000)
//               next1 = now;
      
      }
//    IPPU.RenderThisFrame = ++IPPU.SkippedFrames >= limit;
//    if(IPPU.RenderThisFrame)
//    {
//        IPPU.SkippedFrames = 0;
//    }
//    else
//    {
//        /* If we were behind the schedule, check how much it is */
//        if(timercmp(&next1, &now, <))
//        {
//            unsigned lag =
//                (now.tv_sec - next1.tv_sec) * 1000000
//               + now.tv_usec - next1.tv_usec;
//            if(lag >= 1000000)
//            {
//                /* More than a second behind means probably
//                 * pause. The next line prevents the magic
//                 * fast-forward effect.
//                 */
//                next1 = now;
//            }
//        }
//    }
    
    /* Delay until we're completed this frame */

    /* Can't use setitimer because the sound code already could
     * be using it. We don't actually need it either.
     */

    while(timercmp(&next1, &now, >))
    {
        /* If we're ahead of time, sleep a while */
        unsigned timeleft =
            (next1.tv_sec - now.tv_sec) * 1000000
           + next1.tv_usec - now.tv_usec;
        //fprintf(stderr, "<%u>", timeleft);
        usleep(timeleft);

        CHECK_SOUND(); S9xProcessEvents(FALSE);

        while (gettimeofday (&now, NULL) < 0) ;
        /* Continue with a while-loop because usleep()
         * could be interrupted by a signal
         */
    }

    /* Calculate the timestamp of the next frame. */
    next1.tv_usec += Settings.FrameTime;
    if (next1.tv_usec >= 1000000)
    {
        next1.tv_sec += next1.tv_usec / 1000000;
        next1.tv_usec %= 1000000;
    }
}

static long log2 (long num)
{
    long n = 0;

    while (num >>= 1)
	n++;

    return (n);
}

#if 0 /* Dead code  20031117 */
static long power (int num, int pow)
{
    long val = num;
    int i;
    
    if (pow == 0)
	return (1);

    for (i = 1; i < pow; i++)
	val *= num;

    return (val);
}
#endif 

/* BS: the SGI sound routines. */
#if !defined(NOSOUND) && defined(__sgi)
char	*sgi_sound = "SGI/IRIX sound by Sherman";

#include <audio.h>

static int Rates[8] =
{
    0, 8000, 11025, 16000, 22050, 32000, 44100, 48000
};

static int BufferSizes [8] =
{
#if 1 /*BS: double buffer size */
    0, 512, 512, 512, 1024, 1024, 2048, 2048
#else
    0, 256, 256, 256, 512, 512, 1024, 1024
#endif
};


bool8 S9xOpenSoundDevice (int mode, bool8 stereo, int buffer_size)
{
    /* SGI version */
	long	channels = -1,
		sampsize = -1,
		samprate = -1,
		compress = -1,
		device = -1,
		interleave = -1,
		outsize = -1;
	ALconfig al_config;
	long	al_params[2] = {
			AL_OUTPUT_RATE, -1
		};

	al_config = ALnewconfig();

#if 1 /* by not doing this, num channels will remain at current setting */
	/* number of channels can only be changed before port is open. */
	channels = stereo ? 2 : 1;
	if (ALsetchannels(al_config, channels)) {
		perror("ERROR with ALsetchannels");
	}

	channels = ALgetchannels(al_config);
#  if 0
	printf("channels now = %ld\n", channels);
#  endif
#endif
	so.stereo = channels - 1;

	/* unforunately, this must be called before opening the port.  It'd */
	/*   be nice to have a little more info before calling this, but    */
	/*   we'll have to guess for now.  Not reducing the queue size      */
	/*   results in a delay between events and the corresponding sounds.*/
	if (ALsetqueuesize(al_config, (long)(BufferSizes[mode&7]*channels*3))) {
		perror("ERROR with ALsetqueuesize");
	}

	/* open the audio port */
	so.al_port = ALopenport("Snes9x audio", "w", al_config);
	if (so.al_port == 0) {
		return (FALSE);
	}

	/* get the current settings of the audio port */
	al_config = ALgetconfig(so.al_port);
	channels = ALgetchannels(al_config);
	sampsize = ALgetwidth(al_config);
	outsize = ALgetfillable(so.al_port);
	ALgetparams(AL_DEFAULT_DEVICE, al_params, 2);
	samprate = al_params[1];

#if 0
	/* print machines current settings */
	printf("channels = %ld\n", channels);
	printf("sampsize = %ld\n", sampsize);
	printf("samprate = %ld\n", samprate);
	printf("outsize =  %ld\n", outsize);
	printf("compress = %ld\n", compress);
	printf("device =   %ld\n", device);
	printf("interleave=%ld\n", interleave);
#endif


	/* do not encode */
	so.encoded = 0;

#if 1 /* by not doing this, rate will be left at current setting */
	samprate = Rates [mode & 7];
	al_params[1] = samprate;
	ALsetparams(AL_DEFAULT_DEVICE, al_params, 2);

	ALgetparams(AL_DEFAULT_DEVICE, al_params, 2);
	samprate = al_params[1];
# if 0
	printf("samprate now = %ld\n", samprate);
#endif
#endif
	so.playback_rate = samprate;

#if 0 /* by not doing this, sample size will be left at current setting */
	if (ALsetwidth(al_config, AL_SAMPLE_8)) {
		perror("ERROR with ALsetwidth");
	}
	if (ALsetconfig(so.al_port, al_config)) {
		perror("ERROR with ALsetconfig");
	}

	sampsize = ALgetwidth(al_config);
	printf("sampsize now = %ld\n", sampsize);
#endif


	/* set the sample size */
	switch (sampsize) {
	case AL_SAMPLE_8:
		so.sixteen_bit = 0;
		break;
	case AL_SAMPLE_16:
		so.sixteen_bit = 1;
		break;
	case AL_SAMPLE_24:
	default:
		so.sixteen_bit = 1;
		break;
	}

	/* choose a buffer size based on the sample rate, and increase as nece*/
	for (int i = 1; i < 7; i++)
	if (samprate <= Rates [i])
	    break;
	so.buffer_size = BufferSizes [i];
	if (so.stereo)
		so.buffer_size *= 2;
	if (so.sixteen_bit)
		so.buffer_size *= 2;
	if (so.buffer_size > MAX_BUFFER_SIZE*4)
		so.buffer_size = MAX_BUFFER_SIZE*4;

	printf("SGI sound successfully opened\n");
	printf ("  Rate: %d, Buffer size: %d, 16-bit: %s, Stereo: %s, Encoded: %s\n",
	    so.playback_rate, so.buffer_size, so.sixteen_bit ? "yes" : "no",
	    so.stereo ? "yes" : "no", so.encoded ? "yes" : "no");

	return (TRUE);
}


void S9xUnixProcessSound (void)
{
    /* SGI version */
    uint8		Buf[MAX_BUFFER_SIZE*4];
    int			buf_size;
    signed short	*audio_buf;

    CPU.Flags &= ~PROCESS_SOUND_FLAG;

    if (so.al_port == 0)
	return;
    if (Settings.APUEnabled == FALSE) {
	printf("why am I hear? alport = %p\n", so.al_port);
	return;
    }

    int sample_count = so.buffer_size;
    if (so.sixteen_bit)
	sample_count >>= 1;

    /* BS: return if buffer is full */
    if (ALgetfillable(so.al_port) < so.buffer_size) {
#if 0
	printf("error: fillable space only = %ld\n", ALgetfillable(so.al_port));
#endif
	return;
    }

    S9xMixSamples (Buf, sample_count);

    if (!so.mute_sound) {
	    buf_size = sample_count;
	    audio_buf = (signed short *)Buf;

#if 0
	    printf("about to write buffer %p size %d\n", audio_buf, buf_size);
#endif
	    if (ALwritesamps(so.al_port, audio_buf, buf_size)) {
		perror("ERROR with ALwritesamps");
	    }
    }
}
#endif

#if !defined(NOSOUND) && defined(__sun)
static int Rates[8] =
{
    0, 8000, 11025, 16000, 22050, 32000, 37800, 44100
};

static int BufferSizes [8] =
{
    0, 256, 256, 256, 512, 512, 1024, 1024
};

bool8 S9xOpenSoundDevice (int mode, bool8 stereo, int buffer_size)
{
    /* SUN version */
    int i;

    if ((so.sound_fd = open ("/dev/audio", O_WRONLY)) < 0)
	return (FALSE);

    audio_info_t audio;

    AUDIO_INITINFO (&audio);
    audio.play.sample_rate = Rates [mode & 7];
    audio.play.channels = stereo ? 2 : 1;
    audio.play.precision = 16;
    audio.play.encoding = AUDIO_ENCODING_LINEAR;

    ioctl (so.sound_fd, AUDIO_SETINFO, &audio);
    if (ioctl (so.sound_fd, AUDIO_GETINFO, &audio) != 0)
	return (FALSE);

    so.stereo = audio.play.channels - 1;
    so.playback_rate = audio.play.sample_rate;
    so.encoded = audio.play.encoding != AUDIO_ENCODING_LINEAR;
    so.sixteen_bit = audio.play.precision == 16;

    for (i = 1; i < 7; i++)
	if (audio.play.sample_rate <= Rates [i])
	    break;
    so.buffer_size = BufferSizes [i];
    if (buffer_size > 0)
    if (so.stereo)
	so.buffer_size *= 2;
    if (so.sixteen_bit)
	so.buffer_size *= 2;
    if (so.buffer_size > MAX_BUFFER_SIZE)
	so.buffer_size = MAX_BUFFER_SIZE;
    so.last_eof = -1;

    printf ("Rate: %d, Buffer size: %d, 16-bit: %s, Stereo: %s, Encoded: %s\n",
	    so.playback_rate, so.buffer_size, so.sixteen_bit ? "yes" : "no",
	    so.stereo ? "yes" : "no", so.encoded ? "yes" : "no");

    return (TRUE);
}
#endif

#if !defined(NOSOUND) && defined(__linux)
static int Rates[8] =
{
    0, 8000, 11025, 16000, 22050, 32000, 44100, 48000
};

static int BufferSizes [8] =
{
    0, 256, 256, 256, 512, 512, 1024, 1024
};

bool8 S9xOpenSoundDevice (int mode, bool8 stereo, int buffer_size)
{
    /* Linux version (OSS) */
    int J, K;

    if (Settings.Mute)
    {

	// Do all sound setup without opening /dev/dsp
	so.sixteen_bit = true;
	so.stereo = true;
        so.playback_rate = Rates[mode & 0x07];
    S9xSetPlaybackRate (so.playback_rate);

    if (buffer_size == 0)
	buffer_size = BufferSizes [mode & 7];

    if (buffer_size > MAX_BUFFER_SIZE / 4)
	buffer_size = MAX_BUFFER_SIZE / 4;
    if (so.sixteen_bit)
	buffer_size *= 2;
    if (so.stereo)
	buffer_size *= 2;
        return TRUE;
    }

    if ((so.sound_fd = open ("/dev/dsp", O_NONBLOCK|O_WRONLY)) < 0)
    {
	perror ("/dev/dsp");
	return (FALSE);
    }

#ifdef MMAP_SOUND 
   if (ioctl (so.sound_fd, SNDCTL_DSP_GETCAPS, &J) < 0)
    {
	perror ("ioctl SNDCTL_DSP_GETCAPS");
    }
    else
    {
	if (J & DSP_CAP_MMAP)
	    printf ("DSP_CAP_MMAP supported\n");
    }
    void *ptr;

    if ((ptr = mmap (0, so.buffer_size * 3, PROT_WRITE, 0, so.sound_fd, 0)) == 0)
	fprintf (stderr, "mmap failed\n");

    J = 0;
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETTRIGGER, &J) < 0)
	perror ("ioctl SNDCTL_DSP_SETTRIGGER");
#endif

    //Test and check back to http://snes9x.com/forum/topic.asp?TOPIC_ID=8512
//    J = AFMT_U8;
    J = AFMT_S16_NE;
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETFMT, &J) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SETFMT");
	return (FALSE);
    }

    if (J != AFMT_S16_NE)
    {
	so.sixteen_bit = FALSE;
	J = AFMT_U8;
	if (ioctl (so.sound_fd, SNDCTL_DSP_SETFMT, &J) < 0)
	{
	    perror ("ioctl SNDCTL_DSP_SETFMT");
	    return (FALSE);
	}
    }
    else
	so.sixteen_bit = TRUE;

    so.stereo = stereo;
    if (ioctl (so.sound_fd, SNDCTL_DSP_STEREO, &so.stereo) < 0)
    {
	perror ("ioctl SNDCTL_DSP_STEREO");
	return (FALSE);
    }
    
    so.playback_rate = Rates[mode & 0x07];
    if (ioctl (so.sound_fd, SNDCTL_DSP_SPEED, &so.playback_rate) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SPEED");
	return (FALSE);
    }

    S9xSetPlaybackRate (so.playback_rate);

    if (buffer_size == 0)
	buffer_size = BufferSizes [mode & 7];

    if (buffer_size > MAX_BUFFER_SIZE / 4)
	buffer_size = MAX_BUFFER_SIZE / 4;
    if (so.sixteen_bit)
	buffer_size *= 2;
    if (so.stereo)
	buffer_size *= 2;

    int power2 = log2 (buffer_size);
    J = K = power2 | (3 << 16);
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETFRAGMENT, &J) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SETFRAGMENT");
	return (FALSE);
    }
    ioctl (so.sound_fd, SNDCTL_DSP_GETBLKSIZE, &so.buffer_size);
    
#ifdef MMAP_SOUND
    J = PCM_ENABLE_OUTPUT;
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETTRIGGER, &J) < 0)
	perror ("ioctl SNDCTL_DSP_SETTRIGGER");
#endif

#if 0
    buffmem_desc buff;
    buff.size = so.buffer_size * 3;
    buff.buffer = ptr;
    if (ioctl (so.sound_fd, SNDCTL_DSP_MAPOUTBUF, &buff) < 0)
	perror ("ioctl SNDCTL_DSP_MAPOUTBUF");
#endif

    printf ("Rate: %d, Buffer size: %d, 16-bit: %s, Stereo: %s, Encoded: %s\n",
	    so.playback_rate, so.buffer_size, so.sixteen_bit ? "yes" : "no",
	    so.stereo ? "yes" : "no", so.encoded ? "yes" : "no");

    return (TRUE);
}
#endif


#if !defined(NOSOUND) && (defined (__linux) || defined (__sun))
void S9xUnixProcessSound (void)
{
}

static uint8 Buf[MAX_BUFFER_SIZE];

#define FIXED_POINT 0x10000
#define FIXED_POINT_SHIFT 16
#define FIXED_POINT_REMAINDER 0xffff

static volatile bool8 block_signal = FALSE;
static volatile bool8 block_generate_sound = FALSE;
static volatile bool8 pending_signal = FALSE;
#endif

#if defined(NOSOUND)
static int Rates[8] =
{
    0, 8000, 11025, 16000, 22050, 32000, 44100, 48000
};

static int BufferSizes [8] =
{
    0, 256, 256, 256, 512, 512, 1024, 1024
};

bool8 S9xOpenSoundDevice (int mode, bool8 stereo, int buffer_size)
{
  return false;
}

void S9xGenerateSound ()
{
}

void *S9xProcessSound (void *)
{
    /* No sound version */
}
#endif

#if !defined(NOSOUND) && (defined (__linux) || defined (__sun))
void S9xGenerateSound ()
{
    /* Linux and Sun versions */
    
/* Mass comment-out!
    int bytes_so_far = so.sixteen_bit ? (so.samples_mixed_so_far << 1) :
				        so.samples_mixed_so_far;
    if (Settings.SoundSync == 2)
    {
	// Assumes sound is signal driven
	while (so.samples_mixed_so_far >= so.buffer_size && !so.mute_sound)
	    pause ();
    }
    else
    if (bytes_so_far >= so.buffer_size)
	return;

#ifdef USE_THREADS
    if (Settings.ThreadSound)
    {
	if (block_generate_sound || pthread_mutex_trylock (&mutex))
	    return;
    }
#endif

    block_signal = TRUE;

    so.err_counter += so.err_rate;
    if (so.err_counter >= FIXED_POINT)
    {
        int sample_count = so.err_counter >> FIXED_POINT_SHIFT;
	int byte_offset;
	int byte_count;

        so.err_counter &= FIXED_POINT_REMAINDER;
	if (so.stereo)
	    sample_count <<= 1;
	byte_offset = bytes_so_far + so.play_position;
	    
	do
	{
	    int sc = sample_count;
	    byte_count = sample_count;
	    if (so.sixteen_bit)
		byte_count <<= 1;
	    
	    if ((byte_offset & SOUND_BUFFER_SIZE_MASK) + byte_count > SOUND_BUFFER_SIZE)
	    {
		sc = SOUND_BUFFER_SIZE - (byte_offset & SOUND_BUFFER_SIZE_MASK);
		byte_count = sc;
		if (so.sixteen_bit)
		    sc >>= 1;
	    }
	    if (bytes_so_far + byte_count > so.buffer_size)
	    {
		byte_count = so.buffer_size - bytes_so_far;
		if (byte_count == 0)
		    break;
		sc = byte_count;
		if (so.sixteen_bit)
		    sc >>= 1;
	    }
	    S9xMixSamplesO (Buf, sc,
			    byte_offset & SOUND_BUFFER_SIZE_MASK);
	    so.samples_mixed_so_far += sc;
	    sample_count -= sc;
	    bytes_so_far = so.sixteen_bit ? (so.samples_mixed_so_far << 1) :
	 	           so.samples_mixed_so_far;
	    byte_offset += byte_count;
	} while (sample_count > 0);
    }
    block_signal = FALSE;

#ifdef USE_THREADS
    if (Settings.ThreadSound)
	pthread_mutex_unlock (&mutex);
    else
#endif    
    if (pending_signal)
    {
	S9xProcessSound (NULL);
	pending_signal = FALSE;
    }
    
    */
	uint8 Buf[262144];
	unsigned N = so.playback_rate * Settings.FrameTime / 1000000;
	if (so.stereo) N *= 2;
	S9xMixSamplesO(Buf, N, 0);
	if (so.sixteen_bit) N *= 2;
	
	if (!Settings.Mute)
	    write(so.sound_fd, Buf, N);
	
	AudioLogger(Buf, N);
}

void *S9xProcessSound (void *)
{
    /* Linux and Sun versions */
    
    /* If threads in use, this is to loop indefinitely */
    /* If not, this will be called by timer */
    S9xGenerateSound();
    return NULL;
#ifdef __linux
    audio_buf_info info;

    if (!Settings.ThreadSound &&
	(ioctl (so.sound_fd, SNDCTL_DSP_GETOSPACE, &info) == -1 ||
	 info.bytes < so.buffer_size))
    {
	return (NULL);
    }
#ifdef MMAP_SOUND
    count_info count;

    if (ioctl (so.sound_fd, SNDCTL_DSP_GETOPTR, &count) < 0)
    {
	printf ("F"); fflush (stdout);
    }
    else
    {
	printf ("<%d,%d>", count.blocks, count.bytes); fflush (stdout);
    	return (NULL);
    }
#endif

#endif
#ifdef __sun
    audio_info_t audio;
    if (!Settings.ThreadSound)
    {
	if (ioctl (so.sound_fd, AUDIO_GETINFO, &audio) < 0)
	    return (NULL);

	if (audio.play.eof == 0)
	    so.last_eof = -2;
	else
	if (audio.play.eof == so.last_eof)
	    return (NULL);

	so.last_eof++;
    }
#endif

#ifdef USE_THREADS
    do
    {
#endif

    /* Number of samples to generate now */
    int sample_count = so.buffer_size;
    
    if (so.sixteen_bit)
    {
        /* to prevent running out of buffer space,
         * create less samples
         */
	sample_count >>= 1;
    }
 
#ifdef USE_THREADS
    if (Settings.ThreadSound)
	pthread_mutex_lock (&mutex);
    else
#endif
    if (block_signal)
    {
	pending_signal = TRUE;
	return (NULL);
    }

    block_generate_sound = TRUE;

    /* If we need more audio samples */
    if (so.samples_mixed_so_far < sample_count)
    {
	/* Where to put the samples to */
	unsigned byte_offset = so.play_position + 
		      (so.sixteen_bit ? (so.samples_mixed_so_far << 1)
				      : so.samples_mixed_so_far);
//printf ("%d:", sample_count - so.samples_mixed_so_far); fflush (stdout);
	if (Settings.SoundSync == 2)
	{
	    /*memset (Buf + (byte_offset & SOUND_BUFFER_SIZE_MASK), 0,
		    sample_count - so.samples_mixed_so_far);*/
	}
	else
	{
	    /* Mix the missing samples */
	    S9xMixSamplesO (Buf, sample_count - so.samples_mixed_so_far,
			    byte_offset & SOUND_BUFFER_SIZE_MASK);
        }
	so.samples_mixed_so_far = sample_count;
    }
    
//    if (!so.mute_sound)
    {
	unsigned bytes_to_write = sample_count;
	if(so.sixteen_bit) bytes_to_write <<= 1;

	unsigned byte_offset = so.play_position;
	so.play_position += bytes_to_write;
	so.play_position &= SOUND_BUFFER_SIZE_MASK; /* wrap to beginning */

#ifdef USE_THREADS
	if (Settings.ThreadSound)
	    pthread_mutex_unlock (&mutex);
#endif
	block_generate_sound = FALSE;

	/* Feed the samples to the soundcard until nothing is left */
	for(;;)
	{
	    int I = bytes_to_write;
	    if (byte_offset + I > SOUND_BUFFER_SIZE)
	    {
	        I = SOUND_BUFFER_SIZE - byte_offset;
	    }
	    if(I == 0) break;
	    
            if (!Settings.Mute)
                I = write (so.sound_fd, (char *) Buf + byte_offset, I);
            if (I > 0)
            {
                bytes_to_write -= I;
                byte_offset += I;
                byte_offset &= SOUND_BUFFER_SIZE_MASK; /* wrap */
            }
            /* give up if an unrecoverable error happened */
            if(I < 0 && errno != EINTR) break;
	}
	/* All data sent. */
    }

    so.samples_mixed_so_far -= sample_count;

#ifdef USE_THREADS
    } while (Settings.ThreadSound);
#endif

#ifdef __sun
    if (!Settings.ThreadSound && !Settings.Mute)
	write (so.sound_fd, NULL, 0);
#endif

    return (NULL);
}
#endif

uint32 S9xReadJoypad (int which1)
{
#ifdef _NETPLAY_SUPPORT
    if (Settings.NetPlay)
	return (S9xNetPlayGetJoypad (which1));
#endif
    if (which1 < NumControllers)
	return (0x80000000 | joypads [which1]);
    return (0);
}

#if !defined(NOSOUND) && defined(__sun)
uint8 int2ulaw(int ch)
{
    int mask;

    if (ch < 0) {
      ch = -ch;
      mask = 0x7f;
    }
    else {
      mask = 0xff;
    }

    if (ch < 32) {
	ch = 0xF0 | ( 15 - (ch/2) );
    } else if (ch < 96) {
        ch = 0xE0 | ( 15 - (ch-32)/4 );
    } else if (ch < 224) {
	ch = 0xD0 | ( 15 - (ch-96)/8 );
    } else if (ch < 480) {
	ch = 0xC0 | ( 15 - (ch-224)/16 );
    } else if (ch < 992 ) {
	ch = 0xB0 | ( 15 - (ch-480)/32 );
    } else if (ch < 2016) {
	ch = 0xA0 | ( 15 - (ch-992)/64 );
    } else if (ch < 4064) {
	ch = 0x90 | ( 15 - (ch-2016)/128 );
    } else if (ch < 8160) {
	ch = 0x80 | ( 15 - (ch-4064)/256 );
    } else {
	ch = 0x80;
    }
    return (uint8)(mask & ch);
}
#endif

#if 0
void S9xParseConfigFile ()
{
    int i, t = 0;
    char *b, buf[10];
    struct ffblk f;

    set_config_file("SNES9X.CFG");

    if (findfirst("SNES9X.CFG", &f, 0) != 0)
    {
        set_config_int("Graphics", "VideoMode", -1);
        set_config_int("Graphics", "AutoFrameskip", 1);
        set_config_int("Graphics", "Frameskip", 0);
        set_config_int("Graphics", "Shutdown", 1);
        set_config_int("Graphics", "FrameTimePAL", 20000);
        set_config_int("Graphics", "FrameTimeNTSC", 16667);
        set_config_int("Graphics", "Transparency", 0);
        set_config_int("Graphics", "HiColor", 0);
        set_config_int("Graphics", "Hi-ResSupport", 0);
        set_config_int("Graphics", "CPUCycles", 100);
        set_config_int("Graphics", "Scale", 0);
        set_config_int("Graphics", "VSync", 0);
        set_config_int("Sound", "APUEnabled", 1);
        set_config_int("Sound", "SoundPlaybackRate", 4);
        set_config_int("Sound", "Stereo", 1);
        set_config_int("Sound", "SoundBufferSize", 256);
        set_config_int("Sound", "SPCToCPURatio", 2);
        set_config_int("Sound", "Echo", 1);
        set_config_int("Sound", "SampleCaching", 1);
        set_config_int("Sound", "MasterVolume", 1);
        set_config_int("Peripherals", "Mouse", 1);
        set_config_int("Peripherals", "SuperScope", 1);
        set_config_int("Peripherals", "MultiPlayer5", 1);
        set_config_int("Peripherals", "Controller", 0);
        set_config_int("Controllers", "Type", JOY_TYPE_AUTODETECT);
        set_config_string("Controllers", "Button1", "A");
        set_config_string("Controllers", "Button2", "B");
        set_config_string("Controllers", "Button3", "X");
        set_config_string("Controllers", "Button4", "Y");
        set_config_string("Controllers", "Button5", "TL");
        set_config_string("Controllers", "Button6", "TR");
        set_config_string("Controllers", "Button7", "START");
        set_config_string("Controllers", "Button8", "SELECT");
        set_config_string("Controllers", "Button9", "NONE");
        set_config_string("Controllers", "Button10", "NONE");
    }

    mode = get_config_int("Graphics", "VideoMode", -1);
    Settings.SkipFrames = get_config_int("Graphics", "AutoFrameskip", 1);
    if (!Settings.SkipFrames)
       Settings.SkipFrames = get_config_int("Graphics", "Frameskip", AUTO_FRAMERATE);
    else
       Settings.SkipFrames = AUTO_FRAMERATE;
    Settings.ShutdownMaster = get_config_int("Graphics", "Shutdown", TRUE);
    Settings.FrameTimePAL = get_config_int("Graphics", "FrameTimePAL", 20000);
    Settings.FrameTimeNTSC = get_config_int("Graphics", "FrameTimeNTSC", 16667);
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.Transparency = get_config_int("Graphics", "Transparency", FALSE);
//    Settings.SixteenBit = get_config_int("Graphics", "HiColor", FALSE);
    Settings.SupportHiRes = get_config_int("Graphics", "Hi-ResSupport", FALSE);
    i = get_config_int("Graphics", "CPUCycles", 100);
    Settings.H_Max = (i * SNES_CYCLES_PER_SCANLINE) / i;
    stretch = get_config_int("Graphics", "Scale", 0);
    _vsync = get_config_int("Graphics", "VSync", 0);

    Settings.APUEnabled = get_config_int("Sound", "APUEnabled", TRUE);
    Settings.SoundPlaybackRate = get_config_int("Sound", "SoundPlaybackRate", 4);
    Settings.Stereo = get_config_int("Sound", "Stereo", TRUE);
    Settings.SoundBufferSize = get_config_int("Sound", "SoundBufferSize", 256);
    Settings.SPCTo65c816Ratio = get_config_int("Sound", "SPCToCPURatio", 2);
    Settings.DisableSoundEcho = get_config_int("Sound", "Echo", TRUE) ? FALSE : TRUE;
    Settings.DisableSampleCaching = get_config_int("Sound", "SampleCaching", TRUE) ? FALSE : TRUE;
    Settings.DisableMasterVolume = get_config_int("Sound", "MasterVolume", TRUE) ? FALSE : TRUE;

    Settings.Mouse = get_config_int("Peripherals", "Mouse", TRUE);
    Settings.SuperScope = get_config_int("Peripherals", "SuperScope", TRUE);
    Settings.MultiPlayer5 = get_config_int("Peripherals", "MultiPlayer5", TRUE);
    Settings.ControllerOption = (uint32)get_config_int("Peripherals", "Controller", SNES_MULTIPLAYER5);

    joy_type = get_config_int("Controllers", "Type", JOY_TYPE_AUTODETECT);
    for (i = 0; i < 10; i++)
    {
        sprintf(buf, "Button%d", i+1);
        b = get_config_string("Controllers", buf, "NONE");
        if (!strcasecmp(b, "A"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_A_MASK;}
        else if (!strcasecmp(b, "B"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_B_MASK;}
        else if (!strcasecmp(b, "X"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_X_MASK;}
        else if (!strcasecmp(b, "Y"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_Y_MASK;}
        else if (!strcasecmp(b, "TL"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_TL_MASK;}
        else if (!strcasecmp(b, "TR"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_TR_MASK;}
        else if (!strcasecmp(b, "START"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_START_MASK;}
        else if (!strcasecmp(b, "SELECT"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_SELECT_MASK;}
    }
}
#endif

static int S9xCompareSDD1IndexEntries (const void *p1, const void *p2)
{
    return (*(uint32 *) p1 - *(uint32 *) p2);
}

void S9xLoadSDD1Data ()
{
    char filename [_MAX_PATH + 1];
    char index [_MAX_PATH + 1];
    char data [_MAX_PATH + 1];
    char patch [_MAX_PATH + 1];

    Memory.FreeSDD1Data ();

    strcpy (filename, S9xGetSnapshotDirectory ());

    Settings.SDD1Pack=FALSE;
    if (strncmp (Memory.ROMName, "Star Ocean", 10) == 0){
        if(SDD1_pack) strcpy (filename, SDD1_pack);
#ifdef SDD1_DECOMP
        else Settings.SDD1Pack=TRUE;
#else
	strcat (filename, "/socnsdd1");
#endif
    } else if(strncmp(Memory.ROMName, "STREET FIGHTER ALPHA2", 21)==0){
        if(SDD1_pack) strcpy (filename, SDD1_pack);
#ifdef SDD1_DECOMP
        else Settings.SDD1Pack=TRUE;
#else
	strcat (filename, "/sfa2sdd1");
#endif
    } else {
        if(SDD1_pack) strcpy (filename, SDD1_pack);
#ifdef SDD1_DECOMP
        else Settings.SDD1Pack=TRUE;
#else
	S9xMessage(S9X_WARNING, S9X_ROM_INFO, "WARNING: No default SDD1 pack for this ROM");
#endif
    }

    if(Settings.SDD1Pack) return;

    DIR *dir = opendir (filename);

    index [0] = 0;
    data [0] = 0;
    patch [0] = 0;

    if (dir)
    {
	struct dirent *d;
	
	while ((d = readdir (dir)))
	{
	    if (strcasecmp (d->d_name, "SDD1GFX.IDX") == 0)
	    {
		strcpy (index, filename);
		strcat (index, "/");
		strcat (index, d->d_name);
	    }
	    else
	    if (strcasecmp (d->d_name, "SDD1GFX.DAT") == 0)
	    {
		strcpy (data, filename);
		strcat (data, "/");
		strcat (data, d->d_name);
	    }
	    if (strcasecmp (d->d_name, "SDD1GFX.PAT") == 0)
	    {
		strcpy (patch, filename);
		strcat (patch, "/");
		strcat (patch, d->d_name);
	    }
	}
	closedir (dir);

	if (strlen (index) && strlen (data))
	{
	    FILE *fs = fopen (index, "rb");
	    int len = 0;

	    if (fs)
	    {
		// Index is stored as a sequence of entries, each entry being
		// 12 bytes consisting of:
		// 4 byte key: (24bit address & 0xfffff * 16) | translated block
		// 4 byte ROM offset
		// 4 byte length
		fseek (fs, 0, SEEK_END);
		len = ftell (fs);
		rewind (fs);
		Memory.SDD1Index = (uint8 *) malloc (len);
		fread (Memory.SDD1Index, 1, len, fs);
		fclose (fs);
		Memory.SDD1Entries = len / 12;

		if (!(fs = fopen (data, "rb")))
		{
		    free ((char *) Memory.SDD1Index);
		    Memory.SDD1Index = NULL;
		    Memory.SDD1Entries = 0;
		}
		else
		{
		    fseek (fs, 0, SEEK_END);
		    len = ftell (fs);
		    rewind (fs);
		    Memory.SDD1Data = (uint8 *) malloc (len);
		    fread (Memory.SDD1Data, 1, len, fs);
		    fclose (fs);

		    if (strlen (patch) > 0 &&
			(fs = fopen (patch, "rb")))
		    {
			fclose (fs);
		    }
#ifdef MSB_FIRST
		    // Swap the byte order of the 32-bit value triplets on
		    // MSBFirst machines.
		    uint8 *ptr = Memory.SDD1Index;
		    for (int i = 0; i < Memory.SDD1Entries; i++, ptr += 12)
		    {
			SWAP_DWORD ((*(uint32 *) (ptr + 0)));
			SWAP_DWORD ((*(uint32 *) (ptr + 4)));
			SWAP_DWORD ((*(uint32 *) (ptr + 8)));
		    }
#endif
		    qsort (Memory.SDD1Index, Memory.SDD1Entries, 12,
			   S9xCompareSDD1IndexEntries);
		}
	    }
	}
	else
	{
	    fprintf (stderr, "Decompressed data pack not found in '%s'.\n", 
                     filename);
	}
    }
}




// New additions from the heavy "improvement"

const char *inc_format="%03d";

const char *S9xGetFilename (const char *ex, enum s9x_getdirtype dirtype)
{
    static char filename [PATH_MAX + 1];
    char dir [_MAX_DIR + 1];
    char drive [_MAX_DRIVE + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
    snprintf(filename, sizeof(filename), "%s" SLASH_STR "%s%s",
             S9xGetDirectory(dirtype), fname, ex);
    return (filename);
}

const char *S9xGetFilenameInc (const char *e, enum s9x_getdirtype dirtype)
{
    static char filename [PATH_MAX + 1];
    char dir [_MAX_DIR + 1];
    char drive [_MAX_DRIVE + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    unsigned int i=0;
    struct stat buf;
    const char *d;

    _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
    d=S9xGetDirectory(dirtype);
    do {
        snprintf(filename, sizeof(filename), inc_format, d, fname, i, e);
        i++;
    } while(stat(filename, &buf)==0 && i!=0);

    return (filename);
}


const char *base_dir="~" SLASH_STR ".snes96_snapshots";
const char *snapshot_dir=base_dir;
const char *sram_dir=base_dir;
const char *screenshot_dir=base_dir;
const char *spc_dir=base_dir;
const char *patch_dir=base_dir;
const char *rom_dir="." SLASH_STR "roms";


const char *S9xGetDirectory (enum s9x_getdirtype dirtype)
{
    static char filename [PATH_MAX+1];
    const char *s, *b;
    bool md=false;

    switch(dirtype){
      default:
      case DEFAULT_DIR:     s=base_dir; md=true; break;
      case HOME_DIR:        s=getenv("HOME"); break;
      case ROM_DIR:         s=rom_dir; break;
      case SNAPSHOT_DIR:    s=snapshot_dir; break;
      case SRAM_DIR:        s=sram_dir; break;
	  case BIOS_DIR:        s=sram_dir; break;
      case SCREENSHOT_DIR:  s=screenshot_dir; break;
      case SPC_DIR:         s=spc_dir; break;
      case PATCH_DIR:       s=patch_dir; break;
      case ROMFILENAME_DIR:
        strcpy(filename, Memory.ROMFilename);
        for(int i=strlen(filename); i>=0; i--){
            if(filename[i]==SLASH_CHAR){
                filename[i]='\0';
                break;
            }
        }
        return filename;
    }

    b=NULL;
    if(dirtype!=HOME_DIR){
        if(s[0]==SLASH_CHAR){
            b=NULL;
        } else if(s[0]=='~' && s[1]==SLASH_CHAR){
            b=getenv("HOME");
            md=true;
            s+=2;
        } else if(s[0]=='.' && s[1]==SLASH_CHAR){
            b=NULL;
        } else if(dirtype!=DEFAULT_DIR){
            b=S9xGetDirectory(DEFAULT_DIR);
            md=true;
        } else {
            b=NULL;
        }
    }

    if(b==filename){
        int l=strlen(filename);
        snprintf(filename+l, sizeof(filename)-l, SLASH_STR "%s", s);
    } else if(b){
        snprintf(filename, sizeof(filename), "%s" SLASH_STR "%s", b, s);
    } else {
        snprintf(filename, sizeof(filename), "%s", s);
    }
    if(md){
        if(mkdir(filename, 0777)==0)
            chown(filename, getuid (), getgid ());
    }

    return filename;
}


char *S9xGetFreezeFilename(int slot) {
                   char def [PATH_MAX];
                   char filename [PATH_MAX];
                   char drive [_MAX_DRIVE];
                   char dir [_MAX_DIR];
                   char ext [_MAX_EXT];
                   _splitpath (Memory.ROMFilename, drive, dir, def, ext);
                   sprintf (filename, "%s%s%s.%03d",
                            S9xGetDirectory (SNAPSHOT_DIR), SLASH_STR, def,
                            slot);

	return strdup(filename);
}

#include "conffile.h"

ConfigFile::secvec_t keymaps;

typedef std::pair<std::string,std::string> strpair_t;
void S9xParsePortConfig(ConfigFile &conf, int pass){
    if(pass==0){
        if(conf.GetBool("Unix::UseNewStyleDirs", conf.Exists("Unix::BaseDir"))){
            base_dir="~" SLASH_STR ".snes9x";
            sram_dir="sram";
            snapshot_dir="snapshots";
            screenshot_dir="pics";
            spc_dir="spc";
            patch_dir="patches";
        }
        base_dir=conf.GetStringDup("Unix::BaseDir", base_dir);
        S9xParseDisplayConfig(conf, 0);
    } else if(pass==1){
        char *s;
        if((s=getenv("SNES9X_SNAPSHOT_DIR")) ||
           (s=getenv("SNES96_SNAPSHOT_DIR")))
            snapshot_dir=s;
        if((s=getenv("SNES9X_ROM_DIR")) ||
           (s=getenv("SNES96_ROM_DIR")))
            rom_dir=s;
        sram_dir=conf.GetStringDup("Unix::SRAMDir", sram_dir);
        snapshot_dir=conf.GetStringDup("Unix::SnapshotDir", snapshot_dir);
        screenshot_dir=conf.GetStringDup("Unix::ScreenshotDir", screenshot_dir);
        spc_dir=conf.GetStringDup("Unix::SPCDir", spc_dir);
        rom_dir=conf.GetStringDup("Unix::ROMDir", rom_dir);
        patch_dir=conf.GetStringDup("Unix::PatchDir", patch_dir);

        keymaps.clear();
        if(!conf.GetBool("Controls::ClearAll", false)){
#if 0
            // Using an axis to control Pseudo-pointer #1
            keymaps.push_back(strpair_t("J00:Axis0", "AxisToPointer 1h Var"));
            keymaps.push_back(strpair_t("J00:Axis1", "AxisToPointer 1v Var"));
            keymaps.push_back(strpair_t("PseudoPointer1", "Pointer C=2 White/Black Superscope"));
#elif 0
            // Using an Axis for Pseudo-buttons
            keymaps.push_back(strpair_t("J00:Axis0", "AxisToButtons 1/0 T=50%"));
            keymaps.push_back(strpair_t("J00:Axis1", "AxisToButtons 3/2 T=50%"));
            keymaps.push_back(strpair_t("PseudoButton0", "Joypad1 Right"));
            keymaps.push_back(strpair_t("PseudoButton1", "Joypad1 Left"));
            keymaps.push_back(strpair_t("PseudoButton2", "Joypad1 Down"));
            keymaps.push_back(strpair_t("PseudoButton3", "Joypad1 Up"));
#else
            // Using 'Joypad# Axis'
            keymaps.push_back(strpair_t("J00:Axis0", "Joypad1 Axis Left/Right T=50%"));
            keymaps.push_back(strpair_t("J00:Axis1", "Joypad1 Axis Up/Down T=50%"));
#endif
            keymaps.push_back(strpair_t("J00:B0", "Joypad1 X"));
            keymaps.push_back(strpair_t("J00:B1", "Joypad1 A"));
            keymaps.push_back(strpair_t("J00:B2", "Joypad1 B"));
            keymaps.push_back(strpair_t("J00:B3", "Joypad1 Y"));
#if 1
            keymaps.push_back(strpair_t("J00:B6", "Joypad1 L"));
#else
            // Show off joypad-meta
            keymaps.push_back(strpair_t("J00:X+B6", "JS1 Meta1"));
            keymaps.push_back(strpair_t("J00:M1+B1", "Joypad1 Turbo A"));
#endif
            keymaps.push_back(strpair_t("J00:B7", "Joypad1 R"));
            keymaps.push_back(strpair_t("J00:B8", "Joypad1 Select"));
            keymaps.push_back(strpair_t("J00:B11", "Joypad1 Start"));
        }

        std::string section=S9xParseDisplayConfig(conf, 1);

#ifdef JOYSTICK_SUPPORT
        Settings.JoystickEnabled=conf.GetBool("Unix::EnableJoystick", Settings.JoystickEnabled);
        js_device[0]=conf.GetStringDup("Unix::Joydev1", NULL);
        js_device[1]=conf.GetStringDup("Unix::Joydev2", NULL);
        js_device[2]=conf.GetStringDup("Unix::Joydev3", NULL);
        js_device[3]=conf.GetStringDup("Unix::Joydev4", NULL);
        js_device[4]=conf.GetStringDup("Unix::Joydev5", NULL);
        js_device[5]=conf.GetStringDup("Unix::Joydev6", NULL);
        js_device[6]=conf.GetStringDup("Unix::Joydev7", NULL);
        js_device[7]=conf.GetStringDup("Unix::Joydev8", NULL);
#endif
#if defined(CONFIGURABLE_SOUND_DEVICE)
        sound_device = conf.GetStringDup("Sound::Device", sound_device);
#endif
        Settings.SoundBufferSize=conf.GetUInt("Unix::SoundBufferSize",Settings.SoundBufferSize);
        snapshot_filename=conf.GetStringDup("ROM::LoadSnapshot", NULL);
        SDD1_pack=conf.GetStringDup("ROM::SDD1Pack", NULL);

        ConfigFile::secvec_t sec=conf.GetSection((section+" Controls").c_str());
        for(ConfigFile::secvec_t::iterator c=sec.begin(); c!=sec.end(); c++){
            keymaps.push_back(*c);
        }

        s=conf.GetStringDup("Unix::IncFormat", "%03d");
        bool found=false;
        int i;
        for(i=0; s[i]!='\0'; i++){
            if(s[i]=='%'){
                i++;
                if(s[i+1]=='%') continue;
                found=true;
                while(isdigit(s[i])){ i++; }
                switch(s[i]){
                  case 'd': case 'i':
                    s[i]='u';
                    break;
                  case 'o': case 'u': case 'x': case 'X':
                    break;
                  default:
                    goto bad_inc_format;
                }
                break;
            }
        }
        for(; s[i]!='\0'; i++){
            if(s[i]=='%'){
                i++;
                if(s[i]=='%') continue;
                goto bad_inc_format;
            }
        }
        if(!found){
bad_inc_format:
            S9xMessage(S9X_CONFIG_INFO, S9X_ERROR, "Invalid value for IncFormat in section [Unix]");
            inc_format="%s" SLASH_STR "%s%03d%s";
        } else {
            inc_format=(const char *)malloc(strlen(s)+8);
            strcpy((char *)inc_format, "%s" SLASH_STR "%s");
            strcat((char *)inc_format, s);
            strcat((char *)inc_format, "%s");
        }
        free(s);
    }
}
