/* ===========================================================================
Flamekeys V2.01 - Winamp Plug-in
Autor: Daniel Simoes (ddsimoes@gmx.net)

Testado com Winamp 2.79 (Windows 2000 Server)
Compilado com Borland C++ 5.5 (command-line free)
============================================================================*/

#define wsprintfA _wsprintfA  // Truque para funcionar no compilador da Borland
#define STRICT                // Checagem de tipo rigorosa
#include <windows.h>
#include "gen.h"



// Variaveis globais
BOOL WIN2K=FALSE;          // Indica se eh possivel ou nao executar o fadeout
WNDCLASS wndclass;         // Classe da janela
WNDPROC lpOldWndProc;      // Apontador para o "funcao de janela" do Winamp
HWND hwndDlg;              // Handle da janela de configuracao
HWND PlayListWnd;          // Handle da janela de playlist do winamp
HWND EQWnd;                // Equalizer
HICON icologo;             // Icone do winamp usado no OSD
BOOL OSDEnabled=FALSE;     // Indica se o OSD esta ativado
BOOL PluginEnabled=TRUE;   // Indica se o plugin esta ativado
BOOL HKWarnEnabled=FALSE;  // Mostrar aviso qd nao for possivel ativar uma hotkey?
BOOL FadeoutEnabled=TRUE;  // Precisa dizer?
BOOL SMFullscreen=TRUE;    // Safe mode para fullscreen
BOOL SMTopmost=TRUE;       // Safe mode para topmost
int OSDTime=15;            // Tempo que OSD fica na tela até comecar a desaparecer
HINSTANCE MyDll=NULL;      // Handle da User32.dll para carregar a funcao SetLayeredWindowAttributes
MYPROC SetTransp=NULL;     // Ponteiro para a funcao SetLayeredWindowAttributes para transparencia


/*
 Vetor que contem todas as informacoes sobre as hotkeys e suas funcoes

  caption   = Nome da funcao. Ira aparecer nas configuracoes e no OSD.
  hk        = Estrutura da hotey:
                vk         = codigo 'virtual key' da tecla
                modcontrol = modificador control
                modshift   = modificador shift
                modalt     = modificador alt
                modwin     = modificador win
  hktype    = Tipo da funcao.
  hkcommand = Comando a ser enviado ao Winamp.
*/
struct { char *caption;
         HotKey hk;
         short hktype;
         LPARAM hkcommand;
       } hotkeys[NUM_HK] = { {"Previous",'1',1,1,0,0,1,HKT_NORMAL,WINAMP_BUTTON1},
                             {"Play/Stop",'2',1,1,0,0,1,HKT_PLAYSTOP,NULL},
                             {"Pause",'3',1,1,0,0,1,HKT_NORMAL,WINAMP_BUTTON3},
                             {"Next",'4',1,1,0,0,1,HKT_NORMAL,WINAMP_BUTTON5},
                             {"Play",0,0,0,0,0,0,HKT_NORMAL,WINAMP_BUTTON2},
                             {"Stop",0,0,0,0,0,0,HKT_NORMAL,WINAMP_BUTTON4},
                             {"Eject/Play file(s)",'5',1,1,0,0,1,HKT_WINDOWED,WINAMP_FILE_PLAY},
                             {"Jump to file",0,0,0,0,0,0,HKT_WINDOWED,WINAMP_JUMPFILE},
                             {"Volume Up",0,0,0,0,0,0,HKT_NORMAL,WINAMP_VOLUMEUP},
                             {"Volume Down",0,0,0,0,0,0,HKT_NORMAL,WINAMP_VOLUMEDOWN},
                             {"Fastforward",0,0,0,0,0,0,HKT_NORMAL,WINAMP_FFWD5S},
                             {"Rewind",0,0,0,0,0,0,HKT_NORMAL,WINAMP_REW5S},
                             {"Mute",0,0,0,0,0,0,HKT_MUTE,NULL},
                             {"Play dir",0,0,0,0,0,0,HKT_WINDOWED,WINAMP_DIR_PLAY},
                             {"Play location",0,0,0,0,0,0,HKT_WINDOWED,WINAMP_URL_PLAY},
                             {"Play CD",0,0,0,0,0,0,HKT_NORMAL,WINAMP_CD_PLAY},
                             {"Toggle shuffle",0,0,0,0,0,0,HKT_NORMALNOW,WINAMP_MODE_SHUFFLE},
                             {"Toggle repeat",0,0,0,0,0,0,HKT_NORMALNOW,WINAMP_MODE_REPEAT},
                             {"Show/Hide Winamp",0,0,0,0,0,0,HKT_SHOWHIDE,NULL},
                             {"Start/Stop vis plug-in",0,0,0,0,0,0,HKT_NORMAL,WINAMP_VIS},
                             {"Winamp preferences",0,0,0,0,0,0,HKT_WINDOWED,WINAMP_OPTIONS_PREFS},
                             {"Show/Hide Playlist",0,0,0,0,0,0,HKT_SHOWPL,NULL},
                             {"Playlist Add file(s)",0,0,0,0,0,0,HKT_PLWINDOWED,PLAYLIST_ADD_FILE},
                             {"Playlist Add dir",0,0,0,0,0,0,HKT_PLWINDOWED,PLAYLIST_ADD_DIR},
                             {"Playlist Add location",0,0,0,0,0,0,HKT_PLWINDOWED,PLAYLIST_ADD_URL},
                             {"EQ Load preset",0,0,0,0,0,0,HKT_EQWINDOWED,EQ_LOAD_PRESET},
                             {"Show OSD",0,0,0,0,0,0,HKT_OSD,NULL},
                             {"Close Winamp",0,0,0,0,0,0,HKT_CLOSE,NULL}
                           };


HotKey cfghk[NUM_HK];  // Vetor que contem as alteracoes feitas na configuracao


// Contem as informacoes usadas pelo Winamp para se comunicar com o plugin
winampGeneralPurposePlugin plugin = {GPPHDR_VER,     // Versao (tem que ser GPPHDR_VER=0x10)
                                     "Flamekeys plug-in v2.01                 [gen_fkeys.dll]", // Nome do plugin
                                     init,   // Funcao de Iniciacao
                                     config, // Funcao de configuracao
                                     quit,   // Finalizacao
                                     NULL,   // Contera o handle da janela do Winamp
                                     NULL};  // "Instance" da dll



