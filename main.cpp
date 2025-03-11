#include <iostream>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other includes
#include "Shader.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/constants.hpp>

#define PI 3.1415926535

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Window dimensions
const GLuint WIDTH = 1920, HEIGHT = 1080;

class CScene {
public:
    enum {
        step = 4,
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
    // Вычисляем вершины сферы и заносим их в массив
    const GLfloat fRadius = 1.0;
    for (int alpha = 90, index = 0; alpha <= 270; alpha += step) {
        const double angleOfVertical = (alpha % 360) * PI / 180;
        for (int phi = 0; phi <= 360; phi += step, ++index) {
            const double angleOfHorizontal = (phi % 360) * PI / 180;

            // Координаты вершины
            m_vertices[index].x = fRadius * cos(angleOfVertical) * cos(angleOfHorizontal);
            m_vertices[index].y = fRadius * sin(angleOfVertical);
            m_vertices[index].z = fRadius * cos(angleOfVertical) * sin(angleOfHorizontal);

            // Координаты текстуры
            //m_vertices[index].u = (360 - phi) / 360.0f;
            //m_vertices[index].v = (270 - alpha) / 180.0f;

            // Вектор нормали (совпадает с направлением вершины от центра сферы)
            float length = sqrt(
                m_vertices[index].x * m_vertices[index].x +
                m_vertices[index].y * m_vertices[index].y +
                m_vertices[index].z * m_vertices[index].z
            );
            m_vertices[index].xn = m_vertices[index].x / length;
            //std::cout << "xn[" << index << "] = " << m_vertices[index].xn << std::endl;
            m_vertices[index].yn = m_vertices[index].y / length;
            //std::cout << "yn[" << index << "] = " << m_vertices[index].yn << std::endl;
            m_vertices[index].zn = m_vertices[index].z / length;
            //std::cout << "zn[" << index << "] = " << m_vertices[index].zn << std::endl << std::endl;
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

glm::vec3 lightPos(0.5f, 0.0f, 1.5f);

int main(){
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);

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

    

   

    while (!glfwWindowShouldClose(window)){
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     

        // Activate shader
        ourShader.Use();

        GLint lightPosLoc = glGetUniformLocation(ourShader.Program, "lightPos");
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);

        GLint objectColorLoc = glGetUniformLocation(ourShader.Program, "objectColor");
        GLint lightColorLoc = glGetUniformLocation(ourShader.Program, "lightColor");
        glUniform3f(objectColorLoc, 1.0f, 0.0f, 0.0f);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view;
        glm::mat4 projection;

        
        model = glm::rotate(model, (GLfloat)glfwGetTime() * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));


        //model = glm::rotate(model, (GLfloat)glfwGetTime() * 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
        projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
        GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
        GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
        GLint projLoc = glGetUniformLocation(ourShader.Program, "projection");        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(VAO);
        glDrawElements(GL_POINTS, CScene::numberOfIndices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        lightShader.Use();

        lightPosLoc = glGetUniformLocation(lightShader.Program, "lightPos");
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);

        modelLoc = glGetUniformLocation(lightShader.Program, "model");
        viewLoc = glGetUniformLocation(lightShader.Program, "view");
        projLoc = glGetUniformLocation(lightShader.Program, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        model = glm::mat4();
        //model = glm::rotate(model, (GLfloat)glfwGetTime() * 30.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(lightVAO);
        glDrawElements(GL_TRIANGLE_STRIP, CScene::numberOfIndices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }
    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode){
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}