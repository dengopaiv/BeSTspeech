// *************************************************************
// *                                                           *
// *                    BeSTspeak v1.02                        *
// *                                                           *
// *             Programmed by Anthony C. Bartman              *
// *                                                           *
// *************************************************************

#include "stdafx.h"
#include "resource.h"
#include "windows.h"
#include <mmsystem.h>   // WAVEFORMATEX / WAVEHDR / waveOut* types (windows.h omits these under WIN32_LEAN_AND_MEAN)
#include <commdlg.h>    // GetSaveFileName / OPENFILENAME for the WAV export dialog
#include "bst.h"

#define ID_EDITCHILD    100
#define MAX_LOADSTRING  20
#define WM_STARTUP      (WM_USER+0)
#define WM_EXPORT_DONE  (WM_USER+1)

#define ID_CONTROLS_VOICECHANGEFORWARD  500
#define ID_CONTROLS_VOICECHANGEBACKWARD 501
#define IDS_SELECT                      502
#define MOD_NOREPEAT                    0x4000

// Global Variables:
HINSTANCE hInst;							                // current instance
TCHAR     szTitle[MAX_LOADSTRING];			                // The title bar text
TCHAR     szWindowClass[MAX_LOADSTRING];	                // The title bar text
HWND      hWndEdit;                                         // hWnd for text box

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void				SpeakText(HWND hWnd, const char *text, bool init = FALSE);

// Get current gain
int GetGlobalGain() {
	int *get_gain;
	_bstGetParams(tts_handle, GAIN_SETTING, get_gain);
	return (int)get_gain;
}

// Get current rate
int GetGlobalRate() {
	int *get_rate;
	_bstGetParams(tts_handle, RATE_SETTING, get_rate);
	return (int)get_rate;
}

// Assign text to string using malloc
void AssignToString(char *&string_to_assign, const char *text) {
	string_to_assign = (char *)malloc(strlen(text) + 1);
	strcpy(string_to_assign, text);
}

// Convert int to char string (for pitch adjustment)
void IntToStr(int number) {
	memset(freq_str, 0, freq_str_sz * sizeof(char));

	int n = freq_str_sz;
    freq_str += n;
    *freq_str = '\0';

    while (n--)
    {
        *--freq_str = (number % 10) + '0';
        number /= 10;
    }
}

// Set frequency prefix string
void SetFreqPrefix(char *&prefix_to_assign, int pitch) {
	global_pitch = pitch;
	IntToStr(pitch);

	const char *freq_to_add = freq_str;
	// "~f" (2) + up to freq_str_sz digits + "]" (1) + NUL (1)
	prefix_to_assign = (char *)malloc((freq_str_sz + 4) * sizeof(char));
	strcpy(prefix_to_assign, "~f");
	strcat(prefix_to_assign, freq_to_add);
	strcat(prefix_to_assign, "]");
}

