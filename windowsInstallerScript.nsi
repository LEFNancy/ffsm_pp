; Pythia Windows Instal Script
; Writen to be compiled with NSIS
; Antonello Lobianco, 2012 

;--------------------------------
; General
	; Things that will likelly need to be changed...
	!define VERSIONSTRING "0.9.5"               ; Pythiaversion number (as string). Used to put each version on a separate folder.

	OutFile "pythia_${VERSIONSTRING}_setup.exe" ; Filename of the outputted installer
	!define MINGWDIR "C:\MinGW\bin" ; Directory where thre MinGW DLL is located
	!define QTDIR "C:\Qt\4.8.2\bin"   ; Directory where the Qt run-time DLLs are located
	!define EXEDIR "."                ; Directory where the EXE file is located

	!include "MUI.nsh" ; Include Modern UI
	Name "Pythia" ; Name (?)
	InstallDir "$PROGRAMFILES\Pythia\${VERSIONSTRING}" ; Default installation directory
	!define APP_NAME "Pythia" ; Application name (mainly for links)
	!define APP_FNAME "Pythia ${VERSIONSTRING}" ; Application name with version(mainly for links)
	!define MAIN_APP_EXE "pythia.exe" ; Filename of executable
	!define WEB_SITE "http://www.nancy.inra.fr/lef_eng" ; We-site address
	Var OPTIONALDATA ; We'll use this variable to put links on sample data only if users have selected to intall sample data
	Var LOCALDOC ;If user has selected to install documentation
	Var SM_Folder ; Application shortcuts main folder
	SetCompressor ZLIB ; Compression used
	XPStyle on ; Style of the Wizard (look it doesn't change anything)

;--------------------------------
; Interface Settings ( we define all settings before building the pages - next points)

	!define MUI_ABORTWARNING ; We give a confirmation warning before let the user abort the installation proces 
	!define MUI_ICON "src\imgs\regmas.ico" ; icon for the installer
	!define MUI_UNICON "src\imgs\regmas.ico" ; icon for the uninstaller 
	!define MUI_WELCOMEFINISHPAGE_BITMAP "src\imgs\beech.bmp" ; image for the welcome and finish page 
	!define MUI_UNWELCOMEFINISHPAGE_BITMAP "src\imgs\beech.bmp" ; image for the welcome and finish page (uninstaller)
	!define MUI_COMPONENTSPAGE_SMALLDESC ; section description on the bottom instead on the default right
	!define REG_START_MENU "Start Menu Folder" ; ??
	!define MUI_STARTMENUPAGE_DEFAULTFOLDER "Pythia\${VERSIONSTRING}" ; Default folder where to prompt the user to place links
	!define MUI_FINISHPAGE_RUN "$INSTDIR\${MAIN_APP_EXE}" ; What propose the user to do after the installation is completed
	!define MUI_WELCOMEPAGE_TEXT "Version ${VERSIONSTRING}\n\n Pythia is a flexible, spatially explicit, coupled resource and economic simulator of the Forest Sector, designed for long-term simulations of effects of government policies over different forest systems.\n\n This Wizard will guide you the installation of Pythia. \n\n Press Next to start the installation."
   
;--------------------------------
; Installer pages (steps)

	!insertmacro MUI_PAGE_WELCOME ; Welcome page
	!insertmacro MUI_PAGE_LICENSE "COPYING" ; Accept licence page
	!insertmacro MUI_PAGE_COMPONENTS ; Choose installation components (pieces)
	!insertmacro MUI_PAGE_DIRECTORY ; Directory where to install
	!insertmacro MUI_PAGE_STARTMENU Application $SM_Folder ; Write links (and where)
	!insertmacro MUI_PAGE_INSTFILES ; Installing the files
	!insertmacro MUI_PAGE_FINISH ; "Done!" page
	!insertmacro MUI_UNPAGE_CONFIRM ; Confirmation request before uninstalling
	!insertmacro MUI_UNPAGE_INSTFILES ; Uninstalling files
  
;--------------------------------
; Languages
 
	!insertmacro MUI_LANGUAGE "English" ; ??

;--------------------------------
; Installer Sections (components that user can choose if install it or not)

Section "Main program" MainProgram
	SectionIn RO  ; Read only - the user can not delesect it !
	SetOutPath "$INSTDIR" ; Where files need to be installed
	File "${EXEDIR}\pythia.exe" ; Adding this file or directory to the list of files to be installed
	File "${MINGWDIR}\mingwm10.dll"
	File "${MINGWDIR}\pthreadGC2.dll"
	File "${MINGWDIR}\libgcc_s_dw2-1.dll"
	File "${MINGWDIR}\libgfortran-3.dll"
	File "${MINGWDIR}\libquadmath-0.dll"
	File "${MINGWDIR}\libstdc++-6.dll"
	File "src\win32\lib\libadolc-1.dll"
	File "AUTHORS" 
	File "COPYING" 
	File "NEWS" 
	File "README" 
	WriteUninstaller "$INSTDIR\Uninstall_pythia.exe" ; Creating the uninstaller
SectionEnd

Section "Run-time libraries" Qt
	SetOutPath "$INSTDIR" 
	File "${QTDIR}\QtCore4.dll"
	File "${QTDIR}\QtGui4.dll"  
	File "${QTDIR}\QtXml4.dll" 
SectionEnd

Section "Sample data" Data
	SetOutPath "$INSTDIR\data" 
	File /r "data\*" ; Adding this file or directory to the list of files to be installed. /r means "recursively"
	StrCpy $OPTIONALDATA "true" ; Saving the fact the user has chosen to install the sample data so later on me make the links 
SectionEnd

Section /o "Source" Src   ; option /o means optional - unselected by default
	SetOutPath "$INSTDIR\src" 
	File "src\*.h" 
	File "src\*.cpp" 
SectionEnd

Section /o "Documentation" Doc   ;
	;not yet ready...
	;SetOutPath "$INSTDIR\doc\userManual"
	;File "doc\userManual\pythiaUserManual.pdf"
	;SetOutPath "$INSTDIR\doc\referenceManual\html"
	;File "doc\referenceManual\html\*"
	StrCpy $LOCALDOC "true" 
SectionEnd

;--------------------------------
; Component Descriptions

	; Creating "Language strings" objects for each section...
	LangString DESC_MainProgram ${LANG_ENGLISH} "Main Pythia files"
	LangString DESC_Qt ${LANG_ENGLISH} "Run-time graphical libraries. Unselect this section only if you already have Qt >=4.8 installed on your PC"
	LangString DESC_Data ${LANG_ENGLISH} "Sample input data (recommended)"
	LangString DESC_Src ${LANG_ENGLISH} "Pythia C++ source code (not needed to run Pythia)"
	LangString DESC_Doc ${LANG_ENGLISH} "Local copy of the documentation (NOT YET READY)"

	;Assign "Language strings" objects to sections
	!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
		!insertmacro MUI_DESCRIPTION_TEXT ${MainProgram} $(DESC_MainProgram)
		!insertmacro MUI_DESCRIPTION_TEXT ${Qt} $(DESC_Qt)
		!insertmacro MUI_DESCRIPTION_TEXT ${Data} $(DESC_Data)
		!insertmacro MUI_DESCRIPTION_TEXT ${Src} $(DESC_Src)
		!insertmacro MUI_DESCRIPTION_TEXT ${Doc} $(DESC_Doc)
	!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Links & icons
	
Section -Icons_Reg
	SetOutPath "$INSTDIR" ; Where files need to be installed by default

	!ifdef REG_START_MENU ; If the user has chosen to make links
		!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
			; Create directory for the shrtcuts ($M_Folder has been chosen by the user)
			CreateDirectory "$SMPROGRAMS\$SM_Folder"
			; Shortcut to the main program
			CreateShortCut "$SMPROGRAMS\$SM_Folder\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
			; Desktop shortcut to the main program
			CreateShortCut "$DESKTOP\${APP_FNAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
			; Shortcut to the uninstaller
			CreateShortCut "$SMPROGRAMS\$SM_Folder\Uninstall ${APP_NAME}.lnk" "$INSTDIR\Uninstall_pythia.exe"
			${If} $OPTIONALDATA == 'true' ; If user has installed the sample data
				; Shortcut to data file
				CreateShortCut "$SMPROGRAMS\$SM_Folder\Edit sample data.lnk" "$INSTDIR\data\pythiaInput.ods"
			${EndIf}
			${If} $LOCALDOC == 'true' ; If user has installed a local copy of the documentation
				; Shortcut to user manual file
				CreateShortCut "$SMPROGRAMS\$SM_Folder\User Manual.lnk" "$INSTDIR\doc\userManual\pythiaUserManual.pdf"
				; Shortcut to the reference manual
				CreateShortCut "$SMPROGRAMS\$SM_Folder\Reference Manual.lnk" "$INSTDIR\doc\referenceManual\html\index.html"
			${EndIf}
			; Write internet shortcut to the main web-site and link it from the other START MENU shortcuts..
			WriteIniStr "$INSTDIR\${APP_NAME} website.url" "InternetShortcut" "URL" "${WEB_SITE}"
			CreateShortCut "$SMPROGRAMS\$SM_Folder\${APP_NAME} Website.lnk" "$INSTDIR\${APP_NAME} website.url"
			; Write internet shortcut to the on-line documentation and link it from the other START MENU shortcuts..
			WriteIniStr "$INSTDIR\${APP_NAME} documentation.url" "InternetShortcut" "URL" "${WEB_SITE}/doc"
			CreateShortCut "$SMPROGRAMS\$SM_Folder\${APP_NAME} Documentation.lnk" "$INSTDIR\${APP_NAME} documentation.url"
		!insertmacro MUI_STARTMENU_WRITE_END
	!endif
SectionEnd  
  
;--------------------------------
;Uninstaller Section

Section "Uninstall"
	RMDir /r "$INSTDIR" ; Recursivelly remove all files in the install directory
	RMDir /r "${SMPROGRAMS}\${SM_Folder}" ; Remove links in the Start Menu ..doesn't works !!!
	Delete "$DESKTOP\${APP_FNAME}.lnk" ; Remove desktop link
	;DeleteRegKey /ifempty HKCU "Software\Pythia"
SectionEnd

