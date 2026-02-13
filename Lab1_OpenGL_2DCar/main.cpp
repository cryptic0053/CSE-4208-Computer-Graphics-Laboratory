#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>   
#include <sstream>   

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 1. HELPER FUNCTION: READ FILE
std::string readFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "ERROR::FILE_NOT_FOUND: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 2. SHADER CLASS
class SimpleShader {
public:
    unsigned int ID;

    
    SimpleShader(const char* vertexPath, const char* fragmentPath) {

        // 1. Read code from files
        std::string vCodeStr = readFile(vertexPath);
        std::string fCodeStr = readFile(fragmentPath);

        const char* vShaderCode = vCodeStr.c_str();
        const char* fShaderCode = fCodeStr.c_str();

        // 2. Compile Vertex Shader
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // 3. Compile Fragment Shader
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // 4. Link Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // 5. Cleanup
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() {
        glUseProgram(ID);
    }

    void setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setMat4(const std::string& name, const float* value) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, value);
    }

private:
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};


// 3. GEOMETRY DATA
static float rectVertices[] = {
    -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f
};

static float triVertices[] = {
    -0.5f, -0.5f,  0.5f,  0.0f, -0.5f,  0.5f
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}


// 4. MAIN FUNCTION
int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(900, 600, "Lab 1 - 2D Plane", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // LOAD SHADERS FROM FILES HERE
    SimpleShader shader("plane_vertex.glsl", "plane_fragment.glsl");

    // Setup VAOs
    unsigned int rectVAO, rectVBO, triVAO, triVBO;

    // Rectangle
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Triangle
    glGenVertexArrays(1, &triVAO);
    glGenBuffers(1, &triVBO);
    glBindVertexArray(triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVertices), triVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Variables
    float planeX = 0.0f, planeY = 0.0f;
    float planeScale = 0.5f;
    float planeAngle = 0.0f;
    float propellerAngle = 0.0f;
    float flapAngle = 0.0f;

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window))
    {
        // INPUT
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

        bool isMoving = false;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { planeX -= 0.001f; isMoving = true; }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { planeX += 0.001f; isMoving = true; }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { planeY += 0.001f; isMoving = true; }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { planeY -= 0.001f; isMoving = true; }

        if (isMoving) propellerAngle -= 15.0f;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) planeAngle += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) planeAngle -= 0.01f;

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) planeScale += 0.002f;
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) planeScale -= 0.002f;
        if (planeScale < 0.1f) planeScale = 0.1f;

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) propellerAngle -= 15.0f;
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) flapAngle += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) flapAngle -= 0.01f;

		

        // RENDER
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();

        glm::mat4 globalPlane = glm::mat4(1.0f);
        globalPlane = glm::translate(globalPlane, glm::vec3(planeX, planeY, 0.0f));
        globalPlane = glm::rotate(globalPlane, planeAngle, glm::vec3(0, 0, 1));
        globalPlane = glm::scale(globalPlane, glm::vec3(planeScale, planeScale, 1.0f));

        // 1. FUSELAGE
        glm::mat4 body = glm::scale(globalPlane, glm::vec3(1.5f, 0.3f, 1.0f));
        shader.setMat4("uTransform", glm::value_ptr(body));
        shader.setVec3("uColor", 0.8f, 0.2f, 0.2f);
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 2. COCKPIT
        glm::mat4 windowPart = glm::translate(globalPlane, glm::vec3(0.2f, 0.15f, 0.0f));
        windowPart = glm::scale(windowPart, glm::vec3(0.4f, 0.2f, 1.0f));
        shader.setMat4("uTransform", glm::value_ptr(windowPart));
        shader.setVec3("uColor", 0.6f, 0.8f, 1.0f);
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 3. TAIL
        glm::mat4 tail = glm::translate(globalPlane, glm::vec3(-0.70f, 0.25f, 0.0f));
        tail = glm::rotate(tail, glm::radians(90.0f), glm::vec3(0, 0, 1));
        tail = glm::scale(tail, glm::vec3(0.4f, 0.5f, 1.0f));
        shader.setMat4("uTransform", glm::value_ptr(tail));
        shader.setVec3("uColor", 0.6f, 0.6f, 0.6f);
        glBindVertexArray(triVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 4. NOSE
        glm::mat4 nose = glm::translate(globalPlane, glm::vec3(0.9f, 0.0f, 0.0f));
        nose = glm::scale(nose, glm::vec3(0.3f, 0.3f, 1.0f));
        shader.setMat4("uTransform", glm::value_ptr(nose));
        shader.setVec3("uColor", 0.9f, 0.9f, 0.9f);
        glBindVertexArray(triVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 5. WING
        glm::mat4 wing = glm::translate(globalPlane, glm::vec3(0.1f, -0.05f, 0.0f));
        wing = glm::scale(wing, glm::vec3(0.6f, 0.15f, 1.0f));
        shader.setMat4("uTransform", glm::value_ptr(wing));
        shader.setVec3("uColor", 0.5f, 0.5f, 0.5f);
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 6. FLAP
        glm::mat4 flap = glm::translate(globalPlane, glm::vec3(-0.2f, -0.05f, 0.0f));
        flap = glm::rotate(flap, flapAngle, glm::vec3(0, 0, 1));
        flap = glm::translate(flap, glm::vec3(-0.1f, 0.0f, 0.0f));
        flap = glm::scale(flap, glm::vec3(0.2f, 0.1f, 1.0f));
        shader.setMat4("uTransform", glm::value_ptr(flap));
        shader.setVec3("uColor", 0.2f, 0.8f, 0.2f);
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 7. PROPELLER
        glm::mat4 prop = glm::translate(globalPlane, glm::vec3(1.05f, 0.0f, 0.0f));
        prop = glm::rotate(prop, glm::radians(propellerAngle), glm::vec3(0, 0, 1));
        prop = glm::scale(prop, glm::vec3(0.05f, 1.1f, 1.0f));
        shader.setMat4("uTransform", glm::value_ptr(prop));
        shader.setVec3("uColor", 0.1f, 0.1f, 0.1f);
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &rectVAO);
    glDeleteVertexArrays(1, &triVAO);
    glDeleteBuffers(1, &rectVBO);
    glDeleteBuffers(1, &triVBO);
    glfwTerminate();
    return 0;
}