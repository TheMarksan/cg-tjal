#include "GLTFRenderer.h"

void GLTFRenderer::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
    cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
}

void GLTFRenderer::updateCameraView() {
    cameraTarget = cameraPos + cameraFront;
    view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
}

void GLTFRenderer::processMovement(int direction, float deltaTime) {
    float velocity = cameraSpeed * deltaTime;
    
    // Calcular direção horizontal (sem componente Y) para movimentação no plano
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 horizontalRight = glm::normalize(glm::vec3(cameraRight.x, 0.0f, cameraRight.z));
    
    glm::vec3 movement(0.0f);
    
    if (direction == 0) movement = horizontalFront * velocity;  // W - frente
    if (direction == 1) movement = -horizontalFront * velocity;  // S - trás  
    if (direction == 2) movement = -horizontalRight * velocity;  // A - esquerda
    if (direction == 3) movement = horizontalRight * velocity;  // D - direita
    
    // Testar movimento em cada eixo separadamente para evitar travamento
    glm::vec3 newPos = cameraPos;
    
    // Tentar movimento no eixo X
    glm::vec3 testPosX = cameraPos + glm::vec3(movement.x, 0.0f, 0.0f);
    if (!checkCollision(testPosX)) {
        newPos.x = testPosX.x;
    }
    
    // Tentar movimento no eixo Z
    glm::vec3 testPosZ = glm::vec3(newPos.x, cameraPos.y, cameraPos.z + movement.z);
    if (!checkCollision(testPosZ)) {
        newPos.z = testPosZ.z;
    }
    
    cameraPos = newPos;

    // Ajustar altura com base no piso/escada no ponto atual apenas se não estiver em modo de movimento vertical livre
    if (!freeVerticalMovement) {
        float groundY = groundHeightAt(cameraPos);
        // Suavizar para evitar "quebras" na borda da escada
        if (lastGroundY == 0.0f) lastGroundY = groundY;
        float blended = glm::mix(lastGroundY, groundY, groundFollowLerp);
        lastGroundY = blended;
        cameraPos.y = blended + walkHeight; // olho a uma altura fixa acima do chão/escada
    }
        
    // Atualizar a view matrix
    updateCameraView();
}

void GLTFRenderer::processVerticalMovement(int direction, float deltaTime) {
    const float verticalSpeed = 4.5f;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Ativar modo de movimento vertical livre
    freeVerticalMovement = true;
    
    if (direction == 0) { // Subir (Shift)
        glm::vec3 newPos = cameraPos + up * verticalSpeed * deltaTime;
        
        // Verificar limite superior muito alto para vista aérea (50.0 altura máxima)
        if (newPos.y <= 50.0f && !checkCollision(newPos)) {
            cameraPos = newPos;
        }
    } else if (direction == 1) { // Descer (Space)
        glm::vec3 newPos = cameraPos - up * verticalSpeed * deltaTime;
        
        // Verificar limite inferior com o chão
        float groundHeight = groundHeightAt(glm::vec3(newPos.x, 0.0f, newPos.z));
        if (newPos.y >= groundHeight + 0.5f && !checkCollision(newPos)) { // Altura mínima menor
            cameraPos = newPos;
        } else {
            // Se tentar descer abaixo do limite, desativar movimento livre para voltar ao modo normal
            freeVerticalMovement = false;
        }
    }
    
    // Atualizar a view matrix
    updateCameraView();
}

void GLTFRenderer::rotate(float yawOffset, float pitchOffset) {
    yaw += yawOffset;
    pitch += pitchOffset;
    
    // Limitar o pitch para evitar gimbal lock
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    
    // Atualizar vetores da câmera
    updateCameraVectors();
    // Atualizar a view matrix
    updateCameraView();
}

void GLTFRenderer::processKeyboardRotation(int direction, float deltaTime) {
    float rotationAmount = 60.0f * deltaTime;
    if (direction == 0) rotate(0.0f, rotationAmount);
    if (direction == 1) rotate(0.0f, -rotationAmount);
    if (direction == 2) rotate(-rotationAmount, 0.0f);
    if (direction == 3) rotate(rotationAmount, 0.0f);
}

void GLTFRenderer::updateRealTimePosition(double currentTime) {
    // Função removida - era apenas para debug
}

void GLTFRenderer::toggleRealTimePosition() {
    // Função removida - era apenas para debug
}
