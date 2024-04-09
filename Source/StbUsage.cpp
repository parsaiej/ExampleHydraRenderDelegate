#include <ExampleDelegate/StbUsage.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

void WritePNG(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes)
{
    stbi_write_png(filename, w, h, comp, data, stride_in_bytes);
}