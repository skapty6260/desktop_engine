rm -rf build
meson setup build
meson compile -C build
cd build
ninja install
cd ..