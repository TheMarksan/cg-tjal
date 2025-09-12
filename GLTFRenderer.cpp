#include "GLTFRenderer.h"
#include "../tjal-modelC/lib/tinygltf/tiny_gltf.h"

static const char* kVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;
    
    out vec3 FragPos;
    out vec3 Normal;
    out vec3 vertexColor;
    out vec2 TexCoord;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        vec4 worldPos = model * vec4(aPos, 1.0);
        FragPos = worldPos.xyz;
        Normal = mat3(transpose(inverse(model))) * aNormal;
        vertexColor = (aPos + 1.0) * 0.5;
        TexCoord = aTexCoord;
        gl_Position = projection * view * worldPos;
    }
)";

static const char* kFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 FragPos;
    in vec3 Normal;
    in vec3 vertexColor;
    in vec2 TexCoord;
    
    uniform vec3 baseColor;
    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform bool useVertexColor;
    uniform bool useTexture;
    uniform bool useWorldTex;
    uniform float worldTexScale;
    uniform sampler2D ourTexture;
    
    void main() {
        vec3 color;
        
        if (useTexture) {
            vec2 uv = TexCoord;
            if (useWorldTex) {
                uv = FragPos.xz * worldTexScale;
            }
            vec3 texColor = texture(ourTexture, uv).rgb;
            color = texColor;
        } else if (useVertexColor) {
            color = vertexColor;
        } else {
            color = baseColor;
        }
        
        vec3 lightColor = vec3(1.0, 1.0, 1.0);
        vec3 ambient = 0.3 * color;
        
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor * color;
        
        vec3 result = ambient + diffuse;
        FragColor = vec4(result, 1.0);
    }
)";

GLTFRenderer::GLTFRenderer() {
    model = glm::mat4(1.0f);
    cameraPos = glm::vec3(0.0f, 0.8f, -2.5f);
    yaw = -270.0f;
    pitch = -5.0f;
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    walkHeight = 1.7f; // Altura dos olhos de uma pessoa
    lastPositionUpdate = 0.0;
    cameraSpeed = 3.0f; // Velocidade um pouco maior para pessoa maior
    mouseSensitivityX = 0.1f;
    mouseSensitivityY = 0.1f;
    keySensitivity = 0.012f;
    firstMouse = true;
    lastX = 400.0f;
    lastY = 300.0f;
    updateCameraVectors();
    updateCameraView();
    projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.01f, 100.0f);
}

GLuint GLTFRenderer::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERRO DE COMPILAÇÃO DO SHADER: " << infoLog << std::endl;
        return 0;
    }
    return shader;
}

bool GLTFRenderer::initOpenGL() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
    if (vertexShader == 0 || fragmentShader == 0) return false;

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERRO DE LINKING DO SHADER: " << infoLog << std::endl;
        return false;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glViewport(0, 0, 800, 600);
    return true;
}

void GLTFRenderer::setMatrix4(const std::string& name, const glm::mat4& mat) {
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void GLTFRenderer::setVec3(const std::string& name, const glm::vec3& vec) {
    glUniform3fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, glm::value_ptr(vec));
}

void GLTFRenderer::setBool(const std::string& name, bool value) {
    glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), (int)value);
}

// (sem utilitários de culling)
