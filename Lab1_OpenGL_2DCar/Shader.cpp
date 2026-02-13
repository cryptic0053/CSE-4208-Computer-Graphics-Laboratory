#include "Shader.h"
#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <iostream>

// Helper to read file content
static std::string readFile(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "ERROR::SHADER::FILE_NOT_FOUND: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    // 1. Retrieve the vertex/fragment source code from filePath
    std::string vStr = readFile(vertexPath);
    std::string fStr = readFile(fragmentPath);

    // Stop if files were not found
    if (vStr.empty() || fStr.empty()) {
        std::cout << "ERROR::SHADER::EMPTY_FILE_OR_READ_FAILURE" << std::endl;
        return;
    }

    const char* vCode = vStr.c_str();
    const char* fCode = fStr.c_str();

    unsigned int vShader, fShader;
    int success;
    char infoLog[512];

    // 2. Compile Vertex Shader
    vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vCode, nullptr);
    glCompileShader(vShader);

    // Check for compilation errors
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 3. Compile Fragment Shader
    fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fCode, nullptr);
    glCompileShader(fShader);

    // Check for compilation errors
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 4. Link Program
    ID = glCreateProgram();
    glAttachShader(ID, vShader);
    glAttachShader(ID, fShader);
    glLinkProgram(ID);

    // Check for linking errors
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // 5. Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

void Shader::use() const
{
    glUseProgram(ID);
}

void Shader::setMat4(const std::string& name, const float* value) const
{
    // Using glGetUniformLocation every frame can be slow in large engines, 
    // but for this assignment, it is perfectly fine.
    unsigned int loc = glGetUniformLocation(ID, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    unsigned int loc = glGetUniformLocation(ID, name.c_str());
    glUniform3f(loc, x, y, z);
}