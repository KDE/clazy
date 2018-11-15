# This script shouldn't be run directly, but instead invoked by make_appimage.py
# which runs this script in a Centos 6.8 docker container to create an app image

# Arguments
#   $1 clazy sha1 to build
#   $2 user uid to chown the files to before leaving docker

cp -r /clazy.AppDir /tmp/clazy_work/
cd /clazy
git clean -fdx .
git checkout .

echo "Running git pull..."
git pull

echo "Checking out $1..."
git checkout $1

echo "Building..."
cmake3 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/tmp/clazy_work/clazy.AppDir/usr . && make -j12 && make install

echo "Fixing permissions..."
chown -R $2 /tmp/clazy_work/clazy.AppDir/

cp /clazy/README.md /tmp/clazy_work/clazy.AppDir/
cp /clazy/COPYING-LGPL2.txt /tmp/clazy_work/clazy.AppDir/

echo "Cleanup..."
rm -rf /tmp/clazy_work/clazy.AppDir/usr/share/
rm -rf /tmp/clazy_work/clazy.AppDir/usr/include/
cd /tmp/clazy_work/clazy.AppDir/usr/bin/
rm ll* bugpoint clang-format clang-import-test clang-refactor diagtool git-clang-format obj2yaml sancov scan-build verify-uselistorder c-index-test clang-check clang-cpp clang-func-mapping clang-offload-bundler clang-rename dsymutil hmaptool opt sanstats scan-view yaml2obj


echo "Done"
echo
