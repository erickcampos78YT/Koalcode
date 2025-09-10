# Documentação da Linguagem >_KoalCode

## Sobre a Linguagem
KoalCode é uma linguagem de programação ULTRA simples interpretada com sintaxe inspirada diretamente em Lua, que suporta operações aritméticas, lógicas, bitwise, estruturas de controle, funções de I/O e programação gráfica com SDL2/OpenGL
e possui suporte a conexão internet.

## Como Compilar e Executar o Interpretador


## Compilação e Execução

### Requisitos Básicos
- Compilador GCC
- Sistema Linux/BSD/Unix/macOS ou Windows com MSYS2/MinGW
- Bibliotecas: libm (matemática), libpthread (threading)

### Requisitos para Gráficos
- SDL2 development libraries
- OpenGL libraries

### Requisitos para Rede
- libcurl development libraries
- pthreads (threading)

### Instalação de Dependências
#### Arch Linux
```bash
pacman -S gcc sdl2 sdl2_image sdl2_mixer sdl2_ttf mesa curl
```

#### Windows (Recomendado: usar vcpkg)
```cmd
# Instalar vcpkg primeiro
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Instalar dependências
.\vcpkg install sdl2:x64-windows sdl2-image:x64-windows curl:x64-windows pthreads:x64-windows
```

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libcurl4-openssl-dev
```

#### Fedora/CentOS
```bash
sudo dnf install gcc SDL2-devel SDL2_image-devel mesa-libGL-devel libcurl-devel
```

#### macOS
```bash
brew install gcc sdl2 sdl2_image curl
```

#### FreeBSD
```bash
pkg install gcc sdl2 sdl2_image sdl2_mixer sdl2_ttf mesa-libGL-devel mesa-libGLU-devel curl
```

### Comandos de Compilação

```bash
gcc koalcode.c -o koalcode -lm -lpthread -lSDL2 -lSDL2_image -lGL -lcurl 
##(eu sei que dá para muda o nome do executavel mas por boas praticas deixe como esta :D )


# Execução
./koalcode meu_scriptmain.kc



## Tipos de Dados

### Números (doubles)
- **Inteiros**: `42`, `100`, `-15`
- **Decimais**: `3.14`, `0.5`, `2.718`

### Strings
- **Sintaxe**: `"texto aqui"`
- **Uso**: Para funções `print()`, `writef()`, `readf()` e parâmetros gráficos
- **Limitação**: Não podem ser atribuídas diretamente a variáveis

### Variáveis
- **Declaração implícita**: Variáveis são criadas automaticamente na primeira atribuição
- **Nomes**: Devem começar com letra ou `_`, podem conter letras, números, `_` e `.`

## Operadores

### Aritméticos
```koalcode
a = 10 + 5     -- Adição
b = 10 - 5     -- Subtração
c = 10 * 5     -- Multiplicação
d = 10 / 5     -- Divisão
e = 10 % 3     -- Módulo
f = 2 ** 3     -- Potenciação (2³ = 8)
```

### Relacionais
```koalcode
a = 5 < 10     -- Menor que
b = 5 <= 10    -- Menor ou igual
c = 5 > 10     -- Maior que
d = 5 >= 10    -- Maior ou igual
e = 5 == 10    -- Igual
f = 5 != 10    -- Diferente (ou ~=)
```

### Lógicos
```koalcode
a = 1 && 0     -- E lógico
b = 1 || 0     -- OU lógico
c = !1         -- NÃO lógico
```

### Bitwise
```koalcode
a = 5 & 3      -- E bitwise
b = 5 | 3      -- OU bitwise
c = 5 ^ 3      -- XOR bitwise
d = 5 << 2     -- Deslocamento à esquerda
e = 20 >> 2    -- Deslocamento à direita
f = ~5         -- Complemento bitwise
```

### Atribuição
```koalcode
-- Atribuição simples
x = 10

-- Atribuições compostas
x += 5    -- x = x + 5
x -= 3    -- x = x - 3
x *= 2    -- x = x * 2
x /= 4    -- x = x / 4
x %= 3    -- x = x % 3
x &= 7    -- x = x & 7
x |= 1    -- x = x | 1
x ^= 2    -- x = x ^ 2
x <<= 1   -- x = x << 1
x >>= 2   -- x = x >> 2
x **= 2   -- x = x ** 2
```

### Unários
```koalcode
a = -x     -- Negação
b = +x     -- Positivo (sem efeito)
c = !x     -- NÃO lógico
d = ~x     -- Complemento bitwise
```

## Estruturas de Controle

