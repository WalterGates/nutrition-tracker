#!/usr/bin/sh
/usr/bin/cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++ -S. -Bbuild -G"Unix Makefiles" && /usr/bin/cmake --build build --config Debug --target all -j 18 --
