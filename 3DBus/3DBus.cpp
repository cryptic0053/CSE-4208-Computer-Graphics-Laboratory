#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>

// Screen settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;


// SHADERS
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 objectColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(objectColor, 1.0f);\n"
"}\n\0";

// GLOBAL VARIABLES
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- BUS DRIVING STATE ---
glm::vec3 busPos = glm::vec3(0.0f, 0.0f, 0.0f);
float busAngle = 0.0f; // yaw degrees

// --- CAMERA STATE (Flying simulator + pitch/yaw/roll) ---
glm::vec3 camPos = glm::vec3(0.0f, 7.0f, 18.0f);
glm::vec3 camFront = glm::vec3(0.0f, -0.15f, -1.0f);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 camUp = worldUp;
glm::vec3 camRight = glm::normalize(glm::cross(camFront, worldUp));

// Euler-like angles for camera control
float camYaw = -90.0f; // facing -Z initially
float camPitch = -10.0f;
float camRoll = 0.0f;

// Orbit & Bird eye toggles
bool orbitMode = false;
float orbitAngle = 0.0f;
float orbitRadius = 18.0f;

bool birdEyeMode = false;

// --- Animation States ---
bool fanOn = false;
float fanAngle = 0.0f;

bool doorOpen = false;
float doorAngle = 0.0f;

// Toggle key guards
bool gKeyPressed = false;
bool oKeyPressed = false;
bool bKeyPressed = false;
bool fKeyPressed = false;

// CUBE DATA
float vertices[] = {
     0.5f,  0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,

     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f
};

unsigned int indices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    7, 3, 0, 0, 4, 7,
    6, 2, 1, 1, 5, 6,
    0, 1, 5, 5, 4, 0,
    3, 2, 6, 6, 7, 3
};

// Forward decl
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
unsigned int createShader(const char* vShader, const char* fShader);

void updateCameraVectors(); // recompute camFront, camRight, camUp

