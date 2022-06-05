echo VKUI
cd vkui/shaders
glslangValidator -V default.vert  -o default_vert.spv
glslangValidator -V color.frag    -o color_frag.spv
glslangValidator -V image.frag    -o image_frag.spv
glslangValidator -V tricolor.frag -o tricolor_frag.spv
cd ../..

# shaders
bfs $1 blobSet vkui/shaders/default_vert.spv
bfs $1 blobSet vkui/shaders/color_frag.spv
bfs $1 blobSet vkui/shaders/image_frag.spv
bfs $1 blobSet vkui/shaders/tricolor_frag.spv
rm vkui/shaders/*.spv

# font
bfs $1 blobSet vkui/fonts/BarlowSemiCondensed-Bold-64.png
bfs $1 blobSet vkui/fonts/BarlowSemiCondensed-Bold-64.xml
bfs $1 blobSet vkui/fonts/BarlowSemiCondensed-Regular-64.png
bfs $1 blobSet vkui/fonts/BarlowSemiCondensed-Regular-64.xml
bfs $1 blobSet vkui/fonts/BarlowSemiCondensed-Medium-32.png
bfs $1 blobSet vkui/fonts/BarlowSemiCondensed-Medium-32.xml

# icons
bfs $1 blobSet vkui/icons/ic_check_box_outline_blank_white_24dp.png
bfs $1 blobSet vkui/icons/ic_check_box_white_24dp.png
bfs $1 blobSet vkui/icons/ic_radio_button_checked_white_24dp.png
bfs $1 blobSet vkui/icons/ic_radio_button_unchecked_white_24dp.png
bfs $1 blobSet vkui/icons/ic_arrow_back_white_24dp.png
bfs $1 blobSet vkui/icons/ic_cancel_white_24dp.png
