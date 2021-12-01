echo VKUI
cd vkui/shaders
glslangValidator -V default.vert  -o default_vert.spv
glslangValidator -V color.frag    -o color_frag.spv
glslangValidator -V image.frag    -o image_frag.spv
glslangValidator -V tricolor.frag -o tricolor_frag.spv
cd ../..

# shaders
pak -a $1 vkui/shaders/default_vert.spv
pak -a $1 vkui/shaders/color_frag.spv
pak -a $1 vkui/shaders/image_frag.spv
pak -a $1 vkui/shaders/tricolor_frag.spv
rm vkui/shaders/*.spv

# font
pak -a $1 vkui/fonts/BarlowSemiCondensed-Bold-64.texz
pak -a $1 vkui/fonts/BarlowSemiCondensed-Bold-64.xml
pak -a $1 vkui/fonts/BarlowSemiCondensed-Regular-64.texz
pak -a $1 vkui/fonts/BarlowSemiCondensed-Regular-64.xml
pak -a $1 vkui/fonts/BarlowSemiCondensed-Medium-32.texz
pak -a $1 vkui/fonts/BarlowSemiCondensed-Medium-32.xml

# icons
pak -a $1 vkui/icons/ic_check_box_outline_blank_white_24dp.texz
pak -a $1 vkui/icons/ic_check_box_white_24dp.texz
pak -a $1 vkui/icons/ic_radio_button_checked_white_24dp.texz
pak -a $1 vkui/icons/ic_radio_button_unchecked_white_24dp.texz
