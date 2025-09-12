#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <cfloat>
#include <iomanip>
#include <unordered_map>

// Evite incluir tinygltf aqui com IMPLEMENTATION para não gerar múltiplas definições.
// Apenas adiante as declarações necessárias.
namespace tinygltf {
class Model;
class Primitive;
class Accessor;
}

// Estrutura para armazenar dados do mesh
struct Mesh {
    GLuint VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    size_t indexCount;
    bool isValid;
    std::string name;

    Mesh() : VAO(0), VBO(0), EBO(0), indexCount(0), isValid(false) {}
};

// Estrutura para bounding box de colisão
struct BoundingBox {
    glm::vec3 min, max;
    std::string meshName;
    
    bool contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
};

// Estrutura para portas
struct Door {
    std::string name;
    int meshIndex = -1;
    BoundingBox box;
    bool hingeLeft = true;
    bool isOpen = false;
    float angle = 0.0f;
    float target = 0.0f;
    float speed = 180.0f;
    glm::vec3 hinge;
};

class GLTFRenderer {
private:
    std::vector<Mesh> meshes;
    std::vector<BoundingBox> collisionBoxes;
    GLuint shaderProgram;
    glm::mat4 model, view, projection;

    // Portas
    std::vector<Door> doors;
    std::unordered_map<std::string, int> doorIndexByName;

    // Variáveis para armazenar as dimensões do modelo
    glm::vec3 modelSize;
    glm::vec3 modelCenter;

    // Chão
    GLuint floorVAO = 0, floorVBO = 0;
    glm::vec3 floorColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 floorMin, floorMax;

    // Texturas
    GLuint floorTexture = 0;
    GLuint checkerTexture = 0;
    GLuint chaoTexture = 0;
    bool useFloorTexture = false;
    int textureType = 0;
    int chaoMeshIndex = -1;
    float chaoWorldTexScale = 0.5f;

    // Camera variables
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    glm::vec3 cameraRight;
    glm::vec3 cameraTarget;
    float walkHeight;
    bool freeVerticalMovement; // Controla se a câmera está em modo de movimento vertical livre
    double lastPositionUpdate;
    float cameraSpeed;
    float mouseSensitivityX;
    float mouseSensitivityY;
    float keySensitivity;
    float yaw, pitch;
    bool firstMouse;
    float lastX, lastY;
    float lastGroundY = 0.0f;
    float groundFollowLerp = 0.5f;

    // Shaders are defined in the .cpp at file scope

    // Métodos privados
    bool loadPrimitive(const tinygltf::Primitive& primitive,
                       const tinygltf::Model& model,
                       const std::vector<unsigned char>& binaryData,
                       const std::string& meshName = "",
                       const glm::mat4& nodeTransform = glm::mat4(1.0f));
    
    bool loadAccessorData(const tinygltf::Accessor& accessor,
                          const tinygltf::Model& model,
                          const std::vector<unsigned char>& binaryData,
                          std::vector<float>& out,
                          bool isTexCoord = false);
    
    bool loadAccessorIndices(const tinygltf::Accessor& accessor, 
                             const tinygltf::Model& model, 
                             const std::vector<unsigned char>& binaryData, 
                             std::vector<unsigned int>& indices);
    
    GLuint compileShader(GLenum type, const char* source);
    bool setupMeshBuffers(Mesh& mesh);
    void setMatrix4(const std::string& name, const glm::mat4& mat);
    void setVec3(const std::string& name, const glm::vec3& vec);
    void setBool(const std::string& name, bool value);
    GLuint createTextureFromFile(const std::string& path, bool flipY = false);
    void updateCameraVectors();
    void updateCameraView();
    bool checkCollision(const glm::vec3& newPos);
    float groundHeightAt(const glm::vec3& posXZ);

    // (sem otimizações de culling)

public:
    GLTFRenderer();
    
    // Texturas
    GLuint createCheckerboardTexture(int width, int height, int checkerSize);
    GLuint createStoneTexture(int width, int height);
    GLuint createGridTexture(int width, int height);
    void createFloor();
    
    // OpenGL
    bool initOpenGL();
    
    // GLTF
    bool loadGLTF(const std::string& filepath);
    // Carregar com transformação base adicional (para posicionar itens no mundo)
    bool loadGLTF(const std::string& filepath, const glm::mat4& baseTransform);
    // Carregar em uma posição do mundo (Y ajustado ao chão)
    bool loadGLTFAt(const std::string& filepath, const glm::vec3& worldPos);
    // Carregar em uma posição do mundo com rotação no eixo Z (graus). Y é ajustado ao chão
    bool loadGLTFAtRotZ(const std::string& filepath, const glm::vec3& worldPos, float degreesZ);
    // Carregar em uma posição do mundo com rotação no eixo Y (graus). Y é ajustado ao chão
    bool loadGLTFAtRotY(const std::string& filepath, const glm::vec3& worldPos, float degreesY);
    // Carregar próximo a um mesh âncora, com deslocamento XZ (Y ajustado ao chão)
    bool loadGLTFAtNear(const std::string& filepath, const std::string& anchorMesh, const glm::vec2& offsetXZ);
    // Carregar sobre o 'chao' usando deslocamento XZ relativo ao centro do chao (Y ajustado ao chão)
    bool loadGLTFAtOnChao(const std::string& filepath, const glm::vec2& offsetXZFromCenter);
    void initDoors();
    
    // Renderização
    void render();
    
    // Movimento e controles
    void processMovement(int direction, float deltaTime);
    void processVerticalMovement(int direction, float deltaTime); // Shift=subir, Space=descer
    void updateDoors(float deltaTime);
    void toggleNearestDoor();
    void rotate(float yawOffset, float pitchOffset);
    void processKeyboardRotation(int direction, float deltaTime);
    void toggleFloorTexture();
    void updateRealTimePosition(double currentTime);
    void toggleRealTimePosition();

    // Spawn helpers
    void spawnInFrontOf(const std::string& meshName, float distance);
};
