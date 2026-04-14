/* Leitor / visualizador 3D — parser .obj (v/vt/vn), Phong, câmera FPS, múltiplos objetos */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstddef>
#include <algorithm>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "Camera.h"

const GLuint WIDTH = 1000, HEIGHT = 800;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

struct SceneObject {
    GLuint VAO = 0;
    GLuint VBO = 0;
    size_t vertexCount = 0;
    glm::vec3 position{0.f};
    glm::vec3 rotation{0.f};
    glm::vec3 scale{1.f};
    glm::vec3 albedo{0.8f};
    string name;

    glm::vec3 matKa{0.15f};
    glm::vec3 matKd{0.75f};
    glm::vec3 matKs{0.35f};
    float shininess = 48.f;
};

enum TransformMode { MODE_ROTATE = 0, MODE_TRANSLATE, MODE_SCALE };

Camera camera(glm::vec3(0.0f, 1.2f, 5.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -12.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool perspective = true;

int selectedObject = 0;
TransformMode currentMode = MODE_TRANSLATE;
vector<SceneObject> scene;

glm::vec3 lightPos(2.5f, 4.0f, 3.0f);
glm::vec3 lightColor(1.0f, 0.98f, 0.92f);

bool cameraMode = false;
bool showWireOverlay = false;
bool showGridAxes = true;

bool firstMouse = true;
double lastMouseX = 0.0, lastMouseY = 0.0;

GLuint lineShaderProgram = 0;
GLuint gridVAO = 0, gridVBO = 0;
GLsizei gridVertexCount = 0;
GLuint axesVAO = 0, axesVBO = 0;

const char* modeNames[] = { "ROTACAO", "TRANSLACAO", "ESCALA" };

// ─── Prototypes ──────────────────────────────────────────────
vector<Vertex> loadOBJ(const string& path);
SceneObject createObject(const string& path, const string& name,
                         glm::vec3 albedo, glm::vec3 pos);
int  setupLightingShader();
int  setupLineShader();
void setupScene();
void setupGridAndAxes();
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void printControls();
void drawGridAxes(const glm::mat4& proj, const glm::mat4& view);

// ─── Phong shaders (posição, normal, UV) ─────────────────────
const GLchar* vertexShaderSource = R"glsl(
#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * normal;
    TexCoord = texcoord;
    gl_Position = projection * view * worldPos;
}
)glsl";

const GLchar* fragmentShaderSource = R"glsl(
#version 410 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 materialKa;
uniform vec3 materialKd;
uniform vec3 materialKs;
uniform float shininess;
uniform vec3 albedo;

uniform bool  useOverrideColor;
uniform vec4  overrideColor;

out vec4 FragColor;

void main()
{
    if (useOverrideColor) {
        FragColor = overrideColor;
        return;
    }

    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);
    vec3 R = reflect(-L, N);

    vec3 ambient  = materialKa * albedo * lightColor;
    float diffK   = max(dot(N, L), 0.0);
    vec3 diffuse  = materialKd * albedo * lightColor * diffK;
    float specK   = pow(max(dot(V, R), 0.0), shininess);
    vec3 specular = materialKs * lightColor * specK;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)glsl";

const GLchar* lineVert = R"glsl(
#version 410 core
layout (location = 0) in vec3 position;
uniform mat4 mvp;
void main() { gl_Position = mvp * vec4(position, 1.0); }
)glsl";