### If / Else
```koalcode
-- if simples
if x > 0 {
    print("x é positivo!")
}

-- if com else
if numero % 2 == 0 {
    print("O número é par")
} else {
    print("O número é ímpar")
}

-- if else if
--(so nao vai usar essa porra para if recursivo ne aprende outra maneira)
if nota >= 90 {
    print("Conceito: A")
} else if nota >= 80 {
    print("Conceito: B")
} else if nota >= 70 {
    print("Conceito: C")
} else {
    print("Conceito: D")
}

-- if sem chaves (uma instrução)
if x > 0
    print("Positivo")

-- if aninhados
if temperatura > 20 {
    if chovendo == 0 {
        print("Bom dia para sair!")
    } else {
        print("Leve guarda-chuva o seu corno")
    }
}

-- if com operadores lógicos
if idade >= 18 && tem_carteira == 1 {
    print("Pode dirigir!")
}
```

### While
```koalcode
-- Com parênteses
while (x < 10) {
    print("x é", x)
    x += 1
}

-- Sem parênteses
while x > 0 {
    print("Contando:", x)
    x -= 1
}

-- Corpo sem chaves (uma única instrução)
while x < 5
    x += 1
```

### Blocos
```koalcode
-- Bloco
{
    a = 10
    b = 20
    print("Soma:", a + b)
}
```

## Funções Built-in

### Função de Saída

#### print()
Imprime valores na tela, separados por espaços e seguidos de nova linha.

```koalcode
print("Olá mundo")
print("Valores:", 10, 20, 30)
print("Resultado:", 2 + 3)

-- Misturando strings e números
nome = "João"
idade = 25
print("Nome:", nome, "Idade:", idade)
```

### Funções de Entrada/IO

### Posição do Mouse(Mickey)

#### input.mouse.x()
Retorna posição X atual do mouse na janela.

```koalcode
mouse_x = input.mouse.x()
print("Mouse X:", mouse_x)
```

#### input.mouse.y()
Retorna posição Y atual do mouse na janela.

```koalcode
mouse_y = input.mouse.y()
print("Mouse Y:", mouse_y)
```

### Movimento Relativo do Mouse

#### input.mouse.relx()
Retorna movimento relativo X do mouse.

```koalcode
rel_x = input.mouse.relx()
if rel_x != 0 {
    print("Mouse moveu horizontalmente:", rel_x)
}
```

#### input.mouse.rely()
Retorna movimento relativo Y do mouse.

```koalcode
rel_y = input.mouse.rely()
if rel_y != 0 {
    print("Mouse moveu verticalmente:", rel_y)
}
```

### Botões do Mouse

#### input.mouse.left()
--estado do botão esquerdo do mouse.

**Retorno:** `1` = pressionado, `0` = não pressionado

```koalcode
if input.mouse.left() == 1 {
    print("Botão esquerdo pressionado")
}
```

#### input.mouse.right()
--botão direito do mouse.

### Teclas do Teclado

#### input.key.w(), input.key.a(), input.key.s(), input.key.d()
Retornam estado das teclas WASD.

```koalcode
-- Sistema de movimento
if input.key.w() == 1 {
    print("Movendo para frente")
}
if input.key.s() == 1 {
    print("Movendo para trás")
}
if input.key.a() == 1 {
    print("Movendo para esquerda")
}
if input.key.d() == 1 {
    print("Movendo para direita")
}
```

#### input.key.space()
Retorna estado da barra de espaço.

```koalcode
if input.key.space() == 1 {
    print("Espaço pressionado - Pular!")
}
```

#### input.key.shift()
Retorna estado da tecla Shift esquerda.

```koalcode
if input.key.shift() == 1 {
    print("Shift pressionado")
}
```


#### inputn.io()
Lê um número do usuário e converte automaticamente para double.


```koalcode
print("Digite um número:")
numero = inputn.io()
print("Você digitou:", numero)
print("O dobro é:", numero * 2)
print("O quadrado é:", numero ** 2)

-- Exemplo de Soma
print("Digite o primeiro valor:")
a = inputn.io()
print("Digite o segundo valor:")
b = inputn.io()
print("Soma:", a + b)
```

#### input.io()
Lê uma linha de texto do usuário (funcionalidade básica).

**Retorno:** 0.0 (placeholder)

```koalcode
print("Digite seu nome:")
input.io()
print("Obrigado!")

-- Aguardar Enter do usuário
print("Pressione Enter para continuar...")
input.io()
```

### Funções de Sistema

#### clear()
Limpa a tela do terminal.

**Compatibilidade:** Linux/Unix/MacOS (clear)e Windows (cls)