// Set up prefix string with chosen voice parameter
void SetVoice(int voice_id) {
	voice_select = voice_id;
	switch (voice_id) {
		case FRED:
			AssignToString(prefix, V_FRED);
			AssignToString(prefix_rate, V_FRED_R);
			SetFreqPrefix(prefix_freq, V_FRED_F);
			break;
		case SARAH:
			AssignToString(prefix, V_SARAH);
			AssignToString(prefix_rate, V_SARAH_R);
			SetFreqPrefix(prefix_freq, V_SARAH_F);
			break;
		case HARRY:
			AssignToString(prefix, V_HARRY);
			AssignToString(prefix_rate, V_HARRY_R);
			SetFreqPrefix(prefix_freq, V_HARRY_F);
			break;
		case WENDY:
			AssignToString(prefix, V_WENDY);
			AssignToString(prefix_rate, V_WENDY_R);
			SetFreqPrefix(prefix_freq, V_WENDY_F);
			break;
		case DEXTER:
			AssignToString(prefix, V_DEXTER);
			AssignToString(prefix_rate, V_DEXTER_R);
			SetFreqPrefix(prefix_freq, V_DEXTER_F);
			break;
		case ALIEN:
			AssignToString(prefix, V_ALIEN);
			AssignToString(prefix_rate, V_ALIEN_R);
			SetFreqPrefix(prefix_freq, V_ALIEN_F);
			break;
		case KIT:
			AssignToString(prefix, V_KIT);
			AssignToString(prefix_rate, V_KIT_R);
			SetFreqPrefix(prefix_freq, V_KIT_F);
			break;
		case BRUNO:
			AssignToString(prefix, V_BRUNO);
			AssignToString(prefix_rate, V_BRUNO_R);
			SetFreqPrefix(prefix_freq, V_BRUNO_F);
			break;
		case GHOST:
			AssignToString(prefix, V_GHOST);
			AssignToString(prefix_rate, V_GHOST_R);
			SetFreqPrefix(prefix_freq, V_GHOST_F);
			break;
		case PEEPER:
			AssignToString(prefix, V_PEEPER);
			AssignToString(prefix_rate, V_PEEPER_R);
			SetFreqPrefix(prefix_freq, V_PEEPER_F);
			break;
		case DRACULA:
			AssignToString(prefix, V_DRACULA);
			AssignToString(prefix_rate, V_DRACULA_R);
			SetFreqPrefix(prefix_freq, V_DRACULA_F);
			break;
		case GRANNY:
			AssignToString(prefix, V_GRANNY);
			AssignToString(prefix_rate, V_GRANNY_R);
			SetFreqPrefix(prefix_freq, V_GRANNY_F);
			break;
		case MARTHA:
			AssignToString(prefix, V_MARTHA);
			AssignToString(prefix_rate, V_MARTHA_R);
			SetFreqPrefix(prefix_freq, V_MARTHA_F);
			break;
		case TIM:
			AssignToString(prefix, V_TIM);
			AssignToString(prefix_rate, V_TIM_R);
			SetFreqPrefix(prefix_freq, V_TIM_F);
			break;
		default:
			AssignToString(prefix, V_FRED);
			AssignToString(prefix_rate, V_FRED_R);
			SetFreqPrefix(prefix_freq, V_FRED_F);
			voice_select = FRED;
	}
	_bstSetParams(tts_handle, GAIN_SETTING, -5);
}

// Increase voice speed
void IncreaseRate() {
	int global_rate = GetGlobalRate() - RATE_ADJUST;
	if (global_rate < -100) {
		global_rate = -100;
	}
	_bstSetParams(tts_handle, RATE_SETTING, global_rate);
}

// Decrease voice speed
void DecreaseRate() {
	int global_rate = GetGlobalRate() + RATE_ADJUST;
	_bstSetParams(tts_handle, RATE_SETTING, global_rate);
}

// Increase voice gain
void IncreaseGain() {
	int global_gain = GetGlobalGain() + GAIN_ADJUST;
	if (global_gain > 0) {
		global_gain = 0;
	}
	_bstSetParams(tts_handle, GAIN_SETTING, global_gain);
}

// Decrease voice gain
void DecreaseGain() {
	int global_gain = GetGlobalGain() - GAIN_ADJUST;
	if (global_gain < -35) {
		global_gain = -35;
	}
	_bstSetParams(tts_handle, GAIN_SETTING, global_gain);
}

// Increase pitch
void IncreasePitch() {
	global_pitch += PITCH_ADJUST;
	if (global_pitch > 600) {
		global_pitch = 600;
	}
	SetFreqPrefix(prefix_freq, global_pitch);
}

// Decrease pitch
void DecreasePitch() {
	global_pitch -= PITCH_ADJUST;
	if (global_pitch < 45) {
		global_pitch = 45;
	}
	SetFreqPrefix(prefix_freq, global_pitch);
}

