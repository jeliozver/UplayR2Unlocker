# Usage:
#   build.ps1 32 Debug
#   build.ps1 32 Release
#   build.ps1 64 Debug
#   build.ps1 64 Release

$arch = $args[0]
if ($arch -notmatch '^(32|64)$') {
    throw "Invalid architecute. Expected: '32' or '64'. Got: '$arch'"
}
$platform = If ($arch -eq '32') { 'Win32' } Else { 'x64' }

$config = $args[1]
if ($config -notmatch '^(Debug|Release|RelWithDebInfo)$') {
    throw "Invalid architecute. Expected: 'Debug' or 'Release' or 'RelWithDebInfo'. Got: '$config'"
}

if ($config -eq 'Debug') {
    $Env:VERSION_SUFFIX = "-debug"
}

$Env:BUILD_DIR = "build\$arch"

function Build-Project {
    cmake -G "Visual Studio 17 2022" -A $platform -B "$Env:BUILD_DIR" "$Env:CMAKE_OPTIONS"

    cmake --build "$Env:BUILD_DIR" --config $config  "$Env:CMAKE_BUILD_OPTIONS"
}
