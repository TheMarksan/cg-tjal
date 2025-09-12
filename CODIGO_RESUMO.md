
## 🏗️ Arquitetura do Projeto

### Estrutura de Arquivos
```
├── main.cpp              # Ponto de entrada e loop principal
├── GLTFRenderer.h/.cpp    # Classe principal do renderizador
├── GLTFLoader.cpp    ### 🔧 Características Técnicas
- **Performance**: Loop otimizado com diferentes frequências de update, VSync para 60 FPS estáveis
- **Robustez**: Tratamento de erros OpenGL e fallbacks para assets
- **Modularidade**: Código separado por responsabilidade
- **Escalabilidade**: Sistema facilmente extensível para novos objetos/funcionalidades# Carregamento de modelos glTF
├── Camera.cpp             # Sistema de câmera e movimento
├── Physics.cpp            # Detecção de colisão e física básica
├── Render.cpp             # Pipeline de renderização e cores
├── Textures.cpp           # Gerenciamento de texturas
├── Makefile              # Sistema de build
├── models/               # Diretório de assets 3D
└── README.md            # Documentação básica
```

## 🎮 Sistema de Controles

### Movimento da Câmera
- **WASD**: Movimento horizontal (frente/trás/esquerda/direita)
- **Setas direcionais**: Rotação da câmera (olhar ao redor)
- **Shift**: Movimento vertical para cima (modo livre)
- **Space**: Movimento vertical para baixo
- **Escape**: Sair da aplicação

### Interações
- **E**: Abrir/fechar porta mais próxima
- **T**: Alternar textura do piso
- **P**: Toggle de posição em tempo real
- **F11**: Alternar modo tela cheia
- **Tab**: Controle auxiliar

## 🔧 Componentes Técnicos

### 1. GLTFRenderer (Classe Principal)
**Responsabilidades:**
- Inicialização do contexto OpenGL
- Gerenciamento do loop de renderização
- Coordenação entre subsistemas

**Características Técnicas:**
- OpenGL 3.3 Core Profile
- Shaders GLSL programáveis
- Sistema de matrizes MVP (Model-View-Projection)

### 2. Sistema de Câmera (Camera.cpp)
**Funcionalidades:**
- **Movimento Horizontal**: Navegação no plano XZ com detecção de colisão
- **Movimento Vertical Livre**: Sistema de dois modos:
  - **Modo Normal**: Câmera segue altura do terreno
  - **Modo Livre**: Movimento vertical independente (altura máxima: 50 unidades)
- **Rotação Suave**: Controle de yaw/pitch com sensibilidade configurável
- **Seguimento de Terreno**: Ajuste automático à altura do chão/escadas

**Variáveis de Estado:**
```cpp
glm::vec3 cameraPos;           // Posição da câmera
glm::vec3 cameraFront;         // Direção frontal
float walkHeight = 1.7f;       // Altura dos olhos
bool freeVerticalMovement;     // Controle do modo livre
float cameraSpeed = 3.0f;      // Velocidade de movimento
```

### 3. Sistema de Física (Physics.cpp)
**Detecção de Colisão:**
- AABB (Axis-Aligned Bounding Boxes) para paredes e objetos
- Teste de colisão por eixo separado (X e Z independentes)
- Prevenção de travamento em cantos

**Portas Interativas:**
- Sistema de detecção por proximidade
- Animação de rotação em dobradiças
- Suporte a múltiplas portas (porta_front_1, porta_front_2, porta_interna_1)

**Terreno:**
- Função `groundHeightAt()` para altura do piso
- Suporte a escadas e diferentes níveis
- Interpolação suave entre alturas

### 4. Carregamento de Modelos (GLTFLoader.cpp)
**Biblioteca:** tinygltf + stb_image

**Funcionalidades:**
- Carregamento de arquivos .gltf e .glb
- Processamento de materiais e texturas
- Transformações TRS (Translation, Rotation, Scale)
- Sistemas de posicionamento:
  - `loadGLTFAt()`: Posição explícita XZ
  - `loadGLTFAtOnChao()`: Relativo ao chão
  - `spawnInFrontOf()`: Relativo a objeto existente

### 5. Pipeline de Renderização (Render.cpp)
**Sistema de Cores:**
- Mapeamento por nome de mesh:
  - `"color_3"` → #938F86 (cinza claro)
  - `"titulo"` → #545761 (cinza escuro)
  - `"Cube.036"` → #846945 (marrom - sofás)
  - `"Cylinder.005"` → #846945 (marrom - banco)

**Iluminação:**
- Shader com iluminação ambiente (0.45)
- Iluminação difusa suavizada
- Suporte a texturas com fallback para cores sólidas

## 📦 Gerenciamento de Assets

