#include <iostream>
#include <vector>
#include <string>
#include <thread>   // Для std::this_thread
#include <chrono>
#include <iomanip>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other includes
#include "Shader.h"
#include "Camera.h"
#include "Satpredictor.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/constants.hpp>
#include <date.h>


#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

using namespace date;
using namespace std::chrono;

struct DateTimeResult {
    std::string formatted;   // Готовый строковый формат
    std::tm time_struct;      // Стандартная структура времени
    int milliseconds;         // Миллисекунды
    double total_seconds;     // Общее время в секундах с эпохи Unix
};

#define PI 3.1415926535

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();
DateTimeResult convertTLEEpoch(double tle_epoch);
DateTimeResult convertSecondsToDateTime(double seconds);
std::string tle_epoch_to_datetime(double tle_epoch);

bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Camera
Camera camera(glm::vec3(1.0f, 0.0f, 0.0f));
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

const float EARTH_SPIN_TIME = 86164.0905f;

float simulationTime = 0.0f;  // Время симуляции (не зависит от скорости)
float prevAnimationSpeed = 1.0f;
bool speedChanged = false;
float animationSpeed = 10.0f;
bool is_paused = false;
bool cameraMode = true; // true - камера управляется мышью, false - курсор виден
bool uiMode = false;    // Дополнительный флаг для режима UI (если нужно)

