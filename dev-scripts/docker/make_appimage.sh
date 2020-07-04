# This script shouldn't be run directly, but instead invoked by make_appimage.py
# which runs this script in a Centos 6.8 docker container to create an app image

# Arguments
#   $1 clazy sha1 to build
#   $2 user uid to chown the files to before leaving docker

mkdir /tmp/clazy_work/
cp -r /clazy.AppDir/ /tmp/clazy_work/


cd /clazy
echo "Clazy sha1: `git rev-parse HEAD`" >> /tmp/clazy_work/clazy.AppDir/sha1

git clean -fdx .
git checkout .

echo "Running git pull..."
git pull

echo "Checking out $1..."
git checkout $1

echo "Building..."
cmake -DCMAKE_BUILD_TYPE=Release -DAPPIMAGE_HACK=ON -DLINK_CLAZY_TO_LLVM=OFF -DCMAKE_INSTALL_PREFIX=/tmp/clazy_work/clazy.AppDir/usr . && make -j12 && make install

echo "Fixing permissions..."
chown -R $2 /tmp/clazy_work/clazy.AppDir/

cp /clazy/README.md /tmp/clazy_work/clazy.AppDir/
cp /clazy/COPYING-LGPL2.txt /tmp/clazy_work/clazy.AppDir/


echo "Done"
echo