const GLchar* lineFrag = R"glsl(
#version 410 core
out vec4 color;
uniform vec3 lineColor;
void main() { color = vec4(lineColor, 1.0); }
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
                                          "ReaderViewer3D — Phong + OBJ", NULL, NULL);
    if (!window) {
        cerr << "Falha ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    GLuint shaderID = setupLightingShader();
    lineShaderProgram = setupLineShader();
    setupScene();
    setupGridAndAxes();
    printControls();

    GLuint modelLoc       = glGetUniformLocation(shaderID, "model");
    GLuint viewLoc        = glGetUniformLocation(shaderID, "view");
    GLuint projLoc        = glGetUniformLocation(shaderID, "projection");
    GLuint viewPosLoc     = glGetUniformLocation(shaderID, "viewPos");
    GLuint lightPosLoc    = glGetUniformLocation(shaderID, "lightPos");
    GLuint lightColorLoc  = glGetUniformLocation(shaderID, "lightColor");
    GLuint matKaLoc       = glGetUniformLocation(shaderID, "materialKa");
    GLuint matKdLoc       = glGetUniformLocation(shaderID, "materialKd");
    GLuint matKsLoc       = glGetUniformLocation(shaderID, "materialKs");
    GLuint shininessLoc   = glGetUniformLocation(shaderID, "shininess");
    GLuint albedoLoc      = glGetUniformLocation(shaderID, "albedo");
    GLuint overrideLoc    = glGetUniformLocation(shaderID, "useOverrideColor");
    GLuint overrideClrLoc = glGetUniformLocation(shaderID, "overrideColor");

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection;
        if (perspective)
            projection = glm::perspective(glm::radians(45.0f),
                                          (float)WIDTH / (float)HEIGHT,
                                          0.1f, 100.0f);
        else
            projection = glm::ortho(-6.0f, 6.0f, -5.0f, 5.0f, 0.1f, 100.0f);

        glUseProgram(shaderID);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.position));
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

        for (int i = 0; i < (int)scene.size(); i++) {
            SceneObject& obj = scene[i];

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
            model = glm::scale(model, obj.scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(matKaLoc, 1, glm::value_ptr(obj.matKa));
            glUniform3fv(matKdLoc, 1, glm::value_ptr(obj.matKd));
            glUniform3fv(matKsLoc, 1, glm::value_ptr(obj.matKs));
            glUniform1f(shininessLoc, obj.shininess);
            glUniform3fv(albedoLoc, 1, glm::value_ptr(obj.albedo));

            glUniform1i(overrideLoc, GL_FALSE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)obj.vertexCount);

            if (showWireOverlay) {
                glUniform1i(overrideLoc, GL_TRUE);
                if (i == selectedObject)
                    glUniform4f(overrideClrLoc, 1.0f, 1.0f, 0.35f, 1.0f);
                else
                    glUniform4f(overrideClrLoc, 0.12f, 0.12f, 0.14f, 1.0f);

                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(1.2f);
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0f, -1.0f);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)obj.vertexCount);
                glDisable(GL_POLYGON_OFFSET_LINE);
            }
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform1i(overrideLoc, GL_FALSE);

        if (showGridAxes)
            drawGridAxes(projection, view);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto& obj : scene) {
        if (obj.VAO) glDeleteVertexArrays(1, &obj.VAO);
        if (obj.VBO) glDeleteBuffers(1, &obj.VBO);
    }
    if (gridVAO) glDeleteVertexArrays(1, &gridVAO);
    if (gridVBO) glDeleteBuffers(1, &gridVBO);
    if (axesVAO) glDeleteVertexArrays(1, &axesVAO);
    if (axesVBO) glDeleteBuffers(1, &axesVBO);
    if (lineShaderProgram) glDeleteProgram(lineShaderProgram);
    glDeleteProgram(shaderID);

    glfwTerminate();
    return 0;
}

// ─── Índices OBJ (1-based; negativos relativos ao fim) ───────
static int resolveIndex(int idx, size_t count)
{
    if (idx < 0)
        return static_cast<int>(count) + idx;
    return idx - 1;
}

static bool parseFaceCorner(const string& tok, int& vi, int& vti, int& vni)
{
    vi = vti = vni = 0;
    if (tok.empty()) return false;

    size_t s1 = tok.find('/');
    if (s1 == string::npos) {
        vi = stoi(tok);
        return true;
    }
    vi = stoi(tok.substr(0, s1));

    size_t s2 = tok.find('/', s1 + 1);
    if (s2 == string::npos) {
        if (s1 + 1 < tok.size())
            vti = stoi(tok.substr(s1 + 1));
        return true;
    }
    if (s2 > s1 + 1) {
        string mid = tok.substr(s1 + 1, s2 - s1 - 1);
        if (!mid.empty())
            vti = stoi(mid);
    }
    if (s2 + 1 < tok.size())
        vni = stoi(tok.substr(s2 + 1));
    return true;
}

