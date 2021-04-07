@echo on
cd %NINTENDO_SDK_ROOT%
Tools\CommandLineTools\RunOnTarget.exe ^
TargetTools\NX-NXFP2-a64\DevMenuCommand\Release\DevMenuCommand.nsp ^
-- application install "%BARONY_NX64_RELEASE_LOCATION%" --force
exit