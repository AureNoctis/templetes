#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define GL_GLEXT_PROTOTYPES
// to expose function declarations (prototypes) for modern OpenGL
// functions and extensions within the standard Khronos headers
#include "glad/include/glad/glad.h"
#include <GLFW/glfw3.h>


#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

#define _then_

// ========= shader ==========
const char* failed_vert_source =
    "#version 460 core\n"
    "\n"
    "void main(void) {\n"
    "    int gray = gl_VertexID ^ (gl_VertexID >> 1);\n"
    "\n"
    "    gl_Position = vec4(\n"
    "        2.0 * (gray / 2) - 1.0,\n"
    "        2.0 * (gray % 2) - 1.0,\n"
    "        0.0,\n"
    "        1.0\n"
    "    );\n"
    "};\n";

const char* failed_frag_source =
    "#version 460 core\n"
    "\n"
    "out vec4 color;\n"
    "\n"
    "void main(void){\n"
    "    color = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "};\n";


// ========= global ==========
GLuint program = 0;
GLuint failed_program = 0;



void panic_errno(const char* fmt, ...){
    fprintf(stderr, "ERROR: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
}

char* slrup_file(const char* file_path){
#define SLRUP_FILE_PANIC panic_errno("Could not read file `%s`", file_path);

    FILE* f = fopen(file_path, "r");
    if(f == NULL)                   _then_   SLRUP_FILE_PANIC
    if(fseek(f, 0, SEEK_END) < 0)   _then_   SLRUP_FILE_PANIC

    long size = ftell(f);
    if(size < 0)                    _then_   SLRUP_FILE_PANIC

    char* buffer = malloc(size + 1);
    if(buffer == NULL)              _then_   SLRUP_FILE_PANIC

    if(fseek(f, 0, SEEK_SET) < 0)   _then_   SLRUP_FILE_PANIC
    fread(buffer, 1, size, f);

    if(ferror(f) < 0)               _then_   SLRUP_FILE_PANIC
    buffer[size] = '\0';

    fclose(f);
    return buffer;
#undef SLRUP_FILE_PANIC
}

bool compile_shader_source(const char* source, GLenum shader_type, GLuint* shader){
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled){
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }
    return true;
}

bool compile_shader_file(const char* file, GLenum shader_type, GLuint* shader){
    const char* source = slrup_file(file);
    bool result = compile_shader_source(source, shader_type, shader);

    free((void*)source);
    return result;
}

bool link_program(GLuint vert_shader, GLuint frag_shader, GLuint* program){
    *program = glCreateProgram();

    glAttachShader(*program, vert_shader);
    glAttachShader(*program, frag_shader);
    glLinkProgram(*program);

    GLint linked = 0;
    glGetProgramiv(*program, GL_LINK_STATUS, &linked);
    if(!linked){
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
        fprintf(stderr, "Program Linking: %.*s\n",message_size, message);
        return false;
    }
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    return true;
}

void init_failed_program(void){
    GLuint vert = 0;
    if(!compile_shader_source(failed_vert_source, GL_VERTEX_SHADER, &vert))    _then_   exit(1);

    GLuint frag = 0;
    if(!compile_shader_source(failed_frag_source, GL_FRAGMENT_SHADER, &frag))  _then_   exit(1);

    if(!link_program(vert, frag, &failed_program))                             _then_   exit(1);
}

void reload_shader(void){
    static int counter = 1;
    glDeleteProgram(program);

    GLuint frag = 0;
    if(!compile_shader_file("vizualiser.frag", GL_FRAGMENT_SHADER, &frag)) {
        glUseProgram(failed_program);
        return;
    }

    GLuint vert = 0;
    if(!compile_shader_file("vizualiser.vert", GL_VERTEX_SHADER, &vert)) {
        glUseProgram(failed_program);
        return;
    }

    GLuint program = 0;
    if(!link_program(vert, frag, &program)) {
        glUseProgram(failed_program);
        return;
    }

    glUseProgram(program);
    printf("%d : Successfully reloaded the shaders\n", counter++);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    (void) window;
    (void) key;
    (void) scancode;
    (void) action;
    (void) mods;

    if(key == GLFW_KEY_F5 && action == GLFW_PRESS){
        reload_shader();
    }
}

void window_resize_callback(GLFWwindow* window, int width, int height){
    (void) window;

    glViewport((width - SCREEN_WIDTH) >> 1,
                (height - SCREEN_HEIGHT) >> 1,
                SCREEN_WIDTH,
                SCREEN_HEIGHT);
}

int main(){

    if(!glfwInit()){
        fprintf(stderr, "ERROR: could not initialize GLFW\n");
        exit(1);
    }

    GLFWwindow* window = glfwCreateWindow(
                            SCREEN_WIDTH,
                            SCREEN_HEIGHT,
                            "automata",
                            NULL,
                            NULL);
    if(window == NULL){
        fprintf(stderr, "ERROR: could not create a window\n");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "ERROR: Failed to initialize GLAD\n");
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, window_resize_callback);

    init_failed_program();
    reload_shader();

    while(!glfwWindowShouldClose(window)){
        glClear(GL_COLOR_BUFFER_BIT);
        // glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

        glDrawArrays(GL_QUADS, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}