```koalcode
print("Esta mensagem vai sumir...")
print("Pressione Enter para limpar:")
input.io()
clear()
print("Tela limpa!")

-- Exemplo de menu
opcao = 1
while opcao != 0 {
    clear()
    print("=== MENU ===")
    print("1 - Opção 1")
    print("0 - Sair")
    opcao = inputn.io()
}
```

### Funções de Arquivo

#### writef(arquivo, conteudo)
Escreve texto em um arquivo. Cria o arquivo se não existir, sobrescreve se existir.

**Parâmetros:**
- `arquivo` (string): Nome do arquivo
- `conteudo` (string): Texto a ser escrito

**Retorno:**
- `1` = Sucesso
- `0` = Falha

```koalcode
-- Criar arquivo simples
resultado = writef("dados.txt", "Olá mundo!")
print("Resultado:", resultado)

-- Criar arquivo de exemplo de configuração
config_ok = writef("config.txt", "debug=true\nversao=1.0\nautor=KoalCode")

-- Verificar se o arquivo foi criado com sucesso
resultado = writef("log.txt", "inicia")
print("Log criado:", resultado)
```

#### readf(arquivo)
Verifica se um arquivo existe e pode ser lido.

**Parâmetros:**
- `arquivo` (string): Nome do arquivo a verificar

**Retorno:**
- `1` = Arquivo existe
- `0` = Arquivo não existe

```koalcode
-- Verificar se arquivo existe
existe = readf("dados.txt")
print("Arquivo existe:", existe)

-- Sistema de configuração
config_existe = readf("config.txt")
print("Status configuração:", config_existe)

-- Criar arquivo se não existir
log_existe = readf("app.log")
if log_existe == 0 {
    writef("app.log", "Log criado automaticamente")
    print("Log criado!")
}
```

## Funções Gráficas (SDL2/OpenGL)

### Inicialização e Controle

#### graphics.init()
Inicializa SDL2 e OpenGL, cria janela.

**Retorno:** `1` = sucesso, `0` = falha

```koalcode
resultado = graphics.init()
if resultado == 1 {
    print("Gráficos inicializados!")
    -- Seu código gráfico aqui
    graphics.quit()
}
```

#### graphics.quit()
Finaliza sistema gráfico e fecha janela.

**Retorno:** `1` = sucesso, `0` = falha

```koalcode
graphics.quit()
```

#### graphics.events()
Processa eventos de io dentro da janela SDL (teclas, mouse, fechar janela).

**Retorno:**
- Número de eventos processados
- `-1` = usuário fechou janela

```koalcode
eventos = graphics.events()
if eventos == -1 {
    print("Usuário fechou janela")
    sair = 1
}
```

### Renderização

#### graphics.clear()
Limpa buffers de cor e profundidade.

```koalcode
graphics.clear()  -- Limpa tela para preto
```

#### graphics.swap()
Apresenta o frame renderizado na tela.

```koalcode
graphics.swap()  -- Mostra o que foi desenhado
```

#### graphics.color(r, g, b)
Define cor atual para desenho (valores 0.0 a 1.0).

```koalcode
graphics.color(1, 0, 0)    -- Vermelho
graphics.color(0, 1, 0)    -- Verde
graphics.color(0, 0, 1)    -- Azul
graphics.color(1, 1, 1)    -- Branco
graphics.color(0.5, 0.5, 0.5)  -- Cinza
graphics.color(0, 0, 0)    -- Preto
```

### Primitivas de Desenho

#### graphics.point(x, y, z)
Desenha um ponto no espaço 3D.

```koalcode
graphics.point(0, 0, 0)      -- Ponto na origem
graphics.point(1, 1, -5)     -- Ponto afastado
```

#### graphics.line(x1, y1, z1, x2, y2, z2)
Desenha uma linha entre dois pontos.

```koalcode
graphics.line(0, 0, 0, 1, 1, 0)  -- Linha diagonal
graphics.line(-1, 0, -5, 1, 0, -5)  -- Linha horizontal
```

#### graphics.triangle(x1,y1,z1, x2,y2,z2, x3,y3,z3)
Desenha um triângulo preenchido.

```koalcode
graphics.triangle(-1, -1, 0,  1, -1, 0,  0, 1, 0)  -- Triângulo
```

### Transformações 3D

#### graphics.loadmatrix()
Carrega matriz identidade (reset das transformações).

```koalcode
graphics.loadmatrix()  -- Resetar transformações
```

#### graphics.translate(x, y, z)
Aplica translação (movimento).

```koalcode
graphics.translate(0, 0, -5)  -- Move 5 unidades para frente
graphics.translate(2, 1, 0)   -- Move 2 à direita, 1 acima
```

#### graphics.rotate(angulo, x, y, z)
Aplica rotação em graus ao redor de um eixo.

