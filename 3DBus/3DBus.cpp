#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include<string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>

// =========================
// SCREEN
// =========================
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

// =========================
// SHADERS (Phong + multiple lights + emissive)
// =========================
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // normal matrix
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalize(normalMat * aNormal);

    gl_Position = projection * view * worldPos;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;

uniform vec3 objectColor;

// toggles
uniform bool enableDir;
uniform bool enablePoints;
uniform bool enableSpot;

uniform bool enableAmbient;
uniform bool enableDiffuse;
uniform bool enableSpecular;

// emissive
uniform vec3 emissiveColor;
uniform float emissiveStrength;

// material
uniform float shininess;

// directional
uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;

// point lights
#define NR_POINT_LIGHTS 4
uniform vec3 pointPos[NR_POINT_LIGHTS];
uniform vec3 pointColor[NR_POINT_LIGHTS];

// spot
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform vec3 spotColor;
uniform float spotCutoff; // cos(cutoffAngle)

// helper: phong component
vec3 PhongLight(vec3 lightDir, vec3 lightCol, vec3 N, vec3 V)
{
    vec3 result = vec3(0.0);

    // ambient
    if(enableAmbient)
        result += 0.12 * lightCol;

    // diffuse
    if(enableDiffuse)
    {
        float diff = max(dot(N, lightDir), 0.0);
        result += diff * lightCol;
    }

    // specular
    if(enableSpecular)
    {
        vec3 R = reflect(-lightDir, N);
        float spec = pow(max(dot(V, R), 0.0), shininess);
        result += 0.45 * spec * lightCol;
    }

    return result;
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 lighting = vec3(0.0);

    // Directional light
    if(enableDir)
    {
        vec3 L = normalize(-dirLightDirection);
        lighting += PhongLight(L, dirLightColor, N, V);
    }

    // Point lights (with attenuation)
    if(enablePoints)
    {
        for(int i=0;i<NR_POINT_LIGHTS;i++)
        {
            vec3 Lvec = pointPos[i] - FragPos;
            float dist = length(Lvec);
            vec3 L = normalize(Lvec);

            // simple attenuation
            float att = 1.0 / (1.0 + 0.12*dist + 0.032*dist*dist);

            lighting += att * PhongLight(L, pointColor[i], N, V);
        }
    }

    // Spot light (single cutoff)
    if(enableSpot)
    {
        vec3 Lvec = spotPos - FragPos;
        float dist = length(Lvec);
        vec3 L = normalize(Lvec);

        float theta = dot(normalize(-spotDir), L); // compare direction
        if(theta > spotCutoff)
        {
            float att = 1.0 / (1.0 + 0.10*dist + 0.020*dist*dist);
            lighting += att * PhongLight(L, spotColor, N, V);
        }
    }

    // base shaded color
    vec3 shaded = lighting * objectColor;

    // emissive add (acts like glowing light)
    shaded += emissiveColor * emissiveStrength;

    FragColor = vec4(shaded, 1.0);
}
)";

// =========================
// GLOBAL TIME
// =========================
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// =========================
// BUS STATE
// =========================
glm::vec3 busPos = glm::vec3(0.0f, 0.0f, 0.0f);
float busAngle = 0.0f;

// =========================
// CAMERA (main free camera)
// =========================
glm::vec3 worldUp = glm::vec3(0, 1, 0);

glm::vec3 camPos = glm::vec3(0.0f, 7.0f, 18.0f);
float camYaw = -90.0f;
float camPitch = -10.0f;
float camRoll = 0.0f;

glm::vec3 camFront(0, 0, -1), camRight(1, 0, 0), camUp(0, 1, 0);

bool orbitMode = false;
float orbitAngle = 0.0f;
float orbitRadius = 18.0f;

bool birdEyeMode = false;

// =========================
// ANIM
// =========================
bool fanOn = false;
float fanAngle = 0.0f;

bool doorOpen = false;
float doorAngle = 0.0f;

// =========================
// KEY GUARDS
// =========================
bool gKeyPressed = false, oKeyPressed = false, bKeyPressed = false, fKeyPressed = false;

