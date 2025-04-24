#include <iostream>
#include <vector>
#include <string>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other includes
#include "Shader.h"
#include "Camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/constants.hpp>


#define PI 3.1415926535

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Window dimensions
const GLuint WIDTH = 1920, HEIGHT = 1080;

class CScene {
public:
    enum {
        step = 8,
        numberOfVertices = ((180 / step) + 1) * ((360 / step) + 1),
        numberOfIndices = 2 * (numberOfVertices - (360 / step) - 1)
    };

    struct {
        GLfloat x, y, z;               // координаты точки
        //GLfloat u, v;                // текстурные координаты
        GLfloat xn, yn, zn;            // вектора нормали
    } m_vertices[numberOfVertices];    // массив вершин

    GLuint m_indices[numberOfIndices]; // массив индексов

    CScene(void);
};

CScene::CScene(void) {
    const GLfloat fRadius = 1.0;
    for (int alpha = 90, index = 0; alpha <= 270; alpha += step) {
        const double angleOfVertical = (alpha % 360) * PI / 180;
        for (int phi = 0; phi <= 360; phi += step, ++index) {
            const double angleOfHorizontal = (phi % 360) * PI / 180;

            m_vertices[index].x = fRadius * cos(angleOfVertical) * cos(angleOfHorizontal);
            m_vertices[index].y = fRadius * sin(angleOfVertical);
            m_vertices[index].z = fRadius * cos(angleOfVertical) * sin(angleOfHorizontal);

            float length = sqrt(
                m_vertices[index].x * m_vertices[index].x +
                m_vertices[index].y * m_vertices[index].y +
                m_vertices[index].z * m_vertices[index].z
            );
            m_vertices[index].xn = m_vertices[index].x / length;            
            m_vertices[index].yn = m_vertices[index].y / length;            
            m_vertices[index].zn = m_vertices[index].z / length;
            
        }
    }

    int index = 0;
    int rows = 180 / step;
    int cols = 360 / step + 1;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col <= 360 / step; col++) {
            // Добавляем две вершины - одну из текущего кольца, одну из следующего
            m_indices[index++] = row * cols + col;
            m_indices[index++] = (row + 1) * cols + col;
        }
    }
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

glm::vec3 lightPos(12.6f, 0.0f, 0.0f);

const float EARTH_RADIUS_KM = 6371.0f;  // Радиус Земли в км
const float EARTH_SPIN_TIME = 86164.0905f;
// Пример TLE для спутника (например, МКС)
std::string tle_line1 = "1 25544U 98067A   22348.84664469  .00001402  00000+0  31829-4 0  9994";
std::string tle_line2 = "2 25544  51.6423 318.7503 0006835  36.0298 324.1326 15.50086584378600";

