// Apenas inclui a API do renderizador já separada
#include "GLTFRenderer.h"

GLTFRenderer* g_renderer = nullptr;

// Callback para detectar quando a janela está sendo fechada
void windowCloseCallback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// Callback para erros GLFW
void errorCallback(int error, const char* description) {
    std::cerr << "❌ Erro GLFW " << error << ": " << description << std::endl;
}

// Função para verificar erros OpenGL
void checkOpenGLError(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "❌ Erro OpenGL em " << operation << ": " << error << std::endl;
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "❌ Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    // Configurar callback de erro
    glfwSetErrorCallback(errorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "GLTF Renderer", NULL, NULL);
    if (!window) {
        std::cerr << "❌ Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Configurar callback para fechamento da janela
    glfwSetWindowCloseCallback(window, windowCloseCallback);
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Habilitar VSync para 60 FPS

    if (glewInit() != GLEW_OK) {
        std::cerr << "❌ Falha ao inicializar GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    GLTFRenderer renderer;
    g_renderer = &renderer;

    if (!renderer.initOpenGL()) return -1;

    // Tenta carregar do novo diretório models/ com variações de nome
    const char* candidates[] = {
        "models/TJAL.gltf",
        "models/tjal.gltf",
        "TJAL.gltf",
        "tjal.gltf"
    };
    bool loaded = false;
    for (const char* path : candidates) {
        if (renderer.loadGLTF(path)) { loaded = true; break; }
    }
    if (!loaded) {
        std::cerr << "Falha ao carregar o modelo GLTF (tente colocar TJAL.gltf em models/)." << std::endl;
        glfwTerminate();
        return -1;
    }

    // Posicionar o usuário em frente à escada a 3.0 unidades de distância
    renderer.spawnInFrontOf("escada", 3.0f);

    // Carregar outros móveis com posições XZ explícitas (Y ajustado ao chão automaticamente)
    renderer.loadGLTFAt("models/sofa.gltf",    glm::vec3(-4.0f, 0.0f, 8.0f));
    renderer.loadGLTFAt("models/sofa.gltf",    glm::vec3(-4.0f, 0.0f, 10.0f));
    renderer.loadGLTFAt("models/sofa.gltf",    glm::vec3(-4.0f, 0.0f, 12.0f));
    
    // Tapete no centro, bem no nível do chão
    renderer.loadGLTFAt("models/tapete.gltf",  glm::vec3(-2.0f, 0.0f, -5.5f));
    
    renderer.loadGLTFAt("models/cadeira3.gltf",glm::vec3(-1.0f, 0.0f, 9.0f));
    renderer.loadGLTFAt("models/cadeira3.gltf",glm::vec3(-1.0f, 0.0f, 11.0f));
    renderer.loadGLTFAt("models/cadeira3.gltf",glm::vec3(-1.0f, 0.0f, 13.0f));
    
    // Projetor no teto, posicionado no centro da sala - usando matriz para evitar ajuste automático do Y
    glm::mat4 projetorTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 5.5f, -12.5f));
    renderer.loadGLTF("models/projetor.gltf", projetorTransform);




    double lastTime = glfwGetTime();
    bool tabPressed = false, pPressed = false, tPressed = false, ePressed = false;
    bool f11Pressed = false;
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        frameCount++;

        if (frameCount % 2 == 0) {
            renderer.updateRealTimePosition(currentTime);
        }
        
        glfwPollEvents();

        // Controles principais
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) renderer.processMovement(0, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) renderer.processMovement(1, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) renderer.processMovement(2, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) renderer.processMovement(3, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) renderer.processVerticalMovement(0, deltaTime); // Subir
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) renderer.processVerticalMovement(1, deltaTime); // Descer
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) renderer.processKeyboardRotation(0, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) renderer.processKeyboardRotation(1, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) renderer.processKeyboardRotation(2, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) renderer.processKeyboardRotation(3, deltaTime);

        // Controles secundários (verificados menos frequentemente)
    if (frameCount % 5 == 0) {
            if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed) { renderer.toggleRealTimePosition(); pPressed = true; }
            if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) pPressed = false;

            if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) { tabPressed = true; }
            if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) tabPressed = false;
            
            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tPressed) { renderer.toggleFloorTexture(); tPressed = true; }
            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) tPressed = false;

            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !ePressed) { renderer.toggleNearestDoor(); ePressed = true; }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) ePressed = false;

            // Toggle fullscreen (F11)
            if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && !f11Pressed) {
                f11Pressed = true;
                static int prevX = 0, prevY = 0, prevW = 800, prevH = 600;
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                if (glfwGetWindowMonitor(window)) {
                    // currently fullscreen -> go windowed
                    glfwSetWindowMonitor(window, NULL, prevX, prevY, prevW, prevH, 0);
                } else {
                    // windowed -> remember size/pos and go fullscreen
                    glfwGetWindowPos(window, &prevX, &prevY);
                    glfwGetWindowSize(window, &prevW, &prevH);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                }
                
                // Reconfigurar VSync após mudança de modo
                glfwSwapInterval(1);
                
                // Update viewport after mode change
                int w, h; glfwGetFramebufferSize(window, &w, &h);
                glViewport(0, 0, w, h);
                
                // Garantir que o contexto está ativo
                glfwMakeContextCurrent(window);
            }
            if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE) f11Pressed = false;
        }

        if (frameCount % 30 == 0) {
            if (!glfwGetCurrentContext()) {
                break;
            }
        }

    // Atualizações por frame
    renderer.updateDoors(deltaTime);
    renderer.render();
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}