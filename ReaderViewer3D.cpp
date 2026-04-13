/* OBJ Viewer – Exercício de Transformações */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"

const GLuint WIDTH = 1000, HEIGHT = 800;

struct VertexData {
    float x, y, z;
    float r, g, b;
};

struct SceneObject {
    GLuint VAO;
    size_t vertexCount;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::vec3 color;
    string name;
};

enum TransformMode { MODE_ROTATE = 0, MODE_TRANSLATE, MODE_SCALE };

Camera camera(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool perspective = true;

int selectedObject = 0;
TransformMode currentMode = MODE_TRANSLATE;
vector<SceneObject> scene;

const char* modeNames[] = { "ROTACAO", "TRANSLACAO", "ESCALA" };

// ─── Prototypes ──────────────────────────────────────────────
vector<VertexData> loadOBJ(const string& path, glm::vec3 color);
SceneObject createObject(const string& path, const string& name,
                          glm::vec3 color, glm::vec3 pos);
int  setupShader();
void setupScene();
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void printControls();

// ─── Shaders ─────────────────────────────────────────────────
const GLchar* vertexShaderSource = R"glsl(
#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

out vec4 finalColor;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    finalColor = vec4(color, 1.0);
}
)glsl";

const GLchar* fragmentShaderSource = R"glsl(
#version 410 core
in vec4 finalColor;
out vec4 color;

uniform bool  useOverrideColor;
uniform vec4  overrideColor;

void main()
{
    if (useOverrideColor)
        color = overrideColor;
    else
        color = finalColor;
}
)glsl";

// ─── Main ────────────────────────────────────────────────────
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Object Exercise – Transformacoes 3D", NULL, NULL);
    if (!window) {
        cerr << "Falha ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    setupScene();
    printControls();

    GLuint modelLoc      = glGetUniformLocation(shaderID, "model");
    GLuint viewLoc       = glGetUniformLocation(shaderID, "view");
    GLuint projLoc       = glGetUniformLocation(shaderID, "projection");
    GLuint overrideLoc   = glGetUniformLocation(shaderID, "useOverrideColor");
    GLuint overrideClrLoc = glGetUniformLocation(shaderID, "overrideColor");

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection;
        if (perspective)
            projection = glm::perspective(glm::radians(45.0f),
                                          (float)WIDTH / (float)HEIGHT,
                                          0.1f, 100.0f);
        else
            projection = glm::ortho(-5.0f, 5.0f, -4.0f, 4.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        for (int i = 0; i < (int)scene.size(); i++) {
            SceneObject& obj = scene[i];

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
            model = glm::scale(model, obj.scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Pass 1: filled solid
            glUniform1i(overrideLoc, GL_FALSE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);

            // Pass 2: wireframe overlay
            glUniform1i(overrideLoc, GL_TRUE);
            if (i == selectedObject)
                glUniform4f(overrideClrLoc, 1.0f, 1.0f, 1.0f, 1.0f);
            else
                glUniform4f(overrideClrLoc, 0.05f, 0.05f, 0.05f, 1.0f);

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.0f);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0f, -1.0f);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
            glDisable(GL_POLYGON_OFFSET_LINE);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto& obj : scene)
        glDeleteVertexArrays(1, &obj.VAO);

    glfwTerminate();
    return 0;
}

// ─── OBJ Loader (handles triangles, quads, n-gons) ──────────
vector<VertexData> loadOBJ(const string& path, glm::vec3 color)
{
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo: " << path << endl;
        return {};
    }

    vector<glm::vec3> temp_vertices;
    vector<VertexData> vertices;
    string line;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        stringstream ss(line);
        string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        else if (type == "f") {
            vector<int> faceIndices;
            string token;
            while (ss >> token) {
                int idx = stoi(token.substr(0, token.find('/'))) - 1;
                faceIndices.push_back(idx);
            }
            // Fan triangulation for n-gon faces
            for (size_t j = 1; j + 1 < faceIndices.size(); j++) {
                int i0 = faceIndices[0];
                int i1 = faceIndices[j];
                int i2 = faceIndices[j + 1];
                glm::vec3 p0 = temp_vertices[i0];
                glm::vec3 p1 = temp_vertices[i1];
                glm::vec3 p2 = temp_vertices[i2];
                vertices.push_back({p0.x, p0.y, p0.z, color.r, color.g, color.b});
                vertices.push_back({p1.x, p1.y, p1.z, color.r, color.g, color.b});
                vertices.push_back({p2.x, p2.y, p2.z, color.r, color.g, color.b});
            }
        }
    }

    cout << "Carregado: " << path << " (" << vertices.size() << " vertices)" << endl;
    return vertices;
}

// ─── Create a SceneObject from an .obj file ──────────────────
SceneObject createObject(const string& path, const string& name,
                          glm::vec3 color, glm::vec3 pos)
{
    vector<VertexData> vertices = loadOBJ(path, color);

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(VertexData),
                 vertices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    SceneObject obj;
    obj.VAO         = VAO;
    obj.vertexCount = vertices.size();
    obj.position    = pos;
    obj.rotation    = glm::vec3(0.0f);
    obj.scale       = glm::vec3(1.0f);
    obj.color       = color;
    obj.name        = name;
    return obj;
}

