#ifndef READTEXTURE_HPP
#define READTEXTURE_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

int readTiffImage(const char *filename, unsigned int *outTextureID);
int writeTiff(const char *filename, const char *description,
              int x, int y, int width, int height, int compression);

unsigned int loadTexture(const char* path);

#endif // READTEXTURE_HPP