// lighting toggles (required)
bool key1Pressed = false, key2Pressed = false, key3Pressed = false;
bool key5Pressed = false, key6Pressed = false, key7Pressed = false;

bool enableDir = true;
bool enablePoints = true;
bool enableSpot = true;

bool enableAmbient = true;
bool enableDiffuse = true;
bool enableSpecular = true;

// =========================
// PROTOTYPES
// =========================
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
unsigned int createShader(const char* vShader, const char* fShader);
void updateCameraVectors();

// OUR OWN LOOKAT (Requirement #5)
glm::mat4 myLookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up)
{
    glm::vec3 f = glm::normalize(center - eye);      // forward
    glm::vec3 s = glm::normalize(glm::cross(f, up)); // right
    glm::vec3 u = glm::cross(s, f);                  // true up

    glm::mat4 M(1.0f);
    M[0][0] = s.x; M[1][0] = s.y; M[2][0] = s.z;
    M[0][1] = u.x; M[1][1] = u.y; M[2][1] = u.z;
    M[0][2] = -f.x; M[1][2] = -f.y; M[2][2] = -f.z;

    glm::mat4 T(1.0f);
    T[3][0] = -eye.x;
    T[3][1] = -eye.y;
    T[3][2] = -eye.z;

    return M * T;
}

// =========================
// CUBE WITH NORMALS (36 vertices)
// =========================
float cubeVertices[] = {
    // positions          // normals
    // FRONT
   -0.5f,-0.5f, 0.5f,     0,0,1,
    0.5f,-0.5f, 0.5f,     0,0,1,
    0.5f, 0.5f, 0.5f,     0,0,1,
    0.5f, 0.5f, 0.5f,     0,0,1,
   -0.5f, 0.5f, 0.5f,     0,0,1,
   -0.5f,-0.5f, 0.5f,     0,0,1,

   // BACK
  -0.5f,-0.5f,-0.5f,     0,0,-1,
  -0.5f, 0.5f,-0.5f,     0,0,-1,
   0.5f, 0.5f,-0.5f,     0,0,-1,
   0.5f, 0.5f,-0.5f,     0,0,-1,
   0.5f,-0.5f,-0.5f,     0,0,-1,
  -0.5f,-0.5f,-0.5f,     0,0,-1,

  // LEFT
 -0.5f, 0.5f, 0.5f,    -1,0,0,
 -0.5f, 0.5f,-0.5f,    -1,0,0,
 -0.5f,-0.5f,-0.5f,    -1,0,0,
 -0.5f,-0.5f,-0.5f,    -1,0,0,
 -0.5f,-0.5f, 0.5f,    -1,0,0,
 -0.5f, 0.5f, 0.5f,    -1,0,0,

 // RIGHT
 0.5f, 0.5f, 0.5f,     1,0,0,
 0.5f,-0.5f,-0.5f,     1,0,0,
 0.5f, 0.5f,-0.5f,     1,0,0,
 0.5f,-0.5f,-0.5f,     1,0,0,
 0.5f, 0.5f, 0.5f,     1,0,0,
 0.5f,-0.5f, 0.5f,     1,0,0,

 // TOP
-0.5f, 0.5f,-0.5f,     0,1,0,
-0.5f, 0.5f, 0.5f,     0,1,0,
 0.5f, 0.5f, 0.5f,     0,1,0,
 0.5f, 0.5f, 0.5f,     0,1,0,
 0.5f, 0.5f,-0.5f,     0,1,0,
-0.5f, 0.5f,-0.5f,     0,1,0,

// BOTTOM
-0.5f,-0.5f,-0.5f,     0,-1,0,
 0.5f,-0.5f, 0.5f,     0,-1,0,
-0.5f,-0.5f, 0.5f,     0,-1,0,
 0.5f,-0.5f, 0.5f,     0,-1,0,
-0.5f,-0.5f,-0.5f,     0,-1,0,
 0.5f,-0.5f,-0.5f,     0,-1,0
};