```koalcode
graphics.rotate(45, 0, 1, 0)  -- Rotação de 45° em Y
graphics.rotate(90, 1, 0, 0)  -- Rotação de 90° em X
```


## Comentários
```koalcode
-- Este é um comentário de linha
-- Comentários começam com dois hífens

x = 10  -- Comentário no final da linha

-- Comentário de múltiplas linhas
-- deve usar múltiplas linhas com --
-- em cada linha
```

## Precedência de Operadores
(Do maior para o menor)

1. **Unários**: `!`, `~`, `-`, `+`
2. **Exponenciação**: `**` (associativa à direita)
3. **Multiplicativos**: `*`, `/`, `%`
4. **Aditivos**: `+`, `-`
5. **Shift**: `<<`, `>>`
6. **Relacionais**: `<`, `<=`, `>`, `>=`
7. **Igualdade**: `==`, `!=`, `~=`
8. **Bitwise AND**: `&`
9. **Bitwise XOR**: `^`
10. **Bitwise OR**: `|`
11. **Lógico AND**: `&&`
12. **Lógico OR**: `||`
13. **Atribuição**: `=`, `+=`, `-=`, etc.

## Exemplos Completos

### Exemplo 1: Calculadora Básica
```koalcode
-- Calculadora simples
print("=== CALCULADORA ===")
print("Digite o primeiro número:")
a = inputn.io()
print("Digite o segundo número:")
b = inputn.io()

print("Resultados:")
print("Soma:", a + b)
print("Subtração:", a - b)
print("Multiplicação:", a * b)
if b != 0 {
    print("Divisão:", a / b)
}
print("Potência:", a ** b)
```

### Exemplo 2: Sistema com Arquivos
```koalcode
-- Sistema de dados com arquivos
print("=== SISTEMA DE DADOS ===")

-- Verificar se arquivo de dados existe
dados_existem = readf("dados.txt")
print("Dados existem:", dados_existem)

if dados_existem == 0 {
    print("Criando arquivo de dados...")
    resultado = writef("dados.txt", "Dados iniciais do sistema")
    print("Arquivo criado:", resultado)
}

-- Coletar dados do usuário
print("Digite seu número favorito:")
numero = inputn.io()
print("Número salvo internamente:", numero)

-- Criar log da operação
log_ok = writef("operacao.log", "Usuario inseriu dados com sucesso")
print("Log criado:", log_ok)
```

### Exemplo 3: Menu Interativo
```koalcode
-- Menu com limpeza de tela e if/else
opcao = 1
while opcao != 0 {
    clear()
    print("=== MENU PRINCIPAL ===")
    print("1 - Calculadora")
    print("2 - Salvar dados")
    print("3 - Verificar arquivos")
    print("0 - Sair")
    print()
    print("Digite sua opção:")
    opcao = inputn.io()

    if opcao == 1 {
        print("Digite dois números:")
        a = inputn.io()
        b = inputn.io()
        print("Soma:", a + b)
    } else if opcao == 2 {
        resultado = writef("dados.txt", "Dados salvos pelo menu")
        print("Dados salvos:", resultado)
    } else if opcao == 3 {
        existe = readf("dados.txt")
        if existe == 1 {
            print("Arquivo encontrado!")
        } else {
            print("Arquivo não existe")
        }
    } else if opcao != 0 {
        print("Opção inválida!")
    }

    if opcao != 0 {
        print("Pressione Enter para continuar...")
        input.io()
    }
}

clear()
print("Programa finalizado!")
```

### Exemplo 4: App Gráfico Básico
```koalcode
print("=== GRÁFICOS 3D ===")

resultado = graphics.init()
if resultado == 1 {
    print("Gráficos inicializados! Janela aberta.")

    frame = 0
    rodando = 1

    while rodando == 1 && frame < 180 {
        -- Processar eventos
        eventos = graphics.events()
        if eventos == -1 {
            rodando = 0
        }

        -- Limpar tela
        graphics.clear()

        -- Resetar transformações
        graphics.loadmatrix()

        -- Mover objeto para frente
        graphics.translate(0, 0, -5)

        -- Rotaciocao multiplicado no frame
        angulo = frame * 2
        graphics.rotate(angulo, 1, 1, 0)

        -- Desenhar triângulo vermelho
        graphics.color(1, 0, 0)
        graphics.triangle(-1, -1, 0,  1, -1, 0,  0, 1, 0)

        -- Desenhar linhas azuis
        graphics.color(0, 0, 1)
        graphics.line(-1, 0, 0,  1, 0, 0)
        graphics.line(0, -1, 0,  0, 1, 0)

        -- Mostrar na tela
        graphics.swap()

        frame += 1
    }

    graphics.quit()
    print("Gráficos finalizados.")
} else {
    print("Erro ao inicializar gráficos!")
}
```