/*
  Assegura que o plug-in seja carregado para uma unica instancia do Winamp.
  Carrega e libera a funcao SetLayeredWindowAttributes.

  O plugin só se permite carregar uma unica vez por causa da sua natureza de hotkeys.
  Se ele for aberto duas vezes irá dar erro de ativacao para todas as sua hotkeys, ja
  que ela ja estaram registradas.
*/
BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {

  HANDLE hMutex = NULL;

  switch (fdwReason) {

    case DLL_PROCESS_ATTACH:   // um processo esta abrindo a dll

      /* Se for segunda "instancia" nao permitir abertura da dll*/
      hMutex = CreateMutex(NULL,FALSE,"flamekeysmutex");
      if ((hMutex == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS)) {
        plugin.description = "Flamekeys (First instance only)   [gen_fkeys.dll]";
        plugin.init = init2;
        plugin.config = foofun;
        plugin.quit = foofun;
        return TRUE;
      }

      /* Carregar a funcao SetLayeredWindowAttributes */
      MyDll=LoadLibrary("user32.dll");
      if (MyDll != NULL) {
         SetTransp=(MYPROC)GetProcAddress(MyDll,"SetLayeredWindowAttributes");
         if (SetTransp != NULL)
           WIN2K=TRUE;  // Se carregou, sinalizar a presenca do recurso
      }
      break;

    case DLL_THREAD_ATTACH:  // uma nova thread esta abrindo a dll, portanto nao carregar
      return FALSE;          // pois o plugin, pela sua natureza, nao eh multithreaded.

    case DLL_PROCESS_DETACH:    // Descarregando a dll.
      FreeLibrary(MyDll);       // descarregar a user32.dll.
      CloseHandle(hMutex);
      break;

  }
  return TRUE;
}



/*
  winampGetGeneralPurposePlugin()

  Funcao usada pelo Winamp para pegar o endereco da variavel que contem as
  informacoes a respeito do plugin.
*/
winampGeneralPurposePlugin* winampGetGeneralPurposePlugin() {

  return &plugin;
}



/*
  RegHotKeys()

  Funcao auxiliar pra registrar as teclas de atalho
*/
void RegHotKeys(void) {
  int i;
  char buf[400];

  for (i=0;i<NUM_HK;i++) {
    if (!KeyValid(i)) hotkeys[i].hk.enabled = 0; // senao eh valida, desativa.

    /* Se a hotkey estiver ativada entao registra-la */
    if ( hotkeys[i].hk.enabled )
      if ( (!RegisterHotKey(WinampWnd,PRIHK+i+1,
                           MOD_SHIFT*hotkeys[i].hk.modshift+
                           MOD_CONTROL*hotkeys[i].hk.modcontrol+
                           MOD_ALT*hotkeys[i].hk.modalt+
                           MOD_WIN*hotkeys[i].hk.modwin,
                           hotkeys[i].hk.vk)) && (HKWarnEnabled) ) {
        /* Mostrar msg em caso de insucesso, mas somente se HKWarnEnabled = TRUE */
        wsprintf(buf,"Can't register hotkey for function %s.\n"
                     "\n"
                     "The combination keys you choosed are refused from Windows.\n"
                     "This can be caused by another app that registered this hotkey first\n"
                     "or because it are reserved keys.\n"
                     "\n"
                     "Please configure another combination.\n", hotkeys[i].caption);
        MessageBox(WinampWnd,buf,"Warning",MB_OK+MB_ICONWARNING);
      }
  }
}


/*
  UnRegHotKeys()

  Funcao auxiliar pra desregistrar as teclas de atalho
*/
void UnRegHotKeys(void) {
  int i;

  for (i=0;i<NUM_HK;i++)
    UnregisterHotKey(WinampWnd,PRIHK+1+i);
}


/*
  BufToHotKey()

  Converte um vetor para uma estrutura HotKey.
  Usada para converter as informacoes carregadas do registro.
*/
int BufToHotKey(byte *buf, HotKey *hk) {

  (*hk).vk = (UINT)buf[0];
  (*hk).modcontrol = buf[1];
  (*hk).modshift = buf[2];
  (*hk).modalt = buf[3];
  (*hk).modwin = buf[4];
  (*hk).enabled = buf[5];
  return 1;
}


/*
  HotKeyToBuf()

  Converte uma estrutura HotKey para um vetor.
  Usada para converter as informacoes para serem salvas no registro.
*/
int HotKeyToBuf(byte *buf, HotKey hk) {

  buf[0]=(byte)(hk.vk);
  buf[1]=hk.modcontrol;
  buf[2]=hk.modshift;
  buf[3]=hk.modalt;
  buf[4]=hk.modwin;
  buf[5]=hk.enabled;
  return 1;
}


#define SaveCfgVar(a,b)  tmpword=(DWORD)(b); erro = RegSetValueEx(subkey,(a),NULL,REG_DWORD,(BYTE *)&tmpword,sizeof(DWORD))


/*
  SaveCfg2()

  Salva as configuracoes do plugin.
*/
void SaveCfg2(void) {
  HKEY subkey;
  char chave[6];
  LONG erro;
  int i;
  DWORD dispo,tmpword;

  erro = RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Winamp\\Flamekeys Plug-in",0,"",REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,NULL,&subkey,&dispo);
  if (erro == ERROR_SUCCESS) {
    for (i=0;i<NUM_HK;i++) {
      HotKeyToBuf(chave,hotkeys[i].hk);
      erro = RegSetValueEx(subkey,hotkeys[i].caption,NULL,REG_BINARY,chave,6);
      if (erro != ERROR_SUCCESS)
        MessageBox(WinampWnd,hotkeys[i].caption,"Error - Can't save hotkey info",MB_OK+MB_ICONERROR);
    }

    SaveCfgVar("Enable OSD",OSDEnabled);
    SaveCfgVar("Enable hotkey warning",HKWarnEnabled);
    SaveCfgVar("Enable Plug-in",PluginEnabled);
    SaveCfgVar("Enable fade out",FadeoutEnabled);
    SaveCfgVar("OSD display time",OSDTime);
    SaveCfgVar("Fullscreen safemode",SMFullscreen);
    SaveCfgVar("Topmost safemode",SMTopmost);

    RegCloseKey(subkey);
  }
  else
    MessageBox(WinampWnd,"HKEY_CURRENT_USER\\Software\\Winamp\\Flamekeys Plug-in","Error - Can't open/create regkey",MB_OK+MB_ICONERROR);

}