// Function for initializing/reset speech synthesizer
int InitSpeech() {
	int current_gain  = 0;
	int current_rate  = 0;
	if (tts_handle) {
		current_gain  = GetGlobalGain();
		current_rate  = GetGlobalRate();
		_bstShutup(tts_handle);
		_bstClose(tts_handle);
		_bstDestroy();
		tts_handle = 0;
	}
	if (!tts_handle) {
		int stat = _bstCreate(tts_handle);
		if (stat != 0) {
			MessageBox(NULL, "TTS cannot be initialized!", "Init Error!", MB_ICONERROR);
			return 0;
		}
		_bstSetParams(tts_handle, GAIN_SETTING, current_gain);
		_bstSetParams(tts_handle, RATE_SETTING, current_rate);
		_bstSetParams(tts_handle, BIT_DEPTH_SETTING, 16);      // always set this for 16-bit audio
	}
	return 1;
}

// Close speech engine if present (when closing program)
void CloseSpeech() {
	if (tts_handle) {
		_bstShutup(tts_handle);
		_bstClose(tts_handle);
		_bstDestroy();
		tts_handle = 0;
	}
}

// ------------------------------------------------------------------
//  WAV export (see BST.h for the overall approach)
// ------------------------------------------------------------------

// Patch one entry in hMod's import address table so that calls the module
// makes to dll_name!func_name are redirected to new_func. Returns the
// address that used to be in the slot (i.e. the real function) so the
// caller can still invoke it for passthrough, or 0 if the import was not
// found.
static void *HookImport(HMODULE hMod, const char *dll_name, const char *func_name, void *new_func) {
	BYTE               *base = (BYTE *)hMod;
	IMAGE_DOS_HEADER   *dos  = (IMAGE_DOS_HEADER *)base;
	IMAGE_NT_HEADERS   *nt   = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
	IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (!dir.VirtualAddress) {
		return 0;
	}

	IMAGE_IMPORT_DESCRIPTOR *imp = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress);
	for (; imp->Name; imp++) {
		const char *name = (const char *)(base + imp->Name);
		if (lstrcmpiA(name, dll_name) != 0) {
			continue;
		}
		// The names live in OriginalFirstThunk; the writable pointers we
		// want to patch live in the parallel FirstThunk (the IAT itself).
		DWORD name_rva = imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk;
		IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(base + name_rva);
		IMAGE_THUNK_DATA *slots = (IMAGE_THUNK_DATA *)(base + imp->FirstThunk);
		for (; names->u1.AddressOfData; names++, slots++) {
			if (names->u1.Ordinal & IMAGE_ORDINAL_FLAG32) {
				continue; // imported by ordinal, no name to match
			}
			IMAGE_IMPORT_BY_NAME *ibn = (IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData);
			if (lstrcmpiA((const char *)ibn->Name, func_name) != 0) {
				continue;
			}
			void *orig = (void *)slots->u1.Function;
			DWORD old_prot;
			VirtualProtect(&slots->u1.Function, sizeof(DWORD), PAGE_READWRITE, &old_prot);
			slots->u1.Function = (DWORD)new_func;
			VirtualProtect(&slots->u1.Function, sizeof(DWORD), old_prot, &old_prot);
			return orig;
		}
	}
	return 0;
}

// Append a chunk of freshly synthesized PCM to the capture buffer.
static void CaptureAppend(const char *data, unsigned int len) {
	if (g_cap_size + len > g_cap_cap) {
		unsigned int new_cap = g_cap_cap ? g_cap_cap : 0x10000;
		while (g_cap_size + len > new_cap) {
			new_cap *= 2;
		}
		g_cap_buf = (char *)realloc(g_cap_buf, new_cap);
		g_cap_cap = new_cap;
	}
	memcpy(g_cap_buf + g_cap_size, data, len);
	g_cap_size += len;
}