// ─── Scene Setup (2x2 grid) ─────────────────────────────────
void setupScene()
{
    float spacing = 2.5f;

    scene.push_back(createObject("assets/suzanne.obj", "Suzanne",
                                  glm::vec3(1.0f, 0.8f, 0.0f),
                                  glm::vec3(-spacing / 2,  spacing / 2, 0)));

    scene.push_back(createObject("assets/cube.obj", "Cube",
                                  glm::vec3(0.0f, 0.8f, 0.6f),
                                  glm::vec3( spacing / 2,  spacing / 2, 0)));

    scene.push_back(createObject("assets/airplane.obj", "Airplane",
                                  glm::vec3(0.2f, 0.4f, 1.0f),
                                  glm::vec3(-spacing / 2, -spacing / 2, 0)));

    scene.push_back(createObject("assets/robot.obj", "Robot",
                                  glm::vec3(0.9f, 0.2f, 0.4f),
                                  glm::vec3( spacing / 2, -spacing / 2, 0)));

    cout << "\n>> " << scene.size() << " objetos carregados na cena." << endl;
    cout << ">> Objeto selecionado: [0] " << scene[0].name << endl;
    cout << ">> Modo atual: " << modeNames[currentMode] << endl << endl;
}

// ─── Key Callback (single-press actions) ─────────────────────
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_P)
        perspective = !perspective;

    // Switch transform mode
    if (key == GLFW_KEY_1) {
        currentMode = MODE_ROTATE;
        cout << "Modo: " << modeNames[currentMode] << endl;
    }
    if (key == GLFW_KEY_2) {
        currentMode = MODE_TRANSLATE;
        cout << "Modo: " << modeNames[currentMode] << endl;
    }
    if (key == GLFW_KEY_3) {
        currentMode = MODE_SCALE;
        cout << "Modo: " << modeNames[currentMode] << endl;
    }

    // Cycle object selection
    if (key == GLFW_KEY_TAB) {
        selectedObject = (selectedObject + 1) % scene.size();
        cout << "Selecionado: [" << selectedObject << "] "
             << scene[selectedObject].name << endl;
    }
}

// ─── Continuous Input (held keys) ────────────────────────────
void processInput(GLFWwindow* window)
{
    if (scene.empty()) return;
    SceneObject& obj = scene[selectedObject];

    float rotSpeed   = 90.0f  * deltaTime;
    float transSpeed = 2.0f   * deltaTime;
    float scaleSpeed = 1.5f   * deltaTime;

    switch (currentMode) {
    case MODE_ROTATE:
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            obj.rotation.x -= rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            obj.rotation.x += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            obj.rotation.y += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            obj.rotation.y -= rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            obj.rotation.z += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            obj.rotation.z -= rotSpeed;
        break;

    case MODE_TRANSLATE:
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            obj.position.x -= transSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            obj.position.x += transSpeed;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            obj.position.y += transSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            obj.position.y -= transSpeed;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            obj.position.z += transSpeed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            obj.position.z -= transSpeed;
        break;

    case MODE_SCALE:
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            obj.scale.x = glm::max(0.1f, obj.scale.x - scaleSpeed);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            obj.scale.x += scaleSpeed;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            obj.scale.y += scaleSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            obj.scale.y = glm::max(0.1f, obj.scale.y - scaleSpeed);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            obj.scale.z += scaleSpeed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            obj.scale.z = glm::max(0.1f, obj.scale.z - scaleSpeed);

        // Uniform scale: + / -
        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
            obj.scale += glm::vec3(scaleSpeed);
        }
        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
            obj.scale -= glm::vec3(scaleSpeed);
            obj.scale = glm::max(obj.scale, glm::vec3(0.1f));
        }
        break;
    }
}

// ─── Shader Setup ────────────────────────────────────────────
int setupShader()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    GLint success;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vs, 512, NULL, log);
        cerr << "Vertex shader error:\n" << log << endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(fs, 512, NULL, log);
        cerr << "Fragment shader error:\n" << log << endl;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, NULL, log);
        cerr << "Shader link error:\n" << log << endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ─── Print Controls ──────────────────────────────────────────
void printControls()
{
    cout << "========================================" << endl;
    cout << "   CONTROLES – Object Exercise" << endl;
    cout << "========================================" << endl;
    cout << " TAB        : Proximo objeto (ciclico)" << endl;
    cout << " 1          : Modo ROTACAO" << endl;
    cout << " 2          : Modo TRANSLACAO" << endl;
    cout << " 3          : Modo ESCALA" << endl;
    cout << "----------------------------------------" << endl;
    cout << " W/S ou ↑/↓ : Eixo Y (ou X rot)" << endl;
    cout << " A/D ou ←/→ : Eixo X" << endl;
    cout << " Q/E        : Eixo Z" << endl;
    cout << " +/-        : Escala uniforme (modo 3)" << endl;
    cout << "----------------------------------------" << endl;
    cout << " P          : Perspectiva / Ortografica" << endl;
    cout << " ESC        : Sair" << endl;
    cout << "========================================" << endl;
    cout << endl;
}
