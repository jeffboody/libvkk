echo VKUI
cd vkui
glslangValidator -V default.vert  -o default_vert.spv
glslangValidator -V color.frag    -o color_frag.spv
glslangValidator -V image.frag    -o image_frag.spv
glslangValidator -V tricolor.frag -o tricolor_frag.spv
cd ..

pak -a $1 vkui/default_vert.spv
pak -a $1 vkui/color_frag.spv
pak -a $1 vkui/image_frag.spv
pak -a $1 vkui/tricolor_frag.spv
rm vkui/*.spv
