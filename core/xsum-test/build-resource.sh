export RESOURCE=$PWD/resource/resource.bfs

# clean resource
rm $RESOURCE

echo RESOURCES
cd resource

# shaders
cd shaders
glslangValidator -V xsum.comp -o xsum.spv
cd ..

# add resources
bfs $RESOURCE blobSet readme.txt
bfs $RESOURCE blobSet shaders/xsum.spv

# cleanup shaders
rm shaders/*.spv
cd ..

echo CONTENTS
bfs $RESOURCE blobList
