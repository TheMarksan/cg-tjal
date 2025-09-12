#include "GLTFRenderer.h"
#include <cstring>
#include <cctype>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tjal-modelC/lib/tinygltf/tiny_gltf.h"

bool GLTFRenderer::loadGLTF(const std::string& filepath) {
    return loadGLTF(filepath, glm::mat4(1.0f));
}

bool GLTFRenderer::loadGLTF(const std::string& filepath, const glm::mat4& baseTransform) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    // Detectar extensão para suportar .gltf (ASCII) e .glb (binário)
    auto toLower = [](std::string s){ for (auto& c : s) c = (char)std::tolower(c); return s; };
    std::string lower = toLower(filepath);
    bool ok = false;
    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".glb") {
        ok = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filepath);
    } else {
        ok = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath);
    }
    if (!warn.empty()) std::cerr << "Aviso GLTF: " << warn << std::endl;
    if (!ok) {
        std::cerr << "Erro ao carregar GLTF: " << err << std::endl;
        return false;
    }

    if (gltfModel.buffers.empty()) return false;
    const auto& buffer = gltfModel.buffers[0].data;

    bool loaded = false;
    
    // Carregar meshes com suas transformações dos nós
    for (const auto& node : gltfModel.nodes) {
        if (node.mesh >= 0 && node.mesh < gltfModel.meshes.size()) {
            const auto& mesh = gltfModel.meshes[node.mesh];
            
            // Construir matriz de transformação do nó
            glm::mat4 nodeTransform(1.0f);
            if (node.matrix.size() == 16) {
                // Usar matriz diretamente se fornecida
                nodeTransform = glm::mat4(
                    node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
                    node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
                    node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
                    node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]
                );
            } else {
                // Compor TRS (Translation, Rotation, Scale)
                glm::vec3 translation(0.0f);
                glm::vec3 scale(1.0f);
                glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f); // identidade
                
                if (!node.translation.empty()) {
                    translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
                }
                if (!node.scale.empty()) {
                    scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
                }
                if (!node.rotation.empty()) {
                    // glTF usa [x, y, z, w] mas glm::quat é [w, x, y, z]
                    rotation = glm::quat((float)node.rotation[3], (float)node.rotation[0], 
                                       (float)node.rotation[1], (float)node.rotation[2]);
                }
                
                // Compor: T * R * S
                glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
                glm::mat4 R = glm::mat4_cast(rotation);
                glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
                nodeTransform = T * R * S;
            }
            
            for (const auto& primitive : mesh.primitives) {
                // Usar o nome do nó em vez do nome do mesh para detecção de interações
                std::string interactionName = !node.name.empty() ? node.name : mesh.name;
                if (loadPrimitive(primitive, gltfModel, buffer, interactionName, baseTransform * nodeTransform)) {
                    loaded = true;
                }
            }
        }
    }
    if (loaded) {
        initDoors();
        createFloor();
        // Criar textura de pedra para o mesh "chao"
        chaoTexture = createTextureFromFile("chao.png", false);
    }
    return loaded;
}

bool GLTFRenderer::loadGLTFAt(const std::string& filepath, const glm::vec3& worldPos) {
    // Ajustar Y ao chão local na posição desejada
    glm::vec3 pos = worldPos;
    pos.y = groundHeightAt(worldPos);
    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    std::cout << "Colocando '" << filepath << "' em (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    return loadGLTF(filepath, T);
}

bool GLTFRenderer::loadGLTFAtRotZ(const std::string& filepath, const glm::vec3& worldPos, float degreesZ) {
    // Ajusta Y ao chão e aplica rotação ao redor do eixo Z na posição desejada
    glm::vec3 pos = worldPos;
    pos.y = groundHeightAt(worldPos);
    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), glm::radians(degreesZ), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 M = T * Rz; // Rotação aplicada na posição final (gira no próprio eixo)
    std::cout << "Colocando (Rz=" << degreesZ << ") '" << filepath << "' em (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    return loadGLTF(filepath, M);
}

