// Define global variables for BST synthesizer
//
// Author: Anthony C. Bartman (@rommix0)
//
// v1.00 - first version             (11/28/2024)
//
// v1.01 - minor changes and fixes   (12/04/2024)
//         +  added pitch adjustment.
//         +  added keyboard hotkeys.
//         +  replaced BST.DLL with original B32_TTS.DLL.
//		   +  Added WM_ACTIVATE for disabling hotkeys when window is out of focus.
//         +  Re-arranged hotkeys to make better sense.
//
// v1.02 - fixes and updates         (12/15/2024)
//         +  Fixed hotkey handling. Now uses Unregisterhotkey routine to free up hotkeys
//            for other programs when BeSTspeak is not in focus.
//         +  Increased text box character limit.
//         +  Added functionality for selecting text using Ctrl+A.
//         +  Shortened startup announcement.
//         +  Added LIBCTINY.LIB to linker (makes executable smaller)

// Voice IDs
#define FRED 0
#define SARAH 1
#define HARRY 2
#define MARTHA 3
#define TIM 4
#define DEXTER 5
#define ALIEN 6
#define KIT 7
#define WENDY 8
#define BRUNO 9
#define GRANNY 10
#define GHOST 11
#define PEEPER 12
#define DRACULA 13

// Voices parameters
//-----------------------------------------------------------
// FRED
const char *V_FRED      = "~v0]~e3]~h0]~u0]";
const char *V_FRED_R    = "~r0]";
const int   V_FRED_F    = 80;

// SARAH
const char *V_SARAH     = "~v2]~e3]~h-20]~u0]";
const char *V_SARAH_R   = "~r0]";
const int   V_SARAH_F   = 175;

// HARRY
const char *V_HARRY     = "~v3]~e3]~h10]~u0]";
const char *V_HARRY_R   = "~r5]";
const int   V_HARRY_F   = 65;

// WENDY
const char *V_WENDY     = "~v2]~e1]~h50]~u0]";
const char *V_WENDY_R   = "~r-5]";
const int   V_WENDY_F   = 150;

// DEXTER
const char *V_DEXTER    = "~v6]~e6]~h0]~u-12]"; // unvoiced gain -25 -> -12
const char *V_DEXTER_R  = "~r7]";
const int   V_DEXTER_F  = 90;

// ALIEN
const char *V_ALIEN     = "~v4]~e6]~h-50]~u-20]";
const char *V_ALIEN_R   = "~r-20]";
const int   V_ALIEN_F   = 115;

// KIT
const char *V_KIT       = "~v5]~e3]~h40]~u0]";
const char *V_KIT_R     = "~r-10]";
const int   V_KIT_F     = 230;

// BRUNO
const char *V_BRUNO     = "~v3]~e3]~h50]~u0]";
const char *V_BRUNO_R   = "~r8]";
const int   V_BRUNO_F   = 60;

// GHOST
const char *V_GHOST     = "~v3]~e2]~h50]~u6]"; // unvoiced gain 0 -> 6
const char *V_GHOST_R   = "~r8]";
const int   V_GHOST_F   = 60;

// PEEPER
const char *V_PEEPER    = "~v2]~e2]~h0]~u5]";
const char *V_PEEPER_R  = "~r0]";
const int   V_PEEPER_F  = 80;

// DRACULA
const char *V_DRACULA   = "~v3]~e3]~h45]~u-5]";
const char *V_DRACULA_R = "~r10]";
const int   V_DRACULA_F = 47;

// GRANNY
const char *V_GRANNY    = "~v4]~e3]~h-60]~u0]";
const char *V_GRANNY_R  = "~r20]";
const int   V_GRANNY_F  = 350;

// MARTHA
const char *V_MARTHA    = "~v6]~e4]~h100]~u-5]";
const char *V_MARTHA_R  = "~r-10]";
const int   V_MARTHA_F  = 300;

// TIM
const char *V_TIM       = "~v3]~e4]~h-10]~u0]";
const char *V_TIM_R     = "~r-10]";
const int   V_TIM_F     = 60;
//-----------------------------------------------------------

// Variables for pitch adjustment
int         global_pitch = 80;
const int   freq_str_sz  = 3;
// +1 so IntToStr can write the NUL terminator at freq_str[freq_str_sz]
char       *freq_str     = (char *)malloc((freq_str_sz + 1) * sizeof(char));

// Used to tell WndProc that TTS buffer is filled up.
#define TTS_BUFFER_FULL 957

// Settings for BST parameters
#define RATE_SETTING          257
#define GAIN_SETTING          258
#define UNVOICED_GAIN_SETTING 259                // ignore this. unvoiced gain is set with custom voices
#define PITCH_SETTING         260                // ignore this. doesn't work well when custom voices are set
#define BIT_DEPTH_SETTING    4097                // 8 and 16 are valid values for bit depth