### Exemplo 5: DEMO de um cubo em rotacao
```koalcode
-- Mini jogo: Cubo Rotativo
print("=== Cubo Rotativo ===")

grafico_ok = graphics.init()
if grafico_ok == 1 {
    frame = 0
    jogando = 1
    pontos = 0

    while jogando == 1 && frame < 300 {
        eventos = graphics.events()
        if eventos == -1 {
            jogando = 0
        }

        graphics.clear()
        graphics.loadmatrix()
        graphics.translate(0, 0, -5)

        -- Rotação
        graphics.rotate(frame * 3, 1, 1, 1)

        -- Cor baseada no frame
        r = (frame % 60) / 60
        g = 1 - r
        graphics.color(r, g, 0.5)

        -- Desenhar cubo
        -- Face frontal
        graphics.triangle(-1, -1, 1,  1, -1, 1,  1, 1, 1)
        graphics.triangle(-1, -1, 1,  1, 1, 1,  -1, 1, 1)

        -- Face traseira
        graphics.color(g, 0.5, r)
        graphics.triangle(1, -1, -1,  -1, -1, -1,  -1, 1, -1)
        graphics.triangle(1, -1, -1,  -1, 1, -1,  1, 1, -1)

        graphics.swap()
        frame += 1

        if frame % 60 == 0 {
            pontos += 1
            print("Pontos:", pontos)
        }
    }

    graphics.quit()
    print("Jogo finalizado! Pontos finais:", pontos)
} else {
    print("Não foi possível inicializar gráficos.")
}
```

### Exemplo 6: Texturas e Modelos 3D
```koalcode
print("=== DEMO 3D COM TEXTURAS ===")

grafico_ok = graphics.init()
if grafico_ok == 1 {
    -- Ativar transparência
    graphics.enable_blend()

    -- Carregar textura
    texture_id = graphics.load_texture("assets/texture.png")
    if texture_id >= 0 {
        print("Textura carregada!")
    }

    -- Carregar modelo 3D
    model_id = graphics.load_model("assets/cube.obj")
    if model_id >= 0 {
        print("Modelo 3D carregado!")
    }

    frame = 0
    while frame < 180 {
        graphics.events()
        graphics.clear()
        graphics.loadmatrix()
        graphics.translate(0, 0, -5)
        graphics.rotate(frame * 2, 1, 1, 0)

        -- Desenhar com textura se disponível
        if texture_id >= 0 {
            graphics.bind_texture(texture_id)
            graphics.color(1, 1, 1)  -- Branco para não alterar textura

            if model_id >= 0 {
                graphics.draw_model(model_id)
            } else {
                -- Quad texturizado
                graphics.quad_textured(-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0)
            }

            graphics.bind_texture(-1)  -- Desativar textura
        } else {
            -- Sem textura, usar cores
            graphics.color(1, 0.5, 0)
            if model_id >= 0 {
                graphics.draw_model(model_id)
            } else {
                graphics.triangle(-1, -1, 0,  1, -1, 0,  0, 1, 0)
            }
        }

        graphics.swap()
        frame = frame + 1
    }

    graphics.disable_blend()
    graphics.quit()
}
```

### Exemplo 7: Transparência e Efeitos
```koalcode
print("=== DEMO TRANSPARÊNCIA ===")

grafico_ok = graphics.init()
if grafico_ok == 1 {
    graphics.enable_blend()

    frame = 0
    while frame < 120 {
        graphics.events()
        graphics.clear()
        graphics.loadmatrix()
        graphics.translate(0, 0, -5)

        -- Triângulo semi-transparente rotativo
        graphics.rotate(frame * 3, 0, 0, 1)
        alpha = 0.7  -- 70% opaco
        graphics.color(1, 0, 0)  -- Vermelho com transparência
        graphics.triangle(-1, -1, 0,  1, -1, 0,  0, 1, 0)

        -- Segundo triângulo sobreposto
        graphics.rotate(45, 0, 0, 1)
        graphics.color(0, 1, 0)  -- Verde com transparência
        graphics.triangle(-0.5, -0.5, 0.1,  0.5, -0.5, 0.1,  0, 0.5, 0.1)

        graphics.swap()
        frame = frame + 1
    }

    graphics.disable_blend()
    graphics.quit()
}
```

