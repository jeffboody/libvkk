export RESOURCE=$PWD/resource/resource.bfs

# clean resource
rm $RESOURCE

echo RESOURCES
cd resource

# shaders
cd shaders
glslangValidator -V xsq.comp -o xsq.spv
cd ..

# add resources
bfs $RESOURCE blobSet readme.txt
bfs $RESOURCE blobSet shaders/xsq.spv

# cleanup shaders
rm shaders/*.spv
cd ..

echo CONTENTS
bfs $RESOURCE blobList