/*
 Funcao auxiliar para carregar um valor booleano de uma chave do registro
*/
BOOL LoadCfgBOOL(HKEY subkey, char *valuename, BOOL *cfgvar){
  LONG erro;
  DWORD tipo=REG_DWORD;
  DWORD tmpword,tam=sizeof(DWORD);

  erro = RegQueryValueEx(subkey,valuename,NULL,&tipo,(BYTE *)&tmpword,&tam);
  if (erro != ERROR_SUCCESS)
    return FALSE;
  (*cfgvar)=(BOOL)tmpword;
  return TRUE;
}

#define LoadCfgBOOLM(a,b) LoadCfgBOOL(subkey,(a),(b))

/*
  LoadCfg2()

  Carrega as configuracoes do plugin
*/
void LoadCfg2(void) {
  HKEY subkey;
  char chave[6];
  DWORD tipo,tam,tmpword;
  LONG erro;
  int i;
  BOOL loadok;

  erro = RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Winamp\\Flamekeys Plug-in",0,KEY_QUERY_VALUE,&subkey);
  if (erro == ERROR_SUCCESS) {
    tam=6;
    tipo=REG_BINARY;
    loadok=TRUE;
    for (i=0;i<NUM_HK;i++) {
      erro = RegQueryValueEx(subkey,hotkeys[i].caption,NULL,&tipo,chave,&tam);
      if (erro == ERROR_SUCCESS) {
        BufToHotKey(chave,&(hotkeys[i].hk));
      }
      else {
        loadok=FALSE;
      }
    }

    tipo=REG_DWORD;
    tam=sizeof(DWORD);

    erro = LoadCfgBOOLM("Enable OSD",&OSDEnabled);
    erro = LoadCfgBOOLM("Enable hotkey warning",&HKWarnEnabled);
    erro = LoadCfgBOOLM("Enable Plug-in",&PluginEnabled);
    erro = LoadCfgBOOLM("Enable fade out",&FadeoutEnabled);
    erro = LoadCfgBOOLM("Fullscreen safemode",&SMFullscreen);
    erro = LoadCfgBOOLM("Topmost safemode",&SMTopmost);

    erro = RegQueryValueEx(subkey,"OSD display time",NULL,&tipo,(BYTE *)&tmpword,&tam);
    if (erro != ERROR_SUCCESS)
      loadok=FALSE;
    else
      OSDTime=(int)tmpword;

    RegCloseKey(subkey);
    if (!loadok) SaveCfg2();
  }
  else {
    SaveCfg2();
  }
}



/*
  Pega um nome de arquivo no formato "drive:\pasta1\..\pastan\arquivo.ext" e devolve
  so o nome "drive:\pasta1\..\pastan"
*/
void CropPath(char *buf) {
  int i;

  i=lstrlen(buf)-1;
  while((buf[i] != '\\') && (i>0))
    i--;
  buf[i] = '\0';
}

/*
  Devolve o nome do diretorio onde o Winamp esta instalado
*/
void GetWinampDir(char *buf, DWORD nSize) {
  GetModuleFileName((HINSTANCE)GetWindowLong(WinampWnd,GWL_HINSTANCE), buf, nSize);
  CropPath(buf);
}


/*
  int init()

  Funcao chamada pelo Winamp logo que o plugin eh carregado
  Inicia o funcionamento do plugin:
    - Cria uma janela (necessaria para receber as notificacoes de hotkey)
    - Registra as hotkeys
*/
int init() {

  HINSTANCE wampmod;
  int cfgvol,cfgpos;

  if (!WIN2K)
    FadeoutEnabled=FALSE;
  LoadCfg2(); // Carrega as configuracoes
  if (!WIN2K)
    FadeoutEnabled=FALSE;

  if (PluginEnabled)
    RegHotKeys(); // Registra as "hotkeys"

  PlayListWnd = FindWindow("Winamp PE",NULL);
  EQWnd = FindWindow("Winamp EQ",NULL);

  wampmod = (HINSTANCE)GetWindowLong(WinampWnd,GWL_HINSTANCE);
  icologo=LoadIcon(wampmod,MAKEINTRESOURCE(164));

  // Modifica a "WNDPROC" do Winamp
  lpOldWndProc = (WNDPROC)GetWindowLong(WinampWnd,GWL_WNDPROC);
  SetWindowLong(WinampWnd,GWL_WNDPROC,(LONG)WndProc);

  return 0;
}


/*
  void config()

  Funcao chamada pelo Winamp para efetuar configuracoes no plugin.
*/
void config() {

  if (PluginEnabled)
    UnRegHotKeys();
  DialogBox(plugin.hDllInstance,MAKEINTRESOURCE(CFGDIAG),GetForegroundWindow(),(DLGPROC)DialogProc);
  if (PluginEnabled)
    RegHotKeys();
  SaveCfg2();
}



/*
  void quit()

  Funcao chamada para efetuar a finalizacao do plugin:
    - limpar memoria;
    - desabilitar "hotkeys";
    - etc.
*/
void quit() {

  if (PluginEnabled)
    UnRegHotKeys();
  SetWindowLong(WinampWnd,GWL_WNDPROC,(LONG)lpOldWndProc);
  return;
}

#define OSDSHOW(x) OSDShow(plugin.hDllInstance,(x))

void OSD(char *buf) {
  if (OSDEnabled)
    OSDSHOW(buf);
  return;
}

#define IPC_STARTPLAY 102

