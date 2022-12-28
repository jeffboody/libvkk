cd vkk/vg/shaders
glslangValidator -V line.vert    -o line_vert.spv
glslangValidator -V line.frag    -o line_frag.spv
glslangValidator -V polygon.vert -o polygon_vert.spv
glslangValidator -V polygon.frag -o polygon_frag.spv
glslangValidator -V image.vert  -o image_vert.spv
glslangValidator -V image.frag  -o image_frag.spv
cd ../../..

# shaders
bfs $1 blobSet vkk/vg/shaders/line_vert.spv
bfs $1 blobSet vkk/vg/shaders/line_frag.spv
bfs $1 blobSet vkk/vg/shaders/polygon_vert.spv
bfs $1 blobSet vkk/vg/shaders/polygon_frag.spv
bfs $1 blobSet vkk/vg/shaders/image_vert.spv
bfs $1 blobSet vkk/vg/shaders/image_frag.spv
rm vkk/vg/shaders/*.spv
