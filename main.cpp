#include <iostream>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other includes
#include "Shader.h"
#include <stb/stb_image.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/constants.hpp>


// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Window dimensions
const GLuint WIDTH = 1920, HEIGHT = 1080;

class CScene {
public:
    enum {
        step = 2,
        numberOfVertices = ((180 / step) + 1) * ((360 / step) + 1),
        // Обновите количество индексов для GL_QUAD_STRIP
        //numberOfIndices = 4 * (180 / step) * (360 / step)
        numberOfIndices = 2 * (numberOfVertices - (360 / step) - 1)
    }; // enum

    struct {
        GLfloat x, y, z; // координаты точки
    } m_vertices[numberOfVertices]; // массив вершин

    GLuint m_indices[numberOfIndices]; // массив индексов

    CScene(void);
}; // class CScene

CScene::CScene(void) {
    /// вычисляем вершины сферы и заносим их в массив
    const GLfloat fRadius = 1.0;
    for (int alpha = 90, index = 0; alpha <= 270; alpha += step) {
        const int angleOfVertical = alpha % 360;
        for (int phi = 0; phi <= 360; phi += step, ++index) {
            const int angleOfHorizontal = phi % 360;
            /// вычисляем координаты точки
            m_vertices[index].x = fRadius * cos(angleOfVertical) * cos(angleOfHorizontal);
            m_vertices[index].y = fRadius * sin(angleOfVertical);
            m_vertices[index].z = fRadius * cos(angleOfVertical) * sin(angleOfHorizontal);
        } // for
    } // for

    /// вычисляем индексы вершин и заносим их в массив
    for (int index = 0; index < numberOfIndices; index += 2) {
        m_indices[index] = index >> 1;
        m_indices[index + 1] = m_indices[index] + (360 / step) + 1;
    } // for

} // constructor CScene

// The MAIN function, from here we start the application and run the game loop
int main()
{
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
    glewInit();

    // Define the viewport dimensions
    glViewport(0, 0, WIDTH, HEIGHT);

    // Setup OpenGL options
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Build and compile our shader program
    Shader ourShader("D:/OpenGL_Projects/Project_1_HABR/Project_1_HABR/default.vs", "D:/OpenGL_Projects/Project_1_HABR/Project_1_HABR/default.frag");

    // Set up vertex data (and buffer(s)) and attribute pointers

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

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);  

    glBindVertexArray(0); // Unbind VAO

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();

        // Render
        // Clear the colorbuffer
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        ourShader.Use();
        // Create transformations
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        model = glm::rotate(model, (GLfloat)glfwGetTime() * 10.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
        //view = glm::lookAt(cameraPos, cameraTarget, upVector);
        // Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
        projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
        // Get their uniform location
        GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
        GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
        GLint projLoc = glGetUniformLocation(ourShader.Program, "projection");
        // Pass them to the shaders
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw container
        glBindVertexArray(VAO);
        glDrawElements(GL_POINTS, sizeof(earth.m_indices), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Swap the screen buffers
        glfwSwapBuffers(window);
    }
    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}