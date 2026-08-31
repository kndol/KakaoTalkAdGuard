!include "MUI.nsh"
!include "WinMessages.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "nsDialogs.nsh"
!include "x64.nsh"

# Define consts
!define PRODUCT_FULLNAME "KakaoTalk AdGuard"
!define PRODUCT_NAME "KakaoTalkAdGuard"
!define PRODUCT_COMMENTS "카카오톡 윈도 버전용 광고 제거 프로그램"
!define PRODUCT_VERSION "1.0.1.0"
!define BUILD_ARCH "x64"
!define PRODUCT_PUBLISHER "loopback.kr / KnDol Soft"
!define PRODUCT_DIR_REGKEY "Software\${PRODUCT_NAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define MUI_ICON "res/install.ico"
!define MUI_UNICON "res/uninstall.ico"

# Variables
Var InstallMode     ; "AllUsers" 또는 "CurrentUser"
Var PrevMode        ; 기존 설치의 설치 유형("" 이면 기존 설치 없음)
Var PrevInstDir     ; 기존 설치 경로
Var PrevUninstExe   ; 기존 설치의 설치 제거 프로그램 경로
Var RemoveOld       ; 1 이면 기존 설치를 제거한 뒤 설치
Var LockMode        ; 1 이면 설치 유형을 변경할 수 없음
Var IsElevated      ; 1 이면 권한 상승을 위해 다시 실행된 인스턴스
Var IsAdmin         ; 1 이면 관리자 권한을 가지고 있음
Var UserName
Var Dialog
Var RadioAllUsers
Var RadioCurrentUser

# Pages
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApp
!define MUI_FINISHPAGE_RUN_TEXT "${PRODUCT_FULLNAME} 실행하기"

!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipPageIfElevated
!insertmacro MUI_PAGE_WELCOME
Page custom ModePageCreate ModePageLeave
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipPageIfElevated
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE DirectoryPageLeave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "Korean"

OutFile "${PRODUCT_NAME}_${PRODUCT_VERSION}.Setup.exe"
InstallDir "$APPDATA\${PRODUCT_NAME}"
Name "${PRODUCT_FULLNAME} ${PRODUCT_VERSION}"
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
VIAddVersionKey "LegalCopyright" "Copyright (C) 2025 loopback.kr / 2026 KnDol Soft"
# VIAddVersionKey "OriginalFilename" "${PRODUCT_NAME} ${PRODUCT_VERSION}.exe"

Function .onInit
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    SendMessage $0 ${WM_CLOSE} 0 0
    ; MessageBox MB_OK|MB_ICONEXCLAMATION "${PRODUCT_FULLNAME} is running. Please close it first." /SD IDOK
    ; Abort
    notRunning:

    ;64bit OS인 경우 64bit Registry View를 사용.
    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}

    StrCpy $RemoveOld 0
    StrCpy $LockMode 0
    StrCpy $IsElevated 0

    UserInfo::GetName
    Pop $UserName
    Call CheckAdmin
    Call DetectPrevInstall

    ;명령행 인수를 확인.
    ${GetParameters} $R0

    ClearErrors
    ${GetOptions} $R0 "/elevated" $R1
    ${IfNot} ${Errors}
        ;권한 상승을 위해 다시 실행된 인스턴스. 부모 인스턴스가 정한 설정을 그대로 사용.
        StrCpy $IsElevated 1
        StrCpy $InstallMode "CurrentUser"
        ClearErrors
        ${GetOptions} $R0 "/allusers" $R1
        ${IfNot} ${Errors}
            StrCpy $InstallMode "AllUsers"
        ${EndIf}
        ClearErrors
        ${GetOptions} $R0 "/removeold" $R1
        ${IfNot} ${Errors}
            StrCpy $RemoveOld 1
        ${EndIf}
        ;$INSTDIR 은 /D 인수로 전달받음.
        Call ApplyInstallMode
        Return
    ${EndIf}

    StrCmp $PrevMode "" noPrevInstall
        StrCpy $InstallMode $PrevMode
        StrCpy $R2 "현재 사용자용"
        StrCmp $PrevMode "AllUsers" 0 +2
        StrCpy $R2 "모든 사용자용"
        MessageBox MB_YESNO|MB_ICONQUESTION "${PRODUCT_FULLNAME}이(가) 이미 설치되어 있습니다.$\n$\n설치 유형: $R2$\n설치 위치: $PrevInstDir$\n$\n기존 설치를 제거한 뒤에 설치할까요?$\n$\n[예] 기존 설치를 제거한 뒤 새로 설치합니다.$\n[아니오] 기존 설치 위에 덮어씁니다. 이 경우 설치 유형은 변경할 수 없습니다." /SD IDYES IDYES removeOldInstall
        StrCpy $LockMode 1
        Goto prevInstallDone
    removeOldInstall:
        StrCpy $RemoveOld 1
        Goto prevInstallDone
    noPrevInstall:
        StrCpy $InstallMode "CurrentUser"
    prevInstallDone:

    Call ApplyInstallMode
    Call UpdateInstDir