static glm::vec3 faceNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    glm::vec3 e1 = b - a;
    glm::vec3 e2 = c - a;
    glm::vec3 n = glm::cross(e1, e2);
    float len = glm::length(n);
    if (len < 1e-8f) return glm::vec3(0.f, 1.f, 0.f);
    return n / len;
}

vector<Vertex> loadOBJ(const string& path)
{
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo: " << path << endl;
        return {};
    }

    vector<glm::vec3> positions;
    vector<glm::vec3> normals;
    vector<glm::vec2> texcoords;
    vector<Vertex> out;
    string line;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        stringstream ss(line);
        string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (type == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (type == "vt") {
            glm::vec2 t;
            ss >> t.x >> t.y;
            texcoords.push_back(t);
        }
        else if (type == "f") {
            struct Corner { int vi, vti, vni; };
            vector<Corner> corners;
            string tok;
            while (ss >> tok) {
                int vi = 0, vti = 0, vni = 0;
                if (!parseFaceCorner(tok, vi, vti, vni)) continue;
                Corner c;
                c.vi  = resolveIndex(vi, positions.size());
                c.vti = vti ? resolveIndex(vti, texcoords.size()) : -1;
                c.vni = vni ? resolveIndex(vni, normals.size()) : -1;
                corners.push_back(c);
            }
            if (corners.size() < 3) continue;

            auto cornerPos = [&](const Corner& c) -> glm::vec3 {
                if (c.vi < 0 || c.vi >= (int)positions.size()) return glm::vec3(0.f);
                return positions[(size_t)c.vi];
            };
            auto cornerNor = [&](const Corner& c, const glm::vec3& fn) -> glm::vec3 {
                if (c.vni >= 0 && c.vni < (int)normals.size())
                    return normals[(size_t)c.vni];
                return fn;
            };
            auto cornerUv = [&](const Corner& c) -> glm::vec2 {
                if (c.vti >= 0 && c.vti < (int)texcoords.size())
                    return texcoords[(size_t)c.vti];
                return glm::vec2(0.f);
            };

            for (size_t j = 1; j + 1 < corners.size(); j++) {
                const Corner& c0 = corners[0];
                const Corner& c1 = corners[j];
                const Corner& c2 = corners[j + 1];
                glm::vec3 p0 = cornerPos(c0);
                glm::vec3 p1 = cornerPos(c1);
                glm::vec3 p2 = cornerPos(c2);
                glm::vec3 fn = faceNormal(p0, p1, p2);

                out.push_back({p0, cornerNor(c0, fn), cornerUv(c0)});
                out.push_back({p1, cornerNor(c1, fn), cornerUv(c1)});
                out.push_back({p2, cornerNor(c2, fn), cornerUv(c2)});
            }
        }
    }

    cout << "Carregado: " << path << " (" << out.size() << " vertices expandidos)" << endl;
    return out;
}