bool GLTFRenderer::loadGLTFAtRotY(const std::string& filepath, const glm::vec3& worldPos, float degreesY) {
    // ROTAÇÃO NA ORIGEM (0,0,0) DEPOIS VOLTA PARA POSIÇÃO ORIGINAL
    // Sequência correta: R * T (rotação PRIMEIRO, translação DEPOIS)
    glm::vec3 pos = worldPos;
    pos.y = groundHeightAt(worldPos);
    
    // 1. Rotação na origem (0,0,0) - aplicada PRIMEIRO
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), degreesY, glm::vec3(0.0f, 1.0f, 0.0f));
    // 2. Translação para posição final - aplicada DEPOIS
    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    
    // ORDEM CORRETA: R * T = rotação primeiro, depois translação
    glm::mat4 M = R * T;  // Mudança: R * T em vez de T * R
    
    std::cout << "Colocando (Ry=" << glm::degrees(degreesY) << "°) '" << filepath << "' em (" << pos.x << ", " << pos.y << ", " << pos.z << ") [ROTAÇÃO-PRIMEIRO]" << std::endl;
    return loadGLTF(filepath, M);
}

bool GLTFRenderer::loadGLTFAtNear(const std::string& filepath, const std::string& anchorMesh, const glm::vec2& offsetXZ) {
    // Encontrar AABB do anchor
    BoundingBox target{};
    bool found = false;
    for (const auto& box : collisionBoxes) {
        if (box.meshName == anchorMesh) { target = box; found = true; break; }
    }
    if (!found) return false;
    glm::vec3 center = (target.min + target.max) * 0.5f;
    glm::vec3 pos(center.x + offsetXZ.x, 0.0f, center.z + offsetXZ.y);
    pos.y = groundHeightAt(pos);
    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    std::cout << "Colocando (chao) '" << filepath << "' em (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    return loadGLTF(filepath, T);
}

bool GLTFRenderer::loadGLTFAtOnChao(const std::string& filepath, const glm::vec2& offsetXZFromCenter) {
    // Encontrar AABB do chao
    BoundingBox chaoBox{};
    bool found = false;
    for (const auto& box : collisionBoxes) {
        if (box.meshName == "chao") { chaoBox = box; found = true; break; }
    }
    if (!found) return false;
    glm::vec3 center = (chaoBox.min + chaoBox.max) * 0.5f;
    glm::vec3 pos(center.x + offsetXZFromCenter.x, 0.0f, center.z + offsetXZFromCenter.y);
    pos.y = groundHeightAt(pos);
    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    return loadGLTF(filepath, T);
}