FunctionEnd

;관리자 권한 보유 여부를 $IsAdmin 에 저장.
Function CheckAdmin
    StrCpy $IsAdmin 0
    ClearErrors
    UserInfo::GetAccountType
    Pop $0
    ${If} $0 == "Admin"
        StrCpy $IsAdmin 1
    ${EndIf}
FunctionEnd

;기존 설치 정보를 $PrevMode, $PrevInstDir, $PrevUninstExe 에 저장.
Function DetectPrevInstall
    StrCpy $PrevMode ""
    StrCpy $PrevInstDir ""
    StrCpy $PrevUninstExe ""

    ;모든 사용자용 설치를 먼저 확인.
    ReadRegStr $0 HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
    StrCmp $0 "" 0 checkAllUsers
    ReadRegStr $0 HKLM "${PRODUCT_DIR_REGKEY}" "InstallPath"
checkAllUsers:
    StrCmp $0 "" tryCurrentUser
    IfFileExists "$0\Uninstall.exe" 0 tryCurrentUser
    StrCpy $PrevMode "AllUsers"
    Goto prevFound

    ;현재 사용자용 설치를 확인.
tryCurrentUser:
    ReadRegStr $0 HKCU "${PRODUCT_UNINST_KEY}" "InstallLocation"
    StrCmp $0 "" 0 checkCurrentUser
    ReadRegStr $0 HKCU "${PRODUCT_DIR_REGKEY}" "InstallPath"
checkCurrentUser:
    StrCmp $0 "" detectDone
    IfFileExists "$0\Uninstall.exe" 0 detectDone
    StrCpy $PrevMode "CurrentUser"

prevFound:
    StrCpy $PrevInstDir $0
    StrCpy $PrevUninstExe "$0\Uninstall.exe"
detectDone:
FunctionEnd

;설치 유형에 맞게 Shell Var Context(SHCTX 포함)를 설정.
Function ApplyInstallMode
    ${If} $InstallMode == "AllUsers"
        SetShellVarContext all
    ${Else}
        SetShellVarContext current
    ${EndIf}
FunctionEnd

;설치 유형에 맞는 기본 설치 경로를 $INSTDIR 에 설정.
;기존 설치가 있고 설치 유형이 같으면 기존 설치 경로를 사용.
Function UpdateInstDir
    ${If} $PrevInstDir != ""
    ${AndIf} $PrevMode == $InstallMode
        StrCpy $INSTDIR $PrevInstDir
    ${ElseIf} $InstallMode == "AllUsers"
        ${If} ${RunningX64}
            StrCpy $INSTDIR "$PROGRAMFILES64\${PRODUCT_NAME}"
        ${Else}
            StrCpy $INSTDIR "$PROGRAMFILES\${PRODUCT_NAME}"
        ${EndIf}
    ${Else}
        StrCpy $INSTDIR "$APPDATA\${PRODUCT_NAME}"
    ${EndIf}
FunctionEnd

;권한 상승으로 다시 실행된 인스턴스에서는 입력 화면을 건너뜀.
Function SkipPageIfElevated
    ${If} $IsElevated == 1
        Abort
    ${EndIf}
FunctionEnd

;설치 유형 선택 화면.
Function ModePageCreate
    ${If} $IsElevated == 1
        Abort
    ${EndIf}

    !insertmacro MUI_HEADER_TEXT "설치 유형 선택" "${PRODUCT_FULLNAME}을(를) 설치할 대상을 선택하세요."

    nsDialogs::Create 1018
    Pop $Dialog
    ${If} $Dialog == error
        Abort
    ${EndIf}

    ${NSD_CreateRadioButton} 0 0 100% 12u "모든 사용자용으로 설치(&A)"
    Pop $RadioAllUsers
    ${NSD_CreateLabel} 14u 14u 95% 20u "이 컴퓨터를 사용하는 모든 사용자가 사용할 수 있도록 설치합니다.$\n설치할 때 관리자 권한이 필요합니다."
    Pop $0

    ${NSD_CreateRadioButton} 0 44u 100% 12u "현재 사용자($UserName)용으로만 설치(&C)"
    Pop $RadioCurrentUser
    ${NSD_CreateLabel} 14u 58u 95% 20u "현재 로그인한 사용자만 사용할 수 있도록 설치합니다.$\n관리자 권한이 필요하지 않습니다."
    Pop $0

    ${If} $InstallMode == "AllUsers"
        ${NSD_Check} $RadioAllUsers
    ${Else}
        ${NSD_Check} $RadioCurrentUser
    ${EndIf}

    ${If} $LockMode == 1
        EnableWindow $RadioAllUsers 0
        EnableWindow $RadioCurrentUser 0
        ${NSD_CreateLabel} 0 88u 100% 24u "기존 설치를 제거하지 않고 그대로 두기 때문에 설치 유형을 변경할 수 없습니다."
        Pop $0
    ${EndIf}

    nsDialogs::Show