//DRAW HELPERS
void drawCube(unsigned int VAO,
    glm::mat4 model,
    int modelLoc,
    int colorLoc,
    glm::vec3 scale,
    glm::vec3 color)
{
    glm::mat4 m = glm::scale(model, scale);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
    glUniform3f(colorLoc, color.x, color.y, color.z);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void drawWheelFakeCylinder(unsigned int VAO,
    const glm::mat4& base,
    int modelLoc,
    int colorLoc,
    float radius,
    float width,
    glm::vec3 color)
{
    const int slices = 10;
    for (int i = 0; i < slices; i++) {
        float a = (float)i / (float)slices * 2.0f * 3.1415926f;
        glm::mat4 m = base;
        m = glm::rotate(m, a, glm::vec3(1, 0, 0));          // around axle
        m = glm::translate(m, glm::vec3(0.0f, radius, 0.0f));
        drawCube(VAO, m, modelLoc, colorLoc,
            glm::vec3(width, radius * 0.25f, radius * 0.25f),
            color);
    }
}

// =========================================================================
// MAIN
// =========================================================================
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment B2: 3D Bus (Full Controls)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int shaderProgram = createShader(vertexShaderSource, fragmentShaderSource);

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    // init camera vectors from angles
    updateCameraVectors();

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // --- ANIMATION UPDATE ---
        if (fanOn) {
            fanAngle += 360.0f * deltaTime;
            if (fanAngle > 360.0f) fanAngle -= 360.0f;
        }

        float doorSpeed = 120.0f;
        if (doorOpen && doorAngle < 75.0f) doorAngle += doorSpeed * deltaTime;
        if (!doorOpen && doorAngle > 0.0f) doorAngle -= doorSpeed * deltaTime;

        // ---- CAMERA MODES ----
        glm::vec3 target = busPos + glm::vec3(0, 0.8f, 0);

        if (birdEyeMode) {
            // fixed top view
            camPos = busPos + glm::vec3(0.0f, 22.0f, 0.01f);
            camFront = glm::normalize(target - camPos);
            camRight = glm::normalize(glm::cross(camFront, worldUp));
            camUp = glm::normalize(glm::cross(camRight, camFront));
        }
        else if (orbitMode) {
            orbitAngle += 35.0f * deltaTime; // orbit speed
            float rad = glm::radians(orbitAngle);

            camPos.x = target.x + orbitRadius * cos(rad);
            camPos.z = target.z + orbitRadius * sin(rad);
            camPos.y = target.y + 7.0f;

            camFront = glm::normalize(target - camPos);
            camRight = glm::normalize(glm::cross(camFront, worldUp));
            camUp = glm::normalize(glm::cross(camRight, camFront));

            // roll still works visually if you want:
            if (fabs(camRoll) > 0.0001f) {
                glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::radians(camRoll), camFront);
                camUp = glm::normalize(glm::vec3(r * glm::vec4(camUp, 0.0f)));
                camRight = glm::normalize(glm::cross(camFront, camUp));
            }
        }
        else {
            
        }

        // RENDER
        glClearColor(0.80f, 0.90f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 300.0f);

        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(VAO);


        // BUS MASTER MATRIX
        glm::mat4 busMatrix = glm::mat4(1.0f);
        busMatrix = glm::translate(busMatrix, busPos);
        busMatrix = glm::rotate(busMatrix, glm::radians(busAngle), glm::vec3(0, 1, 0));

        // Colors
        glm::vec3 bodyColor = glm::vec3(1.0f, 0.45f, 0.05f);
        glm::vec3 roofColor = glm::vec3(0.95f, 0.95f, 0.95f);
        glm::vec3 glassColor = glm::vec3(0.10f, 0.20f, 0.30f);
        glm::vec3 trimColor = glm::vec3(0.15f, 0.15f, 0.15f);
        glm::vec3 lightYellow = glm::vec3(1.00f, 0.95f, 0.60f);
        glm::vec3 redLight = glm::vec3(0.90f, 0.10f, 0.10f);

        // BODY
        {
            glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 0.55f, 0.0f));
            drawCube(VAO, m, modelLoc, colorLoc,
                glm::vec3(2.4f, 1.1f, 6.0f),
                bodyColor);
        }

        // ROOF
        {
            glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 1.35f, -0.2f));
            drawCube(VAO, m, modelLoc, colorLoc,
                glm::vec3(2.35f, 0.35f, 5.6f),
                roofColor);
        }

        // FRONT WINDSHIELD
        {
            glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 1.0f, 3.05f));
            drawCube(VAO, m, modelLoc, colorLoc,
                glm::vec3(2.1f, 1.0f, 0.08f),
                glassColor);

            m = glm::translate(busMatrix, glm::vec3(0.0f, 1.55f, 3.05f));
            drawCube(VAO, m, modelLoc, colorLoc,
                glm::vec3(2.1f, 0.15f, 0.10f),
                trimColor);
        }

        // SIDE WINDOWS
        for (int i = 0; i < 5; i++) {
            float z = 2.0f - i * 1.0f;

            glm::mat4 mL = glm::translate(busMatrix, glm::vec3(-1.22f, 1.15f, z));
            drawCube(VAO, mL, modelLoc, colorLoc,
                glm::vec3(0.05f, 0.55f, 0.75f),
                glassColor);

            glm::mat4 mR = glm::translate(busMatrix, glm::vec3(1.22f, 1.15f, z));
            drawCube(VAO, mR, modelLoc, colorLoc,
                glm::vec3(0.05f, 0.55f, 0.75f),
                glassColor);
        }

        // FRONT BUMPER
        {
            glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 0.35f, 3.15f));
            drawCube(VAO, m, modelLoc, colorLoc,
                glm::vec3(2.45f, 0.25f, 0.20f),
                trimColor);
        }

        // HEADLIGHTS
        {
            glm::mat4 m1 = glm::translate(busMatrix, glm::vec3(-0.9f, 0.40f, 3.26f));
            drawCube(VAO, m1, modelLoc, colorLoc,
                glm::vec3(0.25f, 0.15f, 0.08f),
                lightYellow);

            glm::mat4 m2 = glm::translate(busMatrix, glm::vec3(0.9f, 0.40f, 3.26f));
            drawCube(VAO, m2, modelLoc, colorLoc,
                glm::vec3(0.25f, 0.15f, 0.08f),
                lightYellow);
        }

        // REAR LIGHTS
        {
            glm::mat4 m1 = glm::translate(busMatrix, glm::vec3(-0.95f, 0.50f, -3.05f));
            drawCube(VAO, m1, modelLoc, colorLoc,
                glm::vec3(0.18f, 0.18f, 0.08f),
                redLight);

            glm::mat4 m2 = glm::translate(busMatrix, glm::vec3(0.95f, 0.50f, -3.05f));
            drawCube(VAO, m2, modelLoc, colorLoc,
                glm::vec3(0.18f, 0.18f, 0.08f),
                redLight);
        }

        // DOOR (hinge)
        {
            glm::vec3 hinge = glm::vec3(1.24f, 0.65f, 1.7f);
            glm::mat4 m = busMatrix;

            m = glm::translate(m, hinge);
            m = glm::rotate(m, glm::radians(doorAngle), glm::vec3(0, 1, 0));
            m = glm::translate(m, glm::vec3(-0.10f, 0.0f, 0.0f));

            drawCube(VAO, m, modelLoc, colorLoc,
                glm::vec3(0.10f, 1.0f, 0.70f),
                glm::vec3(0.25f, 0.25f, 0.70f));
        }

        // WHEELS (fake cylinder)
        {
            float wheelRadius = 0.45f;
            float wheelWidth = 0.22f;

            glm::vec3 wheelPos[4] = {
                glm::vec3(-1.15f, 0.20f,  2.20f),
                glm::vec3(1.15f, 0.20f,  2.20f),
                glm::vec3(-1.15f, 0.20f, -2.20f),
                glm::vec3(1.15f, 0.20f, -2.20f)
            };

            for (int i = 0; i < 4; i++) {
                glm::mat4 w = glm::translate(busMatrix, wheelPos[i]);
                drawCube(VAO, w, modelLoc, colorLoc,
                    glm::vec3(wheelWidth, wheelRadius * 1.2f, wheelRadius * 1.2f),
                    glm::vec3(0.05f, 0.05f, 0.05f));

                drawWheelFakeCylinder(VAO, w, modelLoc, colorLoc,
                    wheelRadius, wheelWidth,
                    glm::vec3(0.08f, 0.08f, 0.08f));
            }
        }

        // FAN
        {
            glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 1.55f, 0.0f));
            m = glm::rotate(m, glm::radians(fanAngle), glm::vec3(0, 1, 0));
            drawCube(VAO, m, modelLoc, colorLoc,
                glm::vec3(1.0f, 0.05f, 0.12f),
                glm::vec3(0.92f, 0.92f, 0.92f));

            glm::mat4 m2 = glm::translate(busMatrix, glm::vec3(0.0f, 1.55f, 0.0f));
            m2 = glm::rotate(m2, glm::radians(fanAngle + 90.0f), glm::vec3(0, 1, 0));
            drawCube(VAO, m2, modelLoc, colorLoc,
                glm::vec3(1.0f, 0.05f, 0.12f),
                glm::vec3(0.92f, 0.92f, 0.92f));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// CAMERA VECTOR UPDATE (Yaw/Pitch + Roll applied)
