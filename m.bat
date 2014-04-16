@echo off
if not exist .build mkdir .build
pushd .build
cmake .. -G Ninja
if errorlevel 1 goto FAIL
ninja
if errorlevel 1 goto FAIL
cd..
.build\debugcanvas
:FAIL
popd
