#include "GLTFRenderer.h"

void GLTFRenderer::initDoors() {
    std::cout << "Inicializando portas..." << std::endl; // Debug
    // Evitar duplicatas quando chamado após múltiplos loadGLTF
    doors.clear();
    doorIndexByName.clear();
    // Descobrir caixas e meshes das portas
    auto findMeshIndexByName = [&](const std::string& n) -> int {
        for (size_t i = 0; i < meshes.size(); ++i) {
            if (meshes[i].name == n) return (int)i;
        }
        return -1;
    };
    for (const auto& box : collisionBoxes) {
        std::cout << "Verificando mesh: '" << box.meshName << "'" << std::endl; // Debug
        if (box.meshName == "porta_front_1" || box.meshName == "porta_front_2" || box.meshName == "porta_interna_1") {
            std::cout << "Porta encontrada: " << box.meshName << std::endl; // Debug
            Door d;
            d.name = box.meshName;
            d.meshIndex = findMeshIndexByName(box.meshName);
            d.box = box;
            d.hingeLeft = (box.meshName == "porta_front_1" || box.meshName == "porta_interna_1");
            // Eixo de rotação: linha vertical no lado do batente.
            // Escolhe o eixo de largura (maior entre X e Z) para usar o extremo correto.
            glm::vec3 size = box.max - box.min;
            bool widthIsX = std::abs(size.x) >= std::abs(size.z);
            float hx, hz;
            if (widthIsX) {
                hx = d.hingeLeft ? box.min.x : box.max.x;
                hz = (box.min.z + box.max.z) * 0.5f;
            } else {
                hx = (box.min.x + box.max.x) * 0.5f;
                hz = d.hingeLeft ? box.min.z : box.max.z;
            }
            float hy = (box.min.y + box.max.y) * 0.5f;
            d.hinge = glm::vec3(hx, hy, hz);
            d.isOpen = false;
            d.angle = 0.0f;
            d.target = 0.0f;
            int idx = (int)doors.size();
            doors.push_back(d);
            doorIndexByName[d.name] = idx;
        }
    }
    std::cout << "Total de portas encontradas: " << doors.size() << std::endl; // Debug
}

void GLTFRenderer::updateDoors(float deltaTime) {
    // Animar ângulo em direção ao alvo
    for (auto& d : doors) {
        if (std::abs(d.angle - d.target) < 0.1f) {
            d.angle = d.target;
            // Atualizar estado final somente quando atinge o alvo
            d.isOpen = (std::abs(d.target) > 1.0f);
            continue;
        }
        float dir = (d.angle < d.target) ? 1.0f : -1.0f;
        d.angle += dir * d.speed * deltaTime;
        if ((dir > 0.0f && d.angle > d.target) || (dir < 0.0f && d.angle < d.target)) {
            d.angle = d.target;
            d.isOpen = (std::abs(d.target) > 1.0f);
        }
    }
}

void GLTFRenderer::toggleNearestDoor() {
    std::cout << "Tentando abrir porta. Posição camera: " << cameraPos.x << ", " << cameraPos.z << std::endl; // Debug
    // Achar porta mais próxima em XZ dentro de um raio
    float bestDist2 = 5.0f * 5.0f; // Aumentar raio para 5 unidades
    int best = -1;
    for (size_t i = 0; i < doors.size(); ++i) {
        const auto& d = doors[i];
        float cx = (d.box.min.x + d.box.max.x) * 0.5f;
        float cz = (d.box.min.z + d.box.max.z) * 0.5f;
        glm::vec2 diff(cameraPos.x - cx, cameraPos.z - cz);
        float dist2 = glm::dot(diff, diff);
        float dist = sqrt(dist2);
        std::cout << "Porta " << d.name << " centro: " << cx << ", " << cz << " distancia: " << dist << std::endl; // Debug
        if (dist2 < bestDist2) { bestDist2 = dist2; best = (int)i; }
    }
    if (best >= 0) {
        std::cout << "Abrindo porta: " << doors[best].name << std::endl; // Debug
        auto& d = doors[best];
        // Alternar destino: se está aberta, fechar (0); senão abrir (±90)
        bool willOpen = !d.isOpen;
        float base = willOpen ? 90.0f : 0.0f;
        float sign = d.hingeLeft ? 1.0f : -1.0f;
        d.target = base * -sign; // abrir para fora
        // Não altera d.isOpen aqui; será definido quando o ângulo alcançar o alvo
    } else {
        std::cout << "Nenhuma porta próxima encontrada" << std::endl; // Debug
    }
}

