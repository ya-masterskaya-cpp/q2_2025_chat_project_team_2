1. Добавляйте в корень файлы conanfile_debug.txt и conanfile_release.txt
2. Заменяйте корневой CMakeLists.txt
3. Команды сборки
- Для Debug:
```
conan install conanfile_debug.txt --output-folder=build/debug -s build_type=Debug --build=missing
cmake -S . -B build/debug -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=build/debug/conan_toolchain.cmake
cmake --build build/debug --config Debug
```
- Для Release:
```
conan install conanfile_release.txt --output-folder=build/release -s build_type=Release --build=missing
cmake -S . -B build/release -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=build/release/conan_toolchain.cmake
cmake --build build/release --config Release
```