// Resample the captured PCM to a standard 44.1 kHz / 16-bit file, keeping
// it mono unless the synthesizer proposed stereo, and write it to
// g_export_path. Uses linear interpolation (fine for this vintage voice).
static void FinalizeExport() {
	if (!g_cap_buf || g_cap_size == 0) {
		return;
	}

	int          bytes_per_sample = g_cap_bits / 8;
	if (bytes_per_sample < 1) {
		bytes_per_sample = 2;
	}
	int          in_ch      = g_cap_channels < 1 ? 1 : g_cap_channels;
	unsigned int src_frames = (g_cap_size / bytes_per_sample) / in_ch;
	if (src_frames == 0) {
		return;
	}

	// Normalize the source to interleaved signed 16-bit.
	short *src = (short *)malloc(src_frames * in_ch * sizeof(short));
	unsigned int i;
	if (g_cap_bits == 8) {
		unsigned char *s8 = (unsigned char *)g_cap_buf; // 8-bit PCM is unsigned
		for (i = 0; i < src_frames * (unsigned int)in_ch; i++) {
			src[i] = (short)(((int)s8[i] - 128) << 8);
		}
	} else {
		short *s16 = (short *)g_cap_buf;
		for (i = 0; i < src_frames * (unsigned int)in_ch; i++) {
			src[i] = s16[i];
		}
	}

	int    out_ch = (in_ch == 2) ? 2 : 1; // honor stereo, otherwise mono
	double ratio  = (double)TARGET_SAMPLE_RATE / (double)g_cap_rate;
	unsigned int dst_frames = (unsigned int)((double)src_frames * ratio);
	if (dst_frames == 0) {
		dst_frames = 1;
	}

	short *dst = (short *)malloc(dst_frames * out_ch * sizeof(short));
	for (i = 0; i < dst_frames; i++) {
		double       src_pos = (double)i / ratio;
		unsigned int i0      = (unsigned int)src_pos;
		double       frac    = src_pos - (double)i0;
		unsigned int i1      = i0 + 1;
		if (i1 >= src_frames) {
			i1 = src_frames - 1;
		}
		for (int c = 0; c < out_ch; c++) {
			int    ch = (c < in_ch) ? c : 0; // >2 source channels collapse to ch 0
			double a  = (double)src[i0 * in_ch + ch];
			double b  = (double)src[i1 * in_ch + ch];
			double v  = a + (b - a) * frac;
			if (v >  32767.0) v =  32767.0;
			if (v < -32768.0) v = -32768.0;
			dst[i * out_ch + c] = (short)v;
		}
	}
	free(src);

	unsigned int data_bytes = dst_frames * out_ch * sizeof(short);
	WavHeader    h;
	memcpy(h.riff, "RIFF", 4);
	memcpy(h.wave, "WAVE", 4);
	memcpy(h.fmt,  "fmt ", 4);
	memcpy(h.data, "data", 4);
	h.fmt_size    = 16;
	h.fmt_tag     = 1;
	h.channels    = (unsigned short)out_ch;
	h.sample_rate = TARGET_SAMPLE_RATE;
	h.bits        = 16;
	h.block_align = (unsigned short)(out_ch * 2);
	h.byte_rate   = TARGET_SAMPLE_RATE * h.block_align;
	h.data_bytes  = data_bytes;
	h.size        = 36 + data_bytes;

	HANDLE f = CreateFile(g_export_path, GENERIC_WRITE, 0, NULL,
	                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f != INVALID_HANDLE_VALUE) {
		DWORD written;
		WriteFile(f, &h,  sizeof(h),  &written, NULL);
		WriteFile(f, dst, data_bytes, &written, NULL);
		CloseHandle(f);
	}
	free(dst);
}

// --- WINMM hooks: passthrough plus, while capturing, a tap of the stream ---

MMRESULT WINAPI HookedWaveOutOpen(LPHWAVEOUT phwo, UINT device, LPCWAVEFORMATEX fmt,
                                  DWORD callback, DWORD instance, DWORD flags) {
	if (g_capturing && fmt) {
		g_cap_rate     = fmt->nSamplesPerSec; // the format the synthesizer proposes
		g_cap_channels = fmt->nChannels;
		g_cap_bits     = fmt->wBitsPerSample;
	}
	return _origWaveOutOpen(phwo, device, fmt, callback, instance, flags);
}

