#include "readTexture.hpp"
#include <tiffio.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <GL/gl.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

static const int MAX_TEX_SIZE = 8192;

int readTiffImage(const char *filename, unsigned int *outTextureID)
{
    TIFF *tif = TIFFOpen(filename, "r");
    if (!tif) {
        std::cerr << "Failed to open TIFF file: " << filename << std::endl;
        return 0;
    }

    TIFFRGBAImage img;
    char emsg[1024];
    if (!TIFFRGBAImageBegin(&img, tif, 0, emsg)) {
        std::cerr << "TIFFRGBAImageBegin failed: " << emsg << std::endl;
        TIFFClose(tif);
        return 0;
    }

    uint32_t w = img.width;
    uint32_t h = img.height;

    std::cout << "Loaded TIFF: " << filename << " (" << w << "x" << h << ")\n";

    uint32_t *raster = static_cast<uint32_t *>(_TIFFmalloc((size_t)w * h * sizeof(uint32_t)));
    if (!raster) {
        std::cerr << "Failed to allocate TIFF raster buffer." << std::endl;
        TIFFRGBAImageEnd(&img);
        TIFFClose(tif);
        return 0;
    }

    if (!TIFFRGBAImageGet(&img, raster, w, h)) {
        std::cerr << "TIFFRGBAImageGet failed for " << filename << std::endl;
        _TIFFfree(raster);
        TIFFRGBAImageEnd(&img);
        TIFFClose(tif);
        return 0;
    }

    for (uint32_t y = 0; y < h / 2; ++y) {
        uint32_t *top = raster + y * w;
        uint32_t *bottom = raster + (h - 1 - y) * w;
        for (uint32_t x = 0; x < w; ++x)
            std::swap(top[x], bottom[x]);
    }

    uint32_t uploadW = w, uploadH = h;
    uint32_t *uploadRaster = raster;

    if (w > MAX_TEX_SIZE || h > MAX_TEX_SIZE) {
        float aspect = (float)w / (float)h;
        if (w >= h) {
            uploadW = MAX_TEX_SIZE;
            uploadH = (uint32_t)(uploadW / aspect);
        } else {
            uploadH = MAX_TEX_SIZE;
            uploadW = (uint32_t)(uploadH * aspect);
        }

        std::cout << "Downscaling TIFF to " << uploadW << "x" << uploadH << std::endl;

        uint32_t *temp = static_cast<uint32_t *>(_TIFFmalloc((size_t)uploadW * uploadH * sizeof(uint32_t)));
        stbir_resize_uint8_linear(
            reinterpret_cast<const unsigned char *>(raster),
            w, h, 0,
            reinterpret_cast<unsigned char *>(temp),
            uploadW, uploadH, 0,
            STBIR_RGBA
        );

        _TIFFfree(raster);
        uploadRaster = temp;
    }

    glBindTexture(GL_TEXTURE_2D, *outTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, uploadW, uploadH, 0, GL_RGBA, GL_UNSIGNED_BYTE, uploadRaster);
    glGenerateMipmap(GL_TEXTURE_2D);

    _TIFFfree(uploadRaster);
    TIFFRGBAImageEnd(&img);
    TIFFClose(tif);

    std::cout << "TIFF successfully uploaded to GPU!\n";

    return 1;
}


#include <stb_image.h>
#include <string>
#include <algorithm>

unsigned int loadTexture(const char* path) {
    std::string filePath = path;
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    std::string ext;
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos)
        ext = filePath.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "tif" || ext == "tiff") {
        readTiffImage(const_cast<char*>(filePath.c_str()), &textureID);
        return textureID;
    }
    
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 1) ? GL_RED : 
                       (nrComponents == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}
