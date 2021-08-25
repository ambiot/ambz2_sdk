Option Explicit
DIM fso    

Dim WshShell, ret

Set fso = CreateObject("Scripting.FileSystemObject")

If (fso.FileExists("""" & WScript.Arguments(0) & """\..\..\..\component\soc\realtek\8710c\misc\iar_utility\prebuild.bat")) Then
  MsgBox "script not exist " & WScript.Arguments(0) & "\..\..\..\component\soc\realtek\8710c\misc\iar_utility\prebuild.bat" , vbinformation
  WScript.Quit(-1)
End If

Set WshShell = WScript.CreateObject("WScript.Shell")

ret = WshShell.Run("cmd /c """"" & WScript.Arguments(0) & "\..\..\..\component\soc\realtek\8710c\misc\iar_utility\prebuild.bat"" """+WScript.Arguments(0)+""" "+WScript.Arguments(1)+"""", 0, true)
If ret <> 0 Then  WScript.Quit(-1)