int main(){
    double orbitDuration = 5400;   // продолжительность орбиты в секундах (примерно 90 минут)
    double dt = 10;  // шаг времени в секундах

    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Options
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    glewInit();

    // Define the viewport dimensions
    glViewport(0, 0, WIDTH, HEIGHT);

    // Setup OpenGL options
    glEnable(GL_DEPTH_TEST);

    // Build and compile our shader program
    Shader ourShader("D:/OpenGL_Projects/satellite_tracker/default.vs", "D:/OpenGL_Projects/satellite_tracker/default.frag");
    Shader lightShader("D:/OpenGL_Projects/satellite_tracker/light.vs", "D:/OpenGL_Projects/satellite_tracker/light.frag");
    Shader skyboxShader("D:/OpenGL_Projects/satellite_tracker/skybox.vs", "D:/OpenGL_Projects/satellite_tracker/skybox.frag");
    Shader satelliteShader("D:/OpenGL_Projects/satellite_tracker/satellite.vs", "D:/OpenGL_Projects/satellite_tracker/satellite.frag");

    float satelliteVertices[] = {
        // Нижнее основание (нижняя грань)
        -0.5f, -0.5f, -0.5f,  // v0
         0.5f, -0.5f, -0.5f,  // v1
         0.5f, -0.5f,  0.5f,  // v2
        -0.5f, -0.5f,  0.5f,  // v3

        // Верхнее основание (верхняя грань)
        -0.5f,  0.5f, -0.5f,  // v4
         0.5f,  0.5f, -0.5f,  // v5
         0.5f,  0.5f,  0.5f,  // v6
        -0.5f,  0.5f,  0.5f   // v7
    };

    unsigned int satelliteIndices[] = {
        // Нижняя грань
        0, 1, 2,
        2, 3, 0,

        // Верхняя грань
        4, 5, 6,
        6, 7, 4,

        // Передняя грань
        3, 2, 6,
        6, 7, 3,

        // Задняя грань
        0, 1, 5,
        5, 4, 0,

        // Левая грань
        0, 3, 7,
        7, 4, 0,

        // Правая грань
        1, 2, 6,
        6, 5, 1
    };

    CScene earth;
    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(earth.m_vertices), earth.m_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(earth.m_indices), earth.m_indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    GLuint satVBO, satVAO, satEBO;
    glGenVertexArrays(1, &satVAO);
    glGenBuffers(1, &satVBO);
    glGenBuffers(1, &satEBO);
    glBindVertexArray(satVAO);
    glBindBuffer(GL_ARRAY_BUFFER, satVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(satelliteVertices), satelliteVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, satEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(satelliteIndices), satelliteIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    GLuint lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(earth.m_vertices), earth.m_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(earth.m_indices), earth.m_indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    std::vector<std::string> faces{
            "Earth/posx.jpg",
            "Earth/negx.jpg",
            "Earth/posy.jpg",
            "Earth/negy.jpg",
            "Earth/posz.jpg",
            "Earth/negz.jpg"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    std::vector<std::string> skyfaces = {
            "skybox/right.jpg", 
            "skybox/left.jpg", 
            "skybox/top.jpg", 
            "skybox/bottom.jpg", 
            "skybox/front.jpg", 
            "skybox/back.jpg"
    };
    unsigned int skyTexture = loadCubemap(skyfaces);

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };   

    GLuint skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)){
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;        
        glfwPollEvents();
        Do_Movement();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.Use();
        GLint lightPosLoc = glGetUniformLocation(ourShader.Program, "lightPos");
        GLint objectColorLoc = glGetUniformLocation(ourShader.Program, "objectColor");
        GLint lightColorLoc = glGetUniformLocation(ourShader.Program, "lightColor");
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(objectColorLoc, 1.0f, 0.0f, 0.0f);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();        
        glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);        
        //model = glm::rotate(model, 23.4f, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, -90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        //glm::radians(180.0f)
        //model = glm::rotate(model, (GLfloat)glfwGetTime() * 12, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f));
        GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
        GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
        GLint projLoc = glGetUniformLocation(ourShader.Program, "projection");        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawElements(GL_TRIANGLE_STRIP, CScene::numberOfIndices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);        

        satelliteShader.Use();
        modelLoc = glGetUniformLocation(lightShader.Program, "model");
        viewLoc = glGetUniformLocation(lightShader.Program, "view");
        projLoc = glGetUniformLocation(lightShader.Program, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.2f));
        //model = glm::translate(model, glm::vec3(0.573, 0.873, 0.299));
        model = glm::translate(model, glm::vec3(0.302f, 0.872f, -0.573f));
        model = glm::scale(model, glm::vec3(0.01f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(satVAO);
        glDrawElements(GL_TRIANGLE_STRIP, 36, GL_UNSIGNED_INT, 0);
        //glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);      


        lightShader.Use();
        lightPosLoc = glGetUniformLocation(lightShader.Program, "lightPos");
        modelLoc = glGetUniformLocation(lightShader.Program, "model");
        viewLoc = glGetUniformLocation(lightShader.Program, "view");
        projLoc = glGetUniformLocation(lightShader.Program, "projection");
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        model = glm::mat4();
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.05f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(lightVAO);        
        glDrawElements(GL_TRIANGLE_STRIP, CScene::numberOfIndices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glDepthFunc(GL_LEQUAL);
        skyboxShader.Use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // Уберите трансформацию позиции
        projection = glm::perspective(45.0f, (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

void Do_Movement()
{
    // Camera controls
    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    //cout << key << endl;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}