int main() {    
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

    //imgui initzialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    // Setup OpenGL options
    glEnable(GL_DEPTH_TEST);

    // Build and compile our shader program
    Shader ourShader("D:/OpenGL_Projects/satellite_tracker/default.vs", "D:/OpenGL_Projects/satellite_tracker/default.frag");
    Shader lightShader("D:/OpenGL_Projects/satellite_tracker/light.vs", "D:/OpenGL_Projects/satellite_tracker/light.frag");
    Shader skyboxShader("D:/OpenGL_Projects/satellite_tracker/skybox.vs", "D:/OpenGL_Projects/satellite_tracker/skybox.frag");
    Shader satelliteShader("D:/OpenGL_Projects/satellite_tracker/satellite.vs", "D:/OpenGL_Projects/satellite_tracker/satellite.frag");
    Shader orbitShader("D:/OpenGL_Projects/satellite_tracker/orbit.vs", "D:/OpenGL_Projects/satellite_tracker/orbit.frag");

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

    SatellitePredictor iss(
        15.49604105510665,  // meanMotion (оборотов/сутки)
        0.0002193,    // eccentricity
        51.6355 ,      // inclination (градусы)
        124.9576,     // argumentOfPerigee (градусы)
        90.7571,     // rightAscension (градусы)
        235.1619      // meanAnomaly (градусы)
    );

    // Прогнозируем позицию через 1.5 часа (5400 секунд)
    std::vector<glm::vec3> PosSize[24000];
    std::vector<glm::vec3> orbitPoints;
    for (size_t i = 0; i != 24000; ++i) {
        orbitPoints.push_back(iss.toGLMCoordinates(iss.predictPositionECEF(i)));
    }
    ////////////////////////////////////////////////////////////////

    GLuint orbitVAO, orbitVBO;

    glGenVertexArrays(1, &orbitVAO);
    glGenBuffers(1, &orbitVBO);
    glBindVertexArray(orbitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
    glBufferData(GL_ARRAY_BUFFER, orbitPoints.size() * sizeof(glm::vec3), orbitPoints.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)) {
        ImGuiIO& io = ImGui::GetIO();
        if (cameraMode) {
            io.WantCaptureMouse = false; // Позволяем камере получать события мыши
        }
        else {
            io.WantCaptureMouse = true; // Даем ImGui управление курсором
        }
        
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

        if (!is_paused) {
            // Плавное изменение скорости
            if (speedChanged) {
                simulationTime += deltaTime * prevAnimationSpeed;
                prevAnimationSpeed = animationSpeed;
                speedChanged = false;
            }
            else {
                simulationTime += deltaTime * animationSpeed;
            }
        }

        satelliteShader.Use();       
        float currentTime = glfwGetTime();
        float orbitDuration = 24000; // Период обращения в секундах
        float normalizedTime = fmod(simulationTime, orbitDuration);
        glm::vec3 currentPosition = iss.toGLMCoordinates(iss.predictPositionECEF(normalizedTime));
        modelLoc = glGetUniformLocation(lightShader.Program, "model");
        viewLoc = glGetUniformLocation(lightShader.Program, "view");
        projLoc = glGetUniformLocation(lightShader.Program, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glm::mat4 satmodel = glm::mat4(1.0f);
        satmodel = glm::rotate(satmodel, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        satmodel = glm::scale(satmodel, glm::vec3(0.2f));
        satmodel = glm::translate(satmodel, currentPosition);
        //satmodel = glm::translate(satmodel, glm::vec3(0.0f, 0.0f, 1.0f));
        satmodel = glm::scale(satmodel, glm::vec3(0.01f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(satmodel));
        glBindVertexArray(satVAO);
        glDrawElements(GL_TRIANGLE_STRIP, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        //////////////////

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Создаем UI-окно с элементами управления
        ImGui::Begin("Time Control");
        {
            double tle_epoch = 25139.18441541;
            /*auto dt = convertTLEEpoch(tle_epoch);
            auto dt2 = convertSecondsToDateTime(dt.total_seconds);
            ImGui::Text("TLE Time: %s", dt2.formatted.c_str());*/

            std::string datetime_str = tle_epoch_to_datetime(tle_epoch);
            ImGui::Text("TLE Time: %s", datetime_str.c_str());

            ImGui::TextColored(ImVec4(0, 1, 0, 1), "X: %.3f km", currentPosition.x * EARTH_RADIUS_KM);
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y: %.3f km", currentPosition.y * EARTH_RADIUS_KM);
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Z: %.3f km", currentPosition.z * EARTH_RADIUS_KM);

            // Визуальный разделитель
            ImGui::Separator();

            // Дополнительная информация
            ImGui::Text("Earth-Centered, Earth-Fixed");
            ImGui::Text("Units: Kilometers");

            if (ImGui::Button(is_paused ? "Resume" : "Pause")) {
                switch (is_paused) {
                case false:
                    animationSpeed = 0.0f;
                    is_paused = !is_paused;
                    break;
                case true:
                    animationSpeed = 10.0f;
                    is_paused = false;
                    break;
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Speed x2") && !is_paused) {
                prevAnimationSpeed = animationSpeed;
                animationSpeed *= 2.0f;
                speedChanged = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Speed /2") && !is_paused) {
                prevAnimationSpeed = animationSpeed;
                animationSpeed *= 0.5f;
                speedChanged = true;
            }

            ImGui::Text("Current speed: %.1fx", is_paused ? 0.0f : animationSpeed);
            ImGui::Text("Orbit time: %.1f / %.1f sec", fmod(simulationTime, orbitDuration), orbitDuration);
        }
        ImGui::End();

        /////////////////

        orbitShader.Use();
        glm::mat4 orbitModel = glm::mat4(1.0f);
        orbitModel = glm::scale(orbitModel, glm::vec3(0.2f));
        orbitModel = glm::rotate(orbitModel, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(orbitShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(orbitModel));
        glUniformMatrix4fv(glGetUniformLocation(orbitShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(orbitShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(orbitShader.Program, "lineColor"), 0.5f, 0.8f, 1.0f);
        glBindVertexArray(orbitVAO);
        glDrawArrays(GL_LINE_STRIP, 0, orbitPoints.size());
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
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); 
        projection = glm::perspective(45.0f, (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        cameraMode = !cameraMode;
        // Переключаем режим курсора
        glfwSetInputMode(window, GLFW_CURSOR,
            cameraMode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        // Костыль для предотвращения мгновенного переключения обратно
        //std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
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
    if (!cameraMode) return;
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

DateTimeResult convertTLEEpoch(double tle_epoch) {
    DateTimeResult result;

    // 1. Разбор входных данных
    int full_days = static_cast<int>(tle_epoch);
    double fractional_day = tle_epoch - full_days;

    // 2. Извлечение года и дня года
    int year = 2000 + (full_days / 1000);
    int day_of_year = full_days % 1000;

    // 3. Преобразование дня года в дату
    std::tm tm = { 0 };
    tm.tm_year = year - 1900;
    tm.tm_mday = day_of_year;

    mktime(&tm);  // Нормализация структуры времени

    // 4. Преобразование дробной части дня
    double total_day_seconds = fractional_day * 86400.0;
    result.milliseconds = static_cast<int>((total_day_seconds - floor(total_day_seconds)) * 1000);

    tm.tm_hour = static_cast<int>(total_day_seconds / 3600);
    tm.tm_min = static_cast<int>(fmod(total_day_seconds, 3600) / 60);
    tm.tm_sec = static_cast<int>(fmod(total_day_seconds, 60));

    // 5. Вычисление общего времени в секундах
    time_t base_seconds = mktime(&tm);
    result.total_seconds = static_cast<double>(base_seconds) + (result.milliseconds / 1000.0);

    // 6. Форматирование строки
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    result.formatted = std::string(buffer) + "." + std::to_string(result.milliseconds).substr(0, 3);
    result.time_struct = tm;

    return result;
}

// Функция преобразования секунд Unix в структуру времени без gmtime
DateTimeResult convertSecondsToDateTime(double seconds) {
    DateTimeResult result;
    result.total_seconds = seconds;

    // 1. Разделение на целые секунды и миллисекунды
    time_t total_sec = static_cast<time_t>(seconds);
    result.milliseconds = static_cast<int>((seconds - total_sec) * 1000);

    // 2. Ручной расчёт компонентов времени UTC (начиная с 1970-01-01)
    const int SEC_PER_MIN = 60;
    const int SEC_PER_HOUR = 3600;
    const int SEC_PER_DAY = 86400;
    const int DAYS_PER_YEAR = 365;

    int days = total_sec / SEC_PER_DAY;
    int rem_sec = total_sec % SEC_PER_DAY;

    // Вычисление часов, минут, секунд
    result.time_struct.tm_hour = rem_sec / SEC_PER_HOUR;
    rem_sec %= SEC_PER_HOUR;
    result.time_struct.tm_min = rem_sec / SEC_PER_MIN;
    result.time_struct.tm_sec = rem_sec % SEC_PER_MIN;

    // Вычисление года и дня года
    int year = 1970;
    while (days >= DAYS_PER_YEAR + is_leap_year(year)) {
        days -= DAYS_PER_YEAR + is_leap_year(year);
        year++;
    }
    result.time_struct.tm_year = year - 1900;

    // Вычисление месяца и дня месяца
    static int month_days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (is_leap_year(year)) month_days[1] = 29;

    int month = 0;
    while (days >= month_days[month]) {
        days -= month_days[month];
        month++;
    }
    result.time_struct.tm_mon = month;
    result.time_struct.tm_mday = days + 1; // Дни начинаются с 1

    // 3. Форматирование строки
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &result.time_struct);
    result.formatted = std::string(buffer) + "." +
        std::to_string(result.milliseconds).substr(0, 3);

    return result;
}

std::string tle_epoch_to_datetime(double tle_epoch) {
    using namespace date;
    using namespace std::chrono;

    double integral;
    double fractional = modf(tle_epoch, &integral);

    int epoch_int = static_cast<int>(integral);
    int year = 2000 + (epoch_int / 1000);
    int day_of_year = epoch_int % 1000;

    // Создание даты через sys_days
    auto ymd = year_month_day{
        sys_days(date::year(year) / 1 / 1) + // 1 января указанного года
        days(day_of_year - 1)            // добавляем N-1 дней
    };

    // Преобразование дробной части в миллисекунды
    auto day_duration = duration_cast<milliseconds>(duration<double>(fractional * 86400));
    hh_mm_ss<milliseconds> time(day_duration);

    std::ostringstream oss;
    oss << ymd << " " << time;
    return oss.str();
}