SceneObject createObject(const string& path, const string& name,
                         glm::vec3 albedo, glm::vec3 pos)
{
    vector<Vertex> vertices = loadOBJ(path);
    if (vertices.empty()) {
        cerr << "Aviso: mesh vazia para " << path << endl;
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    SceneObject obj;
    obj.VAO         = VAO;
    obj.VBO         = VBO;
    obj.vertexCount = vertices.size();
    obj.position    = pos;
    obj.rotation    = glm::vec3(0.0f);
    obj.scale       = glm::vec3(1.0f);
    obj.albedo      = albedo;
    obj.name        = name;
    return obj;
}

void setupScene()
{
    float spacing = 2.5f;

    scene.push_back(createObject("assets/suzanne.obj", "Suzanne",
                                 glm::vec3(1.0f, 0.75f, 0.35f),
                                 glm::vec3(-spacing / 2, 0.0f, spacing / 2)));

    scene.push_back(createObject("assets/cube.obj", "Cube",
                                 glm::vec3(0.35f, 0.85f, 0.55f),
                                 glm::vec3(spacing / 2, 0.0f, spacing / 2)));

    scene.push_back(createObject("assets/airplane.obj", "Airplane",
                                 glm::vec3(0.25f, 0.45f, 1.0f),
                                 glm::vec3(-spacing / 2, 0.0f, -spacing / 2)));

    scene.push_back(createObject("assets/robot.obj", "Robot",
                                 glm::vec3(0.95f, 0.25f, 0.35f),
                                 glm::vec3(spacing / 2, 0.0f, -spacing / 2)));

    cout << "\n>> " << scene.size() << " objetos na cena." << endl;
    if (!scene.empty()) {
        cout << ">> Selecionado: [0] " << scene[0].name << endl;
        cout << ">> Modo: " << modeNames[currentMode] << endl << endl;
    }
}

void setupGridAndAxes()
{
    const float extent = 14.f;
    const float step = 1.f;
    vector<glm::vec3> gridVerts;
    for (float z = -extent; z <= extent + 1e-4f; z += step) {
        gridVerts.push_back(glm::vec3(-extent, 0.f, z));
        gridVerts.push_back(glm::vec3( extent, 0.f, z));
    }
    for (float x = -extent; x <= extent + 1e-4f; x += step) {
        gridVerts.push_back(glm::vec3(x, 0.f, -extent));
        gridVerts.push_back(glm::vec3(x, 0.f,  extent));
    }
    gridVertexCount = (GLsizei)gridVerts.size();

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(glm::vec3), gridVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    vector<glm::vec3> axVerts = {
        glm::vec3(0, 0, 0), glm::vec3(2.5f, 0, 0),
        glm::vec3(0, 0, 0), glm::vec3(0, 2.5f, 0),
        glm::vec3(0, 0, 0), glm::vec3(0, 0, 2.5f),
    };
    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);
    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, axVerts.size() * sizeof(glm::vec3), axVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void drawGridAxes(const glm::mat4& proj, const glm::mat4& view)
{
    glm::mat4 vp = proj * view;
    GLint mvpLoc = glGetUniformLocation(lineShaderProgram, "mvp");
    GLint colLoc = glGetUniformLocation(lineShaderProgram, "lineColor");

    glUseProgram(lineShaderProgram);
    glm::mat4 id(1.f);

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * id));
    glUniform3f(colLoc, 0.28f, 0.30f, 0.34f);
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertexCount);

    glLineWidth(2.5f);
    glBindVertexArray(axesVAO);
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * id));
    glUniform3f(colLoc, 0.95f, 0.25f, 0.2f);
    glDrawArrays(GL_LINES, 0, 2);
    glUniform3f(colLoc, 0.25f, 0.9f, 0.35f);
    glDrawArrays(GL_LINES, 2, 2);
    glUniform3f(colLoc, 0.25f, 0.45f, 1.f);
    glDrawArrays(GL_LINES, 4, 2);
    glLineWidth(1.2f);
    glBindVertexArray(0);
}

void mouse_callback(GLFWwindow*, double xpos, double ypos)
{
    if (!cameraMode) return;

    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }
    float xoff = static_cast<float>(xpos - lastMouseX);
    float yoff = static_cast<float>(lastMouseY - ypos);
    lastMouseX = xpos;
    lastMouseY = ypos;

    camera.processMouseMovement(xoff, yoff);
}