/*
  WndProc()

  Recebe e responde as mensagens enviadas a janela.
  No caso desse plugin, recebe as notificacoes de "hotkey"
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

  int hotk;

  if (message == WM_HOTKEY) {

    hotk = wParam-PRIHK-1;

    switch (hotkeys[hotk].hktype) {

      case HKT_NORMAL:
        WinampCommand(hotkeys[hotk].hkcommand);
        OSD(hotkeys[hotk].caption);
        return 0L;

      case HKT_NORMALNOW:
        WinampCommandNow(hotkeys[hotk].hkcommand);
        OSD(hotkeys[hotk].caption);
        return 0L;

      case HKT_PLAYSTOP:
        if (SendMessage(WinampWnd,WM_WA_IPC,0,IPC_ISPLAYING) == 1) {
          WinampCommand(WINAMP_BUTTON4);
          OSD("Stop");
        }
        else {
          WinampCommand(WINAMP_BUTTON2);
          OSD("Play");
        }
        return 0L;

      case HKT_WINDOWED:
        SetForegroundWindow(WinampWnd);
        WinampCommand(hotkeys[hotk].hkcommand);
        return 0L;

      case HKT_PLWINDOWED:
        SetForegroundWindow(PlayListWnd);
        PLCommand(hotkeys[hotk].hkcommand);
        return 0L;

      case HKT_EQWINDOWED:
        SetForegroundWindow(EQWnd);
        EQCommand(hotkeys[hotk].hkcommand);
        return 0L;

      case HKT_SHOWHIDE:

        if (IsIconic(WinampWnd)) {
          ShowWindow(WinampWnd,SW_RESTORE);
          SetForegroundWindow(WinampWnd);
        }
        else {
          SendMessage(WinampWnd,WM_SYSCOMMAND,SC_MINIMIZE,(LPARAM)0L);
        }
        return 0L;

      case HKT_SHOWPL:
        SetForegroundWindow(PlayListWnd);
        WinampCommand(WINAMP_OPTIONS_PLEDIT);
        return 0L;

      case HKT_MUTE:
        SendMessage(WinampWnd,WM_WA_IPC,0,IPC_SETVOLUME);
        OSD("Mute");
        return 0L;

      case HKT_CLOSE:
        PostMessage(WinampWnd,WM_CLOSE,0,0);
        return 0L;

      case HKT_OSD:
        OSDSHOW("Song info");
        return 0L;

    }
  }
  return CallWindowProc((WNDPROC)lpOldWndProc,WinampWnd,message,wParam,lParam);
}



/*
  vk2ascii()

  Converte uma "virtual key" para seu equivalente em ascii.
*/
char vk2ascii(UINT vk)
{
   WORD result[3];
   BYTE State[256];
   int i;
   UINT scancode;

   for (i=0;i<256;i++) State[i] = 0;
   scancode=MapVirtualKey(scancode,0);
   if (ToAscii(vk,scancode,State,result,0))
     return (char)CharUpper((char *)result[0]);
   return 0;
}



/*
  KeyLabel()

  Converte uma "virtual key" para seu nome usual.
*/
int KeyLabel(int vk,char* buf) {

  int i;
  short vkfound;
  char vkascii;
  struct { int vk; char *vkname; } vktable[] = { {VK_MULTIPLY, "Num *"},
                                                 {VK_ADD, "Num +"},
                                                 {VK_SEPARATOR,  "Num ,"},
                                                 {VK_SUBTRACT,  "Num -"},
                                                 {VK_DECIMAL,  "Num ."},
                                                 {VK_DIVIDE,  "Num /"},
                                                 {VK_UP,  "Up"},
                                                 {VK_DOWN,  "Down"},
                                                 {VK_LEFT,  "Left"},
                                                 {VK_RIGHT,  "Right"},
                                                 {VK_SPACE,  "Space"},
                                                 {VK_HOME,  "Home"},
                                                 {VK_END,  "End"},
                                                 {VK_DELETE,  "Del"},
                                                 {VK_INSERT,  "Ins"},
                                                 {VK_PRIOR,  "PgUP"},
                                                 {VK_NEXT,  "PgDwn"},
                                                 {VK_SCROLL,  "SLock"},
                                                 {VK_BACK, "Back"}
                                               };

  if ((vk >= VK_NUMPAD0) && (vk <= VK_NUMPAD9))
    wsprintf(buf,"Num %d",vk-VK_NUMPAD0);
  else if ((vk >= VK_F1) && (vk <= VK_F24))
    wsprintf(buf,"F %d",vk-VK_F1+1);
  else {
    vkfound = 0;
    for (i=0; i<19; i++) {
      if (vk == vktable[i].vk) {
        lstrcat(buf,vktable[i].vkname);
        vkfound = 1;
        break;
      }
    }
    if (vkfound == 0) {
      vkascii = vk2ascii(vk);
      if (vkascii) wsprintf(buf,"%c",vkascii);
    }
  }
  return (lstrlen(buf));
}



/*
  Modificadores()

  Formata uma string com o nome dos modificadores desejados.
*/
int Modificadores(char *buf, BYTE modcontrol, BYTE modshift, BYTE modalt, BYTE modwin) {

  int nmod=0;

  if (modcontrol)  { lstrcat(buf,"Ctrl+"); nmod++; }
  if (modshift)    { lstrcat(buf,"Shift+"); nmod++; }
  if (modalt)      { lstrcat(buf,"Alt+"); nmod++; }
  if (modwin)      { lstrcat(buf,"Win+"); nmod++; }
  return nmod;
}



/*
  KeyDescription()

  Retorna uma string formatada com a combinacao de teclas
*/
int KeyDescription(char *buf, int i) {

  char kl[6]="";
  char modis[20]="";

  if ( (KeyLabel(cfghk[i].vk, kl) > 0)
       &&
       (
         (cfghk[i].modalt) || (cfghk[i].modcontrol) || (cfghk[i].modwin) ||
         ((cfghk[i].vk >= VK_F1) && (cfghk[i].vk <= VK_F24))
       )
     ) {
    Modificadores(modis,cfghk[i].modcontrol,cfghk[i].modshift,cfghk[i].modalt,cfghk[i].modwin);
    lstrcat(buf,modis);
    lstrcat(buf,kl);
    return 1;
  }
  return 0;
}



