; =============================================================================
;  YAZIT - NSIS Installer Script
; =============================================================================

Unicode True

!define APP_NAME      "YAZIT"
!define APP_EXE       "YAZIT.exe"
!define APP_VERSION   "1.0.0"
!define APP_PUBLISHER "YAZIT"
!define INSTALL_DIR   "$PROGRAMFILES64\${APP_NAME}"
!define REG_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

; ── Genel ─────────────────────────────────────────────────────────────────────
Name              "${APP_NAME} ${APP_VERSION}"
OutFile           "YAZIT-${APP_VERSION}-Setup.exe"
InstallDir        "${INSTALL_DIR}"
InstallDirRegKey  HKLM "Software\${APP_NAME}" "InstallDir"
RequestExecutionLevel admin
SetCompressor     /SOLID lzma

; ── Modern UI ─────────────────────────────────────────────────────────────────
!include "MUI2.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON   "dist\app.ico"
!define MUI_UNICON "dist\app.ico"

!define MUI_WELCOMEPAGE_TITLE    "YAZIT ${APP_VERSION} Kurulum Sihirbazı"
!define MUI_WELCOMEPAGE_TEXT     "Bu sihirbaz YAZIT uygulamasını bilgisayarınıza kuracak.$\r$\n$\r$\nDevam etmek için İleri'ye tıklayın."
!define MUI_FINISHPAGE_RUN       "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT  "YAZIT'i şimdi başlat"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "Turkish"

; =============================================================================
;  KURULUM
; =============================================================================
Section "Ana Program" SecMain

  SetOutPath "$INSTDIR"

  ; dist/ klasöründeki tüm dosyaları kopyala (exe + dll'ler + ikon)
  File /r "dist\*.*"

  ; ── Kayıt defteri: kurulum yolu ────────────────────────────────────────────
  WriteRegStr HKLM "Software\${APP_NAME}" "InstallDir" "$INSTDIR"

  ; ── Kısayollar (SetOutPath = çalışma dizini; DLL'lerin bulunması için şart) ─
  SetOutPath "$INSTDIR"

  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\app.ico"
  CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\app.ico"

  ; ── App Paths kaydı (Windows'un exe'yi her zaman bulmasını sağlar) ────────
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\${APP_EXE}" ""      "$INSTDIR\${APP_EXE}"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\${APP_EXE}" "Path"  "$INSTDIR"

  ; ── Sağ tık menüsü: tüm dosyalar ──────────────────────────────────────────
  WriteRegStr HKCR "*\shell\${APP_NAME}"         ""      "YAZIT ile Aç"
  WriteRegStr HKCR "*\shell\${APP_NAME}"         "Icon"  "$INSTDIR\app.ico"
  WriteRegStr HKCR "*\shell\${APP_NAME}\command" ""      '"$INSTDIR\${APP_EXE}" "%1"'

  ; ── Sağ tık menüsü: klasörler ──────────────────────────────────────────────
  WriteRegStr HKCR "Directory\shell\${APP_NAME}"         ""      "YAZIT ile Aç"
  WriteRegStr HKCR "Directory\shell\${APP_NAME}"         "Icon"  "$INSTDIR\app.ico"
  WriteRegStr HKCR "Directory\shell\${APP_NAME}\command" ""      '"$INSTDIR\${APP_EXE}" "%1"'

  ; ── Programlar ve Özellikler kaydı ─────────────────────────────────────────
  WriteRegStr   HKLM "${REG_UNINSTALL}" "DisplayName"     "${APP_NAME}"
  WriteRegStr   HKLM "${REG_UNINSTALL}" "DisplayVersion"  "${APP_VERSION}"
  WriteRegStr   HKLM "${REG_UNINSTALL}" "Publisher"       "${APP_PUBLISHER}"
  WriteRegStr   HKLM "${REG_UNINSTALL}" "InstallLocation" "$INSTDIR"
  WriteRegStr   HKLM "${REG_UNINSTALL}" "DisplayIcon"     "$INSTDIR\app.ico"
  WriteRegStr   HKLM "${REG_UNINSTALL}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoModify"        1
  WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoRepair"        1

  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

; =============================================================================
;  KALDIRMA
; =============================================================================
Section "Uninstall"

  ; Sağ tık menüsü kayıtlarını temizle
  DeleteRegKey HKCR "*\shell\${APP_NAME}"
  DeleteRegKey HKCR "Directory\shell\${APP_NAME}"

  ; Kayıt defteri kayıtlarını temizle
  DeleteRegKey HKLM "Software\${APP_NAME}"
  DeleteRegKey HKLM "${REG_UNINSTALL}"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\${APP_EXE}"

  ; Kısayolları sil
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
  RMDir  "$SMPROGRAMS\${APP_NAME}"
  Delete "$DESKTOP\${APP_NAME}.lnk"

  ; Kurulum klasörünü sil
  RMDir /r "$INSTDIR"

SectionEnd
