#define MyAppName "FFGUI++"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Codester28351"
#define MyAppURL "https://github.com/Jsoeph192/FFgui"
#define MyAppExeName "FFGUI++.exe"

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=LICENSE
PrivilegesRequired=lowest
OutputBaseFilename=FFGUI++_Setup
Compression=lzma2/max
SolidCompression=yes
 LZMAUseSeparateProcess=yes
 LZMADictionarySize=1048576
 LZMANumBlockThreads=4
InternalCompressLevel=max
MergeDuplicateFiles=yes
WizardStyle=modern
DisableDirPage=no
DisableProgramGroupPage=no

DirExistsWarning=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.2
Name: "startup"; Description: "Launch application on Windows startup"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Types]
Name: "full"; Description: "Full installation (with Whisper)"
Name: "compact"; Description: "Compact installation (FFmpeg only)"

[Components]
Name: "core"; Description: "Core Application"; Types: full compact; Flags: fixed
Name: "whisper"; Description: "Whisper Speech Recognition"; Types: full

[Files]
Source: "FFGUI++.exe"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "ffmpeg.exe"; DestDir: "{app}"; Flags: ignoreversion 
Source: "ffplay.exe"; DestDir: "{app}"; Flags: ignoreversion 
Source: "ffprobe.exe"; DestDir: "{app}"; Flags: ignoreversion 
Source: "yt-dlp.exe"; DestDir: "{app}"; Flags: ignoreversion 
Source: "libx264-165.dll"; DestDir: "{app}"; Flags: ignoreversion 
Source: "Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "Qt6Concurrent.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libmd4c.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libharfbuzz-0.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libfreetype-6.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libpng16-16.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "zlib1.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libb2-1.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libdouble-conversion.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libicuin78.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libicuuc78.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libicudt78.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libpcre2-16-0.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libzstd.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libbrotlidec.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libglib-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libgraphite2.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libbz2-1.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libintl-8.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libpcre2-8-0.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libbrotlicommon.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "libiconv-2.dll"; DestDir: "{app}"; Flags: ignoreversion nocompression
Source: "qt.conf"; DestDir: "{app}"; Flags: ignoreversion 

Source: "plugins\*"; DestDir: "{app}\plugins"; Flags: ignoreversion recursesubdirs nocompression

Source: "whisper.exe"; DestDir: "{app}"; Components: whisper; Flags: ignoreversion 
Source: "whisper.dll"; DestDir: "{app}"; Components: whisper; Flags: ignoreversion 
Source: "ggml.dll"; DestDir: "{app}"; Components: whisper; Flags: ignoreversion 
Source: "ggml-cpu.dll"; DestDir: "{app}"; Components: whisper; Flags: ignoreversion 


[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Windows\Start Menu\Programs\Startup\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startup

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
