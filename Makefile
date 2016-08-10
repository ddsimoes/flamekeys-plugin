COMP=d:\borland\bcc55\bin\bcc32              # Borland C++ compiler
RCOMP=d:\borland\bcc55\bin\brcc32            # Borland C++ resource compiler
LINKER=d:\borland\bcc55\bin\ilink32          # Borland C++ linker

!ifdef FINAL
CFLAGS=-c -tWD -4 -d -q -w-par -f- -w-8004 -O1 -u-
!else
CFLAGS=-c -tWD -4 -d -q -w-par -f- -w-8004 -O1 -u- -DDEBUGMODE -v- -x-
!endif

RFLAGS=-dRELEASE -id:\borland\bcc55\include
LFLAGS=-Tpd -x -C -Gn -q -c -v- -Gz
ARQIN=gen_fkeys
ARQOUT=fkeys
#ARQFINAL=c:\progra~1\winamp\plugins\gen_fkeys
ARQFINAL=d:\arquiv~1\winamp\plugins\gen_fkeys
LIBS=kernel32.lib user32.lib advapi32.lib gdi32.lib

!ifdef FINAL
Flamekeys.exe: $(ARQFINAL).dll
        c:\progra~1\NSIS\makensis gen_fkeys.nsi
!endif

$(ARQFINAL).dll: $(ARQOUT).dll
        @IF EXIST $(ARQFINAL).dll  del $(ARQFINAL).dll
        @IF EXIST $(ARQOUT).pck  del $(ARQOUT).pck
        upx -9 -o $(ARQOUT).pck $(ARQOUT).dll
        copy $(ARQOUT).pck $(ARQFINAL).dll

$(ARQOUT).dll: $(ARQIN).obj $(ARQIN).res
        $(LINKER) $(LFLAGS) $(ARQIN).obj,$(ARQOUT).dll,,$(LIBS),,$(ARQIN).res

$(ARQIN).res: $(ARQIN).rc
        $(RCOMP) $(RFLAGS) -fo$(ARQIN).res $(ARQIN).rc

$(ARQIN).obj: $(ARQIN).c gen.h
        $(COMP) $(CFLAGS) $(ARQIN).c


clean:
        @IF EXIST $(ARQIN).obj     DEL $(ARQIN).obj
        @IF EXIST $(ARQIN).res     DEL $(ARQIN).res
        @IF EXIST $(ARQOUT).pck    DEL $(ARQOUT).pck
        @IF EXIST $(ARQIN)11.exe   DEL $(ARQIN)11.exe


