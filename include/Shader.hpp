#ifndef SHADER_HPP
#define SHADER_HPP

#include "glm/glm.hpp"
#include <string>

class Shader{
public:
		int ID;
		Shader(const char* vertexPath, const char* fragmentPart);
		void use();

		void setMat4(const char* string, glm::mat4 data);
		void setVec3(const char* name, glm::vec3 data);

		void setFloat(const std::string &name, float value) const ;

		void setBool(const std::string &name, bool value) const;


};



#endif
