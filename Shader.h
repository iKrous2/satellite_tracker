#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h> // ���������� glew ��� ����, ����� �������� ��� ����������� ������������ ����� OpenGL
#include <glm/glm/gtc/type_ptr.hpp> // ��� glm::value_ptr

class Shader
{
public:
    // ������������� ���������
    GLuint Program;
    // ����������� ��������� � �������� ������
    Shader(const GLchar* vertexPath, const GLchar* fragmentPath);
    // ������������� ���������
    void Use();

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(Program, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
};

Shader::Shader(const GLchar* vertexPath, const GLchar* fragmentPath){
    // 1. �������� �������� ��� ������� �� filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // �������������, ��� ifstream ������� ����� ���������� ����������
    vShaderFile.exceptions(std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::badbit);
    try{
        // ��������� �����
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // ��������� ������ � ������
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // ��������� �����
        vShaderFile.close();
        fShaderFile.close();
        // ��������������� ������ � ������ GLchar
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure e){
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    const GLchar* vShaderCode = vertexCode.c_str();
    const GLchar* fShaderCode = fragmentCode.c_str();

    // 2. ������ ��������
    GLuint vertex, fragment;
    GLint success;
    GLchar infoLog[512];

    // ��������� ������
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // ���� ���� ������ - ������� ��
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    };

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    // ���� ���� ������ - ������� ��
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    };

    // ��������� ���������
    this->Program = glCreateProgram();
    glAttachShader(this->Program, vertex);
    glAttachShader(this->Program, fragment);
    glLinkProgram(this->Program);
    //���� ���� ������ - ������� ��
    glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // ������� �������, ��������� ��� ��� � ��������� � ��� ������ �� �����.
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::Use() { glUseProgram(this->Program); }
#endif
