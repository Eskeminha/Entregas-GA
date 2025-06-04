/*
 * MultiSprite.cpp
 *
 * Exemplo de exercício de Processamento Gráfico: vários retângulos (sprites) texturizados.
 * Baseado no HelloSprite.cpp fornecido em aula.
 *
 * - Cria uma classe Sprite que encapsula VAO, textura, posição, escala e rotação.
 * - Usa projeção ortográfica para mapear (0,0)-(800,600) em coordenadas de tela (px).
 * - Desenha múltiplos sprites com texturas diferentes em posições/escala/rotação variadas.
 *
 * Para compilar (exemplo com g++ no Linux):
 *   g++ MultiSprite.cpp -o MultiSprite \
 *       -lglfw -ldl -lGL \
 *       -I/path/para/glad/include \
 *       -I/path/para/glm \
 *       -I/path/para/stb_image \
 *       /path/para/glad/src/glad.c
 *
 * Ajuste os caminhos conforme seu ambiente.
 *
 * Ultima atualização: 03/06/2025
 */

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

using namespace std;

// ========== Dependências OpenGL / GLFW / GLAD / GLM / stb_image ==========
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// =========================================================================
// 1) Configurações iniciais da janela
// =========================================================================

const GLuint SCREEN_WIDTH  = 800;
const GLuint SCREEN_HEIGHT = 600;

// Callback de teclado: fecha a janela se ESC for pressionado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// =========================================================================
// 2) Shaders: vertex e fragment com uniforms de Model e Projection
// =========================================================================

const GLchar* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uProjection;

out vec2 TexCoord;

void main()
{
    gl_Position = uProjection * uModel * vec4(aPos, 1.0);
    TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    FragColor = texture(uTexture, TexCoord);
}
)";

// Função auxiliar para compilar shaders e linkar em um programa
GLuint CreateShaderProgram(const GLchar* vShaderSrc, const GLchar* fShaderSrc)
{
    // 1. Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderSrc, nullptr);
    glCompileShader(vertexShader);
    // Verifica erros de compilação
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cerr << "[ERRO] Vertex Shader compilation failed:\n" << infoLog << endl;
    }

    // 2. Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderSrc, nullptr);
    glCompileShader(fragmentShader);
    // Verifica erros de compilação
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cerr << "[ERRO] Fragment Shader compilation failed:\n" << infoLog << endl;
    }

    // 3. Linka em um programa
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Verifica erros de link
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cerr << "[ERRO] Shader Program link failed:\n" << infoLog << endl;
    }

    // Limpa shaders independentes (já linkados no programa)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// =========================================================================
// 3) Carregamento de texturas (stb_image) – mesma ideia do HelloSprite.cpp
// =========================================================================

GLuint loadTexture(const string& filePath)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configura wrapping/filtering padrão
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;
        else format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        cerr << "[ERRO] Falha ao carregar textura em: " << filePath << endl;
    }
    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

// =========================================================================
// 4) Função auxiliar para criar VAO/VBO de um quad 1×1 centrado na origem
//    (mesma ideia do setupSprite do HelloSprite.cpp, mas retorna VAO pronto).
// =========================================================================