MMRESULT WINAPI HookedWaveOutWrite(HWAVEOUT hwo, LPWAVEHDR hdr, UINT size) {
	if (g_capturing && hdr && hdr->lpData && hdr->dwBufferLength) {
		CaptureAppend(hdr->lpData, hdr->dwBufferLength);
	}
	return _origWaveOutWrite(hwo, hdr, size);
}

MMRESULT WINAPI HookedWaveOutClose(HWAVEOUT hwo) {
	MMRESULT result = _origWaveOutClose(hwo);
	// The synthesizer closes the device once an utterance has finished
	// streaming. Only finalize when we have actually captured audio, so a
	// stray close during engine re-init does not end the export early.
	if (g_capturing && g_cap_size > 0) {
		FinalizeExport();
		g_capturing = false;
		g_cap_size  = 0;
		if (g_hWnd) {
			PostMessage(g_hWnd, WM_EXPORT_DONE, 0, 0);
		}
	}
	return result;
}

// Redirect the synthesizer's WINMM imports through our hooks. Called once,
// after the BST entry points have been resolved.
void InstallWaveHooks() {
	_origWaveOutOpen  = (waveOutOpenF) HookImport(bstLib, "winmm.dll", "waveOutOpen",  (void *)HookedWaveOutOpen);
	_origWaveOutWrite = (waveOutWriteF)HookImport(bstLib, "winmm.dll", "waveOutWrite", (void *)HookedWaveOutWrite);
	_origWaveOutClose = (waveOutCloseF)HookImport(bstLib, "winmm.dll", "waveOutClose", (void *)HookedWaveOutClose);
}

// Prompt for a path and export the current text box contents to a WAV file.
void ExportToWav(HWND hWnd) {
	if (g_capturing) {
		return; // an export is already running
	}

	HWND hwnd_text = GetDlgItem(hWnd, ID_EDITCHILD);
	int  len       = GetWindowTextLength(hwnd_text) + 1;
	char *text     = (char *)malloc(len);
	GetWindowText(hwnd_text, text, len);
	if (len <= 1) {
		free(text);
		SpeakText(hWnd, "Nothing to export.");
		return;
	}

	char path[MAX_PATH];
	lstrcpyA(path, "speech.wav");

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = hWnd;
	ofn.lpstrFilter = "Wave files (*.wav)\0*.wav\0All files (*.*)\0*.*\0";
	ofn.lpstrFile   = path;
	ofn.nMaxFile    = MAX_PATH;
	ofn.lpstrDefExt = "wav";
	ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	if (GetSaveFileName(&ofn)) {
		lstrcpyA(g_export_path, path);
		g_cap_size  = 0;
		g_capturing = true;
		// Synthesize the text: it plays as usual and the hooks tap the
		// stream. FinalizeExport() writes the file when the device closes.
		SpeakText(hWnd, text);
	}
	free(text);
}

// Function for speaking text
void SpeakText(HWND hWnd, const char *text, bool init) {
	if (InitSpeech()) {	
		const char *p_prefix = prefix;
		const char *r_prefix = prefix_rate;
		const char *f_prefix = prefix_freq;
		int         text_len;
		char       *text_to_speak;
		if (init) {
			_bstSetParams(tts_handle, RATE_SETTING, 0); // needs to be called to reset rate
			text_len  = strlen(p_prefix) + strlen(r_prefix) + strlen(f_prefix) + strlen(text);
			text_to_speak = (char *)malloc(text_len + 1); // +1 for the NUL terminator
			strcpy(text_to_speak, p_prefix);
			strcat(text_to_speak, r_prefix);
			strcat(text_to_speak, f_prefix);
			strcat(text_to_speak, text);
		} else {
			text_len  = strlen(p_prefix) + strlen(f_prefix) + strlen(text);
			text_to_speak = (char *)malloc(text_len + 1); // +1 for the NUL terminator
			strcpy(text_to_speak, p_prefix);
			strcat(text_to_speak, f_prefix);
			strcat(text_to_speak, text);
		}

		// Speak the text
		_TtsWav(tts_handle, hWnd, text_to_speak);
		free(text_to_speak);
	}
}

