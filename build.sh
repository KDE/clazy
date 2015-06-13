cmake -DCMAKE_INSTALL_PREFIX=/data/prefix/ -DCMAKE_BUILD_TYPE=Release .
make -j10
echo "Copying..."
cp ./lib/ClangMoreWarningsPlugin.so /data/prefix/lib/ClangMoreWarningsPlugin.so