// =========================
// DRAW HELPERS
// =========================
void setModel(unsigned int modelLoc, const glm::mat4& m)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
}

void setColor(unsigned int colorLoc, glm::vec3 c)
{
    glUniform3f(colorLoc, c.x, c.y, c.z);
}

void drawCube(unsigned int VAO,
    glm::mat4 base,
    unsigned int modelLoc,
    unsigned int colorLoc,
    glm::vec3 scale,
    glm::vec3 color,
    unsigned int emissiveColorLoc,
    unsigned int emissiveStrengthLoc,
    glm::vec3 eColor = glm::vec3(0),
    float eStrength = 0.0f)
{
    glm::mat4 m = glm::scale(base, scale);
    setModel(modelLoc, m);
    setColor(colorLoc, color);

    glUniform3f(emissiveColorLoc, eColor.x, eColor.y, eColor.z);
    glUniform1f(emissiveStrengthLoc, eStrength);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawWheelFakeCylinder(unsigned int VAO,
    const glm::mat4& base,
    unsigned int modelLoc,
    unsigned int colorLoc,
    unsigned int emissiveColorLoc,
    unsigned int emissiveStrengthLoc,
    float radius,
    float width,
    glm::vec3 color)
{
    const int slices = 10;
    for (int i = 0; i < slices; i++)
    {
        float a = (float)i / (float)slices * 2.0f * 3.1415926f;
        glm::mat4 m = base;
        m = glm::rotate(m, a, glm::vec3(1, 0, 0));
        m = glm::translate(m, glm::vec3(0.0f, radius, 0.0f));

        drawCube(VAO, m, modelLoc, colorLoc,
            glm::vec3(width, radius * 0.25f, radius * 0.25f),
            color,
            emissiveColorLoc, emissiveStrengthLoc);
    }
}

// =========================
// MAIN
// =========================
int main()
{
    std::cout <<
        "==== Assignment B2 Controls ====\n"
        "Arrow Keys : Drive bus (move/turn)\n"
        "W/S/A/D    : Move camera (free mode)\n"
        "E/R        : Up/Down camera\n"
        "Y/U        : Yaw + / -\n"
        "X/C        : Pitch + / -\n"
        "Z/V        : Roll + / -\n"
        "F          : Toggle orbit camera\n"
        "B          : Toggle bird-eye camera\n"
        "G          : Toggle fan\n"
        "O          : Toggle door\n"
        "--------------------------------\n"
        "1 : Toggle Directional Light\n"
        "2 : Toggle Point Lights\n"
        "3 : Toggle Spot Light\n"
        "5 : Toggle Ambient\n"
        "6 : Toggle Diffuse\n"
        "7 : Toggle Specular\n"
        "================================\n";

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment B2 - Bus Lighting + 4 Viewports", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to init GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int shaderProgram = createShader(vertexShaderSource, fragmentShaderSource);

    // VAO/VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // uniforms
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    unsigned int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

    unsigned int enableDirLoc = glGetUniformLocation(shaderProgram, "enableDir");
    unsigned int enablePointsLoc = glGetUniformLocation(shaderProgram, "enablePoints");
    unsigned int enableSpotLoc = glGetUniformLocation(shaderProgram, "enableSpot");

    unsigned int ambLoc = glGetUniformLocation(shaderProgram, "enableAmbient");
    unsigned int difLoc = glGetUniformLocation(shaderProgram, "enableDiffuse");
    unsigned int speLoc = glGetUniformLocation(shaderProgram, "enableSpecular");

    unsigned int shinLoc = glGetUniformLocation(shaderProgram, "shininess");

    unsigned int emissiveColorLoc = glGetUniformLocation(shaderProgram, "emissiveColor");
    unsigned int emissiveStrengthLoc = glGetUniformLocation(shaderProgram, "emissiveStrength");

    // light uniforms
    unsigned int dirDirLoc = glGetUniformLocation(shaderProgram, "dirLightDirection");
    unsigned int dirColLoc = glGetUniformLocation(shaderProgram, "dirLightColor");

    unsigned int spotPosLoc = glGetUniformLocation(shaderProgram, "spotPos");
    unsigned int spotDirLoc = glGetUniformLocation(shaderProgram, "spotDir");
    unsigned int spotColLoc = glGetUniformLocation(shaderProgram, "spotColor");
    unsigned int spotCutLoc = glGetUniformLocation(shaderProgram, "spotCutoff");

    // point arrays -> build uniform names
    int pointPosLoc[4];
    int pointColLoc[4];
    for (int i = 0; i < 4; i++)
    {
        std::string p1 = "pointPos[" + std::to_string(i) + "]";
        std::string p2 = "pointColor[" + std::to_string(i) + "]";
        pointPosLoc[i] = glGetUniformLocation(shaderProgram, p1.c_str());
        pointColLoc[i] = glGetUniformLocation(shaderProgram, p2.c_str());
    }

    // init camera vectors
    updateCameraVectors();

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // animations
        if (fanOn)
        {
            fanAngle += 360.0f * deltaTime;
            if (fanAngle > 360.0f) fanAngle -= 360.0f;
        }

        float doorSpeed = 120.0f;
        if (doorOpen && doorAngle < 75.0f) doorAngle += doorSpeed * deltaTime;
        if (!doorOpen && doorAngle > 0.0f) doorAngle -= doorSpeed * deltaTime;

        // camera modes
        glm::vec3 target = busPos + glm::vec3(0, 0.8f, 0);

        if (birdEyeMode)
        {
            camPos = busPos + glm::vec3(0.0f, 22.0f, 0.01f);
            camFront = glm::normalize(target - camPos);
            camRight = glm::normalize(glm::cross(camFront, worldUp));
            camUp = glm::normalize(glm::cross(camRight, camFront));
        }
        else if (orbitMode)
        {
            orbitAngle += 35.0f * deltaTime;
            float rad = glm::radians(orbitAngle);

            camPos.x = target.x + orbitRadius * cos(rad);
            camPos.z = target.z + orbitRadius * sin(rad);
            camPos.y = target.y + 7.0f;

            camFront = glm::normalize(target - camPos);
            camRight = glm::normalize(glm::cross(camFront, worldUp));
            camUp = glm::normalize(glm::cross(camRight, camFront));

            if (fabs(camRoll) > 0.0001f)
            {
                glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::radians(camRoll), camFront);
                camUp = glm::normalize(glm::vec3(r * glm::vec4(camUp, 0.0f)));
                camRight = glm::normalize(glm::cross(camFront, camUp));
            }
        }

        // clear once
        glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // lighting toggle uniforms
        glUniform1i(enableDirLoc, enableDir);
        glUniform1i(enablePointsLoc, enablePoints);
        glUniform1i(enableSpotLoc, enableSpot);

        glUniform1i(ambLoc, enableAmbient);
        glUniform1i(difLoc, enableDiffuse);
        glUniform1i(speLoc, enableSpecular);

        glUniform1f(shinLoc, 32.0f);

        // set global lights
        glUniform3f(dirDirLoc, -0.4f, -1.0f, -0.3f);
        glUniform3f(dirColLoc, 0.9f, 0.9f, 0.9f);

        // spot: from camera like flashlight
        glUniform3f(spotPosLoc, camPos.x, camPos.y, camPos.z);
        glUniform3f(spotDirLoc, camFront.x, camFront.y, camFront.z);
        glUniform3f(spotColLoc, 1.0f, 0.95f, 0.80f);
        glUniform1f(spotCutLoc, cos(glm::radians(14.0f))); // single cutoff angle

        // bus master
        glm::mat4 busMatrix(1.0f);
        busMatrix = glm::translate(busMatrix, busPos);
        busMatrix = glm::rotate(busMatrix, glm::radians(busAngle), glm::vec3(0, 1, 0));

        // point lights in bus local positions -> convert to world
        glm::vec3 localPoints[4] = {
            glm::vec3(-0.9f, 0.40f, 3.26f), // left headlight
            glm::vec3(0.9f, 0.40f, 3.26f), // right headlight
            glm::vec3(0.0f, 1.55f, 0.0f),  // inside roof light area
            glm::vec3(0.0f, 0.40f,-3.10f)  // rear area light
        };

        glm::vec3 pointCols[4] = {
            glm::vec3(1.0f, 0.95f, 0.75f),
            glm::vec3(1.0f, 0.95f, 0.75f),
            glm::vec3(0.65f, 0.75f, 1.0f),
            glm::vec3(1.0f, 0.25f, 0.25f)
        };

        for (int i = 0; i < 4; i++)
        {
            glm::vec4 w = busMatrix * glm::vec4(localPoints[i], 1.0f);
            glUniform3f(pointPosLoc[i], w.x, w.y, w.z);
            glUniform3f(pointColLoc[i], pointCols[i].x, pointCols[i].y, pointCols[i].z);
        }

        // =========================================
        // 4 VIEWPORTS (Requirement #6)
        // =========================================
        int halfW = SCR_WIDTH / 2;
        int halfH = SCR_HEIGHT / 2;

        // Views:
        // 0: Combined lighting (free cam)
        // 1: Top view (isometric-ish)
        // 2: Front view
        // 3: Inside view
        for (int vp = 0; vp < 4; vp++)
        {
            int x = (vp % 2) * halfW;
            int y = (vp / 2) * halfH;
            glViewport(x, y, halfW, halfH);

            float aspect = (float)halfW / (float)halfH;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 300.0f);

            glm::mat4 view;

            glm::vec3 vpos, vfront, vup;

            if (vp == 0)
            {
                vpos = camPos;
                vfront = camFront;
                vup = camUp;

                // combined (whatever toggles user sets)
            }
            else if (vp == 1)
            {
                // top view
                vpos = busPos + glm::vec3(0.0f, 25.0f, 0.01f);
                vfront = glm::normalize(target - vpos);
                glm::vec3 right = glm::normalize(glm::cross(vfront, worldUp));
                vup = glm::normalize(glm::cross(right, vfront));
            }
            else if (vp == 2)
            {
                // front view
                vpos = busPos + glm::vec3(0.0f, 4.0f, 20.0f);
                vfront = glm::normalize(target - vpos);
                glm::vec3 right = glm::normalize(glm::cross(vfront, worldUp));
                vup = glm::normalize(glm::cross(right, vfront));
            }
            else
            {
                // inside view: near front inside cabin
                glm::vec3 insideLocal(0.0f, 1.2f, 2.0f);
                glm::vec4 insideWorld = busMatrix * glm::vec4(insideLocal, 1.0f);
                vpos = glm::vec3(insideWorld);
                glm::vec3 lookLocal(0.0f, 1.2f, -3.0f);
                glm::vec4 lookWorld = busMatrix * glm::vec4(lookLocal, 1.0f);
                vfront = glm::normalize(glm::vec3(lookWorld) - vpos);

                glm::vec3 right = glm::normalize(glm::cross(vfront, worldUp));
                vup = glm::normalize(glm::cross(right, vfront));
            }

            // Use YOUR OWN lookAt
            view = myLookAt(vpos, vpos + vfront, vup);

            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniform3f(viewPosLoc, vpos.x, vpos.y, vpos.z);

            // =========================================
            // DRAW SCENE (BUS)
            // =========================================
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
                    bodyColor,
                    emissiveColorLoc, emissiveStrengthLoc);
            }

            // ROOF
            {
                glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 1.35f, -0.2f));
                drawCube(VAO, m, modelLoc, colorLoc,
                    glm::vec3(2.35f, 0.35f, 5.6f),
                    roofColor,
                    emissiveColorLoc, emissiveStrengthLoc);
            }

            // FRONT WINDSHIELD
            {
                glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 1.0f, 3.05f));
                drawCube(VAO, m, modelLoc, colorLoc,
                    glm::vec3(2.1f, 1.0f, 0.08f),
                    glassColor,
                    emissiveColorLoc, emissiveStrengthLoc);

                glm::mat4 m2 = glm::translate(busMatrix, glm::vec3(0.0f, 1.55f, 3.05f));
                drawCube(VAO, m2, modelLoc, colorLoc,
                    glm::vec3(2.1f, 0.15f, 0.10f),
                    trimColor,
                    emissiveColorLoc, emissiveStrengthLoc);
            }

            // SIDE WINDOWS
            for (int i = 0; i < 5; i++)
            {
                float z = 2.0f - i * 1.0f;

                glm::mat4 mL = glm::translate(busMatrix, glm::vec3(-1.22f, 1.15f, z));
                drawCube(VAO, mL, modelLoc, colorLoc,
                    glm::vec3(0.05f, 0.55f, 0.75f),
                    glassColor,
                    emissiveColorLoc, emissiveStrengthLoc);

                glm::mat4 mR = glm::translate(busMatrix, glm::vec3(1.22f, 1.15f, z));
                drawCube(VAO, mR, modelLoc, colorLoc,
                    glm::vec3(0.05f, 0.55f, 0.75f),
                    glassColor,
                    emissiveColorLoc, emissiveStrengthLoc);
            }

            // FRONT BUMPER
            {
                glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 0.35f, 3.15f));
                drawCube(VAO, m, modelLoc, colorLoc,
                    glm::vec3(2.45f, 0.25f, 0.20f),
                    trimColor,
                    emissiveColorLoc, emissiveStrengthLoc);
            }

            // HEADLIGHTS (EMISSIVE + also point lights exist)
            {
                glm::mat4 m1 = glm::translate(busMatrix, glm::vec3(-0.9f, 0.40f, 3.26f));
                drawCube(VAO, m1, modelLoc, colorLoc,
                    glm::vec3(0.25f, 0.15f, 0.08f),
                    lightYellow,
                    emissiveColorLoc, emissiveStrengthLoc,
                    lightYellow, 1.8f); // emissive glow

                glm::mat4 m2 = glm::translate(busMatrix, glm::vec3(0.9f, 0.40f, 3.26f));
                drawCube(VAO, m2, modelLoc, colorLoc,
                    glm::vec3(0.25f, 0.15f, 0.08f),
                    lightYellow,
                    emissiveColorLoc, emissiveStrengthLoc,
                    lightYellow, 1.8f);
            }

            // REAR LIGHTS (EMISSIVE)
            {
                glm::mat4 m1 = glm::translate(busMatrix, glm::vec3(-0.95f, 0.50f, -3.05f));
                drawCube(VAO, m1, modelLoc, colorLoc,
                    glm::vec3(0.18f, 0.18f, 0.08f),
                    redLight,
                    emissiveColorLoc, emissiveStrengthLoc,
                    redLight, 1.2f);

                glm::mat4 m2 = glm::translate(busMatrix, glm::vec3(0.95f, 0.50f, -3.05f));
                drawCube(VAO, m2, modelLoc, colorLoc,
                    glm::vec3(0.18f, 0.18f, 0.08f),
                    redLight,
                    emissiveColorLoc, emissiveStrengthLoc,
                    redLight, 1.2f);
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
                    glm::vec3(0.25f, 0.25f, 0.70f),
                    emissiveColorLoc, emissiveStrengthLoc);
            }

            // WHEELS
            {
                float wheelRadius = 0.45f;
                float wheelWidth = 0.22f;

                glm::vec3 wheelPos[4] = {
                    glm::vec3(-1.15f, 0.20f,  2.20f),
                    glm::vec3(1.15f, 0.20f,  2.20f),
                    glm::vec3(-1.15f, 0.20f, -2.20f),
                    glm::vec3(1.15f, 0.20f, -2.20f)
                };

                for (int i = 0; i < 4; i++)
                {
                    glm::mat4 w = glm::translate(busMatrix, wheelPos[i]);
                    drawCube(VAO, w, modelLoc, colorLoc,
                        glm::vec3(wheelWidth, wheelRadius * 1.2f, wheelRadius * 1.2f),
                        glm::vec3(0.05f, 0.05f, 0.05f),
                        emissiveColorLoc, emissiveStrengthLoc);

                    drawWheelFakeCylinder(VAO, w, modelLoc, colorLoc,
                        emissiveColorLoc, emissiveStrengthLoc,
                        wheelRadius, wheelWidth,
                        glm::vec3(0.08f, 0.08f, 0.08f));
                }
            }

            // FAN (inside) - a bit emissive for visibility
            {
                glm::mat4 m = glm::translate(busMatrix, glm::vec3(0.0f, 1.55f, 0.0f));
                m = glm::rotate(m, glm::radians(fanAngle), glm::vec3(0, 1, 0));
                drawCube(VAO, m, modelLoc, colorLoc,
                    glm::vec3(1.0f, 0.05f, 0.12f),
                    glm::vec3(0.92f, 0.92f, 0.92f),
                    emissiveColorLoc, emissiveStrengthLoc,
                    glm::vec3(0.65f, 0.75f, 1.0f), 0.25f);

                glm::mat4 m2 = glm::translate(busMatrix, glm::vec3(0.0f, 1.55f, 0.0f));
                m2 = glm::rotate(m2, glm::radians(fanAngle + 90.0f), glm::vec3(0, 1, 0));
                drawCube(VAO, m2, modelLoc, colorLoc,
                    glm::vec3(1.0f, 0.05f, 0.12f),
                    glm::vec3(0.92f, 0.92f, 0.92f),
                    emissiveColorLoc, emissiveStrengthLoc,
                    glm::vec3(0.65f, 0.75f, 1.0f), 0.25f);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// =========================
// CAMERA UPDATE
// =========================
void updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(camYaw)) * cos(glm::radians(camPitch));
    front.y = sin(glm::radians(camPitch));
    front.z = sin(glm::radians(camYaw)) * cos(glm::radians(camPitch));
    camFront = glm::normalize(front);

    camRight = glm::normalize(glm::cross(camFront, worldUp));
    camUp = glm::normalize(glm::cross(camRight, camFront));

    // roll
    if (fabs(camRoll) > 0.0001f)
    {
        glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::radians(camRoll), camFront);
        camUp = glm::normalize(glm::vec3(r * glm::vec4(camUp, 0.0f)));
        camRight = glm::normalize(glm::cross(camFront, camUp));
    }
}

