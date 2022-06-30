cd vkk/ui/shaders
glslangValidator -V default.vert  -o default_vert.spv
glslangValidator -V color.frag    -o color_frag.spv
glslangValidator -V image.frag    -o image_frag.spv
glslangValidator -V tricolor.frag -o tricolor_frag.spv
cd ../../..

# shaders
bfs $1 blobSet vkk/ui/shaders/default_vert.spv
bfs $1 blobSet vkk/ui/shaders/color_frag.spv
bfs $1 blobSet vkk/ui/shaders/image_frag.spv
bfs $1 blobSet vkk/ui/shaders/tricolor_frag.spv
rm vkk/ui/shaders/*.spv

# font
bfs $1 blobSet vkk/ui/fonts/BarlowSemiCondensed-Bold-64.png
bfs $1 blobSet vkk/ui/fonts/BarlowSemiCondensed-Bold-64.xml
bfs $1 blobSet vkk/ui/fonts/BarlowSemiCondensed-Regular-64.png
bfs $1 blobSet vkk/ui/fonts/BarlowSemiCondensed-Regular-64.xml
bfs $1 blobSet vkk/ui/fonts/BarlowSemiCondensed-Medium-32.png
bfs $1 blobSet vkk/ui/fonts/BarlowSemiCondensed-Medium-32.xml

# icons
bfs $1 blobSet vkk/ui/icons/ic_check_box_outline_blank_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_check_box_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_radio_button_checked_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_radio_button_unchecked_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_arrow_back_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_cancel_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_check_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_close_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_create_new_folder_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_folder_open_white_24dp.png
bfs $1 blobSet vkk/ui/icons/ic_folder_white_24dp.png
bfs $1 blobSet vkk/ui/icons/outline_insert_drive_file_white_48dp.png
