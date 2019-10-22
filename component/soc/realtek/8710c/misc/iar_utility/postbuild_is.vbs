Dim WshShell, ret

Set WshShell = WScript.CreateObject("WScript.Shell")

ret = WshShell.Run("cmd /c """""+WScript.Arguments.Item(1)+"\..\..\..\component\soc\realtek\8710c\misc\iar_utility\postbuild_is.bat"" """+WScript.Arguments.Item(0)+""" """+WScript.Arguments.Item(1)+""" """+WScript.Arguments.Item(2)+""" """+WScript.Arguments.Item(3)+"""  """, 1, True)

If ret <> 0 Then  WScript.Quit(-1)