bool GLTFRenderer::loadPrimitive(const tinygltf::Primitive& primitive,
                   const tinygltf::Model& model,
                   const std::vector<unsigned char>& binaryData,
                   const std::string& meshName,
                   const glm::mat4& nodeTransform) {

    if (primitive.indices == -1) return false;
    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) return false;

    Mesh mesh;
    std::vector<float> positions;
    if (!loadAccessorData(model.accessors[posIt->second], model, binaryData, positions)) return false;

    std::vector<float> normals;
    auto normalIt = primitive.attributes.find("NORMAL");
    if (normalIt != primitive.attributes.end()) {
        loadAccessorData(model.accessors[normalIt->second], model, binaryData, normals);
    }

    std::vector<float> texCoords;
    auto texIt = primitive.attributes.find("TEXCOORD_0");
    if (texIt != primitive.attributes.end()) {
        // Passar isTexCoord=true para suportar UBYTE/USHORT normalizados
        loadAccessorData(model.accessors[texIt->second], model, binaryData, texCoords, /*isTexCoord=*/true);
    }

    size_t vertexCount = positions.size() / 3;
    glm::vec3 minPos(FLT_MAX), maxPos(-FLT_MAX);
    for (size_t i = 0; i < vertexCount; ++i) {
        glm::vec3 pos(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
        minPos = glm::min(minPos, pos);
        maxPos = glm::max(maxPos, pos);
    }

    glm::vec3 center = (minPos + maxPos) * 0.5f;
    glm::vec3 size = maxPos - minPos;

    float maxDimension = glm::max(glm::max(size.x, size.y), size.z);
    float scale = 5.0f / maxDimension;
    float scaleX = 1.0f;
    float scaleY = 1.5f;
    float scaleZ = 0.1f;

    // Usar transformações originais do GLTF para todos os meshes
    bool useOriginalTransform = true;
    
    if (useOriginalTransform) {
        scaleX = 1.0f;
        scaleY = 1.0f;
        scaleZ = 1.0f;
        scale = 1.0f; // Usar tamanho real sem escala adicional
    }

    // Salvar as dimensões calculadas para uso posterior
    modelSize = glm::vec3(size.x * scale * scaleX, size.y * scale * scaleY, size.z * scale * scaleZ);
    modelCenter = center;

    floorMin = glm::vec3(-3.0f, -0.1f, -3.0f);
    floorMax = glm::vec3( 3.0f, -0.1f,  3.0f);

    mesh.vertices.reserve(vertexCount * 8);

    float minYNormalized = FLT_MAX;
    std::vector<glm::vec3> normalizedPositions;
    normalizedPositions.reserve(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
        glm::vec3 originalPos(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);
        
        glm::vec3 normalizedPos;
        if (useOriginalTransform) {
            glm::vec4 transformedPos = nodeTransform * glm::vec4(originalPos, 1.0f);
            normalizedPos = glm::vec3(transformedPos); // Sem escala adicional
        } else {
            // Modo legado (não usado mais)
            normalizedPos = (originalPos - center);
            normalizedPos.x *= scale * scaleX;
            normalizedPos.y *= scale * scaleY;
            normalizedPos.z *= scale * scaleZ;
        }
        
        normalizedPositions.push_back(normalizedPos);
        
        if (normalizedPos.y < minYNormalized) {
            minYNormalized = normalizedPos.y;
        }
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        glm::vec3 finalPos = normalizedPositions[i];
        // Manter posição Y original sem ajuste
        // finalPos.y -= minYNormalized;
        
        mesh.vertices.push_back(finalPos.x);
        mesh.vertices.push_back(finalPos.y);
        mesh.vertices.push_back(finalPos.z);
        
        if (!normals.empty()) {
            mesh.vertices.push_back(normals[i * 3 + 0]);
            mesh.vertices.push_back(normals[i * 3 + 1]);
            mesh.vertices.push_back(normals[i * 3 + 2]);
        } else {
            mesh.vertices.push_back(0.0f); mesh.vertices.push_back(1.0f); mesh.vertices.push_back(0.0f);
        }
        
        if (!texCoords.empty()) {
            mesh.vertices.push_back(texCoords[i * 2 + 0]);
            mesh.vertices.push_back(texCoords[i * 2 + 1]);
        } else {
            mesh.vertices.push_back(0.0f); mesh.vertices.push_back(0.0f);
        }
    }

    if (!loadAccessorIndices(model.accessors[primitive.indices], model, binaryData, mesh.indices)) return false;

    mesh.name = meshName;
    if (setupMeshBuffers(mesh)) {
        meshes.push_back(mesh);
        std::cout << "Mesh carregado: '" << meshName << "'" << std::endl; // Debug
        if (meshName == "chao") {
            chaoMeshIndex = (int)meshes.size() - 1;
        }
        
        // Calcular bounding box para física/colisão
        BoundingBox bbox;
        bbox.meshName = meshName;
        bbox.min = glm::vec3(FLT_MAX);
        bbox.max = glm::vec3(-FLT_MAX);
        
        // Calcular limites da bounding box baseado nas posições finais
        for (size_t i = 0; i < normalizedPositions.size(); ++i) {
            glm::vec3 finalPos = normalizedPositions[i];
            // Manter posição Y original para bounding box
            // finalPos.y -= minYNormalized;
            
            bbox.min = glm::min(bbox.min, finalPos);
            bbox.max = glm::max(bbox.max, finalPos);
        }
        
        // Expandir um pouco a bounding box para evitar colisões muito próximas
        // Para a escada e portas, NÃO expandimos para preservar pivô/altura/comprimento reais
        bool noPad = (bbox.meshName == "escada") ||
                     (bbox.meshName == "porta_front_1") ||
                     (bbox.meshName == "porta_front_2");
        float padding = noPad ? 0.0f : 0.1f;
        bbox.min -= glm::vec3(padding);
        bbox.max += glm::vec3(padding);
        
    // Adicionar às colisões
        collisionBoxes.push_back(bbox);
    // (sem culling)
        
        return true;
    }
    return false;
}