void key_callback(GLFWwindow* window, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_P)
        perspective = !perspective;

    if (key == GLFW_KEY_C) {
        cameraMode = !cameraMode;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR,
                         cameraMode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        cout << (cameraMode ? ">> Modo CAMERA (FPS): WASD, mouse, Space/Shift\n"
                            : ">> Modo OBJETO: transformacoes no objeto\n");
    }

    if (key == GLFW_KEY_O)
        showWireOverlay = !showWireOverlay;

    if (key == GLFW_KEY_G)
        showGridAxes = !showGridAxes;

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

    if (key == GLFW_KEY_TAB && !scene.empty()) {
        selectedObject = (selectedObject + 1) % (int)scene.size();
        cout << "Selecionado: [" << selectedObject << "] "
             << scene[selectedObject].name << endl;
    }

    if (!scene.empty()) {
        SceneObject& o = scene[selectedObject];
        if (key == GLFW_KEY_LEFT_BRACKET)
            o.shininess = glm::max(2.f, o.shininess - 6.f);
        if (key == GLFW_KEY_RIGHT_BRACKET)
            o.shininess = glm::min(256.f, o.shininess + 6.f);
        if (key == GLFW_KEY_R)
            o.matKa = glm::max(o.matKa * 0.92f, glm::vec3(0.01f));
        if (key == GLFW_KEY_T)
            o.matKa = glm::min(o.matKa * 1.08f, glm::vec3(1.f));
        if (key == GLFW_KEY_Y)
            o.matKd = glm::max(o.matKd * 0.92f, glm::vec3(0.01f));
        if (key == GLFW_KEY_U)
            o.matKd = glm::min(o.matKd * 1.08f, glm::vec3(1.f));
        if (key == GLFW_KEY_Z)
            o.matKs = glm::max(o.matKs * 0.9f, glm::vec3(0.0f));
        if (key == GLFW_KEY_X)
            o.matKs = glm::min(o.matKs * 1.1f, glm::vec3(1.f));
    }
}

void processInput(GLFWwindow* window)
{
    if (scene.empty()) return;

    if (cameraMode) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard("FORWARD", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard("BACKWARD", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard("LEFT", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard("RIGHT", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.processKeyboard("UP", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera.processKeyboard("DOWN", deltaTime);
        return;
    }

    SceneObject& obj = scene[selectedObject];
    float rotSpeed     = 90.0f * deltaTime;
    float transSpeed   = 2.0f * deltaTime;
    float scaleSpeed   = 1.5f * deltaTime;
    float lightSpeed   = 4.0f * deltaTime;

    bool moveLight = (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS);
    if (moveLight) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) lightPos.y += lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) lightPos.y -= lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) lightPos.x -= lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) lightPos.x += lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) lightPos.z -= lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) lightPos.z += lightSpeed;
    }

    switch (currentMode) {
    case MODE_ROTATE:
        if (moveLight) break;
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
        if (moveLight) break;
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
        if (moveLight) break;
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

int setupLightingShader()
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

int setupLineShader()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &lineVert, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &lineFrag, NULL);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void printControls()
{
    cout << "========================================" << endl;
    cout << "   ReaderViewer3D — Etapa 1 (Phong)" << endl;
    cout << "========================================" << endl;
    cout << " C          : Alterna modo CAMERA FPS / OBJETO" << endl;
    cout << "   (Camera) WASD, mouse olhar, Space/Shift vertical" << endl;
    cout << " TAB        : Proximo objeto" << endl;
    cout << " 1 / 2 / 3  : Modo ROTACAO / TRANSLACAO / ESCALA" << endl;
    cout << " W A S D    : Eixos (ver modo) — setas equivalentes" << endl;
    cout << " Q / E      : Eixo Z (objeto) ou Z da luz (com L)" << endl;
    cout << " +/-        : Escala uniforme (modo 3)" << endl;
    cout << " L + WASD.. : Move a LUZ pontual (segure L)" << endl;
    cout << "----------------------------------------" << endl;
    cout << " P          : Perspectiva / Ortografica" << endl;
    cout << " O          : Wireframe sobreposto (liga/desliga)" << endl;
    cout << " G          : Grid + eixos (liga/desliga)" << endl;
    cout << " [ ]        : Shininess -/+" << endl;
    cout << " R / T      : ka -/+" << endl;
    cout << " Y / U      : kd -/+" << endl;
    cout << " Z / X      : ks -/+" << endl;
    cout << " ESC        : Sair" << endl;
    cout << "========================================" << endl << endl;
}