/*
  KeyValid()

  Testa a validade de uma hotkey.
*/
int KeyValid(int i) {
  char kl[6]="";

  return ( (KeyLabel(hotkeys[i].hk.vk, kl) > 0)
           &&
           (
             (hotkeys[i].hk.modalt) || (hotkeys[i].hk.modcontrol) || (hotkeys[i].hk.modwin)
             ||
             ((hotkeys[i].hk.vk >= VK_F1) && (hotkeys[i].hk.vk <= VK_F24))
           )
         );
}


/*
  KeyValid2()

  Testa a validade de uma hotkey.
*/
int KeyValid2(int i) {
  char kl[6]="";

  return ( (KeyLabel(cfghk[i].vk, kl) > 0)
           &&
           (
             (cfghk[i].modalt) || (cfghk[i].modcontrol) || (cfghk[i].modwin)
             ||
             ((cfghk[i].vk >= VK_F1) && (cfghk[i].vk <= VK_F24))
           )
         );
}



/*
  AtualizarDlg()

  Altera as propriedades da janela de configuracao baseada na funcao selecionada.
*/
void AtualizarDlg(void) {

  char bla[25]="";

  SendDlgItemMessage(hwndDlg, IDC_ENABLED, BM_SETCHECK, (WPARAM)(cfghk[GetHKIdx()].enabled ? BST_CHECKED : BST_UNCHECKED), (LPARAM)0);

  if ( KeyDescription(bla,GetHKIdx()) )
    SendDlgItemMessage(hwndDlg, IDC_EDIT, WM_SETTEXT, (WPARAM) 0, (LPARAM)bla);
  else
    SendDlgItemMessage(hwndDlg, IDC_EDIT, WM_SETTEXT, (WPARAM) 0, (LPARAM)"[None]");
  SendDlgItemMessage(hwndDlg, IDC_EDIT, EM_SETSEL, (WPARAM)0l, (LPARAM)0l);
  return;
}



/*
  EdProc()

  Recebe e responde as mensagens enviadas ao editor de hotkey.
*/
LRESULT CALLBACK EdOSDTimeProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

  if (uMsg == WM_CHAR) {
    if (!(( (((char)wParam) >= '0') && (((char)wParam) <= '9') )
          || (wParam == VK_DELETE) || (wParam == VK_BACK) ))
      return 0L;
  }
  return CallWindowProc((WNDPROC)GetWindowLong(hwnd, GWL_USERDATA), hwnd, uMsg, wParam, lParam);
}


int ToNumber(char *a) {
  int n=0,i=0;

  if (a[0] == '\0') return 0;
  while(a[i] != '\0') {
    n *= 10;
    n += (int)(a[i]-'0');
    i++;
  }
  return n;
}


void OSDControlsEnabled(BOOL enabled) {
  if (WIN2K)
    EnableWindow(GetDlgItem(hwndDlg, IDC_FADEOUT),enabled);
  EnableWindow(GetDlgItem(hwndDlg, IDC_SMTM),enabled);
  EnableWindow(GetDlgItem(hwndDlg, IDC_SMFS),enabled);
  EnableWindow(GetDlgItem(hwndDlg, IDC_OSDTIME),enabled);
  EnableWindow(GetDlgItem(hwndDlg, IDC_GROUPBOX3),enabled);
  EnableWindow(GetDlgItem(hwndDlg, IDC_OSDTXT),enabled);

  return;
}



void CallUninstall(void)  {
  char buf[256];

  if (MessageBox(hwndDlg,"To continue uninstalling Flamekeys Winamp might be closed.\n\n"
                       "Continue closing Winamp?","Flamekeys uninstall",
                 MB_YESNO|MB_ICONWARNING) == IDYES) {
    GetWinampDir(buf,252);
    lstrcat(buf,"\\UninstFkeys.exe");
    WinExec(buf,SW_SHOWNORMAL);
    PostMessage(WinampWnd,WM_CLOSE,0L,0L);
  }
}


