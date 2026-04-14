# Graphic-Computing

Trabalhos da disciplina **Computação Gráfica** (Unisinos, 2026/01), em **C++ / OpenGL** (GLFW, GLAD, GLM).

## Como compilar e executar

Instruções de linha de comando: [how-to-run.txt](how-to-run.txt).

## Projetos no repositório

### 1. Triangle

Triângulo 2D e introdução ao pipeline.

### 2. Hello3D e HelloCamera

Triângulo 3D, transformações e noções de câmera.

### 3. HelloSinteticCamera

Profundidade e movimento de objeto com teclado.

### 4. ObjectExercise

Cena simples com **dois** `.obj`, seleção com TAB, rotação (X/Y/Z) e translação (WASD) no objeto selecionado, troca perspectiva/ortográfica (P). Cores por vértice (sem Phong). Serve como base didática menor.

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

## Objetivo da disciplina (contexto)

Fundamentos de computação gráfica: renderização 2D/3D, transformações, câmera, iluminação e pipeline programável.
