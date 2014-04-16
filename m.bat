@echo off
if not exist .build mkdir .build
pushd .build
cmake .. -G Ninja
if errorlevel 1 goto FAIL
ninja
if errorlevel 1 goto FAIL
cd ..
pushd third_party\bgfx\examples\runtime
..\..\..\..\.build\debugcanvas
:FAIL
popd