// =========================
// INPUT
// =========================
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // BUS
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
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  busAngle += busTurn;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) busAngle -= busTurn;

    // CAMERA MOVE (only in free mode)
    if (!birdEyeMode && !orbitMode)
    {
        float camSpeed = 8.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camFront * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camFront * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= camRight * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += camRight * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camPos += camUp * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) camPos -= camUp * camSpeed;

        float rotSpeed = 60.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) camYaw += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) camYaw -= rotSpeed;

        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) camPitch += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) camPitch -= rotSpeed;

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) camRoll += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) camRoll -= rotSpeed;

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
            orbitMode = false;
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

    // =========================
    // LIGHT TOGGLES (Assignment requirement)
    // =========================

    // 1: directional
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        if (!key1Pressed) { enableDir = !enableDir; key1Pressed = true; }
    }
    else key1Pressed = false;

    // 2: point lights
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        if (!key2Pressed) { enablePoints = !enablePoints; key2Pressed = true; }
    }
    else key2Pressed = false;

    // 3: spot light
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        if (!key3Pressed) { enableSpot = !enableSpot; key3Pressed = true; }
    }
    else key3Pressed = false;

    // 5: ambient
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
        if (!key5Pressed) { enableAmbient = !enableAmbient; key5Pressed = true; }
    }
    else key5Pressed = false;

    // 6: diffuse
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        if (!key6Pressed) { enableDiffuse = !enableDiffuse; key6Pressed = true; }
    }
    else key6Pressed = false;

    // 7: specular
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
        if (!key7Pressed) { enableSpecular = !enableSpecular; key7Pressed = true; }
    }
    else key7Pressed = false;
}

// =========================
// CALLBACK
// =========================
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

// =========================
// SHADER COMPILE
// =========================
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
