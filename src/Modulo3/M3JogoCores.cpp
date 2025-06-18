/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 13/08/2024
 *
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

#include <cmath>
#include <ctime>

// STB Easy Font
#include "stb_easy_font.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

// Protótipos das funções
GLuint createQuad();
int setupShader();
int setupGeometry();
void eliminarSimilares(float tolerancia);
void drawText_GL33_centered(float x, float y, const char* text, float r, float g, float b, float scale);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;
const GLuint ROWS = 6, COLS = 8;
const GLuint QUAD_WIDTH = 100, QUAD_HEIGHT = 100;
const float dMax = sqrt(3.0);

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
uniform mat4 projection;
uniform mat4 model;
void main()	
{
	//...pode ter mais linhas de código aqui!
	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
}
)";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
#version 400
uniform vec4 inputColor;
out vec4 color;
void main()
{
	color = inputColor;
}
)";

struct Quad
{
	vec3 position;
	vec3 dimensions;
	vec3 color;
	bool eliminated;
};

vector<Quad> triangles;

vector<vec3> colors;
int iColor = 0;
int iSelected = -1;

// Criação da grid de quadrados
Quad grid[ROWS][COLS];

// Variáveis globais para pontuação e controle do jogo
int score = 0;
int attempts = 0;
bool gameOver = false;

// Função para reiniciar o jogo
void restartGame()
{
    score = 500;
    attempts = 0;
    iSelected = -1;
    gameOver = false;
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            Quad quad;
            vec2 ini_pos = vec2(QUAD_WIDTH / 2, QUAD_HEIGHT / 2);
            quad.position = vec3(ini_pos.x + j * QUAD_WIDTH, ini_pos.y + i * QUAD_HEIGHT, 0.0);
            quad.dimensions = vec3(QUAD_WIDTH, QUAD_HEIGHT, 1.0);
            float r = rand() % 256 / 255.0;
            float g = rand() % 256 / 255.0;
            float b = rand() % 256 / 255.0;
            quad.color = vec3(r, g, b);
            quad.eliminated = false;
            grid[i][j] = quad;
        }
    }
}

// Função MAIN
int main()
{
	//srand(glfwGetTime()); TODO - Ver como transformar em unsigned int
	srand(time(0));

	// Inicialização da GLFW
	glfwInit();

	// Muita atenção aqui: alguns ambientes não aceitam essas configurações
	// Você deve adaptar para a versão do OpenGL suportada por sua placa
	// Sugestão: comente essas linhas de código para desobrir a versão e
	// depois atualize (por exemplo: 4.5 com 4 e 5)
	/*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);*/

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo das cores! ❤️🩷🧡💛💚", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	GLuint VAO = createQuad();

	// Inicializar a grid
	restartGame();

	glUseProgram(shaderID);

	// Enviando a cor desejada (vec4) para o fragment shader
	// Utilizamos a variáveis do tipo uniform em GLSL para armazenar esse tipo de info
	// que não está nos buffers
	GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");

	// Matriz de projeção paralela ortográfica
	// mat4 projection = ortho(-10.0, 10.0, -10.0, 10.0, -1.0, 1.0);
	mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		glBindVertexArray(VAO); // Conectando ao buffer de geometria

		if (iSelected > -1 && !gameOver)
		{
			eliminarSimilares(0.2);
		}


		for (int i = 0; i < ROWS; i++)
		{
			for (int j = 0; j < COLS; j++)
			{
				if (!grid[i][j].eliminated)
				{
					// Matriz de modelo: transformações na geometria (objeto)
					mat4 model = mat4(1); // matriz identidade
					// Translação
					model = translate(model, grid[i][j].position);
					//  Escala
					model = scale(model, grid[i][j].dimensions);
					glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
					glUniform4f(colorLoc, grid[i][j].color.r, grid[i][j].color.g, grid[i][j].color.b, 1.0f); // enviando cor para variável uniform inputColor
					// Chamada de desenho - drawcall
					// Poligono Preenchido - GL_TRIANGLES
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
				}
			}
		}

		glBindVertexArray(0); // Desconectando o buffer de geometria

		// Desabilita o shader para pipeline fixo do OpenGL
		glUseProgram(0);

		// Desenha a pontuação e tentativas na tela
		char info[128];
		sprintf(info, "Pontuacao: %d  Tentativas: %d", score, attempts);

		// Centralizado no topo
		drawText_GL33_centered(WIDTH/2, 40, info, 1, 1, 1, 2.0f); // 2.0f = dobro do tamanho padrão

		if (gameOver) {
		    drawText_GL33_centered(WIDTH/2, 100, "Fim de jogo! Pressione R para reiniciar.", 1, 1, 0, 2.0f);
		}

		// Volta para o shader para o próximo frame
		glUseProgram(shaderID);

		// Verifica se todos foram eliminados e se tem score
		bool allEliminated = true;
		if(score > 0 && !gameOver)
		{
			for (int i = 0; i < ROWS; i++)
			for (int j = 0; j < COLS; j++)
				if (!grid[i][j].eliminated)
					allEliminated = false;
		} else if(score  <= 0){
			score = 0;
		}
		

		if (allEliminated && !gameOver)
		{
			cout << "Fim de jogo! Pontuação final: " << score << " em " << attempts << " tentativas." << endl;
			cout << "Pressione R para reiniciar." << endl;
			gameOver = true;
		}

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	// glDeleteVertexArrays(1, &VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        restartGame();
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no inicio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !gameOver)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int x = xpos / QUAD_WIDTH;
        int y = ypos / QUAD_HEIGHT;
        if (x >= 0 && x < COLS && y >= 0 && y < ROWS && !grid[y][x].eliminated)
        {
            iSelected = x + y * COLS;
        }
    }
}