void updateCameraVectors()
{
    // 1) compute front from yaw & pitch
    glm::vec3 front;
    front.x = cos(glm::radians(camYaw)) * cos(glm::radians(camPitch));
    front.y = sin(glm::radians(camPitch));
    front.z = sin(glm::radians(camYaw)) * cos(glm::radians(camPitch));
    camFront = glm::normalize(front);

    // 2) right, up from worldUp
    camRight = glm::normalize(glm::cross(camFront, worldUp));
    camUp = glm::normalize(glm::cross(camRight, camFront));

    // 3) apply roll (rotate camUp around camFront axis)
    if (fabs(camRoll) > 0.0001f) {
        glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::radians(camRoll), camFront);
        camUp = glm::normalize(glm::vec3(r * glm::vec4(camUp, 0.0f)));
        camRight = glm::normalize(glm::cross(camFront, camUp));
    }
}

// INPUT

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // BUS DRIVING (Arrow keys)
    float busMove = 6.0f * deltaTime;
    float busTurn = 90.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        busPos.x += sin(glm::radians(busAngle)) * busMove;
        busPos.z += cos(glm::radians(busAngle)) * busMove;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        busPos.x -= sin(glm::radians(busAngle)) * busMove;
        busPos.z -= cos(glm::radians(busAngle)) * busMove;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        busAngle += busTurn;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        busAngle -= busTurn;
    }

    // CAMERA MOVEMENT (Flying sim)
    // W/S/A/D/E/R
    if (!birdEyeMode && !orbitMode) {
        float camSpeed = 8.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camFront * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camFront * camSpeed;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= camRight * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += camRight * camSpeed;

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camPos += camUp * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) camPos -= camUp * camSpeed;

        // PITCH / YAW / ROLL
        // X / Y / Z key
        float rotSpeed = 60.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) camYaw += rotSpeed;     // yaw left
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) camYaw -= rotSpeed;     // optional opposite yaw (U)

        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) camPitch += rotSpeed;   // pitch up
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) camPitch -= rotSpeed;   // optional pitch down (C)

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) camRoll += rotSpeed;    // roll
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) camRoll -= rotSpeed;    // optional opposite roll (V)

        // clamp pitch to avoid flip
        if (camPitch > 89.0f) camPitch = 89.0f;
        if (camPitch < -89.0f) camPitch = -89.0f;

        updateCameraVectors();
    }

    // Toggle Orbit (F)
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fKeyPressed) {
            orbitMode = !orbitMode;
            birdEyeMode = false;
            fKeyPressed = true;
        }
    }
    else fKeyPressed = false;

    // Toggle Bird Eye (B)
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        if (!bKeyPressed) {
            birdEyeMode = !birdEyeMode;
            orbitMode = false; // avoid conflict
            bKeyPressed = true;
        }
    }
    else bKeyPressed = false;

    // Fan (G)
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        if (!gKeyPressed) {
            fanOn = !fanOn;
            gKeyPressed = true;
        }
    }
    else gKeyPressed = false;

    // Door (O)
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        if (!oKeyPressed) {
            doorOpen = !doorOpen;
            oKeyPressed = true;
        }
    }
    else oKeyPressed = false;
}


