#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>

GLuint computeProgram;
GLuint ssbo;

// Compute shader that doubles each value in an array
const char* computeShaderSource = R"(
#version 430 core
layout(local_size_x = 1) in;
layout(std430, binding = 0) buffer Data {
    float values[];
};
void main() {
    uint idx = gl_GlobalInvocationID.x;
    values[idx] *= 2.0;
}
)";

void checkShaderCompile(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader Compilation Failed:\n" << infoLog << std::endl;
    }
}

void checkProgramLink(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Program Linking Failed:\n" << infoLog << std::endl;
    }
}

void initShader() {
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &computeShaderSource, nullptr);
    glCompileShader(computeShader);
    checkShaderCompile(computeShader);

    computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    checkProgramLink(computeProgram);

    glDeleteShader(computeShader); // Shader linked, safe to delete
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();
}

void runComputeShader() {
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};

    // Create SSBO
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    // Run the compute shader
    glUseProgram(computeProgram);
    glDispatchCompute(data.size(), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back results
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    float* ptr = (float*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (ptr) {
        std::cout << "Updated values:" << std::endl;
        for (size_t i = 0; i < data.size(); ++i) {
            std::cout << ptr[i] << " ";
        }
        std::cout << std::endl;
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Compute Shader Array Doubling");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW init error: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    initShader();
    runComputeShader();

    glutDisplayFunc(display);
    glutMainLoop();

    glDeleteProgram(computeProgram);
    glDeleteBuffers(1, &ssbo);

    return 0;
}