// Settings adjust amounts
#define RATE_ADJUST  20
#define GAIN_ADJUST  5
#define PITCH_ADJUST 5

// Setup function types for BST functions
typedef int  (__cdecl *bstCreateFunc)(long*&);
typedef int  (__cdecl *TtsWavFunc)(long*, HWND, char*);
typedef void (__cdecl *bstRelBufFunc)(long*);
typedef void (__cdecl *bstShutupFunc)(long*);
typedef void (__cdecl *bstCloseFunc)(long*);
typedef void (__cdecl *bstDestroyFunc)();
typedef void (__cdecl *bstSetParamsFunc)(long*, int, int);
typedef void (__cdecl *bstGetParamsFunc)(long*, int, int*&);
TtsWavFunc       _TtsWav;
bstCreateFunc    _bstCreate;
bstRelBufFunc    _bstRelBuf;
bstShutupFunc    _bstShutup;
bstDestroyFunc   _bstDestroy;
bstCloseFunc     _bstClose;
bstSetParamsFunc _bstSetParams;
bstGetParamsFunc _bstGetParams;

// Load library and setup related variables
HINSTANCE  bstLib = LoadLibrary("B32_TTS.DLL"); // The speech synthesizer itself.
long      *tts_handle   = 0;                    // The handle where the synthesizer will reside.
char      *prefix;                              // Prefix string for the voice parameters to reside.
char      *prefix_rate;                         // Prefix string for speech rate
char      *prefix_freq;                         // Prefix string for baseline pitch
int        voice_select = 0;                    // Used for voice selection via hotkeys

// ------------------------------------------------------------------
// WAV export support
//
// The synthesizer has no "render to buffer" entry point; it only plays
// audio through WINMM (waveOutOpen/waveOutWrite/waveOutClose, imported
// by name in B32_TTS.DLL). To export a WAV we patch those three slots in
// the DLL's import table and tap the PCM as it streams to the device.
// Playback is untouched -- capture just observes the same buffers.
// ------------------------------------------------------------------
#define TARGET_SAMPLE_RATE 44100        // standard export rate

// Original WINMM entry points, saved when we patch the import table.
typedef MMRESULT (WINAPI *waveOutOpenF )(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD, DWORD, DWORD);
typedef MMRESULT (WINAPI *waveOutWriteF)(HWAVEOUT, LPWAVEHDR, UINT);
typedef MMRESULT (WINAPI *waveOutCloseF)(HWAVEOUT);
waveOutOpenF  _origWaveOutOpen  = 0;
waveOutWriteF _origWaveOutWrite = 0;
waveOutCloseF _origWaveOutClose = 0;

// Capture state (only active during an export).
bool          g_capturing    = false;   // true while an export is in progress
char         *g_cap_buf      = 0;        // raw PCM exactly as the synthesizer emits it
unsigned int  g_cap_size     = 0;        // bytes captured so far
unsigned int  g_cap_cap      = 0;        // allocated capacity of g_cap_buf
int           g_cap_rate     = 11025;    // format the synthesizer proposes at waveOutOpen
int           g_cap_channels = 1;        //   (channels honored: mono unless it proposes stereo)
int           g_cap_bits     = 16;
char          g_export_path[MAX_PATH] = {0};
HWND          g_hWnd         = 0;        // main window, for the "export done" notification

// 44-byte canonical RIFF/WAVE header written ahead of the PCM data.
#pragma pack(push, 1)
typedef struct {
	char           riff[4];       // "RIFF"
	unsigned int   size;          // file size - 8
	char           wave[4];       // "WAVE"
	char           fmt[4];        // "fmt "
	unsigned int   fmt_size;      // 16 for PCM
	unsigned short fmt_tag;       // 1 = PCM
	unsigned short channels;
	unsigned int   sample_rate;
	unsigned int   byte_rate;
	unsigned short block_align;
	unsigned short bits;
	char           data[4];       // "data"
	unsigned int   data_bytes;
} WavHeader;
#pragma pack(pop)

// About and help text for the synthesizer to speak
//const char *version_text = "Best speak version one point oh two is running."; // no longer used.

const char *about_text   =
	"Best speak version one point oh two. "
	"Compiled on december fifteen, twenty twenty four by rommix zero. "
	"T T S version one point oh 1 D, revision 4, released in 1994 by Berkeley Speech Technologies. "
	"End of credits!";

const char *help_text    = 
	"F1 for help. "
	"F2 for credits. "
	"F3 to reset synthesizer. "
	"F4 to stop speaking. "
	"F5 to speak text. "
	"F6 to increase volume. "
	"Control F6 to decrease volume. "
	"F7 to make slower. "
	"F8 to make faster. "
	"F9 to lower pitch. "
	"F10 to raise pitch. "
	"F11 to change voice forwards. "
	"Control F11 to change voice backwards. "
	"Control E to export spoken text to a wave file. "
	"End of help!";
