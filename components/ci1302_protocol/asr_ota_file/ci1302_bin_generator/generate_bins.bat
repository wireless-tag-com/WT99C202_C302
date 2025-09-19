@echo off
chcp 65001 >nul
echo ========================================
echo CI1302 Bin文件生成工具
echo ========================================

echo.
echo 1. 生成 asr.bin...
ci-tool-kit.exe merge asr-file -i asr
if %errorlevel% neq 0 (
    echo 错误：asr.bin 生成失败
    pause
    exit /b 1
)
echo asr.bin 生成成功

echo.
echo 2. 处理 Excel 文件...
ci-tool-kit.exe "cmd-info" "-V2" "user_file\cmd_info\[60000]{cmd_info}.xlsx"
if %errorlevel% neq 0 (
    echo 错误：Excel 文件处理失败
    pause
    exit /b 1
)
echo Excel 文件处理成功

echo.
echo 3. 生成 user_file.bin...
ci-tool-kit.exe merge user-file -i user_file\cmd_info
if %errorlevel% neq 0 (
    echo 错误：user_file.bin 生成失败
    pause
    exit /b 1
)
echo 正在复制到正确位置...
copy "user_file\cmd_info\cmd_info.bin" "user_file\user_file.bin"
if %errorlevel% neq 0 (
    echo 错误：user_file.bin 复制失败
    pause
    exit /b 1
)
echo user_file.bin 生成成功

echo.
echo ========================================
echo 所有 bin 文件生成完成！
echo ========================================
echo.
echo 生成的文件：
echo - asr\asr.bin
echo - user_file\user_file.bin
echo.
pause
