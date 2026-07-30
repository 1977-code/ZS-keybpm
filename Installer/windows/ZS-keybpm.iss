; Inno Setup script for the branded ZS-keybpm Windows installer.
;
; Compiled on CI (windows-latest) after the VST3 is built:
;   iscc /DVersion=1.0.0 /DSourceDir=..\..\dist\vst3 Installer\windows\ZS-keybpm.iss
;
; SourceDir must contain the built ZS-keybpm.vst3 folder. Both defines have local
; defaults, so the script also compiles as-is next to a local MSVC build.
;
; Needs Inno Setup 6.3 or newer: that is where "x64compatible" arrived.

#ifndef Version
  #define Version "1.0.0"
#endif

#ifndef SourceDir
  #define SourceDir "..\..\build\ZSkeybpm_artefacts\Release\VST3"
#endif

#define Name      "ZS-keybpm"
#define Publisher "ZS Records"
#define Site      "https://zsr.artspace1977.ru"

[Setup]
AppId={{9E4B2A71-3C6D-4F58-9B1E-7A5D2C8F1B44}
AppName={#Name}
AppVersion={#Version}
AppVerName={#Name} {#Version}
AppPublisher={#Publisher}
AppPublisherURL={#Site}
AppSupportURL={#Site}
AppCopyright=(c) ZS Records
VersionInfoDescription={#Name} tempo and key detector
VersionInfoProductName={#Name}
VersionInfoVersion={#Version}
VersionInfoCompany={#Publisher}

; The plug-in installs into the shared VST3 folder, so there is nothing to choose.
; A small app dir is kept anyway: it holds the uninstaller, its icon and the
; licences, which is what makes the Apps & features entry behave like a real one.
DefaultDirName={autopf}\{#Publisher}\{#Name}
DisableDirPage=yes
DisableProgramGroupPage=yes
UninstallDisplayName={#Name} {#Version}
UninstallDisplayIcon={app}\zskeybpm.ico

PrivilegesRequired=admin
; x64compatible also covers Arm64 Windows 11, where the x64 bundle runs in the
; emulated hosts people actually use there. Deliberate: "x64os" would refuse.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

OutputDir=..\..\dist
OutputBaseFilename={#Name}-{#Version}-Windows-x64
Compression=lzma2/max
SolidCompression=yes

; --- branding --------------------------------------------------------------
WizardStyle=modern
WizardImageFile=wizard-large.bmp
WizardSmallImageFile=wizard-small.bmp
SetupIconFile=zskeybpm.ico

; The licence page shows the licence itself, not a summary of it.
LicenseFile=..\..\LICENSE

[Languages]
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Messages]
ru.WelcomeLabel2=Будет установлен {#Name} {#Version}.%n%n{#Name} — измерительный плагин студии ZS Records: темп и тональность по самому материалу, сигнал проходит нетронутым. 64-битный VST3 установится в общую папку VST3 для всех DAW на этом компьютере.
en.WelcomeLabel2=This will install {#Name} {#Version} on your computer.%n%n{#Name} is the ZS Records measuring plug-in: tempo and key read from the material itself, with the signal passed through untouched. The 64-bit VST3 will be installed into the shared VST3 folder for every DAW on this machine.

ru.FinishedLabelNoIcons=Установка завершена.%n%nПлагин появится в хосте среди VST3 у производителя ZS Records — перезапустите DAW, если он ещё не виден. Дайте материалу поиграть несколько секунд: темп и тональность набирают уверенность по мере прослушивания. RESET — при смене трека, HOLD — заморозить показания.
en.FinishedLabelNoIcons=Setup is done.%n%nThe plug-in appears among your VST3s under ZS Records — restart the DAW if it is not listed yet. Let the material play for a few seconds: tempo and key gain confidence as they listen. RESET when the track changes, HOLD to freeze the reading.

[Files]
; The plug-in itself — into C:\Program Files\Common Files\VST3.
Source: "{#SourceDir}\{#Name}.vst3\*"; DestDir: "{commoncf64}\VST3\{#Name}.vst3"; \
    Flags: recursesubdirs createallsubdirs ignoreversion

; Icon kept alongside the uninstaller.
Source: "zskeybpm.ico"; DestDir: "{app}"; Flags: ignoreversion

; The licences travel with the binary rather than being promised by a link: AGPLv3
; requires a copy of itself to accompany the object code, and the OFL requires the
; same of the two font families compiled into the plug-in.
Source: "..\..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\..\Resources\Fonts\OFL.txt"; DestDir: "{app}"; DestName: "LICENSE-fonts.txt"; \
    Flags: ignoreversion

[UninstallDelete]
; Everything else was installed by [Files] and is already in the uninstall log.
Type: filesandordirs; Name: "{commoncf64}\VST3\{#Name}.vst3"

[Run]
Filename: "{#Site}"; Description: "Открыть сайт ZS Records"; \
    Flags: postinstall shellexec nowait unchecked
