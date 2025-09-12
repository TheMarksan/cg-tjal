#include "GLTFRenderer.h"
#include <cstring>
#include "../tjal-modelC/lib/tinygltf/stb_image.h"

GLuint GLTFRenderer::createTextureFromFile(const std::string& path, bool flipY) {
    GLuint texture = 0;
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Falha ao carregar textura: " << path << std::endl;
        return 0;
    }
    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return texture;
}

GLuint GLTFRenderer::createCheckerboardTexture(int width, int height, int checkerSize) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Criar dados da textura de tabuleiro de xadrez
    std::vector<unsigned char> data(width * height * 3);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 3;
            bool isWhite = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
            
            if (isWhite) {
                data[index] = 220;     // R - branco cremoso
                data[index + 1] = 220; // G
                data[index + 2] = 200; // B
            } else {
                data[index] = 50;      // R - cinza escuro
                data[index + 1] = 50;  // G
                data[index + 2] = 70;  // B - ligeiramente azulado
            }
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    // Configurar parâmetros da textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

GLuint GLTFRenderer::createStoneTexture(int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Criar textura de pedra mais realística
    std::vector<unsigned char> data(width * height * 3);
    srand(42); // seed fixo para resultado consistente
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 3;
            
            // Base cinza com padrão de pedra
            int baseGray = 100 + (x/16 + y/16) % 2 * 20; // padrão de blocos
            int noise = (rand() % 40) - 20; // ruído
            
            // Adicionar "juntas" entre pedras
            bool isJoint = (x % 64 < 2) || (y % 64 < 2);
            if (isJoint) baseGray -= 30;
            
            unsigned char gray = std::max(40, std::min(180, baseGray + noise));
            
            // Dar um tom ligeiramente marrom/bege
            data[index] = gray;                    // R
            data[index + 1] = gray * 0.9f;       // G (um pouco menos verde)
            data[index + 2] = gray * 0.8f;       // B (bem menos azul)
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

GLuint GLTFRenderer::createGridTexture(int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Criar grid procedural
    std::vector<unsigned char> data(width * height * 3);
    int gridSize = 32; // Tamanho das células do grid
    int lineWidth = 2; // Largura das linhas do grid
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 3;
            
            // Verificar se está numa linha do grid
            bool isGridLine = (x % gridSize < lineWidth) || (y % gridSize < lineWidth);
            
            if (isGridLine) {
                // Linhas do grid - cinza muito claro para suavidade
                data[index] = 220;     // R
                data[index + 1] = 220; // G  
                data[index + 2] = 220; // B
            } else {
                // Fundo - branco como nuvem
                data[index] = 255;     // R
                data[index + 1] = 255; // G
                data[index + 2] = 255; // B
            }
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    // Configurar para repetição infinita
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void GLTFRenderer::createFloor() {
    // Chão infinito em grid - área muito maior
    float y = 0.0f;
    float size = 100.0f; // Grid muito maior para parecer infinito
    float minX = -size, maxX = size;
    float minZ = -size, maxZ = size;
    
    // Coordenadas de textura maiores para criar efeito de repetição do grid
    float texScale = 20.0f; // Repetir o grid 20 vezes na área
    
    float vertices[] = {
        minX, y, minZ,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        maxX, y, minZ,   0.0f, 1.0f, 0.0f,   texScale, 0.0f,
        maxX, y, maxZ,   0.0f, 1.0f, 0.0f,   texScale, texScale,
        minX, y, maxZ,   0.0f, 1.0f, 0.0f,   0.0f, texScale
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    GLuint floorEBO;
    glGenBuffers(1, &floorEBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    
    // Criar textura de grid infinito
    floorTexture = createGridTexture(256, 256);
    checkerTexture = createCheckerboardTexture(256, 256, 32);
    textureType = 1; // Usar grid por padrão
    useFloorTexture = true;
}

void GLTFRenderer::toggleFloorTexture() {
    textureType = (textureType + 1) % 3;
    switch(textureType) {
        case 0:
            useFloorTexture = false; // Cor sólida
            break;
        case 1:
            useFloorTexture = true;  // Grid infinito
            break;
        case 2:
            useFloorTexture = true;  // Xadrez
            break;
    }
}