FunctionEnd

Function ModePageLeave
    ${If} $LockMode != 1
        ${NSD_GetState} $RadioAllUsers $0
        ${If} $0 == ${BST_CHECKED}
            StrCpy $InstallMode "AllUsers"
        ${Else}
            StrCpy $InstallMode "CurrentUser"
        ${EndIf}
    ${EndIf}
    Call ApplyInstallMode
    Call UpdateInstDir
FunctionEnd

;모든 사용자용 설치인데 관리자 권한이 없으면 권한을 상승시켜 다시 실행.
Function DirectoryPageLeave
    ${If} $InstallMode != "AllUsers"
        Return
    ${EndIf}
    ${If} $IsAdmin == 1
        Return
    ${EndIf}

    StrCpy $R0 "/elevated /allusers"
    ${If} $RemoveOld == 1
        StrCpy $R0 "$R0 /removeold"
    ${EndIf}
    ${If} ${Silent}
        StrCpy $R0 "$R0 /S"
    ${EndIf}

    HideWindow
    ClearErrors
    ExecShellWait "runas" "$EXEPATH" '$R0 /D=$INSTDIR'
    ${If} ${Errors}
        ShowWindow $HWNDPARENT ${SW_SHOW}
        BringToFront
        MessageBox MB_OK|MB_ICONEXCLAMATION "관리자 권한을 얻지 못했습니다.$\n모든 사용자용으로 설치하려면 관리자 권한이 필요합니다." /SD IDOK
        Abort
    ${EndIf}
    Quit
FunctionEnd

;설치 완료 화면에서 프로그램을 실행.
Function LaunchApp
    ${If} $IsElevated == 1
        ;관리자 권한으로 실행되지 않도록 탐색기를 통해 바로 가기를 실행.
        Exec '"$WINDIR\explorer.exe" "$SMPROGRAMS\${PRODUCT_FULLNAME}\${PRODUCT_FULLNAME}.lnk"'
    ${Else}
        Exec '"$INSTDIR\${PRODUCT_NAME}.exe" --startup'
    ${EndIf}
FunctionEnd

;기존 설치를 제거.
Function RemovePrevInstall
    IfFileExists "$PrevUninstExe" 0 removeDone
    DetailPrint "기존 설치를 제거하는 중..."

    ;설치 제거 프로그램이 자기 자신을 지울 수 있도록 임시 폴더로 복사한 뒤 실행.
    CopyFiles /SILENT "$PrevUninstExe" "$TEMP\${PRODUCT_NAME}_prev_uninst.exe"
    IfFileExists "$TEMP\${PRODUCT_NAME}_prev_uninst.exe" 0 removeDone

    ${If} $PrevMode == "AllUsers"
    ${AndIf} $IsAdmin != 1
        ;모든 사용자용 설치를 제거하려면 관리자 권한이 필요.
        ExecShellWait "runas" "$TEMP\${PRODUCT_NAME}_prev_uninst.exe" '/S _?=$PrevInstDir'
    ${Else}
        ExecWait '"$TEMP\${PRODUCT_NAME}_prev_uninst.exe" /S _?=$PrevInstDir'
    ${EndIf}

    Delete "$TEMP\${PRODUCT_NAME}_prev_uninst.exe"
    Delete "$PrevInstDir\Uninstall.exe"
    RMDir "$PrevInstDir"
removeDone:
FunctionEnd

