//
//  Emscripten Cubes
//  This demo shows how to create code which targets both OpenGL 3.2 and Emscripten (WebGL 1.0) using SDL and GLM.
//
//  The BSD 3-Clause License:
//  Copyright (c) <YEAR>, <OWNER>
//  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
// following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
// disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
// following disclaimer in the documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
// products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <iostream>
#include <string>
#ifdef EMSCRIPTEN
#   include <emscripten.h>
#   include <SDL/SDL.h>
#   include <SDL/SDL_image.h>
#   include <GLES2/gl2.h>
#else
#   include <SDL2/SDL.h>
#   include <SDL2_image/SDL_image.h>
#   include <OpenGL/gl3.h>
#endif
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
using namespace glm;

// SDL handles and states
SDL_Window *win;
bool quit = false;

// OpenGL handles
GLuint shaderProgram;
#ifndef EMSCRIPTEN
GLuint vertexArrayObject;
#endif
GLuint vertexBuffer;
GLuint vertexElementArrayBuffer;
GLint mvpLocation;
GLint nLocation;

// Geometry
float vertex[] = {-1,-1,1,-1,1,1,1,1,1,1,-1,1,-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1,-1,-1,1,-1,-1,-1,-1,1,-1,-1,1,1,-1,1,1,-1,1,-1,1,1,-1,1,1,1,1,1,1,1,1,-1,1,-1,-1,1,-1,1,-1,-1,-1,-1,-1,1,1,-1,1,1,-1,-1};
int indices[] = {0,1,2,0,2,3,4,5,6,4,6,7,8,9,10,8,10,11,12,13,14,12,14,15,16,17,18,16,18,19,20,21,22,20,22,23};
float normal[] = {0,0,1,0,0,1,0,0,1,0,0,1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,-1,0,0,-1,0,0,-1,0,0,-1,0,0,0,1,0,0,1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0,0,1,0,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0};
float uv[] =    {0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,1};
const int vertexCount = 24;
int indexCount = 36;

void init();
void mainLoop();

int main(int argc, char** argv){

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef EMSCRIPTEN
    SDL_Surface *screen = SDL_SetVideoMode(600, 450, 32, SDL_OPENGL);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    win = SDL_CreateWindow("SDL Cubes", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    if (win == nullptr){
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_GL_CreateContext(win);
#endif

    init();
    mainLoop();
#ifndef EMSCRIPTEN
    SDL_DestroyWindow(win);
#endif
    SDL_Quit();

    return 0;
}

void bindAttributes(){
    int sizeOfFloat = sizeof(float);
    int vertexLength = 8*sizeOfFloat;
    GLint vertexLocation = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, false, vertexLength, 0);
    GLint normalLocation = glGetAttribLocation(shaderProgram, "normal");
    glEnableVertexAttribArray(normalLocation);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, false, vertexLength, BUFFER_OFFSET(3*sizeOfFloat) );
    GLint uvLocation = glGetAttribLocation(shaderProgram, "uv");
    glEnableVertexAttribArray(uvLocation);
    glVertexAttribPointer(uvLocation, 2, GL_FLOAT, false, vertexLength, BUFFER_OFFSET(6*sizeOfFloat));
}

float getCurrentTimeMillis(){
    static auto startTime = std::chrono::system_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::system_clock::now() - startTime).count();
}

void drawCube(vec3 position, vec3 scale, mat4& perspective, mat4& view){
    mat4 model = translate( position );
    model = model * rotate( getCurrentTimeMillis() * 0.002f, vec3(0,1,0));
    model = model * glm::scale(scale);

    mat4 modelView = view * model;
    mat3 normal = glm::inverseTranspose(glm::mat3(modelView));
    mat4 modelViewProjection = perspective * modelView;

    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE,  glm::value_ptr(modelViewProjection));
    glUniformMatrix3fv(nLocation, 1, GL_FALSE,  glm::value_ptr(normal));
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}