bool GLTFRenderer::checkCollision(const glm::vec3& newPos) {
    // Colisão apenas quando a porta está essencialmente fechada (ângulo próximo de 0)
    glm::vec2 p(newPos.x, newPos.z);
    const float closedThreshold = 5.0f; // graus
    const float inflate = 0.02f; // pequenas folgas para evitar atravessar a quina
    for (const auto& d : doors) {
        bool doorBlocks = std::abs(d.angle) <= closedThreshold;
        if (!doorBlocks) continue;
        const auto& b = d.box;
        float minx = b.min.x - inflate, maxx = b.max.x + inflate;
        float minz = b.min.z - inflate, maxz = b.max.z + inflate;
        if (p.x >= minx && p.x <= maxx && p.y >= minz && p.y <= maxz) {
            return true;
        }
    }
    return false;
}

float GLTFRenderer::groundHeightAt(const glm::vec3& posXZ) {
    // Piso base é o grid no Y=0
    float baseY = 0.0f;
    float maxY = baseY;

    // Descobrir o nível do "chao" (andar/platô) se existir
    bool hasChao = false;
    BoundingBox chaoBox;
    for (const auto& box : collisionBoxes) {
        if (box.meshName == "chao") { hasChao = true; chaoBox = box; break; }
    }
    float chaoY = 0.0f;
    if (hasChao) {
        // Considerar a superfície superior como max.y
        chaoY = chaoBox.max.y;
        // Se a posição XZ estiver sobre o chao, considerar esse nível como chão
        bool onChaoXZ = (posXZ.x >= chaoBox.min.x && posXZ.x <= chaoBox.max.x &&
                         posXZ.z >= chaoBox.min.z && posXZ.z <= chaoBox.max.z);
        if (onChaoXZ) maxY = std::max(maxY, chaoY);
    }

    for (const auto& box : collisionBoxes) {
        // Procurar especificamente pelo nó/mesh chamado "escada"
        if (box.meshName == "escada") {
            // Verificar se (x,z) está dentro da projeção da AABB no plano XZ
            bool insideXZ = (posXZ.x >= box.min.x && posXZ.x <= box.max.x &&
                             posXZ.z >= box.min.z && posXZ.z <= box.max.z);
            if (!insideXZ) continue;

            // Dimensões da escada
            glm::vec3 size = box.max - box.min; // tamanho AABB
            float height = size.y;              // subida total

            // Eixo principal no plano XZ (comprimento da escada)
            // Escolhe o maior entre X e Z
            bool alongX = std::abs(size.x) >= std::abs(size.z);
            float length = alongX ? (box.max.x - box.min.x) : (box.max.z - box.min.z);
            if (length <= 1e-5f) continue; // evitar divisão por zero

            // Progresso ao longo do comprimento (0..1)
            float t = alongX
                ? (posXZ.x - box.min.x) / length
                : (posXZ.z - box.min.z) / length;
            t = glm::clamp(t, 0.0f, 1.0f);

            // Altura da escada naquele ponto
            // Interpolar a escada até atingir o nível do "chao" (se existir)
            float stairTop = hasChao ? chaoY : (box.min.y + height);
            float stairY = box.min.y + t * (stairTop - box.min.y);

            // Manter o maior "chão" disponível (útil caso existam várias escadas)
            if (stairY > maxY) maxY = stairY;
        }
    }
    // Dar uma margem mínima para alcançar o topo mesmo com erros de ponto flutuante
    return maxY;
}

void GLTFRenderer::spawnInFrontOf(const std::string& meshName, float distance) {
    // Encontrar a AABB do mesh desejado
    BoundingBox target{};
    bool found = false;
    for (const auto& box : collisionBoxes) {
        if (box.meshName == meshName) { target = box; found = true; break; }
    }
    if (!found) return;

    // Centro e dimensões
    glm::vec3 center = (target.min + target.max) * 0.5f;
    glm::vec3 size = target.max - target.min;

    // Direção "frente" heurística: eixo de maior extensão no plano XZ, apontando para fora
    glm::vec3 forward(0.0f, 0.0f, -1.0f);
    bool alongX = std::abs(size.x) >= std::abs(size.z);
    if (alongX) {
        // Se a escada é mais larga em X, assumimos subida ao longo de X; fique no -Z para ver a frente
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        // Mais longa em Z; fique no -X
        forward = glm::vec3(-1.0f, 0.0f, 0.0f);
    }

    // Posição alvo a uma certa distância do centro (no plano XZ)
    glm::vec3 pos = center - glm::normalize(glm::vec3(forward.x, 0.0f, forward.z)) * distance;

    // Altura do chão nesse XZ + altura dos olhos
    float y = groundHeightAt(pos);
    cameraPos = glm::vec3(pos.x, y + walkHeight, pos.z);

    // Orientar a câmera para olhar para o centro da escada
    glm::vec3 toTarget = glm::normalize(glm::vec3(center.x - cameraPos.x, 0.0f, center.z - cameraPos.z));
    // Calcular yaw a partir do vetor direcional no plano XZ
    yaw = glm::degrees(std::atan2(toTarget.z, toTarget.x));
    pitch = 0.0f;
    updateCameraVectors();
    updateCameraView();
}