/*
  DialogProc()

  Recebe e responde as mensagens enviadas a janela de configuracao.
*/
BOOL CALLBACK DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

  WNDPROC wndproc;

  int i;
  char buf[5];

  switch(uMsg) {

    case WM_INITDIALOG:
      hwndDlg = hwnd;

      for (i=0;i<NUM_HK;i++) {
        cfghk[i] = hotkeys[i].hk;
        SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM)(hotkeys[i].caption));
      }

      SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

      //Opcoes
      #define SetCheckState(cb,x) SendDlgItemMessage(hwndDlg, (cb), BM_SETCHECK, (WPARAM)((x) ? BST_CHECKED : BST_UNCHECKED), (LPARAM)0);
      SetCheckState(IDC_PLUGENABLED, PluginEnabled);
      SetCheckState(IDC_HKWARN, HKWarnEnabled);
      SetCheckState(IDC_OSD, OSDEnabled);
      SetCheckState(IDC_SMTM, SMTopmost);
      SetCheckState(IDC_SMFS, SMFullscreen);

      if (WIN2K) {
        SetCheckState(IDC_FADEOUT, FadeoutEnabled);
      }
      else
        EnableWindow(GetDlgItem(hwndDlg, IDC_FADEOUT),FALSE);

      wsprintf(buf,"%d",OSDTime);
      SendDlgItemMessage(hwndDlg, IDC_OSDTIME, WM_SETTEXT, (WPARAM) 0, (LPARAM)buf);
      SendDlgItemMessage(hwndDlg, IDC_OSDTIME, EM_SETLIMITTEXT, (WPARAM) 3, (LPARAM)0);

      OSDControlsEnabled(OSDEnabled);

      wndproc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_EDIT), GWL_WNDPROC, (LONG)EdProc);
      SetWindowLong(GetDlgItem(hwndDlg, IDC_EDIT), GWL_USERDATA, (LONG)wndproc);
      wndproc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_OSDTIME), GWL_WNDPROC, (LONG)EdOSDTimeProc);
      SetWindowLong(GetDlgItem(hwndDlg, IDC_OSDTIME), GWL_USERDATA, (LONG)wndproc);
      AtualizarDlg();
      return TRUE;

    case WM_CLOSE:
      EndDialog( hwndDlg, TRUE );
      return TRUE;

    case WM_COMMAND:
      switch (HIWORD(wParam)) {

        case CBN_SELENDOK:
          AtualizarDlg();
          break;

        case BN_CLICKED:
          switch (LOWORD(wParam)) {

            case 2:
              EndDialog(hwndDlg,TRUE);
              break;

            case IDC_ENABLED:
              cfghk[GetHKIdx()].enabled = (SendDlgItemMessage(hwndDlg, IDC_ENABLED, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED);
              break;

            case IDC_OSD:
              OSDControlsEnabled((SendDlgItemMessage(hwndDlg, IDC_OSD, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED));
              break;

            case IDC_ABOUT:
              MessageBox(hwndDlg,"Flamekeys plug-in v2.01\n"
                                 "Copyright © 2001-2002 Daniel Simoes.\n\n"
                                 "Valid hotkeys rules:\n"
                                 "1 - You must combine modifiers (Control, Alt, Shift, Winkey) with any another key;\n"
                                 "2 - Shift modifier can only be used with other modifier;\n"
                                 "3 - Function keys (F1, F2, etc.) can be used without modifiers or with Shift only;\n"
                                 "4 - Enter, Tab, Esc, Caps Lock, Num Lock, Print Screen, Pause/Break cannot be used;       \n\n"
                                 "Credits:\n"
                                 "   Programming: Daniel Simoes (ddsimoes@gmx.net)\n"
                                 "   Testing: Danilo Batista (dansouza@gmx.net)\n"
                                 "   English advice: Marcelo Bianchini (midnightsun@bol.com.br)\n\n"
                                 "For updates go to:\n"
                                 "   http://www.flamekeys.unsecurity.com.br\n\n"
                                 "Comments, sugestions:\n"
                                 "   ddsimoes@gmx.net"
                                 ,"Help",MB_OK+MB_ICONINFORMATION);
              break;

            case IDC_CANCEL:
              EndDialog( hwndDlg, TRUE );
              break;

            case IDC_OK:
              #define GetCheckState(cb,x) x = (SendDlgItemMessage(hwndDlg, (cb), BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
              for (i=0;i<NUM_HK;i++) hotkeys[i].hk = cfghk[i];
              GetCheckState(IDC_PLUGENABLED, PluginEnabled);
              GetCheckState(IDC_HKWARN, HKWarnEnabled);
              GetCheckState(IDC_OSD, OSDEnabled);
              GetCheckState(IDC_SMFS, SMFullscreen);
              GetCheckState(IDC_SMTM, SMTopmost);
              if (WIN2K)
                GetCheckState(IDC_FADEOUT, FadeoutEnabled);
              else
                FadeoutEnabled=FALSE;

              SendDlgItemMessage(hwndDlg, IDC_OSDTIME, WM_GETTEXT, (WPARAM)4, (LPARAM)(LPCTSTR)buf);
              OSDTime=ToNumber(buf);

              EndDialog( hwndDlg, TRUE );
              break;

            case IDC_UNINSTALL:
              CallUninstall();
              break;

          } //switch (LOWORD(wParam))
          break;
      } //switch (HIWORD(wParam))
      return TRUE;
  } //switch(uMsg)
  return FALSE;
}



/*
  SetHotKey()

  Define a hotkey escolhida na janela de configuracao.
  Nao define a hotkey em si!
*/
void SetHotKey(int i, int vk) {

  if ((vk == VK_BACK) && (!ModifiersPressed())) {
    cfghk[i].vk = hotkeys[i].hk.vk;
    cfghk[i].modcontrol = hotkeys[i].hk.modcontrol;
    cfghk[i].modshift = hotkeys[i].hk.modshift;
    cfghk[i].modalt = hotkeys[i].hk.modalt;
    cfghk[i].modwin =  hotkeys[i].hk.modwin;
    cfghk[i].enabled =  hotkeys[i].hk.enabled;
  }
  else if ((vk == VK_DELETE) && (!ModifiersPressed())) {
    cfghk[i].vk = 0;
    cfghk[i].modcontrol = 0;
    cfghk[i].modshift = 0;
    cfghk[i].modalt = 0;
    cfghk[i].modwin = 0;
    cfghk[i].enabled = 0;
  }
  else {
    cfghk[i].vk = (UINT)LOBYTE(vk);
    cfghk[i].modcontrol = VerTecla(VK_CONTROL);
    cfghk[i].modshift = VerTecla(VK_SHIFT);
    cfghk[i].modalt = VerTecla(VK_MENU);
    cfghk[i].modwin =  ((VerTecla(VK_LWIN)) || VerTecla(VK_RWIN));
    cfghk[i].enabled = KeyValid2(i);
  }
}



/*
  EdProc()

  Recebe e responde as mensagens enviadas ao editor de hotkey.
*/
LRESULT CALLBACK EdProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

  switch(uMsg) {

    case WM_CHAR:
      return 0L;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
      switch((UINT)wParam) {

        case VK_TAB:
        case VK_ESCAPE:
        case VK_RETURN:
        case VK_CONTROL:
        case VK_SHIFT:
        case VK_MENU:
        case VK_LWIN:
        case VK_RWIN:
        case VK_CAPITAL:
        case VK_NUMLOCK:
        case VK_SNAPSHOT:
        case VK_PAUSE:
          break;

        case VK_DOWN:
          if (!ModifiersPressed())
            if (GetHKIdx() < NUM_HK) {
              SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_SETCURSEL, (WPARAM)(GetHKIdx()+1), (LPARAM)0);
              break;
          }

        case VK_UP:
          if (!ModifiersPressed())
            if (GetHKIdx() > 0) {
              SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_SETCURSEL, (WPARAM)(GetHKIdx()-1), (LPARAM)0);
              break;
          }

        case VK_LEFT:
        case VK_RIGHT:
          if (!ModifiersPressed())
            break;

        default:
          if (lParam >> 30) return 0;  // Verifica se eh auto-repeticao
          SetHotKey(GetHKIdx(),(int)wParam);
          AtualizarDlg();
          return 0L;

      }
      AtualizarDlg();
      return 0L;

  }
  return CallWindowProc((WNDPROC)GetWindowLong(hwnd, GWL_USERDATA), hwnd, uMsg, wParam, lParam);
}


