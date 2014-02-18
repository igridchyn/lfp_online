
attrib -h *.suo
del *.suo
del *.user
del *.ncb
del *.aps
del *.opt
del *.plg
del *.old

del Release\*.htm
del Release\*.obj
del Release\*.pdb
del Release\*.pch
del Release\*.res
del Release\*.idb
del Release\*.sbr
del Release\*.dep
del Release\*.manifest

rmdir Debug /S /Q
rmdir x64 /S /Q
rmdir _Compressed /S /Q
rmdir _ExtractFile /S /Q
rmdir _ExtractResource /S /Q
rmdir _ExtractUrl /S /Q
rmdir Release\_Compressed /S /Q
rmdir Release\_ExtractFile /S /Q
rmdir Release\_ExtractResource /S /Q
rmdir Release\_ExtractUrl /S /Q