Section "Installer Section"
    ${If} $RemoveOld == 1
    ${AndIf} $PrevUninstExe != ""
        Call RemovePrevInstall
    ${EndIf}

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
    CreateDirectory "$SMPROGRAMS\${PRODUCT_FULLNAME}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_FULLNAME}\${PRODUCT_FULLNAME}.lnk" "$INSTDIR\${PRODUCT_NAME}.exe" "--startup"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_FULLNAME}\트레이 아이콘 복원.lnk" "$INSTDIR\${PRODUCT_NAME}.exe" "--restore_tray"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_FULLNAME}\설치 제거.lnk" "$INSTDIR\Uninstall.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr SHCTX "${PRODUCT_DIR_REGKEY}" "InstallPath" "$INSTDIR"
    WriteRegStr SHCTX "${PRODUCT_DIR_REGKEY}" "InstallMode" "$InstallMode"
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_FULLNAME}"
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$\"$INSTDIR\${PRODUCT_NAME}.exe$\""
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "Comments" "${PRODUCT_COMMENTS}"
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD SHCTX "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
    # WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "Contact" "mailto:hyunseoki@outlook.kr"
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "HelpLink" "https://github.com/kndol/KakaoTalkAdGuard/issues"
    # WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "https://github.com/kndol/KakaoTalkAdGuard/issues"
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "URLUpdateInfo" "https://github.com/kndol/KakaoTalkAdGuard#release-notes"
    WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegDWORD SHCTX "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD SHCTX "${PRODUCT_UNINST_KEY}" "NoRepair" 1
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
        ReadRegStr $R9 HKLM "SOFTWARE\Classes\Installer\Dependencies\VC,redist.x64,amd64,14.51,bundle" "Version"
        ; if VS 2022 redist not installed, install it
        IfErrors 0 VSRedistInstalled
        ;Registry Version을 읽는다.
    ${Else}
        ReadRegStr $R9 HKLM "SOFTWARE\Classes\Installer\Dependencies\VC,redist.x86,x86,14.51,bundle" "Version"
        ; if VS 2022 redist not installed, install it
        IfErrors 0 VSRedistInstalled
    ${EndIf}
    StrCpy $R9 "No" ;Registry가 없다면 R9 변수에 No를 복사.
VSRedistInstalled: ;Registry가 있다면 R9 변수에 설치된 버전을 복사.
    Exch $R9
FunctionEnd

Function un.onInit
    ;64bit OS인 경우 64bit Registry View를 사용.
    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}

    ;$INSTDIR 을 기준으로 설치 유형을 판별.
    StrCpy $InstallMode "CurrentUser"
    ReadRegStr $0 HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
    ${If} $0 == $INSTDIR
        StrCpy $InstallMode "AllUsers"
    ${Else}
        ReadRegStr $1 HKCU "${PRODUCT_UNINST_KEY}" "InstallLocation"
        ${If} $1 != $INSTDIR
        ${AndIf} $0 != ""
            StrCpy $InstallMode "AllUsers"
        ${EndIf}
    ${EndIf}

    ${If} $InstallMode == "AllUsers"
        SetShellVarContext all
    ${Else}
        SetShellVarContext current
    ${EndIf}

    ;모든 사용자용 설치는 관리자 권한으로 제거해야 함.
    ${If} $InstallMode == "AllUsers"
    ${AndIfNot} ${Silent}
        Call un.CheckAdmin
        ${If} $IsAdmin != 1
            StrCpy $R1 "$TEMP\${PRODUCT_NAME}_uninst.exe"
            CopyFiles /SILENT "$EXEPATH" "$R1"
            IfFileExists "$R1" 0 elevateFailed

            ;설치 폴더에서 직접 실행 중이면 파일이 잠기므로 기다리지 않고 종료.
            ${GetParent} "$EXEPATH" $R2
            ${If} $R2 == $INSTDIR
                ClearErrors
                ExecShell "runas" "$R1" '_?=$INSTDIR'
                ${If} ${Errors}
                    Delete "$R1"
                    Goto elevateFailed
                ${EndIf}
                Quit
            ${EndIf}

            ClearErrors
            ExecShellWait "runas" "$R1" '_?=$INSTDIR'
            ${If} ${Errors}
                Delete "$R1"
                Goto elevateFailed
            ${EndIf}
            Delete "$R1"
            Quit

        elevateFailed:
            MessageBox MB_OK|MB_ICONEXCLAMATION "관리자 권한을 얻지 못했습니다.$\n모든 사용자용으로 설치된 ${PRODUCT_FULLNAME}을(를) 제거하려면 관리자 권한이 필요합니다." /SD IDOK
            Quit
        ${EndIf}
    ${EndIf}
FunctionEnd

;관리자 권한 보유 여부를 $IsAdmin 에 저장.
Function un.CheckAdmin
    StrCpy $IsAdmin 0
    ClearErrors
    UserInfo::GetAccountType
    Pop $0
    ${If} $0 == "Admin"
        StrCpy $IsAdmin 1
    ${EndIf}
FunctionEnd

Section Uninstall
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    SendMessage $0 ${WM_CLOSE} 0 0
    notRunning:

    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$SMPROGRAMS\${PRODUCT_FULLNAME}"
    RMDir /r "$INSTDIR"
    DeleteRegKey SHCTX "${PRODUCT_UNINST_KEY}"
    DeleteRegKey SHCTX "${PRODUCT_DIR_REGKEY}"
    DeleteRegKey HKCU "${PRODUCT_DIR_REGKEY}"
    DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "${PRODUCT_NAME}"
    SetAutoClose true
SectionEnd