#define xTam 270
#define yTam 120
int timecount;
BOOL onscreen;
char actionstr[50];
int scrollpos=0;

LRESULT WINAPI OSDWndProc(HWND hwnd, UINT message,
                       WPARAM wParam, LPARAM lParam) {

  HDC hdc;
  PAINTSTRUCT ps;
  RECT rect;
  static HFONT hfntLogo,hfntNormal,hfntTime,hfntFunction,hfntOld;
  char meubuf[512];
  int buftam;
  char *tmpstr;
  int tmp_int3;
  int tmp_int1, tmp_int2;
  static HPEN caneta,oldPen;
  LOGBRUSH brocha;
  POINT pontos[5];
  SIZE dimtitle;
  int i;
  DWORD oldTick,newTick,preFX;

  switch (message) {

    case WM_TIMER:
      if (wParam==1) {
        if (++timecount >= OSDTime)
          PostMessage(hwnd,WM_CLOSE,0,0);
        scrollpos += 4;   //4
        rect.left=3; rect.right=xTam-3;
        rect.top=44;rect.bottom=79;
        InvalidateRect(hwnd,&rect,FALSE);
        UpdateWindow(hwnd);
        return 0L;
      }

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
      PostMessage(hwnd,WM_CLOSE,0,0);
      return 0L;

    case WM_PAINT:
      hdc = BeginPaint(hwnd, &ps);
      SetMapMode(hdc, MM_ISOTROPIC);
      SetWindowExtEx(hdc,xTam,yTam,NULL);
      SetViewportExtEx(hdc,xTam,yTam,NULL);
      SetViewportOrgEx(hdc,0,0,NULL);

      SetBkColor(hdc,0x000000); //Preto

      DrawIcon(hdc, 14, 8, icologo);

      // Titulo
      hfntOld=SelectObject(hdc,hfntLogo);
      SetBkMode(hdc,TRANSPARENT);
      SetTextColor(hdc,0x00FFFFFF);
      TextOut(hdc,62,5,"Flamekeys OSD",13);
    /*  SetTextColor(hdc,0x00000000);
      TextOut(hdc,63,6,"Flamekeys OSD",13);
      SetTextColor(hdc,0x00FFFFFF);
      TextOut(hdc,64,7,"Flamekeys OSD",13); */
      SetBkMode(hdc,OPAQUE);


      // Nome da musica
      SelectObject(hdc,hfntNormal);
      SetTextColor(hdc,0xFFFFFF);
      rect.left=95; rect.right=xTam-8;
      rect.top=45;rect.bottom=59;

      if (SendMessage(WinampWnd,WM_WA_IPC,0,IPC_ISPLAYING) == 0) {
        ExtTextOut(hdc,95,45,ETO_OPAQUE|ETO_CLIPPED,&rect,"[not playing]",13,NULL);
      }
      else {
        tmp_int3=(int)SendMessage(WinampWnd,WM_WA_IPC,0,IPC_GETLISTPOS);
        tmpstr=(char *)SendMessage(WinampWnd,WM_WA_IPC,tmp_int3,IPC_GETPLAYLISTTITLE);
        tmp_int1=(int)SendMessage(WinampWnd,WM_WA_IPC,1,IPC_GETOUTPUTTIME); // segundos
        wsprintf(meubuf,"%d. %s (%d:%02d)",tmp_int3+1,tmpstr,tmp_int1/60,tmp_int1%60);
        GetTextExtentPoint32(hdc, meubuf,lstrlen(meubuf),&dimtitle);
        if ( dimtitle.cx > (rect.right-rect.left) ) {
          lstrcat(meubuf,"  ***  "),
          GetTextExtentPoint32(hdc, meubuf,lstrlen(meubuf),&dimtitle);
          if (scrollpos > dimtitle.cx)
            scrollpos=(scrollpos-dimtitle.cx);
          lstrcat(meubuf,meubuf);
          ExtTextOut(hdc,96-scrollpos,45,ETO_OPAQUE|ETO_CLIPPED,&rect,meubuf,lstrlen(meubuf),NULL);
        }
        else
          ExtTextOut(hdc,96,45,ETO_OPAQUE|ETO_CLIPPED,&rect,meubuf,lstrlen(meubuf),NULL);

        // Info
        tmp_int1=(int)SendMessage(WinampWnd,WM_WA_IPC,0,IPC_GETINFO);
        tmp_int2=(int)SendMessage(WinampWnd,WM_WA_IPC,1,IPC_GETINFO);
        tmp_int3=(int)SendMessage(WinampWnd,WM_WA_IPC,2,IPC_GETINFO);
        wsprintf(meubuf,"%dkbps / %dkHz / %s",tmp_int2,tmp_int1,(tmp_int3 == 1) ? "Mono" : "Stereo");
        SetTextColor(hdc,0x808080); // Cinza
        rect.left=95; rect.right=xTam-4;
        rect.top=61;rect.bottom=75;
        ExtTextOut(hdc,96,61,ETO_OPAQUE|ETO_CLIPPED,&rect,meubuf,lstrlen(meubuf),NULL);
      }

      // Repeat and Shuffle
      tmp_int1=(int)SendMessage(WinampWnd,WM_WA_IPC,0,IPC_GETSHUFFLE);
      tmp_int2=(int)SendMessage(WinampWnd,WM_WA_IPC,0,IPC_GETREPEAT);

      SetTextColor(hdc,0x808080); // Cinza
      if (tmp_int1 == 1)
        SetTextColor(hdc,0xFFFFFF);
      TextOut(hdc,12,76,"Shuffle",7);

      SetTextColor(hdc,0x808080); // Cinza
      if (tmp_int2 == 1)
        SetTextColor(hdc,0xFFFFFF);
      TextOut(hdc,55,76,"Repeat",6);

      // Tempo decorrido
      SelectObject(hdc,hfntTime);
      SetTextColor(hdc,0x0000FF); // Vermelho
      tmp_int2=((int)SendMessage(WinampWnd,WM_WA_IPC,0,IPC_GETOUTPUTTIME))/1000;
      wsprintf(meubuf,"%02d:%02d",tmp_int2/60,tmp_int2%60);
      //TextOut(hdc,18,46,meubuf,lstrlen(meubuf));
      rect.left=18; rect.right=90;
      rect.top=46; rect.bottom=70;
      ExtTextOut(hdc,18,46,ETO_OPAQUE|ETO_CLIPPED,&rect,meubuf,lstrlen(meubuf),NULL);

      // Acao executada
      SelectObject(hdc,hfntFunction);
      SetBkColor(hdc,0x808080);  // Cinza
      SetTextColor(hdc,RGB(255,255,255)); //Branco
      rect.left=1; rect.right=xTam-1;
      rect.top=92;rect.bottom=yTam-1;
      SetTextAlign(hdc, TA_CENTER|VTA_CENTER);
      ExtTextOut(hdc,xTam/2,rect.top,ETO_OPAQUE|ETO_CLIPPED,&rect,actionstr,lstrlen(actionstr),NULL);

      SelectObject(hdc,hfntOld);

      oldPen=SelectObject(hdc,caneta);
      pontos[0].x=1;
      pontos[0].y=yTam-2;
      pontos[1].x=1;
      pontos[1].y=1;
      pontos[2].x=xTam-2;
      pontos[2].y=1;
      pontos[3].x=xTam-2;
      pontos[3].y=yTam-2;
      pontos[4].x=1;
      pontos[4].y=yTam-2;
      Polyline(hdc,pontos,5);
      SelectObject(hdc,oldPen);

      ValidateRect(hwnd,NULL);
      EndPaint(hwnd,&ps);
      return 0;

    case WM_CREATE:
      hfntLogo=CreateFont(30,13,0,0,FW_BOLD,FALSE,FALSE,FALSE,OEM_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, VARIABLE_PITCH | FF_DONTCARE,
                          "MS Sans Serif");
      hfntNormal=CreateFont(14,0,0,0,FW_NORMAL+50,FALSE,FALSE,FALSE,OEM_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, VARIABLE_PITCH | FF_DONTCARE,
                          "Arial");
      hfntTime=CreateFont(29,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,OEM_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE,
                          "Arial");
      hfntFunction=CreateFont(27,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,OEM_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, VARIABLE_PITCH | FF_DONTCARE,
                          "Courier New");
      brocha.lbStyle = BS_SOLID;
      brocha.lbHatch = NULL;
      brocha.lbColor = 0xFFFFFF;
      caneta=ExtCreatePen(PS_SOLID,1,&brocha,0,NULL);
      SetTimer(hwnd,1,100,(TIMERPROC)NULL);
      return 0;

    case WM_CLOSE:
      KillTimer(hwnd,1);

      /* Window fadeout efect */
      if (FadeoutEnabled) {
        preFX=GetTickCount();
        for (i=255; i>=0; i-=8) {
          oldTick=GetTickCount();
          if ((oldTick - preFX) > 1000) break; // Limite maximo
          SetTransp(hwnd, 0, (BYTE)i, LWA_ALPHA);
          for(;(GetTickCount()-oldTick) < 15;); // Limite minimo
        }
      }
      DestroyWindow(hwnd);
      onscreen=FALSE;  //Flag sinalizadora do status do osd
      return 1;

    case WM_DESTROY:
      DeleteObject(hfntLogo);
      DeleteObject(hfntNormal);
      DeleteObject(hfntTime);
      DeleteObject(hfntFunction);
      DeleteObject(caneta);
      break;

  }
  return DefWindowProc(hwnd,message,wParam,lParam);
}



