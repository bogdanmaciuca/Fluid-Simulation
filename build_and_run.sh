mkdir -p build
cmake --build build -j $(nproc) && build/Fluids