GLuint CreateQuadVAO()
{
    // Cada vértice: pos.x, pos.y, pos.z, tex.s, tex.t
    // Quad 1×1 centrado: vai de (-0.5,-0.5) até (+0.5,+0.5)
    GLfloat quadVertices[] = {
        // X      Y      Z     S     T
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,  // topo-esquerda
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,  // baixo-esquerda
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,  // topo-direita
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f   // baixo-direita
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 1. Vincula VAO
    glBindVertexArray(VAO);

    // 2. VBO: envia vértices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // 3. Configura atributo “pos” (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // 4. Configura atributo “texCoord” (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // 5. Limpa bindings
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

// =========================================================================
// 5) Classe Sprite
// =========================================================================

class Sprite {
public:
    // Construtor: recebe ID de shader compilado, ID de VAO (quad base), e textura
    Sprite(GLuint shaderID, GLuint quadVAO, GLuint textureID)
        : shaderID(shaderID), VAO(quadVAO), textureID(textureID)
    {
        position = glm::vec2(0.0f, 0.0f);
        scale    = glm::vec2(100.0f, 100.0f); // escala padrão: 100 × 100 px
        rotation = 0.0f;
    }

    // Ajusta posição (em px, no sistema de coordenadas [0,800]×[0,600])
    void SetPosition(float x, float y) {
        position = glm::vec2(x, y);
    }

    // Ajusta escala (largura e altura em px)
    void SetScale(float widthPx, float heightPx) {
        scale = glm::vec2(widthPx, heightPx);
    }

    // Ajusta rotação em graus (sentido anti-horário), pivot no centro
    void SetRotation(float angleDegrees) {
        rotation = angleDegrees;
    }

    // Desenha o sprite, recebendo a matriz de projeção (ortho)
    void Draw(const glm::mat4& projection) {
        // 1. Calcular matriz de modelo: traduzindo centro, rotacionando e escalando
        glm::mat4 model = glm::mat4(1.0f);
        // 1.1 Translação: move o quad (que está centrado na origem) para (position.x, position.y)
        model = glm::translate(model, glm::vec3(position, 0.0f));
        // 1.2 Rotação em torno do centro do quad (que agora está em position)
        model = glm::translate(model, glm::vec3(0.5f * scale.x, 0.5f * scale.y, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-0.5f * scale.x, -0.5f * scale.y, 0.0f));
        // 1.3 Escala para transformar quad de 1×1 em (scale.x)×(scale.y)
        model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0f));

        // 2. Usa shader e envia uniforms
        glUseProgram(shaderID);
        GLint modelLoc = glGetUniformLocation(shaderID, "uModel");
        GLint projLoc  = glGetUniformLocation(shaderID, "uProjection");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(projLoc,  1, GL_FALSE, glm::value_ptr(projection));

        // 3. Textura na unidade 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);

        // 4. Bind no VAO (quad) e desenha
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

private:
    GLuint shaderID;
    GLuint VAO;
    GLuint textureID;

    glm::vec2 position;
    glm::vec2 scale;
    float     rotation; // em graus
};

// =========================================================================
// 6) Função MAIN
// =========================================================================

int main()
{
    // 6.1 Inicializa GLFW
    if (!glfwInit()) {
        cerr << "[ERRO] Falha ao inicializar GLFW" << endl;
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4); // anti-aliasing
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MultiSprite Example", nullptr, nullptr);
    if (!window) {
        cerr << "[ERRO] Falha ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // 6.2 Carrega GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "[ERRO] Falha ao inicializar GLAD" << endl;
        return -1;
    }

    // 6.3 Ajusta viewport
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // 6.4 Compila e linka shader
    GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    // 6.5 Cria VAO base (quad 1×1)
    GLuint quadVAO = CreateQuadVAO();

    // 6.6 Carrega texturas (exemplo: duas texturas diferentes)
    GLuint tex1 = loadTexture("../assets/sprites/microbio.png");
    GLuint tex2 = loadTexture("../assets/sprites/enemies-spritesheet1.png");
    // Adicione quantas texturas quiser:
    // GLuint tex3 = loadTexture("assets/textures/sprite3.png");

    // 6.7 Cria instâncias de Sprite
    vector<Sprite> sprites;
    // Sprite 1
    sprites.emplace_back(shaderProgram, quadVAO, tex1);
    sprites.back().SetPosition(50.0f, 50.0f);      // canto inferior-esquerdo em (50,50)
    sprites.back().SetScale(128.0f, 128.0f);       // 128×128 px
    sprites.back().SetRotation(0.0f);              // sem rotação

    // Sprite 2
    sprites.emplace_back(shaderProgram, quadVAO, tex2);
    sprites.back().SetPosition(300.0f, 200.0f);    // posicionado em (300,200)
    sprites.back().SetScale(200.0f, 100.0f);       // 200×100 px
    sprites.back().SetRotation(45.0f);             // rotacionado 45°

    // (adicione mais sprites conforme desejar)

    // 6.8 Define matriz de projeção ortográfica (coordenadas de tela)
    glm::mat4 projection = glm::ortho(
        0.0f, (float)SCREEN_WIDTH,
        0.0f, (float)SCREEN_HEIGHT,
       -1.0f,  1.0f
    );

    // 6.9 Configurações OpenGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 6.10 Game Loop
    while (!glfwWindowShouldClose(window)) {
        // 6.10.1 Processa eventos
        glfwPollEvents();

        // 6.10.2 Limpa a tela (fundo cinza escuro)
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 6.10.3 Desenha todos os sprites no vetor
        for (auto& sprite : sprites) {
            sprite.Draw(projection);
        }

        // 6.10.4 Troca buffers
        glfwSwapBuffers(window);
    }

    // 6.11 Finaliza
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
