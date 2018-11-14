# This script shouldn't be run directly, but instead invoked by make_appimage.py
# which runs this script in a Centos 6.8 docker container to create an app image

cp -r /clazy.AppDir /tmp/clazy_work/
cd /clazy
git clean -fdx .
git checkout .
git pull
git checkout $1
cmake3 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/tmp/clazy_work/clazy.AppDir/usr . && make -j12 && make install
