/* ===========================================================================
Gen.h

Definicoes para plugin de uso geral do Winamp.
Criado por Daniel.

Baseado em
   gen_sdk by Justin Frankel/Nullsoft (1997)
============================================================================*/
#include "cfgdlg.h"

typedef struct { int version;
                 char *description;
                 int (*init)();
                 void (*config)();
                 void (*quit)();
                 HWND hwndParent;
                 HINSTANCE hDllInstance;
} winampGeneralPurposePlugin;

typedef struct { UINT vk;
                 BYTE modcontrol, modshift, modalt, modwin;
                 char enabled; } HotKey;


#define GPPHDR_VER 0x10
#define WM_WA_IPC WM_USER

#define PRIHK 0xC0FF

#define NUM_HK  28

#define HKT_NORMAL 1
#define HKT_WINDOWED 2
#define HKT_SHOWHIDE 3
#define HKT_PLAYSTOP 4
#define HKT_CLOSE 5    //
#define HKT_GETTITLE 6 //
#define HKT_MUTE 7     //
#define HKT_PLWINDOWED 8
#define HKT_EQWINDOWED 9
#define HKT_SHOWPL 10
#define HKT_OSD 11
#define HKT_NORMALNOW 12

#define WinampWnd plugin.hwndParent

#define GetHKIdx() ((int)SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_GETCURSEL, (WPARAM)0, (LPARAM)0))
#define VerTecla(k) (HIBYTE(LOWORD(GetAsyncKeyState((k)))) >> 7)
#define ModifiersPressed() ((VerTecla(VK_CONTROL) ||  VerTecla(VK_SHIFT) || VerTecla(VK_MENU) || (VerTecla(VK_LWIN)) || VerTecla(VK_RWIN)))


// Macro para o envio de comandos ao Winamp
#define WinampCommand(oque) PostMessage(WinampWnd,WM_COMMAND,(oque),0)
#define WinampCommandNow(oque) SendMessage(WinampWnd,WM_COMMAND,(oque),0)
#define PLCommand(oque) PostMessage(PlayListWnd,WM_COMMAND,(oque),0)
#define EQCommand(oque) PostMessage(EQWnd,WM_COMMAND,(oque),0)


/*
  O "botoes" seguintes, sao os 5 que aparecem no "front-end" do Winamp
  juntamente com suas combinacoes com SHIFT e CTRL.
*/
#define WINAMP_BUTTON1                  40044 // Voltar
#define WINAMP_BUTTON2                  40045 // Play
#define WINAMP_BUTTON3                  40046 // Pause
#define WINAMP_BUTTON4                  40047 // Stop
#define WINAMP_BUTTON5                  40048 // Avancar


// "Comandos" do Winamp
#define WINAMP_FILE_PLAY                40029 // Abrir arquivos
#define WINAMP_JUMP                     40193 // Pular para musica
#define WINAMP_JUMPFILE                 40194 // Pular para tempo
#define WINAMP_VOLUMEUP                 40058 // Aumenta "um pouco" o volume
#define WINAMP_VOLUMEDOWN               40059 // Diminui "um pouco" o volume

#define IPC_ISPLAYING 104

#define WINAMP_DIR_PLAY    40187 // Abrir diretorio
#define WINAMP_URL_PLAY    40155 // Abrir URL
#define WINAMP_CD_PLAY                  40323

#define PLAYLIST_ADD_DIR   1036
#define PLAYLIST_ADD_URL   1039
#define PLAYLIST_ADD_FILE  1032  //(ALT+SHIFT+5)

#define EQ_LOAD_PRESET     40172
#define EQ_LOAD_DEFAULT    40174
#define EQ_LOAD_EQF        40253

#define WINAMP_OPTIONS_PREFS            40012 // Abre preferencias
#define WINAMP_OPTIONS_PLEDIT           40040 // toggles the playlist window

#define WINAMP_FFWD5S                   40060 // fast forwards 5 seconds
#define WINAMP_REW5S                    40061 // rewinds 5 seconds

#define WINAMP_MODE_SHUFFLE             40023
#define WINAMP_MODE_REPEAT              40022

#define WINAMP_VIS         40192

#define IPC_SETVOLUME 122
//SendMessage(hwnd_winamp,WM_WA_IPC,volume,IPC_SETVOLUME);


  #define IPC_GETPLAYLISTTITLE 212
  /*
  ** char *name=SendMessage(hwnd_winamp,WM_WA_IPC,index,IPC_GETPLAYLISTTITLE);
  **
  ** IPC_GETPLAYLISTTITLE gets the title of the playlist entry [index].
  ** returns a pointer to it. returns NULL on error.
  */

  #define IPC_GETLISTPOS 125
  /*
  ** int pos=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTPOS);
  **
  ** IPC_GETLISTPOS returns the playlist position. A lot like IPC_WRITEPLAYLIST
  ** only faster since it doesn't have to write out the list. Heh, silly me.
  */

  #define IPC_GETOUTPUTTIME 105

  /*
  ** int res = SendMessage(hwnd_winamp,WM_WA_IPC,mode,IPC_GETOUTPUTTIME);
  **
  ** IPC_GETOUTPUTTIME returns the position in milliseconds of the
  ** current song (mode = 0), or the song length, in seconds (mode = 1).
  ** Returns -1 if not playing or error.
  */

  #define IPC_GETINFO 126

  /* (requires Winamp 2.05+)
  ** int inf=SendMessage(hwnd_winamp,WM_WA_IPC,mode,IPC_GETINFO);
  **
  ** IPC_GETINFO returns info about the current playing song. The value
  ** it returns depends on the value of 'mode'.
  ** Mode      Meaning
  ** ------------------
  ** 0         Samplerate (i.e. 44100)
  ** 1         Bitrate  (i.e. 128)
  ** 2         Channels (i.e. 2)
  */

#define IPC_JUMPTOTIME 106

#define IPC_GETSHUFFLE 250 // Returns the status of the shuffle option (1 if set)
#define IPC_GETREPEAT 251 // Returns the status of the repeat option (1 if set)

// Prototipos
//int GetHKIdx(void) ;
void AtualizarDlg(void);
int KeyValid(int i);
int KeyValid2(int i);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogProc( HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EdProc( HWND, UINT, WPARAM, LPARAM);

int init();
void config();
void quit();

int init2();
void foofun();


void OSDShow(HINSTANCE, char *);


#ifdef DEBUGMODE
  #define HOTKEYREG(n,k) RegisterHotKey(WinampWnd,PRIHK+n,MOD_SHIFT+MOD_CONTROL, LOBYTE(VkKeyScanEx(k, GetKeyboardLayout(NULL))))
  #define DEBUGMSG(x,f)   MessageBox(NULL,(x),(f),MB_OK);
#endif


// Prototipo da funcao que retorna um ponteiro para a estrutura do plugin
// __declspec( dllexport ) especifica uma funcao exportavel da dll
__declspec( dllexport ) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin();

typedef BOOL (WINAPI *MYPROC)(HWND, COLORREF, BYTE, DWORD);



#define IPC_GETPLAYLISTFILE 211

/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** char *name=SendMessage(hwnd_winamp,WM_WA_IPC,index,IPC_GETPLAYLISTFILE);
**
** IPC_GETPLAYLISTFILE gets the filename of the playlist entry [index].
** returns a pointer to it. returns NULL on error.
*/

#define IPC_GETLISTPOS 125

/* (requires Winamp 2.05+)
** int pos=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTPOS);
**
** IPC_GETLISTPOS returns the playlist position. A lot like IPC_WRITEPLAYLIST
** only faster since it doesn't have to write out the list. Heh, silly me.
*/

