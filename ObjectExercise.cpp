/* OBJ Viewer */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"

const GLuint WIDTH = 600, HEIGHT = 600;

void processInput(GLFWwindow *window);

bool rotateX=false, rotateY=false, rotateZ=false;
bool perspective = true;

Camera camera(glm::vec3(0.0, 0.0, -3.0), glm::vec3(0.0,1.0,0.0),90.0,0.0);
float deltaTime = 0.0;
float lastFrame = 0.0;

GLuint VAO;
size_t vertexCount = 0;

struct VertexData {
    float x,y,z;
    float r,g,b;
};

struct SceneObject {
    GLuint VAO;
    size_t vertexCount;
    glm::vec3 position;
    glm::vec3 rotation;
};

SceneObject createObject(string path, glm::vec3 color, glm::vec3 pos);
vector<VertexData> loadOBJ(string path, glm::vec3 color);

int selectedObject = 0;
vector<SceneObject> scene;
vector<VertexData> meshVertices;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int setupGeometry();
void loadOBJ(string path);

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
    gl_Position = projection * view * model * vec4(position,1.0);
    finalColor = vec4(color,1.0);
}
)glsl";

const GLchar* fragmentShaderSource = R"glsl(
#version 410 core
in vec4 finalColor;
out vec4 color;
void main(){ color = finalColor; }
)glsl";

int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH,HEIGHT,"Object Exercise",NULL,NULL);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window,key_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    VAO = setupGeometry();

    glUseProgram(shaderID);

    while (!glfwWindowShouldClose(window)){
        processInput(window);

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Matrix View (camera)
        glm::mat4 view = camera.getViewMatrix();

        // Matrix Projection
        glm::mat4 projection;
        if (perspective)
            projection = glm::perspective(glm::radians(45.0f),
                                        (float)WIDTH/(float)HEIGHT,
                                        0.1f, 100.0f);
        else
            projection = glm::ortho(-3.0f,3.0f,-3.0f,3.0f,0.1f,100.0f);

        GLuint viewLoc = glGetUniformLocation(shaderID,"view");
        GLuint projLoc = glGetUniformLocation(shaderID,"projection");

        glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(projLoc,1,GL_FALSE,glm::value_ptr(projection));

        // Draw the objects in the screen
        for(int i=0;i<scene.size();i++) {

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, scene[i].position);

            // Rotation
            model = glm::rotate(model, glm::radians(scene[i].rotation.x), glm::vec3(1,0,0));
            model = glm::rotate(model, glm::radians(scene[i].rotation.y), glm::vec3(0,1,0));
            model = glm::rotate(model, glm::radians(scene[i].rotation.z), glm::vec3(0,0,1));

            // Set in the screen the selected object
            if(i == selectedObject)
                model = glm::scale(model, glm::vec3(1.2f));

            GLuint modelLoc = glGetUniformLocation(shaderID,"model");
            glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(model));

            glBindVertexArray(scene[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, scene[i].vertexCount);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

vector<VertexData> loadOBJ(string path, glm::vec3 color){
    ifstream file(path);
    vector<glm::vec3> temp_vertices;
    vector<VertexData> vertices;
    string line;

    while(getline(file,line)){
        stringstream ss(line);
        string type;
        ss >> type;

        if(type=="v"){
            glm::vec3 v; ss>>v.x>>v.y>>v.z;
            temp_vertices.push_back(v);
        }

        if(type=="f"){
            for(int i=0;i<3;i++){
                string vert; ss>>vert;
                int index = stoi(vert.substr(0,vert.find('/'))) - 1;
                glm::vec3 pos = temp_vertices[index];

                vertices.push_back({
                    pos.x,pos.y,pos.z,
                    color.r,color.g,color.b
                });
            }
        }
    }
    return vertices;
}

int setupGeometry(){
    scene.push_back(createObject("assets/suzanne.obj",
                                 glm::vec3(1,1,0),       // yellow
                                 glm::vec3(1.5,0,0)));   // rigth

    scene.push_back(createObject("assets/airplane.obj",
                                 glm::vec3(0,0.3,1),     // blue
                                 glm::vec3(-1.5,0,0)));  // left
    return 0;
}

void key_callback(GLFWwindow* window,int key,int scancode,int action,int mode){
    if(key==GLFW_KEY_ESCAPE && action==GLFW_PRESS) glfwSetWindowShouldClose(window,true);
    if(key==GLFW_KEY_P && action==GLFW_PRESS) perspective=!perspective;
}

int setupShader(){
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSource,NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragmentShaderSource,NULL);
    glCompileShader(fs);

    GLuint prog = glCreateProgram();
    glAttachShader(prog,vs);
    glAttachShader(prog,fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

SceneObject createObject(string path, glm::vec3 color, glm::vec3 pos){
    vector<VertexData> vertices = loadOBJ(path,color);

    GLuint VAO,VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);

    glBufferData(GL_ARRAY_BUFFER,
        vertices.size()*sizeof(VertexData),
        vertices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(VertexData),(void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(VertexData),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    SceneObject obj;
    obj.VAO = VAO;
    obj.vertexCount = vertices.size();
    obj.position = pos;
    obj.rotation = glm::vec3(0.0f);
    return obj;
}

void processInput(GLFWwindow *window){
    // Close the window with ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Change the selected object with TAB
    static bool tabPressed = false;

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed){
        selectedObject = (selectedObject + 1) % scene.size();
        std::cout << "Selected object: " << selectedObject << std::endl;
        tabPressed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE){
        tabPressed = false;
    }
    float speed = 2.0f * deltaTime;

    SceneObject &obj = scene[selectedObject];

    float rotSpeed = 60.0f * deltaTime;

    // X Y Z for Rotation
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        obj.rotation.x += rotSpeed;

    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        obj.rotation.y += rotSpeed;

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        obj.rotation.z += rotSpeed;

    // W A S D for Translation
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        obj.position.z += speed;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        obj.position.z -= speed;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        obj.position.x -= speed;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        obj.position.x += speed;
}