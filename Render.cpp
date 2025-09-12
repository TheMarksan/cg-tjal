#include "GLTFRenderer.h"

void GLTFRenderer::render() {
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Azul céu suave (RGB: 135, 206, 235)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    // Ajustar projeção ao tamanho atual do framebuffer (para fullscreen)
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int fbW = vp[2], fbH = vp[3];
    if (fbW > 0 && fbH > 0) {
        float aspect = (float)fbW / (float)fbH;
        projection = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 200.0f);
    }
    setMatrix4("view", view);
    setMatrix4("projection", projection);
    setVec3("lightPos", cameraPos + glm::vec3(0.0f, 2.0f, 0.0f));
    setVec3("viewPos", cameraPos);

    // Renderizar chão com textura
    setMatrix4("model", glm::mat4(1.0f));
    setBool("useVertexColor", false);
    setBool("useWorldTex", false); // Chão usa UVs normais
    if (useFloorTexture && textureType > 0) {
        setBool("useTexture", true);
        glActiveTexture(GL_TEXTURE0);
        GLuint currentTexture = (textureType == 1) ? floorTexture : checkerTexture;
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
    } else {
        setBool("useTexture", false);
        setVec3("baseColor", floorColor);
    }
    glBindVertexArray(floorVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    if (useFloorTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Renderizar o modelo GLTF completo sem gradiente por vértice
    if (!meshes.empty()) {
        setBool("useVertexColor", false);
        setBool("useTexture", false);
        setVec3("baseColor", glm::vec3(0.82f, 0.82f, 0.82f)); // cinza claro sólido
        // Atualizar planos do frustum uma vez por frame
        glm::mat4 vp = projection * view;
        updateFrustumPlanes(vp);

        for (size_t i = 0; i < meshes.size(); ++i) {
            const auto& mesh = meshes[i];
            if (!mesh.isValid) continue;

            // Frustum culling rápido por AABB
            if (i < meshAABBs.size() && !aabbInFrustum(meshAABBs[i])) {
                continue;
            }

            // Verificar se este mesh é uma porta com rotação
            glm::mat4 m = model; // base do modelo
            bool isDoor = false;
            for (const auto& d : doors) {
                if (d.meshIndex == (int)i) {
                    isDoor = true;
                    // Transformação de rotação em torno da dobradiça (eixo Y)
                    glm::mat4 T1 = glm::translate(glm::mat4(1.0f), d.hinge);
                    glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(d.angle), glm::vec3(0.0f, 1.0f, 0.0f));
                    glm::mat4 T0 = glm::translate(glm::mat4(1.0f), -d.hinge);
                    m = model * (T1 * R * T0);
                    break;
                }
            }
            // Se for o mesh "chao", aplicar textura procedural com UVs em espaço-mundo
            if ((int)i == chaoMeshIndex && chaoTexture != 0) {
                setBool("useTexture", true);
                setBool("useWorldTex", true);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, chaoTexture);
                glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
                glUniform1f(glGetUniformLocation(shaderProgram, "worldTexScale"), chaoWorldTexScale);
            } else {
                setBool("useWorldTex", false);
                setBool("useTexture", false);
            }

            setMatrix4("model", m);
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
}
