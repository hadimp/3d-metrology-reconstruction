#!/bin/bash
curl -L -o metal-cpp.zip https://developer.apple.com/metal/cpp/files/metal-cpp_macOS14.2_iOS17.2.zip
unzip metal-cpp.zip -d ../src/metal-cpp
rm metal-cpp.zip