### Exemplo 8: Funções Personalizadas com Controle de Memória
```koalcode
print("=== FUNÇÕES PERSONALIZADAS ===")

-- Função simples de cálculo
fuktion calcular_area(raio) {
    pi = 3.14159
    area = pi * raio * raio
    return area
}

-- Função com controle de memória (modo FIFO)
fuktion processar_dados(n) {
    memlimit(2, "kb", 0)
    
    i = 0
    while i < n {
        temp_var = "dados_" + i
        resultado = i * 2
        -- processamento...
        i = i + 1
    }
    return "processado"
}

-- Função recursiva para fatorial
fuktion fatorial(n) {
    if n <= 1 {
        return 1
    } else {
        return n * fatorial(n - 1)
    }
}

-- Função com reinício controlado de memória
fuktion gerar_sequencia(limite) {
    memlimit(5, "kb", 1)
    
    i = 0
    while i < limite {
        seq = "item_" + i
        valor = i * 3
        -- mais processamento...
        i = i + 1
    }
    return "sequencia_gerada"
}

-- Testando as funções
print("Área do círculo (raio 5):", calcular_area(5))
print("Fatorial de 6:", fatorial(6))
print("Processamento:", processar_dados(100))
print("Sequência:", gerar_sequencia(50))

-- Exemplo de função que chama outras funções
fuktion calculadora_completa(a, b) {
    soma = a + b
    produto = a * b
    area_circulo = calcular_area(a)
    return soma + produto + area_circulo
}

resultado = calculadora_completa(3, 4)
print("Resultado da calculadora:", resultado)
```

## Características Importantes

### Separadores Opcionais
- Ponto e vírgula (`;`) são opcionais
- Quebras de linha são suficientes para separar instruções

### Identificadores
- Podem conter letras, números, `_` e `.` (para funções como `input.io()`)
- Devem começar com letra ou `_`

### Strings
- Delimitadas por aspas duplas: `"texto"`
- Suporte a caracteres de escape básicos como `\n`
- Não podem ser atribuídas a variáveis (limitação atual)

### Arquivos
- Arquivos são criados no diretório atual
- Encoding padrão do sistema (geralmente UTF-8)
- Sem limite de tamanho específico

### Gráficos 3D
- Sistema de coordenadas padrão OpenGL
- Renderização em tempo real
- Suporte a texturas PNG/JPG com transparência
- Carregamento de modelos 3D em formato OBJ
- Suporte a transformações básicas (rotação, translação)
- Primitivas: pontos, linhas, triângulos

## Funções Definidas pelo Usuário

### fuktion - Definição de Funções

KoalCode suporta a definição de funções personalizadas usando a palavra-chave `fuktion`.

#### Sintaxe Básica
```koalcode
fuktion nome(param1, param2, ...) {
    // corpo da função
    return expressao
}
```

#### Características
- **Parâmetros posicionais**: Não há tipagem explícita
- **Escopo local**: Variáveis definidas dentro da função têm escopo local
- **Return**: Finaliza a execução e retorna um valor (se não especificado, retorna nulo)
- **Recursão**: Suporta chamadas recursivas e chamadas de outras funções
- **memlimit**: Pode ser declarado dentro do corpo da função para controle de memória

#### Exemplo Básico
```koalcode
fuktion soma(a, b) {
    result = a + b
    return result
}

x = soma(2, 3)   -- x passa a valer 5
```

### memlimit - Controle de Memória Local

A instrução `memlimit` permite controlar o uso de memória dentro de funções, útil para funções que criam muitas variáveis temporárias.

#### Sintaxe
```koalcode
memlimit(<valor>, "<unidade>", <modo>)
```

**Parâmetros:**
- `<valor>`: Número inteiro representando a quantidade
- `<unidade>`: String entre aspas - `"kb"`, `"mb"`, ou `"gb"` (case-insensitive)
- `<modo>`: Inteiro que define o comportamento quando o limite é atingido:
  - `1` - Reinício controlado: limpa variáveis locais e reinicia a execução (até 3 tentativas)
  - `0` - Evicção FIFO: remove variáveis mais antigas até caber no limite

#### Exemplos de Uso

**Modo 1 - Reinício Controlado:**
```koalcode
fuktion gerar_muitas() {
    memlimit(10, "kb", 1)
    for i = 1; i <= 1000; i = i + 1 {
        tmp_i = i * 2
    }
    return "feito"
}
```

**Modo 0 - Evicção FIFO:**
```koalcode
fuktion buffer_process() {
    memlimit(1, "mb", 0)
    a = "..."
    b = "..."
    c = "..."
    -- se o limite for ultrapassado, variáveis antigas são removidas
    return "processado"
}
```

#### Observações Importantes
- `memlimit` afeta apenas o escopo da função onde está declarado
- A medição de memória é uma estimativa baseada no número de variáveis e comprimento dos nomes
- No modo 1, todas as variáveis locais são apagadas em um reinício
- No modo 0, variáveis são removidas por ordem de criação (FIFO)