void setupShader(){
#ifndef EMSCRIPTEN
    string shaders[2] = {R"(#version 150

in vec3 position;
in vec3 normal;
in vec2 uv;

uniform mat4 mvp;
uniform mat3 n;

out vec3 vNormal;
out vec2 vUv;

void main(void) {
    gl_Position = mvp * vec4(position, 1.0);
    vNormal = normalize(n * normal);
    vUv = uv;
}

)",R"(#version 150

uniform sampler2D tex;

in vec3 vNormal;
in vec2 vUv;
out vec4 fragColor;

void main(void)
{
    vec3 lightDirEyeSpace = normalize(vec3(1.0,1.0,1.0));
    vec3 diffuseLight = vec3(max(0.0, dot(lightDirEyeSpace, vNormal)));
    vec3 ambientLight = vec3(0.2,0.2,0.2);
    vec3 light = max(diffuseLight, ambientLight);
    fragColor = vec4(  texture(tex, vUv).xyz *
        light,1.0);
}
)"};
#else
    string shaders[2] = {R"(

attribute vec3 position;
attribute vec3 normal;
attribute vec2 uv;

uniform mat4 mvp;
uniform mat3 n;

varying vec3 vNormal;
varying vec2 vUv;

void main(void) {
    gl_Position = mvp * vec4(position, 1.0);
    vNormal = normalize(n * normal);
    vUv = uv;
}

)",R"(precision mediump float;

uniform sampler2D tex;

varying vec3 vNormal;
varying vec2 vUv;


void main(void)
{
    vec3 lightDirEyeSpace = normalize(vec3(1.0,1.0,1.0));
    vec3 diffuseLight = vec3(max(0.0, dot(lightDirEyeSpace, vNormal)));
    vec3 ambientLight = vec3(0.2,0.2,0.2);
    vec3 light = max(diffuseLight, ambientLight);
    gl_FragColor = vec4(  texture2D(tex, vUv).xyz *
        light,1.0);
}
)"};
#endif
    int shaderTypes[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    int shaderObj[2];
    for (int i=0;i<2;i++){
        int shader = glCreateShader(shaderTypes[i]);
        string shaderSrc = shaders[i];
        const GLchar *shaderPtr = shaderSrc.c_str();
        int shaderLen = shaderSrc.length();
        glShaderSource(shader, 1, & shaderPtr, &shaderLen);
        glCompileShader(shader);
        shaderObj[i] = shader;
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == 0)
        {
            GLint infologLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
            if (infologLength > 0)
            {
                char* infoLog = new char[infologLength];
                glGetShaderInfoLog(shader, infologLength, 0, infoLog);
                cout << "Compile error of "<< (i==0?"vertex shader":"fragment shader")<<": " << infoLog << endl;
                delete[] infoLog;
            }
        }
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, shaderObj[0]);
    glAttachShader(shaderProgram, shaderObj[1]);
    glLinkProgram(shaderProgram);

    GLint  linked;
    glGetProgramiv( shaderProgram, GL_LINK_STATUS, &linked );
    if ( !linked ) {
        std::cerr << "Shader program failed to link" << std::endl;
        GLint  logSize;
        glGetProgramiv( shaderProgram, GL_INFO_LOG_LENGTH, &logSize);
        char* logMsg = new char[logSize];
        glGetProgramInfoLog( shaderProgram, logSize, NULL, logMsg );
        std::cerr << logMsg << std::endl;
        delete [] logMsg;
    }

    glUseProgram(shaderProgram);
    mvpLocation = glGetUniformLocation(shaderProgram, "mvp");
    nLocation = glGetUniformLocation(shaderProgram, "n");
}

void createVertexBufferObject(){

    // create interleaved data
    int vertexSize = 3 + 3 + 2;
    float *vertices = new float[vertexCount * vertexSize]; // position, normal, uv
    for (int i=0;i<vertexCount;i++){
        vertices[i*vertexSize] = vertex[i*3];
        vertices[i*vertexSize + 1] = vertex[i*3 + 1];
        vertices[i*vertexSize + 2] = vertex[i*3 + 2];
        vertices[i*vertexSize + 3] = normal[i*3];
        vertices[i*vertexSize + 4] = normal[i*3 + 1];
        vertices[i*vertexSize + 5] = normal[i*3 + 2];
        vertices[i*vertexSize + 6] = uv[i*2];
        vertices[i*vertexSize + 7] = uv[i*2 + 1];
    }
#ifndef EMSCRIPTEN
    // vertex array objects are not supported in OpenGL ES 2.0 / WebGL 1.0
    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);
#endif
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * vertexSize * sizeof(float), vertices, GL_STATIC_DRAW);
    bindAttributes();
    glGenBuffers(1, &vertexElementArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexElementArrayBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(int), indices, GL_STATIC_DRAW);

    delete[] vertices;
}

void update(){
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
        }
    }
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    const float fovy = (float) ((60.0f/180)*M_PI),
            aspect = 1.0f,
            near_ = 0.1f,
            far_ = 100.0f;

    mat4 perspective = glm::perspective(fovy, aspect, near_, far_);
    mat4 view = translate(vec3(0,0,-4));
    vec3 position (0,0,0);
    vec3 scale (0.25,0.25,0.25);
    float delta = 1;
#ifdef ENABLE_BENCHMARK
    delta = 0.01;;
    glFinish();
    float millisStart = getCurrentTimeMillis();
#endif
    for (float x = -1;x <=1 ; x=x+delta){
        position[0] = x;
        for (float y = -1;y<=1;y=y+delta){
            position[1] = y;
            drawCube(position, scale, perspective, view);
        }
    }
#ifdef ENABLE_BENCHMARK
    glFinish();
    cout << (getCurrentTimeMillis() - millisStart) << endl;
#endif
    SDL_GL_SwapWindow(win);
}

void loadTexture(){
    int flags=IMG_INIT_PNG;
    int initted=IMG_Init(flags);
    if((initted&flags) != flags) {
        printf("IMG_Init: %s\n", IMG_GetError());
        // handle error
    }
    SDL_Surface * surface = IMG_Load("cube.png");
    if (surface) {
        GLuint id;
        glGenTextures(1,&id);
        glBindTexture(GL_TEXTURE_2D, id);
#ifdef EMSCRIPTEN
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h,0,GL_RGBA,GL_UNSIGNED_BYTE,surface->pixels);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h,0,GL_BGRA,GL_UNSIGNED_BYTE,surface->pixels);
#endif
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        cerr << "Cannot load image" << endl;
    }
}


void init(){
    glClearColor( 1.0f, 1.0f, 1.0f, 0.0f );
    glEnable(GL_DEPTH_TEST);
    setupShader();
    createVertexBufferObject();
    loadTexture();
    glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0); // texture slot 0
}

void mainLoop() {
#ifdef EMSCRIPTEN
    int fps = 0;
    int simulate_infinite_loop = 1;
    emscripten_set_main_loop(update, fps, simulate_infinite_loop);
#else
    while (!quit){
        update();
        SDL_Delay(16);
    }
#endif
}