void OSDShow(HINSTANCE hInstance, char *Action) {

  static char szAppName[] = "FlamekeysOSD";
  static HWND hwnd;
  WNDCLASSEX wndclass;
  RECT rect;
  DWORD ExStyle=WS_EX_TOOLWINDOW  | WS_EX_TOPMOST;
  DWORD fgExStyle;
  RECT fgRect, tamTela;

  if (!GetClassInfoEx(hInstance,szAppName,&wndclass)) {
    wndclass.cbSize        = sizeof(wndclass);
    wndclass.hIconSm       = NULL;
    wndclass.lpszClassName = szAppName;
    wndclass.hInstance = hInstance;
    wndclass.style = 0;
    wndclass.lpfnWndProc = OSDWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hIcon = NULL;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName = NULL;
    RegisterClassEx(&wndclass);
  }

  lstrcpy(actionstr,Action);
  timecount=0;

  if (!onscreen) {

    if (SMTopmost) {
      fgExStyle=GetWindowLong(GetForegroundWindow(),GWL_EXSTYLE);
      if ((fgExStyle & WS_EX_TOPMOST) == WS_EX_TOPMOST)
        ExStyle=WS_EX_TOOLWINDOW;
    }
    if (SMFullscreen) {
      if (GetForegroundWindow() != FindWindow("Progman",NULL)) {
        GetWindowRect(GetDesktopWindow(),&tamTela);
        GetClientRect(GetForegroundWindow(),&fgRect);
        if ((fgRect.right >= tamTela.right) && (fgRect.bottom >= tamTela.bottom))
          return;
      }
    }
    if (FadeoutEnabled)
      ExStyle=ExStyle | WS_EX_LAYERED;

    GetWindowRect(GetDesktopWindow(),&rect);
    hwnd = CreateWindowEx(ExStyle,
                         szAppName,
                         "",
                         WS_POPUP,
                         ((rect.right-xTam)/2),
                         (rect.bottom-yTam)/2,
                         xTam,
                         yTam,
                         GetDesktopWindow(),
                         NULL,
                         hInstance,
                         NULL);


    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    if (FadeoutEnabled)
      SetTransp(hwnd, 0, 255, LWA_ALPHA);
    UpdateWindow(hwnd);
    onscreen=TRUE;
  }
  else {
    rect.left=3; rect.right=xTam-3;
    rect.top=44;rect.bottom=yTam-3;
    if (FadeoutEnabled)
      SetTransp(hwnd, 0, 255, LWA_ALPHA);
    InvalidateRect(hwnd,&rect,FALSE);
    UpdateWindow(hwnd);
  }
  return;
}

int init2() {
  return 0;
}

void foofun() {
  return;
}