### Modelos 3D Carregados
```cpp
// Modelo principal (ambiente)
"models/TJAL.gltf"

// Móveis posicionados
renderer.loadGLTFAt("models/sofa.gltf",    glm::vec3(-4.0f, 0.0f, 8.0f));
renderer.loadGLTFAt("models/sofa.gltf",    glm::vec3(-4.0f, 0.0f, 10.0f));
renderer.loadGLTFAt("models/sofa.gltf",    glm::vec3(-4.0f, 0.0f, 12.0f));
renderer.loadGLTFAt("models/cadeira3.gltf",glm::vec3(-1.0f, 0.0f, 9.0f));
renderer.loadGLTFAt("models/cadeira3.gltf",glm::vec3(-1.0f, 0.0f, 11.0f));
renderer.loadGLTFAt("models/cadeira3.gltf",glm::vec3(-1.0f, 0.0f, 13.0f));
```

### Sistema de Spawn
- **Posição Inicial**: Frente à escada (3.0 unidades de distância)
- **Ajuste Automático**: Y ajustado ao nível do chão
- **Fallback**: Múltiplos caminhos para encontrar assets

## 🔄 Loop Principal (main.cpp)

### Inicialização
1. **GLFW/GLEW**: Configuração do contexto OpenGL 3.3
2. **Janela**: 800x600 pixels (redimensionável)
3. **Callbacks**: Tratamento de erros e fechamento de janela
4. **Renderer**: Inicialização do sistema de renderização

### Loop de Renderização
```cpp
while (!glfwWindowShouldClose(window)) {
    // 1. Cálculo do deltaTime
    // 2. Processamento de input (60 FPS)
    // 3. Controles secundários (12 FPS - frameCount % 5)
    // 4. Validação de contexto (2 FPS - frameCount % 30)
    // 5. Atualização de portas
    // 6. Renderização e swap de buffers
}
```

### Otimizações de Performance
- **Input Principal**: Verificado a cada frame (WASD, setas, Shift, Space)
- **Input Secundário**: Verificado a cada 5 frames (T, E, P, F11)
- **Validações**: A cada 30 frames
- **VSync Habilitado**: `glfwSwapInterval(1)` para taxa fixa de 60 FPS

## 🎨 Sistema de Cores e Materiais

### Esquema de Cores Atual
- **Paredes/Estrutura**: Cores originais do modelo
- **Detalhes** (`color_3`): #938F86 (cinza claro)
- **Títulos** (`titulo`): #545761 (cinza escuro)
- **Móveis Marrons**: #846945 (sofás e banco)

### Pipeline de Material
1. **Verificação de Nome**: Sistema baseado em string matching
2. **Conversão RGB**: Normalização de hex para float (0.0-1.0)
3. **Aplicação**: Via uniform `baseColor` no fragment shader
4. **Fallback**: Cor branca se não especificada

## 🚪 Sistema de Portas

### Mecânica
- **Detecção**: Por proximidade (distância euclidiana)
- **Animação**: Rotação em torno do eixo Y (dobradiça)
- **Estados**: Fechado (0°) ↔ Aberto (90°)
- **Velocidade**: Configurável via deltaTime

### Portas Implementadas
- `porta_front_1`: Entrada principal
- `porta_front_2`: Entrada secundária  
- `porta_interna_1`: Porta interna

## 🏗️ Sistema de Build

### Makefile
```makefile
# Compilação otimizada (-O2)
# Flags C++17
# Linking: -lglfw -lGLEW -lGL -ldl -lpthread
```

### Dependências
- **GLFW**: Gerenciamento de janela e input
- **GLEW**: Carregamento de extensões OpenGL
- **GLM**: Matemática 3D (matrizes, vetores)
- **tinygltf**: Carregamento de modelos
- **stb_image**: Carregamento de texturas

## 🎯 Funcionalidades Principais

### ✅ Implementado
- [x] Carregamento de modelos glTF/glb
- [x] Sistema de câmera first-person com colisão
- [x] Movimento vertical livre (Shift/Space)
- [x] Interação com portas animadas
- [x] Posicionamento dinâmico de objetos
- [x] Sistema de cores por mesh
- [x] Modo tela cheia
- [x] Detecção de colisão AABB
- [x] Seguimento de terreno/escadas

### Características Técnicas
- **Performance**: Loop otimizado com diferentes frequências de update
- **Robustez**: Tratamento de erros OpenGL e fallbacks para assets
- **Modularidade**: Código separado por responsabilidade
- **Escalabilidade**: Sistema facilmente extensível para novos objetos/funcionalidades

## Métricas do Projeto
- **Linhas de Código**: ~2000+ linhas
- **Arquivos Fonte**: 7 arquivos .cpp/.h
- **Dependências**: 5 bibliotecas principais
- **Controles**: 12 teclas mapeadas
- **Objetos 3D**: 1 ambiente + 6 móveis


