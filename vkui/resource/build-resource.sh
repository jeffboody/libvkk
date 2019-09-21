echo VKUI
cd vkui
glslangValidator -V default.vert  -o default_vert.spv
glslangValidator -V color.frag    -o color_frag.spv
glslangValidator -V image.frag    -o image_frag.spv
glslangValidator -V tricolor.frag -o tricolor_frag.spv
cd ..

# shaders
pak -a $1 vkui/default_vert.spv
pak -a $1 vkui/color_frag.spv
pak -a $1 vkui/image_frag.spv
pak -a $1 vkui/tricolor_frag.spv
rm vkui/*.spv

# font
pak -a $1 vkui/BarlowSemiCondensed-Bold-64.texz
pak -a $1 vkui/BarlowSemiCondensed-Bold-64.xml
pak -a $1 vkui/BarlowSemiCondensed-Regular-64.texz
pak -a $1 vkui/BarlowSemiCondensed-Regular-64.xml

# icons
pak -a $1 vkui/ic_check_box_outline_blank_white_24dp.texz
pak -a $1 vkui/ic_check_box_white_24dp.texz
pak -a $1 vkui/ic_radio_button_checked_white_24dp.texz
pak -a $1 vkui/ic_radio_button_unchecked_white_24dp.texz