## Limitações Atuais

- **Funções**: Suporte básico a funções definidas pelo usuário com `fuktion`
- **Estruturas de dados**: Não possui arrays ou objetos
- **Strings**: Suporte limitado, principalmente para I/O (não podem ser atribuídas a variáveis)
- **Classes**: Sistema declarado mas não funcional (experimental)
- **Threading**: Suporte básico (experimental)
- **Arquivos**: `readf()` apenas verifica existência, não retorna conteúdo
- **For loops**: Apenas `while` loops estão disponíveis
- **Gráficos**: Funcionalidades básicas com suporte a texturas, transparência e modelos 3D

```

### Resolução de Problemas
- **Erro de linking**: Certifique-se de usar as flags corretas
- **SDL2 não encontrado**: Instale as bibliotecas necessárias
- **OpenGL não disponível**: Verifique drivers de sua gpu
- **Arquivo não encontrado**: Verifique se o arquivo `.kc` existe
- **Erro de sintaxe**: Verifique parênteses e chaves balanceadas
- **Janela não abre**: Verifique se há display disponível (não funciona em SSH sem X11)

## Lista Completa de Funções

### E/S Básica
- `print(...)` - Saída de texto
- `input.io()` - Entrada de texto
- `inputn.io()` - Entrada numérica

### Funções Personalizadas
- `fuktion nome(param1, param2, ...) { ... }` - Definir função personalizada
- `memlimit(valor, "unidade", modo)` - Controlar memória local em funções

### Sistema
- `clear()` - Limpar terminal

### Arquivos
- `writef(arquivo, conteudo)` - Escrever arquivo
- `readf(arquivo)` - Verificar arquivo

### Gráficos - Controle
- `graphics.init()` - Inicializar SDL2/OpenGL
- `graphics.quit()` - Finalizar gráficos
- `graphics.events()` - Processar eventos

### Gráficos - Renderização
- `graphics.clear()` - Limpar buffers
- `graphics.swap()` - Apresentar frame
- `graphics.color(r, g, b)` - Definir cor

### Gráficos - Primitivas
- `graphics.point(x, y, z)` - Desenhar ponto
- `graphics.line(x1, y1, z1, x2, y2, z2)` - Desenhar linha
- `graphics.triangle(x1,y1,z1, x2,y2,z2, x3,y3,z3)` - Desenhar triângulo
- `graphics.quad_textured(x1,y1,z1, x2,y2,z2, x3,y3,z3, x4,y4,z4)` - Desenhar quad com textura

### Gráficos - Transformações
- `graphics.loadmatrix()` - Matriz identidade
- `graphics.translate(x, y, z)` - Translação
- `graphics.rotate(angulo, x, y, z)` - Rotação

### Gráficos - Texturas e Modelos 3D
- `graphics.load_texture(filename)` - Carregar textura (PNG/JPG)
- `graphics.bind_texture(texture_id)` - Ativar textura (-1 para desativar)
- `graphics.load_model(filename)` - Carregar modelo 3D (formato OBJ)
- `graphics.draw_model(model_id)` - Desenhar modelo carregado
- `graphics.enable_blend()` - Ativar transparência
- `graphics.disable_blend()` - Desativar transparência
- `graphics.quad_textured(x1,y1,z1, x2,y2,z2, x3,y3,z3, x4,y4,z4)` - Quad com textura

#### Formatos Suportados
**Texturas:**
- PNG (com transparência alfa)
- JPG/JPEG

**Modelos 3D:**
- OBJ (Wavefront) - apenas faces triangulares
- Formato: vértices (v x y z) e faces (f v1 v2 v3)

#### Como Usar Texturas
```koalcode
graphics.init()
graphics.enable_blend()  -- Para transparência

-- Carregar textura
texture_id = graphics.load_texture("assets/minha_textura.png")
if texture_id >= 0 {
    -- Ativar textura
    graphics.bind_texture(texture_id)
    graphics.color(1, 1, 1)  -- Branco para não alterar textura

    -- Desenhar quad texturizado
    graphics.quad_textured(-1,-1,0, 1,-1,0, 1,1,0, -1,1,0)

    -- Desativar textura
    graphics.bind_texture(-1)
}
```

#### Como Usar Modelos 3D
```koalcode
graphics.init()