bool GLTFRenderer::loadAccessorData(const tinygltf::Accessor& accessor,
                      const tinygltf::Model& model,
                      const std::vector<unsigned char>& /*binaryData*/,
                      std::vector<float>& out,
                      bool isTexCoord) {
    // Verificações de segurança - alguns accessors podem ter bufferView = -1 (não utilizados)
    if (accessor.bufferView == -1) {
        // Accessor sem bufferView - normalmente não usado, retornar falso silenciosamente
        return false;
    }
    
    if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
        std::cerr << "bufferView inválido: " << accessor.bufferView << std::endl;
        return false;
    }
    
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    
    if (bufferView.buffer < 0 || bufferView.buffer >= model.buffers.size()) {
        std::cerr << "buffer inválido: " << bufferView.buffer << std::endl;
        return false;
    }
    
    const auto& buffer = model.buffers[bufferView.buffer];
    const unsigned char* base = buffer.data.data();
    size_t offset = bufferView.byteOffset + accessor.byteOffset;
    int componentCount = tinygltf::GetNumComponentsInType(accessor.type);
    size_t compSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    size_t stride = bufferView.byteStride ? bufferView.byteStride : (componentCount * compSize);
    
    // Verificar se não vai acessar fora do buffer
    size_t totalBytesNeeded = offset + (accessor.count - 1) * stride + componentCount * compSize;
    if (totalBytesNeeded > buffer.data.size()) {
        std::cerr << "Tentativa de leitura fora do buffer. Necessário: " << totalBytesNeeded 
                  << ", disponível: " << buffer.data.size() << std::endl;
        return false;
    }
    
    out.resize(accessor.count * componentCount);

    // FLOAT path (positions/normals/texcoords típicos)
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        for (size_t i = 0; i < accessor.count; ++i) {
            const float* src = reinterpret_cast<const float*>(base + offset + i * stride);
            for (int c = 0; c < componentCount; ++c) {
                out[i * componentCount + c] = src[c];
            }
        }
        return true;
    }

    // TEXCOORD normalizado (UNSIGNED_BYTE/UNSIGNED_SHORT)
    if (isTexCoord && (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                       accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)) {
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            for (size_t i = 0; i < accessor.count; ++i) {
                const unsigned char* src = reinterpret_cast<const unsigned char*>(base + offset + i * stride);
                for (int c = 0; c < componentCount; ++c) {
                    out[i * componentCount + c] = static_cast<float>(src[c]) / 255.0f;
                }
            }
        } else { // UNSIGNED_SHORT
            for (size_t i = 0; i < accessor.count; ++i) {
                const unsigned short* src = reinterpret_cast<const unsigned short*>(base + offset + i * stride);
                for (int c = 0; c < componentCount; ++c) {
                    out[i * componentCount + c] = static_cast<float>(src[c]) / 65535.0f;
                }
            }
        }
        return true;
    }

    std::cerr << "Accessor componentType não suportado para este atributo (esperado FLOAT, ou UV normalizado)." << std::endl;
    return false;
}

bool GLTFRenderer::loadAccessorIndices(const tinygltf::Accessor& accessor, const tinygltf::Model& model, const std::vector<unsigned char>& /*binaryData*/, std::vector<unsigned int>& indices) {
    // Verificações de segurança - alguns accessors podem ter bufferView = -1 (não utilizados)
    if (accessor.bufferView == -1) {
        // Accessor sem bufferView - normalmente não usado, retornar falso silenciosamente
        return false;
    }
    
    if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
        std::cerr << "bufferView inválido para índices: " << accessor.bufferView << std::endl;
        return false;
    }
    
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    
    if (bufferView.buffer < 0 || bufferView.buffer >= model.buffers.size()) {
        std::cerr << "buffer inválido para índices: " << bufferView.buffer << std::endl;
        return false;
    }
    
    const auto& buffer = model.buffers[bufferView.buffer];
    size_t offset = bufferView.byteOffset + accessor.byteOffset;
    
    // Verificar tamanho do componente
    size_t compSize = 0;
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) compSize = 4;
    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) compSize = 2;
    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) compSize = 1;
    else {
        std::cerr << "Tipo de índice não suportado: " << accessor.componentType << std::endl;
        return false;
    }
    
    // Verificar se não vai acessar fora do buffer
    size_t totalBytesNeeded = offset + accessor.count * compSize;
    if (totalBytesNeeded > buffer.data.size()) {
        std::cerr << "Tentativa de leitura de índices fora do buffer. Necessário: " << totalBytesNeeded 
                  << ", disponível: " << buffer.data.size() << std::endl;
        return false;
    }
    
    indices.resize(accessor.count);
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        std::memcpy(indices.data(), buffer.data.data() + offset, accessor.count * sizeof(unsigned int));
    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        std::vector<unsigned short> short_indices(accessor.count);
        std::memcpy(short_indices.data(), buffer.data.data() + offset, accessor.count * sizeof(unsigned short));
        for(size_t i = 0; i < accessor.count; ++i) indices[i] = short_indices[i];
    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        std::vector<unsigned char> byte_indices(accessor.count);
        std::memcpy(byte_indices.data(), buffer.data.data() + offset, accessor.count * sizeof(unsigned char));
        for (size_t i = 0; i < accessor.count; ++i) indices[i] = byte_indices[i];
    }
    return true;
}

bool GLTFRenderer::setupMeshBuffers(Mesh& mesh) {
    if (mesh.vertices.empty() || mesh.indices.empty()) return false;
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);
    int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    mesh.indexCount = mesh.indices.size();
    mesh.isValid = true;
    glBindVertexArray(0);
    return true;
}
