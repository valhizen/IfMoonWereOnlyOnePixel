#include "Shader.hpp"
#include "glad/glad.h"
#include "glm/gtc/type_ptr.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

Shader::Shader(const char *vertexPath, const char *fragmentPath) : ID(0) {
  std::ifstream vertexShaderFile, fragmentShaderFile;
  std::string vertexCode, fragmentCode;
  vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  
  try {
    vertexShaderFile.open(vertexPath);
    fragmentShaderFile.open(fragmentPath);
    std::stringstream vertexShaderStream, fragmentShaderStream;
    vertexShaderStream << vertexShaderFile.rdbuf();
    fragmentShaderStream << fragmentShaderFile.rdbuf();
    vertexShaderFile.close();
    fragmentShaderFile.close();
    vertexCode = vertexShaderStream.str();
    fragmentCode = fragmentShaderStream.str();
  } catch (std::ifstream::failure& e) {
    std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
    std::cerr << "Vertex path: " << vertexPath << std::endl;
    std::cerr << "Fragment path: " << fragmentPath << std::endl;
    std::cerr << "Exception: " << e.what() << std::endl;
    throw; // Re-throw so Planet constructor catches it
  }
  
  const char *vertexShaderCode = vertexCode.c_str();
  const char *fragmentShaderCode = fragmentCode.c_str();
  unsigned int vertex, fragment;
  int success;
  char infoLog[512];
  
  // Vertex Shader
  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vertexShaderCode, NULL);
  glCompileShader(vertex);
  glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED" << std::endl;
    std::cerr << "File: " << vertexPath << std::endl;
    std::cerr << infoLog << std::endl;
    throw std::runtime_error("Vertex shader compilation failed");
  }
  
  // Fragment Shader
  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fragmentShaderCode, NULL);
  glCompileShader(fragment);
  glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED" << std::endl;
    std::cerr << "File: " << fragmentPath << std::endl;
    std::cerr << infoLog << std::endl;
    glDeleteShader(vertex);
    throw std::runtime_error("Fragment shader compilation failed");
  }
  
  // Shader Program
  ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  glLinkProgram(ID);
  glGetProgramiv(ID, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(ID, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED" << std::endl;
    std::cerr << infoLog << std::endl;
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    throw std::runtime_error("Shader program linking failed");
  }
  
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  
  std::cout << "Shader compiled and linked successfully! Program ID: " << ID << std::endl;
}

void Shader::use() { 
  glUseProgram(ID); 
}

void Shader::setMat4(const char *string, glm::mat4 data) {
  int viewLoc = glGetUniformLocation(ID, string);
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(data));
}

void Shader::setVec3(const char* name, glm::vec3 data) {
  int loc = glGetUniformLocation(ID, name);
  glUniform3fv(loc, 1, glm::value_ptr(data));
}

void Shader::setFloat(const std::string &name, float value) const {
  glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setBool(const std::string &name, bool value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