-- Carregar modelo
model_id = graphics.load_model("assets/meu_modelo.obj")
if model_id >= 0 {
    graphics.color(0.8, 0.4, 0.9)
    graphics.draw_model(model_id)
}
```



---

## 🌐 FUNCIONALIDADES DE REDE

O KoalCode agora inclui funcionalidades completas de rede para requisições HTTP, conexões socket e utilitários de conectividade.

### Inicialização do Sistema de Rede

```koalcode
network.init()    -- Inicializa o sistema de rede (retorna 1 se sucesso, 0 se falha)
network.quit()    -- Finaliza o sistema de rede
```

**Importante:** Sempre inicialize o sistema de rede com `network.init()` antes de usar qualquer função de rede.

### Requisições HTTP

#### HTTP GET
```koalcode
http.get("https://exemplo.com/api/dados")
```
- Faz uma requisição GET para a URL especificada
- Retorna o código de status HTTP (200, 404, 500, etc.)
- Imprime a resposta no console

#### HTTP POST
```koalcode
http.post("https://exemplo.com/api/dados", "{\"nome\": \"João\"}")
```
- Faz uma requisição POST para a URL especificada
- Envia os dados fornecidos no corpo da requisição
- Retorna o código de status HTTP
- Imprime a resposta no console

### Conexões Socket TCP

#### Conectar a um Servidor
```koalcode
sock = socket.connect("exemplo.com", 80)
```
- Conecta a um servidor TCP
- Retorna o descritor do socket (número) se sucesso, 0 se falha

#### Enviar Dados
```koalcode
socket.send(sock, "dados para enviar")
```
- Envia dados através do socket
- Retorna o número de bytes enviados

#### Receber Dados
```koalcode
socket.recv(sock, 1024)  -- recebe até 1024 bytes
```
- Recebe dados do socket
- Retorna o número de bytes recebidos
- Imprime os dados recebidos no console

#### Fechar Conexão
```koalcode
socket.close(sock)
```
- Fecha a conexão socket
- Retorna 1 se sucesso, 0 se falha

### Utilitários de Rede

#### Teste de Conectividade (Ping)
```koalcode
network.ping("google.com")
```
- Testa a conectividade com um host
- Retorna 1 se o host está acessível, 0 caso contrário

### Exemplo Completo de Uso

```koalcode
-- Inicializar sistema de rede
network.init()

-- Fazer requisição HTTP GET
print("Fazendo requisição HTTP GET...")
status = http.get("https://httpbin.org/get")
print("Status da resposta:", status)

-- Fazer requisição HTTP POST
print("Fazendo requisição HTTP POST...")
status = http.post("https://httpbin.org/post", "{\"mensagem\": \"Olá do KoalCode!\"}")
print("Status da resposta:", status)

-- Testar conectividade
print("Testando conectividade...")
if network.ping("google.com") {
    print("Google está acessível!")
} else {
    print("Google não está acessível.")
}

-- Exemplo de conexão socket
print("Conectando via socket...")
sock = socket.connect("httpbin.org", 80)
if sock > 0 {
    print("Conectado! Enviando requisição HTTP...")
    
    -- Enviar requisição HTTP via socket
    http_request = "GET /get HTTP/1.1\r\nHost: httpbin.org\r\nConnection: close\r\n\r\n"
    bytes_sent = socket.send(sock, http_request)
    print("Bytes enviados:", bytes_sent)
    
    -- Receber resposta
    print("Recebendo resposta...")
    bytes_received = socket.recv(sock, 1024)
    print("Bytes recebidos:", bytes_received)
    
    -- Fechar conexão
    print("Fechando conexão...")
    socket.close(sock)
} else {
    print("Falha na conexão socket")
}

-- Finalizar sistema de rede
network.quit()
print("Exemplo de rede concluído!")
```

### Tratamento de Erros

Todas as funções de rede retornam valores indicando sucesso ou falha:

- **0**: Falha na operação
- **1**: Sucesso (para funções booleanas)
- **Número positivo**: Código de status HTTP ou número de bytes (para operações de dados)

**Sempre verifique o valor de retorno** para garantir que a operação foi bem-sucedida:

```koalcode
-- Exemplo de tratamento de erro
sock = socket.connect("servidor.com", 80)
if sock > 0 {
    print("Conexão estabelecida com sucesso!")
    -- ... usar a conexão ...
    socket.close(sock)
} else {
    print("ERRO: Não foi possível conectar ao servidor")
}
```

### Notas Importantes

1. **Sempre inicialize** o sistema de rede com `network.init()` antes de usar funções HTTP
2. **Feche as conexões socket** com `socket.close()` quando não precisar mais delas
3. **Finalize o sistema de rede** com `network.quit()` ao terminar o programa
4. As funções HTTP são **síncronas** e podem demorar alguns segundos para completar
5. O sistema de rede usa **cURL** para requisições HTTP e **sockets BSD** para conexões TCP
6. Para **Windows**, certifique-se de ter as dependências instaladas (SDL2, cURL, etc.)

---

**KoalCode v1.0 Experimental**
