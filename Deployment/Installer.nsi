!include "MUI.nsh"
!include "WinMessages.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"

# Define consts
!define PRODUCT_FULLNAME "KakaoTalk AdGuard"
!define PRODUCT_NAME "KakaoTalkAdGuard"
!define PRODUCT_COMMENTS "카카오톡 윈도 버전용 광고 제거 프로그램"
!define PRODUCT_VERSION "1.0.0.15"
!define BUILD_ARCH "x64"
!define PRODUCT_PUBLISHER "loopback.kr / KnDol Soft"
!define PRODUCT_REG_ROOTKEY "HKCU"
!define PRODUCT_DIR_REGKEY "Software\${PRODUCT_NAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define MUI_ICON "res/install.ico"
!define MUI_UNICON "res/uninstall.ico"

# Pages
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PRODUCT_NAME}.exe"
!define MUI_FINISHPAGE_RUN_PARAMETERS "--startup"
!define MUI_FINISHPAGE_RUN_TEXT "Run ${PRODUCT_FULLNAME}"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "Korean"

OutFile "${PRODUCT_NAME}_${PRODUCT_VERSION}.Setup.exe"
InstallDirRegKey HKCU "SOFTWARE\${PRODUCT_NAME}" "InstallPath"
InstallDir "$APPDATA\${PRODUCT_NAME}"
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
BrandingText /TRIMRIGHT "${PRODUCT_NAME}"
RequestExecutionLevel user
ShowInstDetails show
ShowUnInstDetails show

VIProductVersion "${PRODUCT_VERSION}"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "FileDescription" "${PRODUCT_FULLNAME} Setup"
VIAddVersionKey "ProductName" "${PRODUCT_FULLNAME}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
# VIAddVersionKey "LegalTrademarks" "Test Application is a trademark of Fake company"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2024 loopback.kr / 2025 KnDol Soft"
# VIAddVersionKey "OriginalFilename" "${PRODUCT_NAME} ${PRODUCT_VERSION}.exe"

Function .onInit
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    SendMessage $0 ${WM_CLOSE} 0 0
    ; MessageBox MB_OK|MB_ICONEXCLAMATION "${PRODUCT_FULLNAME} is running. Please close it first." /SD IDOK
    ; Abort
    notRunning:
FunctionEnd

Section "Installer Section"
    SetOutPath $INSTDIR
    InitPluginsDir
    ${If} ${RunningX64}
        File /oname=${PRODUCT_NAME}.exe "..\Release\x64\${PRODUCT_NAME}.x64.exe"
        File "/oname=$PLUGINSDIR\vc_redist.x64.exe" "..\Redist\vc_redist.x64.exe"
    ${Else}
        File /oname=${PRODUCT_NAME}.exe "..\Release\win32\${PRODUCT_NAME}.x86.exe"
        File "/oname=$PLUGINSDIR\vc_redist.x86.exe" "..\Redist\vc_redist.x86.exe"
    ${EndIf}

    ; File "RestoreTrayIcon.exe"
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_FULLNAME}.lnk" "$INSTDIR\${PRODUCT_NAME}.exe" "--startup"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\트레이 아이콘 복원.lnk" "$INSTDIR\${PRODUCT_NAME}.exe" "--restore_tray"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\설치 제거.lnk" "$INSTDIR\Uninstall.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_FULLNAME}"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$\"$INSTDIR\${PRODUCT_NAME}.exe$\""
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "Comments" "${PRODUCT_COMMENTS}"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
    # WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "Contact" "mailto:hyunseoki@outlook.kr"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "HelpLink" "https://github.com/kndol/KakaoTalkAdGuard/issues"
    # WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "https://github.com/kndol/KakaoTalkAdGuard/issues"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "URLUpdateInfo" "https://github.com/kndol/KakaoTalkAdGuard#release-notes"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegDWORD ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd

;설치가 완료되기 전에 호출되는 Section.
Section -InnoSetup
    ;CheckVCRedist2022 Function을 호출.
    Call CheckVCRedist2022
    Pop $R9 ;R9 변수를 꺼낸다.
    ${If} $R9 == "No"
        ${If} ${RunningX64}
            ExecWait "$PLUGINSDIR\vc_redist.x64.exe"
        ${Else}
            ExecWait "$PLUGINSDIR\vc_redist.x86.exe"
        ${EndIf}
    ${EndIf}
SectionEnd

Function CheckVCRedist2022
    Push $R9
    ClearErrors
    ;64bit OS인 경우.
    SetRegView 64
    ${If} ${RunningX64}
        ;Registry Version을 읽는다.
        ReadRegStr $R9 HKLM "SOFTWARE\Classes\Installer\Dependencies\VC,redist.x64,amd64,14.44,bundle" "Version"
        ; if VS 2022 redist not installed, install it
        IfErrors 0 VSRedistInstalled
        ;Registry Version을 읽는다.
    ${Else}
        ReadRegStr $R9 HKLM "SOFTWARE\Classes\Installer\Dependencies\VC,redist.x86,x86,14.44,bundle" "Version"
        ; if VS 2022 redist not installed, install it
        IfErrors 0 VSRedistInstalled
    ${EndIf}
    StrCpy $R9 "No" ;Registry가 없다면 R9 변수에 No를 복사.
VSRedistInstalled: ;Registry가 있다면 R9 변수에 설치된 버전을 복사.
    Exch $R9
FunctionEnd

Section Uninstall
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    SendMessage $0 ${WM_CLOSE} 0 0
    notRunning:

    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$SMPROGRAMS\${PRODUCT_NAME}"
    RMDir /r "$INSTDIR"
    DeleteRegKey ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey ${PRODUCT_REG_ROOTKEY} "${PRODUCT_DIR_REGKEY}"
    DeleteRegValue ${PRODUCT_REG_ROOTKEY} "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\" "${PRODUCT_NAME}"
    SetAutoClose true
SectionEnd
