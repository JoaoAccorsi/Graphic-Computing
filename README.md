# Graphic-Computing

Trabalhos da disciplina **Computação Gráfica** (Unisinos, 2026/01), em **C++ / OpenGL** (GLFW, GLAD, GLM).

## Como compilar e executar

Instruções completas: [how-to-run.txt](how-to-run.txt).

Exemplo rápido (ObjectExercise):
```bash
clang++ ObjectExercise.cpp Camera.cpp glad.c \
    -Idependencies/include -I/opt/homebrew/include \
    -Ldependencies/library -lglfw.3.4 \
    -rpath @executable_path/dependencies/library \
    -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo \
    -o ObjectExercise
./ObjectExercise
```

## Projetos no repositório

### 1. Triangle

Triângulo 2D e introdução ao pipeline.

### 2. Hello3D e HelloCamera

Triângulo 3D, transformações e noções de câmera.

### 3. HelloSinteticCamera

Profundidade e movimento de objeto com teclado.

### 4. ObjectExercise

Cena com **quatro** modelos `.obj` (Suzanne, Airplane, Cube, Robot), seleção cíclica, transformações interativas (rotação, translação e escala) e wireframe sobreposto no objeto selecionado. Leitor OBJ, e cores por vértice.

| Comando | Ação |
|---------|------|
| **TAB** | Selecionar próximo objeto (cíclico) |
| **X / Y / Z** | Rotacionar no eixo X / Y / Z|
| **W** | Transladar para frente (eixo Z+) |
| **S** | Transladar para trás (eixo Z-) |
| **A** | Transladar para esquerda (eixo X-) |
| **D** | Transladar para direita (eixo X+) |
| **Q** | Transladar para baixo (eixo Y-) |
| **E** | Transladar para cima (eixo Y+) |
| **+** / **-** | Escala uniforme (todos os eixos) |
| **1** / **2** | Aumentar / diminuir escala no eixo X |
| **3** / **4** | Aumentar / diminuir escala no eixo Y |
| **5** / **6** | Aumentar / diminuir escala no eixo Z |
| **P** | Alternar perspectiva / ortográfica |
| **ESC** | Fechar a janela |

### 5. ReaderViewer3D (entrega principal — Etapa 1)

Visualizador com:

| Requisito | Implementação |
|-----------|----------------|
| Parser `.obj` | Próprio: lê `v`, `vn`, `vt`, faces triangulares ou polígonos triangulados em leque; índices `v/vt/vn` e índices negativos |
| Atributos na GPU | VAO + VBO: posição (0), normal (1), UV (2) |
| Vários objetos | Quatro modelos em grade; **TAB** alterna seleção |
| Transformações | **1** rotação, **2** translação, **3** escala (por eixo + **+/-** uniforme) |
| Câmera FPS | **C** alterna modo câmera / objeto: WASD, **Space** / **Shift** (vertical), **mouse** |
| Projeção | **P** perspectiva ↔ ortográfica |
| Phong | Ambiente + difusa + especular nos shaders; luz pontual |
| Luz | Posição ajustável: segure **L** e use **WASD** + **Q/E** (eixo Z) |
| Material (ka, kd, ks) | Objeto selecionado: **`[` `]`** shininess; **R/T** ka; **Y/U** kd; **Z/X** ks |
| Wireframe sobreposto | **O** liga/desliga |
| Extras | **G** grid no chão + eixos RGB |

Controles completos são impressos no terminal ao iniciar o programa.

**Nota:** não há `.ply` nem Assimp no momento; o enunciado aceita **uma** das opções (parser próprio em `.obj` atende).
