# This script shouldn't be run directly, but instead invoked by make_appimage.py
# which runs this script in a Centos 6.8 docker container to create an app image

# Arguments
#   $1 clazy sha1 to build
#   $2 user uid to chown the files to before leaving docker

cp -r /clazy.AppDir /tmp/clazy_work/
cd /clazy
git clean -fdx .
git checkout .
git pull
git checkout $1
cmake3 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/tmp/clazy_work/clazy.AppDir/usr . && make -j12 && make install
chown -R $2 /tmp/clazy_work/clazy.AppDir/