// CALLBACK

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// SHADER COMPILE
unsigned int createShader(const char* vShaderCode, const char* fShaderCode)
{
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    unsigned int ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return ID;
}



//#include <glad/glad.h>
//#include <GLFW/glfw3.h>
//
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//
//#include <iostream>
//#include <cmath>
//
////Screen
//const unsigned int SCR_WIDTH = 800;
//const unsigned int SCR_HEIGHT = 600;
//
////SHADERS
//const char* vertexShaderSource = "#version 330 core\n"
//"layout (location = 0) in vec3 aPos;\n"
//"uniform mat4 model;\n"
//"uniform mat4 view;\n"
//"uniform mat4 projection;\n"
//"void main()\n"
//"{\n"
//"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
//"}\0";
//
//const char* fragmentShaderSource = "#version 330 core\n"
//"out vec4 FragColor;\n"
//"uniform vec3 objectColor;\n"
//"void main()\n"
//"{\n"
//"   FragColor = vec4(objectColor, 1.0f);\n"
//"}\n\0";
//
////GLOBAL
//float deltaTime = 0.0f;
//float lastFrame = 0.0f;
//
////Fan States
//bool bladesOn = false;          
//float bladeAngle = 0.0f;        
//
//float headYaw = 0.0f;           
//float headPitch = 0.0f;         
//
//bool tPressed = false;         
//
////CUBE DATA
//float vertices[] = {
//     0.5f,  0.5f,  0.5f,
//     0.5f, -0.5f,  0.5f,
//    -0.5f, -0.5f,  0.5f,
//    -0.5f,  0.5f,  0.5f,
//
//     0.5f,  0.5f, -0.5f,
//     0.5f, -0.5f, -0.5f,
//    -0.5f, -0.5f, -0.5f,
//    -0.5f,  0.5f, -0.5f
//};
//
//unsigned int indices[] = {
//    0, 1, 2, 2, 3, 0,
//    4, 5, 6, 6, 7, 4,
//    7, 3, 0, 0, 4, 7,
//    6, 2, 1, 1, 5, 6,
//    0, 1, 5, 5, 4, 0,
//    3, 2, 6, 6, 7, 3
//};
//
////Forward decl
//void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//void processInput(GLFWwindow* window);
//unsigned int createShader(const char* vShader, const char* fShader);
//
////DRAW HELPERS
//void drawCube(unsigned int VAO,
//    glm::mat4 model,
//    int modelLoc,
//    int colorLoc,
//    glm::vec3 scale,
//    glm::vec3 color)
//{
//    glm::mat4 m = glm::scale(model, scale);
//    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
//    glUniform3f(colorLoc, color.x, color.y, color.z);
//    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
//}
//
//
//
//
////MAIN
//int main()
//{
//    glfwInit();
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Minimal 3D Table Fan (2 Pivots)", NULL, NULL);
//    if (window == NULL) {
//        std::cout << "Failed to create window\n";
//        glfwTerminate();
//        return -1;
//    }
//    glfwMakeContextCurrent(window);
//    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
//
//    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
//        std::cout << "Failed to init GLAD\n";
//        return -1;
//    }
//
//    glEnable(GL_DEPTH_TEST);
//
//    unsigned int shaderProgram = createShader(vertexShaderSource, fragmentShaderSource);
//
//    unsigned int VAO, VBO, EBO;
//    glGenVertexArrays(1, &VAO);
//    glGenBuffers(1, &VBO);
//    glGenBuffers(1, &EBO);
//
//    glBindVertexArray(VAO);
//
//    glBindBuffer(GL_ARRAY_BUFFER, VBO);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
//
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(0);
//
//    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
//    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
//    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
//    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
//
//    while (!glfwWindowShouldClose(window))
//    {
//        float currentFrame = (float)glfwGetTime();
//        deltaTime = currentFrame - lastFrame;
//        lastFrame = currentFrame;
//
//        processInput(window);
//
//        //blade spin
//        if (bladesOn) {
//            bladeAngle += 600.0f * deltaTime;
//            if (bladeAngle > 360.0f) bladeAngle -= 360.0f;
//        }
//
//        //RENDER
//        glClearColor(0.80f, 0.90f, 0.95f, 1.0f);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//        glUseProgram(shaderProgram);
//
//        //Camera
//        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
//            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
//
//        glm::vec3 camPos = glm::vec3(0.0f, 6.0f, 14.0f);
//        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 2.5f, 0.0f), glm::vec3(0, 1, 0));
//
//        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
//        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
//
//        glBindVertexArray(VAO);
//
//        //FAN DRAW
//        glm::mat4 fan = glm::mat4(1.0f);
//
//        glm::vec3 baseColor = glm::vec3(0.30f, 0.30f, 0.30f);
//        glm::vec3 poleColor = glm::vec3(0.65f, 0.65f, 0.65f);
//        glm::vec3 headColor = glm::vec3(0.20f, 0.32f, 0.55f);
//        glm::vec3 bladeColor = glm::vec3(0.92f, 0.92f, 0.92f);
//
//        //Base
//        {
//            glm::mat4 base = fan;
//            base = glm::translate(base, glm::vec3(0.0f, 0.18f, 0.0f));
//            drawCube(VAO, base, modelLoc, colorLoc,
//                glm::vec3(1.6f, 0.18f, 1.6f),
//                baseColor);
//        }
//
//        //Shaft
//        {
//            float poleH = 2.2f;
//            glm::mat4 pole = fan;
//            pole = glm::translate(pole, glm::vec3(0.0f, 0.18f + poleH * 0.5f, 0.0f));
//            drawCube(VAO, pole, modelLoc, colorLoc,
//                glm::vec3(0.25f, poleH, 0.25f),
//                poleColor);
//        }
//
//        //HEAD JOINT
//        glm::vec3 pivot2 = glm::vec3(0.0f, 0.18f + 2.2f, 0.0f);
//
//        glm::mat4 headJoint = fan;
//        headJoint = glm::translate(headJoint, pivot2);
//
//        drawCube(
//            VAO,headJoint,modelLoc,colorLoc,glm::vec3(0.35f, 0.35f, 0.35f),glm::vec3(0.45f, 0.45f, 0.45f) 
//        );
//        headJoint = glm::rotate(headJoint, glm::radians(headYaw), glm::vec3(0, 1, 0));
//        headJoint = glm::rotate(headJoint, glm::radians(headPitch), glm::vec3(1, 0, 0));
//
//
//        //Head body
//        glm::mat4 head = glm::translate(headJoint, glm::vec3(0.0f, 0.0f, 0.9f));
//        drawCube(VAO, head, modelLoc, colorLoc, glm::vec3(1.4f, 1.1f, 1.0f), headColor);
//
//        //Pivot 1
//        glm::mat4 bladePivot = head;
//        bladePivot = glm::translate(bladePivot, glm::vec3(0.0f, 0.0f, 0.60f));
//        bladePivot = glm::rotate(bladePivot, glm::radians(bladeAngle), glm::vec3(0, 0, 1));
//
//        //Hub
//        drawCube(VAO, bladePivot, modelLoc, colorLoc,
//            glm::vec3(0.25f, 0.25f, 0.25f),
//            glm::vec3(0.85f, 0.85f, 0.85f));
//
//        //4 blades
//        for (int i = 0; i < 4; i++) {
//            glm::mat4 b = bladePivot;
//            b = glm::rotate(b, glm::radians(90.0f * i), glm::vec3(0, 0, 1));
//            b = glm::translate(b, glm::vec3(0.75f, 0.0f, 0.0f));
//            drawCube(VAO, b, modelLoc, colorLoc,
//                glm::vec3(1.2f, 0.18f, 0.15f),
//                bladeColor);
//        }
//
//        glfwSwapBuffers(window);
//        glfwPollEvents();
//    }
//
//    glDeleteVertexArrays(1, &VAO);
//    glDeleteBuffers(1, &VBO);
//    glDeleteBuffers(1, &EBO);
//    glDeleteProgram(shaderProgram);
//
//    glfwTerminate();
//    return 0;
//}
//
////INPUT
//void processInput(GLFWwindow* window)
//{
//    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//        glfwSetWindowShouldClose(window, true);
//
//    //Toggle spin
//    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
//        if (!tPressed) {
//            bladesOn = !bladesOn;
//            tPressed = true;
//        }
//    }
//    else tPressed = false;
//
//    //Pivot 2 controls
//    float rot = 60.0f * deltaTime;
//
//    //yaw left-right
//    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) headYaw += rot;
//    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) headYaw -= rot;
//
//    //pitch up-down
//    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) headPitch += rot;
//    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) headPitch -= rot;
//
//    //clamp pitch
//    if (headPitch > 40.0f) headPitch = 40.0f;
//    if (headPitch < -40.0f) headPitch = -40.0f;
//}
//
//void framebuffer_size_callback(GLFWwindow* window, int width, int height)
//{
//    glViewport(0, 0, width, height);
//}
//
////SHADER COMPILE
//unsigned int createShader(const char* vShaderCode, const char* fShaderCode)
//{
//    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vertex, 1, &vShaderCode, NULL);
//    glCompileShader(vertex);
//
//    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(fragment, 1, &fShaderCode, NULL);
//    glCompileShader(fragment);
//
//    unsigned int ID = glCreateProgram();
//    glAttachShader(ID, vertex);
//    glAttachShader(ID, fragment);
//    glLinkProgram(ID);
//
//    glDeleteShader(vertex);
//    glDeleteShader(fragment);
//    return ID;
//}