GLuint createQuad()
{
	GLuint VAO;

	GLfloat vertices[] = {
		// x    y    z
		// T0
		-0.5, 0.5, 0.0,	 // v0
		-0.5, -0.5, 0.0, // v1
		0.5, 0.5, 0.0,	 // v2
		0.5, -0.5, 0.0	 // v3
	};

	GLuint VBO;
	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);
	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
	//  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	//  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
	//  Tipo do dado
	//  Se está normalizado (entre zero e um)
	//  Tamanho em bytes
	//  Deslocamento a partir do byte zero
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

void eliminarSimilares(float tolerancia)
{
    int x = iSelected % COLS;
    int y = iSelected / COLS;
    vec3 C = grid[y][x].color;
    int eliminados = 0;
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            if (!grid[i][j].eliminated)
            {
                vec3 O = grid[i][j].color;
                float d = sqrt(pow(C.r - O.r, 2) + pow(C.g - O.g, 2) + pow(C.b - O.b, 2));
                float dd = d / dMax;
                if (dd <= tolerancia)
                {
                    grid[i][j].eliminated = true;
                    eliminados++;
                }
            }
        }
    }
    // Pontuação: mais eliminações = mais pontos, penaliza tentativas
    if (eliminados > 0)
        score -= eliminados * 10 - attempts * 1.2;
    attempts++;
    iSelected = -1;
}


void drawText_GL33_centered(float x, float y, const char* text, float r, float g, float b, float scale)
{
    static GLuint vbo = 0, vao = 0;
    static GLuint shader = 0;
    static GLint uniColor = -1, uniProjection = -1;

    char buffer[99999];
    int num_quads = stb_easy_font_print(0, 0, (char*)text, NULL, buffer, sizeof(buffer));

    // Calcula largura do texto
    int text_width = stb_easy_font_width((char*)text);

    // Centraliza horizontalmente
    float x_offset = x - (text_width * scale) / 2.0f;

    // Aplica escala nos vértices
    float* verts = (float*)buffer;
    for (int i = 0; i < num_quads * 4; ++i) {
        verts[i * 4 + 0] = verts[i * 4 + 0] * scale + x_offset;
        verts[i * 4 + 1] = verts[i * 4 + 1] * scale + y;
    }

    if (shader == 0) {
        // Vertex shader simples para 2D
        const char* vs =
            "#version 330 core\n"
            "layout(location=0) in vec2 pos;\n"
            "uniform mat4 projection;\n"
            "void main(){ gl_Position = projection * vec4(pos,0,1); }\n";
        // Fragment shader simples para cor uniforme
        const char* fs =
            "#version 330 core\n"
            "uniform vec3 color;\n"
            "out vec4 FragColor;\n"
            "void main(){ FragColor = vec4(color,1); }\n";
        GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(vsId, 1, &vs, NULL); glCompileShader(vsId);
        glShaderSource(fsId, 1, &fs, NULL); glCompileShader(fsId);
        shader = glCreateProgram();
        glAttachShader(shader, vsId); glAttachShader(shader, fsId);
        glLinkProgram(shader);
        glDeleteShader(vsId); glDeleteShader(fsId);
        uniColor = glGetUniformLocation(shader, "color");
        uniProjection = glGetUniformLocation(shader, "projection");
    }

    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, num_quads * 4 * 16, buffer, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*)0);

    glUseProgram(shader);

    // Projeção ortográfica para 2D
    float ortho[16] = {
        2.0f/WIDTH, 0, 0, 0,
        0, -2.0f/HEIGHT, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUniformMatrix4fv(uniProjection, 1, GL_FALSE, ortho);
    glUniform3f(uniColor, r, g, b);

    glDrawArrays(GL_QUADS, 0, num_quads * 4);

    glBindVertexArray(0);
    glUseProgram(0);
}