// Toggle hotkeys (for use and to free up for other programs when window is out of focus)
void ToggleHotkeys(HWND hWnd, BOOL to_enable) {
	if (to_enable) {
		RegisterHotKey(hWnd, IDM_EXIT,                        MOD_NOREPEAT,               VK_ESCAPE);
		RegisterHotKey(hWnd, IDM_HELP,                        MOD_NOREPEAT,               VK_F1);
		RegisterHotKey(hWnd, IDM_ABOUT,                       MOD_NOREPEAT,               VK_F2);
		RegisterHotKey(hWnd, ID_CONTROLS_RESET,               MOD_NOREPEAT,               VK_F3);
		RegisterHotKey(hWnd, ID_CONTROLS_SHUTUP,              MOD_NOREPEAT,               VK_F4);
		RegisterHotKey(hWnd, ID_CONTROLS_SPEAKTEXT,           MOD_NOREPEAT,               VK_F5);

		RegisterHotKey(hWnd, ID_CONTROLS_DECREASEVOLUME,      MOD_CONTROL | MOD_NOREPEAT, VK_F6);
		RegisterHotKey(hWnd, ID_CONTROLS_INCREASEVOLUME,      MOD_NOREPEAT,               VK_F6);

		RegisterHotKey(hWnd, ID_CONTROLS_SLOWERSPEED,         MOD_NOREPEAT,               VK_F7);
		RegisterHotKey(hWnd, ID_CONTROLS_FASTERSPEED,         MOD_NOREPEAT,               VK_F8);
		RegisterHotKey(hWnd, ID_CONTROLS_PITCHDOWN,           MOD_NOREPEAT,               VK_F9);
		RegisterHotKey(hWnd, ID_CONTROLS_PITCHUP,             MOD_NOREPEAT,               VK_F10);

		RegisterHotKey(hWnd, ID_CONTROLS_VOICECHANGEBACKWARD, MOD_CONTROL | MOD_NOREPEAT, VK_F11);
		RegisterHotKey(hWnd, ID_CONTROLS_VOICECHANGEFORWARD,  MOD_NOREPEAT,               VK_F11);

		RegisterHotKey(hWnd, IDS_SELECT,                      MOD_CONTROL | MOD_NOREPEAT, 0x41);

		RegisterHotKey(hWnd, ID_FILE_EXPORTWAV,               MOD_CONTROL | MOD_NOREPEAT, 0x45); // Ctrl+E
	} else {
		UnregisterHotKey(hWnd, IDM_EXIT);
		UnregisterHotKey(hWnd, IDM_HELP);
		UnregisterHotKey(hWnd, IDM_ABOUT);
		UnregisterHotKey(hWnd, ID_CONTROLS_RESET);
		UnregisterHotKey(hWnd, ID_CONTROLS_SHUTUP);
		UnregisterHotKey(hWnd, ID_CONTROLS_SPEAKTEXT);
		UnregisterHotKey(hWnd, ID_CONTROLS_DECREASEVOLUME);
		UnregisterHotKey(hWnd, ID_CONTROLS_INCREASEVOLUME);
		UnregisterHotKey(hWnd, ID_CONTROLS_SLOWERSPEED);
		UnregisterHotKey(hWnd, ID_CONTROLS_FASTERSPEED);
		UnregisterHotKey(hWnd, ID_CONTROLS_PITCHDOWN);
		UnregisterHotKey(hWnd, ID_CONTROLS_PITCHUP);
		UnregisterHotKey(hWnd, ID_CONTROLS_VOICECHANGEBACKWARD);
		UnregisterHotKey(hWnd, ID_CONTROLS_VOICECHANGEFORWARD);
		UnregisterHotKey(hWnd, IDS_SELECT);
		UnregisterHotKey(hWnd, ID_FILE_EXPORTWAV);
	}
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// Check if BST.DLL is loaded into memory
	// If not, output a message and exit with return code 1
	if (!bstLib) {
		MessageBox(NULL, "T-T-S not found!", "TTS Engine Error!", MB_ICONERROR);
		return 1;
	}

	// Load the important BST functions
	_bstCreate    = (bstCreateFunc)GetProcAddress(bstLib, "bstCreate");
	_TtsWav       = (TtsWavFunc)GetProcAddress(bstLib, "TtsWav");
	_bstRelBuf    = (bstRelBufFunc)GetProcAddress(bstLib, "bstRelBuf");
	_bstShutup    = (bstShutupFunc)GetProcAddress(bstLib, "bstShutup");
	_bstDestroy   = (bstDestroyFunc)GetProcAddress(bstLib, "bstDestroy");
	_bstClose     = (bstCloseFunc)GetProcAddress(bstLib, "bstClose");
	_bstSetParams = (bstSetParamsFunc)GetProcAddress(bstLib, "bstSetParams");
	_bstGetParams = (bstGetParamsFunc)GetProcAddress(bstLib, "bstGetParams");

	// Redirect the synthesizer's WINMM calls so we can tap audio for WAV export.
	InstallWaveHooks();

	// Initialize speech synthesizer
	int stat = InitSpeech();
	if (!stat) {
		return 1;
	}
	SetVoice(FRED);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BESTSPEAK, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	HWND hWnd;
	MSG msg;

	hInst = hInstance; // Store instance handle in our global variable

	// Create our main window here.
	// If unsuccessful, exit out with return code 3
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		                CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		return 3;
	}
	g_hWnd = hWnd; // used by the export hooks to signal completion

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Register hotkeys on startup
	ToggleHotkeys(hWnd, TRUE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_BESTSPEAK);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_BESTSPEAK;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

