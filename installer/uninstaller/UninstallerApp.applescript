-- UninstallerApp.applescript
-- Wraps uninstall.sh with a native macOS dialog flow.
-- Compiled into Uninstaller.app by build_dmg.sh via osacompile.

set appName to "MIDI Splitter Uninstaller"
set uninstallScript to POSIX path of (path to me) & "Contents/Resources/uninstall.sh"

-- Confirm dialog
set response to button returned of (display dialog "This will remove MIDI Splitter (VST3 and AU) from your system, along with the PDF user guide if present." & return & return & "Do you want to continue?" with title appName buttons {"Cancel", "Uninstall"} default button "Cancel" with icon caution)

if response is "Cancel" then
    return
end if

-- Run uninstall script with admin privileges
try
    set result to do shell script "bash " & quoted form of uninstallScript with administrator privileges
    display dialog "MIDI Splitter has been successfully uninstalled." & return & return & result with title appName buttons {"OK"} default button "OK" with icon note
on error errMsg
    display dialog "Uninstall encountered an error:" & return & return & errMsg with title appName buttons {"OK"} default button "OK" with icon stop
end try
