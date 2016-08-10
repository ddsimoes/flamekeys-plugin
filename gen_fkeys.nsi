; Install Script

!packhdr gentmp.dat "upx -9 gentmp.dat"

CRCCheck on
SetDatablockOptimize on

Name "Flamekeys Plug-in V2.01"
OutFile "flamekeys21.exe"
InstallDir $PROGRAMFILES\Winamp
DirText "This will install Flamekeys V2.01 plug-in on your computer. Select your Winamp directory."

Section "bla"
 Inicio:
 ProcuraVelho:
  IfFileExists "$INSTDIR\Plugins\gen_hkamp.dll" AchouVelho ProcuraNovo
 ProcuraNovo:
  IfFileExists "$INSTDIR\Plugins\gen_fkeys.dll" AchouNovo Continuar2
 AchouVelho:
  MessageBox MB_YESNO|MB_ICONQUESTION "Found a old incompatible version of the plugin on your machine.$\nYou have to uninstall it in order to install the new one.$\n$\nUninstall old plugin?" IDNO NoInstall
  FindWindow $0 "Winamp v1.x"
  IntCmp $0 0 Continuar1
  MessageBox MB_YESNO|MB_ICONQUESTION "Winamp should be closed to continue. Close Winamp?" IDNO NoInstall
  Call CloseWinamp
  Continuar1:
    Call RemoverVelho
    Goto Continuar2
 AchouNovo:
  FindWindow $0 "Winamp v1.x"
  IntCmp $0 0 Continuar2
  MessageBox MB_YESNO|MB_ICONQUESTION "Winamp should be closed to continue. Close Winamp?" IDNO NoInstall
  Call CloseWinamp

 Continuar2:
  SetOutPath $INSTDIR\Plugins
  File "d:\arquiv~1\winamp\plugins\gen_fkeys.dll"
  File "gen_fkeys.txt"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Flamekeys" "DisplayName" "Flamekeys plug-in (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Flamekeys" "UninstallString" '"$INSTDIR\UninstFkeys.exe"'
  ExecShell open "$INSTDIR\Plugins\gen_fkeys.txt" SW_SHOWMAXIMIZED
  SetAutoClose true

  FindWindow $0 "Winamp v1.x"
  IntCmp $0 0 OpenAnswer
    MessageBox MB_YESNO|MB_ICONQUESTION "Winamp should be reloaded in order to start plugin. Restart Winamp?" IDNO NoInstall
    Call CloseWinamp
    ExecShell open "$INSTDIR\Winamp.exe"
    Goto Continuar3
  OpenAnswer:
    MessageBox MB_YESNO|MB_ICONQUESTION "Flamekeys plug-in sucessfully instaled on you computer.$\n$\nRun Winamp?" IDNO Fim
    ExecShell open "$INSTDIR\Winamp.exe"
  Continuar3:
    Goto Fim
 NoInstall:
  MessageBox MB_YESNO|MB_ICONQUESTION "Abort install?" IDNO Inicio
  Abort "Aborted by user."
 Fim:
SectionEnd


Function CloseWinamp
  Push $0
  loop:
    FindWindow $0 "Winamp v1.x"
    IntCmp $0 0 done
     SendMessage $0 16 0 0
     Sleep 100
     Goto loop
  done:
  Pop $0
FunctionEnd


Function RemoverVelho
  Delete "$INSTDIR\Plugins\gen_hkamp.dll"
  Delete "$INSTDIR\Plugins\gen_hkamp.txt"
FunctionEnd

;=====================================================================================;
UninstallEXEName "UninstFkeys.exe"
UninstallText "This will uninstall the Flamekeys plug-in from your system."


Section Uninstall
  MessageBox MB_YESNO|MB_ICONQUESTION "Are you sure you want to remove Flamekeys plug-in?" IDNO NoUninstall
  FindWindow $0 "Winamp v1.x"
  IntCmp $0 0 Continuar
  MessageBox MB_YESNO|MB_ICONQUESTION "Winamp should be closed to uninstall the plugin. Close Winamp?" IDNO NoUninstall
  Call un.CloseWinamp
 Continuar:
  MessageBox MB_YESNO|MB_ICONQUESTION "Remove registry configuration data?" IDNO Arquivos
  DeleteRegKey HKCU "Software\Winamp\Flamekeys Plug-in"
 Arquivos:
  Delete "$INSTDIR\Plugins\gen_fkeys.dll"
  Delete "$INSTDIR\Plugins\gen_fkeys.txt"
  Delete "$INSTDIR\UninstFkeys.exe"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Flamekeys"
  Goto Fim
 NoUninstall:
  Abort "Aborted by user."
 Fim:
SectionEnd

Function un.CloseWinamp
  Push $0
  loop:
    FindWindow $0 "Winamp v1.x"
    IntCmp $0 0 done
     SendMessage $0 16 0 0
     Sleep 100
     Goto loop
  done:
  Pop $0
FunctionEnd