// Main callback function where the majority of functions happen
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit; // static keyword is important for textbox fill to work.
	HWND   hwnd_text;
	char  *speak_buf;
	int    wmId;
	int    len;

	switch (message) 
	{
		case WM_CREATE:
			hwndEdit = CreateWindow(
				"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL,
				0, 0, 0, 0, hWnd,
				(HMENU) ID_EDITCHILD,
				(HINSTANCE) GetWindowLong(hWnd, -6),
				NULL);
			SendMessage(hwndEdit, EM_SETLIMITTEXT,      0, 0); // Increase text character limit.
			PostMessage(hWnd,     WM_STARTUP, wParam, lParam); // Send message to announce TTS version after window is open.
			break;
		case WM_HOTKEY:
			wmId = LOWORD(wParam);
			switch (wmId)
			{
				case ID_CONTROLS_VOICECHANGEBACKWARD:
					voice_select -= 1;
					if (voice_select < FRED) {
						voice_select = DRACULA;
					}
					PostMessage(hWnd, WM_COMMAND, ID_VOICES_FRED + voice_select, lParam);
					break;
				case ID_CONTROLS_VOICECHANGEFORWARD:
					voice_select += 1;
					if (voice_select > DRACULA) {
						voice_select = FRED;
					}
					PostMessage(hWnd, WM_COMMAND, ID_VOICES_FRED + voice_select, lParam);
					break;
				default:
					PostMessage(hWnd, WM_COMMAND, wmId, lParam);
			}
			break;
		case WM_COMMAND:
			// Parse the menu selections:
			switch (LOWORD(wParam))
			{
				case IDS_SELECT:
					// Select all text using Ctrl+A
					SendMessage(hwndEdit, EM_SETSEL, 0, -1);
					break;
				case IDM_ABOUT:
					SpeakText(hWnd, about_text);
					break;
				case IDM_HELP:
					SpeakText(hWnd, help_text);
					break;
				case IDM_EXIT:
					SpeakText(hWnd, "Exiting.");
					DestroyWindow(hWnd);
					break;
				case ID_CONTROLS_SPEAKTEXT:
					// Grab text from EDIT control and allocate memory for it for the synthesizer to speak it.
					hwnd_text = GetDlgItem(hWnd, ID_EDITCHILD);
					len = GetWindowTextLength(hwnd_text) + 1;
					speak_buf = (char *)malloc(len);
					GetWindowText(hwnd_text, speak_buf, len);

					// Speak the text
					SpeakText(hWnd, speak_buf);
					free(speak_buf);
					break;
				case ID_FILE_EXPORTWAV:
					ExportToWav(hWnd);
					break;
				case ID_CONTROLS_SHUTUP:
					SpeakText(hWnd, "");
					break;
				case ID_CONTROLS_RESET:
					SetVoice(FRED);
					SpeakText(hWnd, "Reset.", TRUE);
					break;
				case ID_CONTROLS_INCREASEVOLUME:
					IncreaseGain();
					SpeakText(hWnd, "Louder.");
					break;
				case ID_CONTROLS_REDUCEVOLUME:
					DecreaseGain();
					SpeakText(hWnd, "Quieter.");
					break;
				case ID_CONTROLS_FASTERSPEED:
					IncreaseRate();
					SpeakText(hWnd, "Faster.");
					break;
				case ID_CONTROLS_SLOWERSPEED:
					DecreaseRate();
					SpeakText(hWnd, "Slower.");
					break;
				case ID_CONTROLS_PITCHUP:
					IncreasePitch();
					SpeakText(hWnd, "Higher.");
					break;
				case ID_CONTROLS_PITCHDOWN:
					DecreasePitch();
					SpeakText(hWnd, "Lower.");
					break;
				case ID_VOICES_FRED:
					SetVoice(FRED);
					SpeakText(hWnd, "Fred", TRUE);
					break;
				case ID_VOICES_SARAH:
					SetVoice(SARAH);
					SpeakText(hWnd, "Sarah", TRUE);
					break;
				case ID_VOICES_HARRY:
					SetVoice(HARRY);
					SpeakText(hWnd, "Harry", TRUE);
					break;
				case ID_VOICES_MARTHA:
					SetVoice(MARTHA);
					SpeakText(hWnd, "Martha", TRUE);
					break;
				case ID_VOICES_TIM:
					SetVoice(TIM);
					SpeakText(hWnd, "Tim", TRUE);
					break;
				case ID_VOICES_DEXTER:
					SetVoice(DEXTER);
					SpeakText(hWnd, "Dexter", TRUE);
					break;
				case ID_VOICES_ALIEN:
					SetVoice(ALIEN);
					SpeakText(hWnd, "Alien", TRUE);
					break;
				case ID_VOICES_KIT:
					SetVoice(KIT);
					SpeakText(hWnd, "Kit", TRUE);
					break;
				case ID_VOICES_WENDY:
					SetVoice(WENDY);
					SpeakText(hWnd, "Wendy", TRUE);
					break;
				case ID_VOICES_BRUNO:
					SetVoice(BRUNO);
					SpeakText(hWnd, "Bruno", TRUE);
					break;
				case ID_VOICES_GRANNY:
					SetVoice(GRANNY);
					SpeakText(hWnd, "Granny", TRUE);
					break;
				case ID_VOICES_GHOST:
					SetVoice(GHOST);
					SpeakText(hWnd, "Ghost", TRUE);
					break;
				case ID_VOICES_PEEPER:
					SetVoice(PEEPER);
					SpeakText(hWnd, "Peeper", TRUE);
					break;
				case ID_VOICES_DRACULA:
					SetVoice(DRACULA);
					SpeakText(hWnd, "Dracula", TRUE);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
				}
			break;
		case WM_SETFOCUS:
			SetFocus(hwndEdit);
			break;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE) {
				ToggleHotkeys(hWnd, FALSE);
			} else {
				ToggleHotkeys(hWnd, TRUE);
			}
			break;
		case WM_SIZE:
			MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
			break;
		case WM_DESTROY:
			CloseSpeech();
			FreeLibrary(bstLib);
			PostQuitMessage(0);
			break;
		case WM_STARTUP:
			// startup announcement shortened for v1.02
			SpeakText(hWnd, "Ready!", TRUE);
			break;
		case WM_EXPORT_DONE:
			// Posted by the WAV export hook once the file has been written.
			SpeakText(hWnd, "Exported.");
			break;
		case TTS_BUFFER_FULL:
			_bstRelBuf(tts_handle);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
