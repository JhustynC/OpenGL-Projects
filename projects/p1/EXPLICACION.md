# Explicacion Completa del Programa OpenGL

Este programa dibuja un **rectangulo naranja** en una ventana usando OpenGL moderno (version 3.3). A continuacion se desglosa absolutamente todo.

---

## 1. Las Bibliotecas (Includes)

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
```

### GLAD (`glad/glad.h`)

OpenGL es solo una **especificacion** (un documento que dice "estas funciones deben existir"). Los fabricantes de tarjetas graficas (NVIDIA, AMD, Intel) implementan esas funciones en sus drivers. El problema es que **la ubicacion de esas funciones varia** segun el sistema operativo y el driver.

GLAD es un **cargador de funciones OpenGL**. Su trabajo es buscar en tu sistema donde estan todas las funciones de OpenGL y darte punteros a ellas para que las puedas usar. Sin GLAD, tendrias que buscar manualmente cada funcion con codigo especifico del sistema operativo.

### GLFW (`GLFW/glfw3.h`)

GLFW (Graphics Library Framework) maneja todo lo que **no es dibujar**:

- Crear ventanas
- Capturar input del teclado/mouse
- Crear el contexto OpenGL (la conexion entre tu programa y la GPU)

OpenGL por si solo NO puede crear ventanas ni leer el teclado. Necesita una biblioteca como GLFW para eso.

### iostream

Biblioteca estandar de C++ para imprimir mensajes en la consola (para errores, depuracion, etc).

---

## 2. Declaraciones Adelantadas (Forward Declarations)

```cpp
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
```

Esto le dice al compilador: "estas funciones existen, las voy a definir mas abajo". Asi puedes usarlas en `main()` antes de escribir su codigo completo. Es una practica comun en C/C++.

---

## 3. Constantes de Configuracion

```cpp
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
```

Definen el tamano inicial de la ventana: **800 pixeles de ancho** y **600 de alto**. Se usan como `const` para evitar modificarlas accidentalmente.

---

## 4. Los Shaders (El Corazon del Renderizado)

### Que es un Shader?

Un shader es un **pequeno programa que se ejecuta en la GPU** (tarjeta grafica), no en la CPU. La GPU tiene miles de nucleos pequenos que pueden ejecutar estos programas en paralelo. OpenGL moderno **requiere** al menos dos shaders para dibujar cualquier cosa:

1. **Vertex Shader** - Procesa cada vertice (punto/esquina)
2. **Fragment Shader** - Decide el color de cada pixel

### Que es el Pipeline Grafico?

Imagina una fabrica con estaciones de trabajo en cadena:

```
Vertices (datos) --> [Vertex Shader] --> [Ensamblado de Primitivas] --> [Rasterizacion] --> [Fragment Shader] --> Pixeles en pantalla
```

1. **Vertex Shader**: Recibe cada vertice y decide su posicion final en pantalla
2. **Ensamblado de Primitivas**: Conecta los vertices en triangulos (o lineas, puntos)
3. **Rasterizacion**: Determina que pixeles de la pantalla cubre cada triangulo
4. **Fragment Shader**: Para cada pixel cubierto, decide su color final

### Vertex Shader

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
void main()
{
   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
```

Este es codigo **GLSL** (OpenGL Shading Language) escrito como un string de C++. Desglose linea por linea:

- **`#version 330 core`**: Usa GLSL version 3.30 en modo "core" (sin funciones obsoletas). Esta version corresponde a OpenGL 3.3.

- **`layout (location = 0) in vec3 aPos;`**:
  - `layout (location = 0)`: Este atributo de entrada esta en la **posicion/slot 0**. Es como decir "el primer dato que me llegue va aqui". Esto debe coincidir con lo que configuramos en `glVertexAttribPointer`.
  - `in`: Es una variable de **entrada** (viene de los datos que le pasamos desde la CPU).
  - `vec3`: Es un vector de 3 componentes (x, y, z). Un `vec3` es un tipo de dato de GLSL que guarda 3 floats.
  - `aPos`: Nombre de la variable (la "a" es convencion para "attribute").

- **`gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);`**:
  - `gl_Position` es una variable **especial predefinida** de GLSL. Es la salida obligatoria del vertex shader: la posicion final del vertice.
  - Se convierte de `vec3` a `vec4` anadiendo un cuarto componente (`1.0`). Este cuarto componente `w` se usa para la **perspectiva** (division de perspectiva). Con `w = 1.0`, la posicion no cambia. Si fuera otro valor, OpenGL dividiria x, y, z entre w.

### Fragment Shader

```glsl
#version 330 core
out vec4 FragColor;
void main()
{
   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
```

- **`out vec4 FragColor;`**: Variable de **salida**. El color final del pixel. Es un `vec4` porque los colores tienen 4 componentes: **R, G, B, A** (rojo, verde, azul, alfa/transparencia).

- **`FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);`**: Asigna un color fijo a TODOS los pixeles:
  - `1.0f` = Rojo al maximo (255)
  - `0.5f` = Verde al 50% (127)
  - `0.2f` = Azul al 20% (51)
  - `1.0f` = Alfa al maximo (totalmente opaco)
  - Resultado: un color **naranja**.

  Los colores en OpenGL van de `0.0` a `1.0`, no de 0 a 255 como en CSS.

---

## 5. La Funcion `main()` - Inicializacion de GLFW

```cpp
glfwInit();
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

### `glfwInit()`

- **Proposito**: Inicializa la biblioteca GLFW internamente. Carga todo lo necesario para poder crear ventanas y contextos.
- **Parametros**: Ninguno.
- **Resultado**: `GLFW_TRUE` si tuvo exito, `GLFW_FALSE` si fallo. (Aqui no se verifica el retorno, pero deberia hacerse en un programa robusto.)

### `glfwWindowHint(hint, value)`

- **Proposito**: Configura opciones para la **proxima** ventana que se cree. Son como "preferencias".
- **Parametros**:
  - `hint`: Que opcion configurar (una constante de GLFW).
  - `value`: El valor de esa opcion.
- **Resultado**: Nada (void). Modifica estado interno de GLFW.

Las tres llamadas significan:

- `GLFW_CONTEXT_VERSION_MAJOR, 3`: Version mayor de OpenGL = **3**
- `GLFW_CONTEXT_VERSION_MINOR, 3`: Version menor = **3** (total: OpenGL 3.3)
- `GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE`: Usar el perfil **Core** (solo funciones modernas, sin funciones obsoletas como `glBegin`/`glEnd`).

```cpp
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
```

Esto solo se compila en **macOS**. Apple requiere que se active la compatibilidad "forward" para usar OpenGL 3.3+. En Windows/Linux se ignora.

---

## 6. Creacion de la Ventana

```cpp
GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
if (window == NULL)
{
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
}
glfwMakeContextCurrent(window);
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
```

### `glfwCreateWindow(width, height, title, monitor, share)`

- **Proposito**: Crea una ventana con un contexto OpenGL.
- **Parametros**:
  - `width` (int): Ancho en pixeles (800).
  - `height` (int): Alto en pixeles (600).
  - `title` (const char*): Titulo de la ventana ("LearnOpenGL").
  - `monitor` (GLFWmonitor*): Si pones un monitor, sera pantalla completa. `NULL` = ventana normal.
  - `share` (GLFWwindow*): Para compartir recursos OpenGL con otra ventana. `NULL` = no compartir.
- **Resultado**: Un puntero a `GLFWwindow`, o `NULL` si fallo.

### `glfwMakeContextCurrent(window)`

- **Proposito**: Hace que el **contexto OpenGL** de esta ventana sea el "activo" en el hilo actual. Todas las llamadas a OpenGL que hagas despues afectaran a esta ventana.
- **Parametros**: La ventana cuyo contexto quieres activar.
- **Resultado**: Nada (void).

**Que es un contexto OpenGL?** Es como una "sesion de trabajo" con la GPU. Contiene todo el estado: shaders activos, buffers, texturas, configuraciones. Un programa puede tener multiples contextos (uno por ventana), pero solo uno puede estar activo a la vez por hilo.

### `glfwSetFramebufferSizeCallback(window, callback)`

- **Proposito**: Registra una funcion que se llamara **automaticamente** cada vez que el usuario cambie el tamano de la ventana.
- **Parametros**:
  - `window`: La ventana a monitorear.
  - `callback`: Puntero a la funcion que se ejecutara (en este caso `framebuffer_size_callback`).
- **Resultado**: Retorna el callback anterior (o NULL si no habia uno). Generalmente se ignora.

---

## 7. Inicializacion de GLAD

```cpp
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
{
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
}
```

### `gladLoadGLLoader(loader)`

- **Proposito**: Le dice a GLAD que use una funcion especifica para buscar y cargar todas las funciones de OpenGL.
- **Parametros**: Un puntero a una funcion que, dado un nombre (string), devuelve la direccion en memoria de esa funcion OpenGL.
- **Resultado**: Distinto de 0 si tuvo exito, 0 si fallo.

### `glfwGetProcAddress`

- **Proposito**: Funcion de GLFW que sabe como encontrar funciones OpenGL en el sistema operativo actual.
- Se la pasamos a GLAD como herramienta de busqueda.

**Importante**: GLAD debe inicializarse **despues** de `glfwMakeContextCurrent` porque necesita un contexto OpenGL activo para poder buscar las funciones.

Despues de esta llamada, todas las funciones `gl*` (como `glCreateShader`, `glClear`, etc.) ya estan disponibles para usar.

---

## 8. Compilacion de Shaders

### Vertex Shader

```cpp
unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
glCompileShader(vertexShader);
// check for shader compile errors
int success;
char infoLog[512];
glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
if (!success)
{
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
}
```

### `glCreateShader(type)`

- **Proposito**: Crea un **objeto shader** vacio en la GPU.
- **Parametros**:
  - `type`: Que tipo de shader. `GL_VERTEX_SHADER` o `GL_FRAGMENT_SHADER`.
- **Resultado**: Un `unsigned int` (ID numerico) que identifica al shader. OpenGL usa IDs numericos para todo, como "tickets" para referenciar objetos en la GPU.

### `glShaderSource(shader, count, strings, lengths)`

- **Proposito**: Asocia codigo fuente GLSL al objeto shader.
- **Parametros**:
  - `shader` (GLuint): El ID del shader.
  - `count` (GLsizei): Cuantos strings de codigo fuente estamos pasando. Aqui es `1` porque todo el codigo esta en un solo string.
  - `strings` (const GLchar**): Un arreglo de punteros a strings con el codigo. Pasamos `&vertexShaderSource` (la direccion del puntero al string).
  - `lengths` (const GLint*): Longitud de cada string. `NULL` significa que los strings terminan en `\0` (null-terminated), asi que OpenGL calcula la longitud solo.
- **Resultado**: Nada (void).

### `glCompileShader(shader)`

- **Proposito**: Compila el codigo GLSL a codigo maquina que la GPU puede ejecutar.
- **Parametros**: El ID del shader a compilar.
- **Resultado**: Nada (void). Pero el resultado se almacena internamente y se puede consultar.

### `glGetShaderiv(shader, pname, params)`

- **Proposito**: Consulta informacion sobre un shader.
- **Parametros**:
  - `shader`: ID del shader.
  - `pname`: Que informacion quieres. `GL_COMPILE_STATUS` = si compilo bien.
  - `params` (GLint*): Puntero donde se escribe el resultado (`GL_TRUE` o `GL_FALSE`).
- **Resultado**: Escribe el valor en `params`.

### `glGetShaderInfoLog(shader, maxLength, length, infoLog)`

- **Proposito**: Obtiene el mensaje de error de compilacion.
- **Parametros**:
  - `shader`: ID del shader.
  - `maxLength` (GLsizei): Tamano maximo del buffer (512).
  - `length` (GLsizei*): Donde escribir cuantos caracteres se escribieron. `NULL` = no me importa.
  - `infoLog` (GLchar*): Buffer donde se escribe el mensaje de error.
- **Resultado**: Escribe el mensaje en `infoLog`.

### Fragment Shader

El proceso es identico, pero con `GL_FRAGMENT_SHADER` y `fragmentShaderSource`.

---

## 9. Enlace del Programa de Shaders

```cpp
unsigned int shaderProgram = glCreateProgram();
glAttachShader(shaderProgram, vertexShader);
glAttachShader(shaderProgram, fragmentShader);
glLinkProgram(shaderProgram);
// check for linking errors
glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
}
glDeleteShader(vertexShader);
glDeleteShader(fragmentShader);
```

### `glCreateProgram()`

- **Proposito**: Crea un **programa de shaders** vacio. Un programa es la combinacion de un vertex shader + fragment shader (y opcionalmente otros) enlazados juntos.
- **Parametros**: Ninguno.
- **Resultado**: Un ID numerico del programa.

### `glAttachShader(program, shader)`

- **Proposito**: Adjunta un shader compilado al programa.
- **Parametros**: ID del programa y ID del shader.
- **Resultado**: Nada.

### `glLinkProgram(program)`

- **Proposito**: **Enlaza** todos los shaders adjuntos en un programa ejecutable. Esto es como el "linking" en C++: conecta las salidas del vertex shader con las entradas del fragment shader y verifica que todo encaje.
- **Parametros**: ID del programa.
- **Resultado**: Nada (el estado se consulta con `glGetProgramiv`).

### `glGetProgramiv` y `glGetProgramInfoLog`

Funcionan igual que sus equivalentes de shader, pero para programas. Verifican si el enlace fue exitoso.

### `glDeleteShader(shader)`

- **Proposito**: Elimina un objeto shader. Una vez que el shader esta enlazado al programa, ya no necesitamos los objetos shader individuales.
- **Parametros**: ID del shader.
- **Resultado**: Nada. Libera recursos.

---

## 10. Datos de Vertices y Buffers

### Los Datos

```cpp
float vertices[] = {
     0.5f,  0.5f, 0.0f,  // top right
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f,  // bottom left
    -0.5f,  0.5f, 0.0f   // top left
};
unsigned int indices[] = {  // note that we start from 0!
    0, 1, 3,  // first Triangle
    1, 2, 3   // second Triangle
};
```

**`vertices`**: Las 4 esquinas del rectangulo en **coordenadas normalizadas** (NDC - Normalized Device Coordinates). En este sistema:

- `(0, 0)` es el **centro** de la pantalla
- `(-1, -1)` es la esquina inferior izquierda
- `(1, 1)` es la esquina superior derecha
- El eje Z apunta "hacia ti" (hacia afuera de la pantalla)

Cada vertice tiene 3 valores: `x, y, z`. Como es 2D, `z = 0.0f`.

**`indices`**: OpenGL solo sabe dibujar **triangulos**. Un rectangulo se forma con **2 triangulos**. Sin indices, tendriamos que repetir vertices. Los indices dicen:

- Triangulo 1: vertices 0, 1, 3 (top-right, bottom-right, top-left)
- Triangulo 2: vertices 1, 2, 3 (bottom-right, bottom-left, top-left)

Esto ahorra memoria: 4 vertices + 6 indices en lugar de 6 vertices (2 repetidos).

### Creacion de Buffers

```cpp
unsigned int VBO, VAO, EBO;
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);
glGenBuffers(1, &EBO);
```

**Que es un buffer?** Es un bloque de memoria en la **GPU**. Enviar datos de la CPU a la GPU es lento, asi que los enviamos una vez y los dejamos ahi.

- **VBO (Vertex Buffer Object)**: Almacena los datos de vertices (posiciones, colores, normales, etc.) en la GPU.
- **EBO (Element Buffer Object)**: Almacena los indices en la GPU.
- **VAO (Vertex Array Object)**: **NO** almacena datos de vertices. Es un "contenedor de configuracion" que recuerda:
  - Que VBO esta vinculado
  - Que EBO esta vinculado
  - Como interpretar los datos del VBO (formato, stride, offset)

Piensa en el VAO como una "receta" que dice: "para dibujar este objeto, usa este VBO, este EBO, y los datos estan organizados asi".

### `glGenVertexArrays(n, arrays)`

- **Proposito**: Genera `n` IDs de VAOs.
- **Parametros**: Cantidad y puntero donde guardar los IDs.
- **Resultado**: Escribe los IDs en `arrays`.

### `glGenBuffers(n, buffers)`

- **Proposito**: Genera `n` IDs de buffers (VBO o EBO).
- **Parametros**: Cantidad y puntero donde guardar los IDs.
- **Resultado**: Escribe los IDs en `buffers`.

### Configuracion del VAO, VBO y EBO

```cpp
glBindVertexArray(VAO);

glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
```

### `glBindVertexArray(vao)`

- **Proposito**: Activa un VAO. A partir de aqui, todo lo que configures (VBOs, EBOs, atributos) se "grabara" en este VAO.
- **Parametros**: ID del VAO. `0` para desvincular.
- **Resultado**: Nada.

### `glBindBuffer(target, buffer)`

- **Proposito**: Vincula un buffer a un "punto de vinculacion" (target).
- **Parametros**:
  - `target`: `GL_ARRAY_BUFFER` para datos de vertices (VBO), `GL_ELEMENT_ARRAY_BUFFER` para indices (EBO).
  - `buffer`: ID del buffer. `0` para desvincular.
- **Resultado**: Nada. Todas las operaciones posteriores sobre ese target afectaran a este buffer.

### `glBufferData(target, size, data, usage)`

- **Proposito**: **Copia datos** de la CPU a la GPU, al buffer actualmente vinculado en ese target.
- **Parametros**:
  - `target`: A que tipo de buffer enviar (`GL_ARRAY_BUFFER` o `GL_ELEMENT_ARRAY_BUFFER`).
  - `size` (GLsizeiptr): Tamano en bytes de los datos. `sizeof(vertices)` = 4 vertices x 3 floats x 4 bytes = **48 bytes**.
  - `data` (const void*): Puntero a los datos en la CPU.
  - `usage`: Pista para OpenGL sobre como se usaran los datos:
    - `GL_STATIC_DRAW`: Los datos se configuran una vez y se usan muchas veces para dibujar. OpenGL puede optimizar ubicandolos en memoria rapida de la GPU.
    - Otras opciones: `GL_DYNAMIC_DRAW` (cambian frecuentemente), `GL_STREAM_DRAW` (cambian cada frame).
- **Resultado**: Nada. Los datos ahora estan en la GPU.

### `glVertexAttribPointer(index, size, type, normalized, stride, offset)` - MUY IMPORTANTE

- **Proposito**: Le dice a OpenGL **como interpretar** los datos del VBO. Los datos son solo bytes; esta funcion explica su estructura.
- **Parametros**:
  - `index` (GLuint) = `0`: A que "location" del vertex shader corresponde. Recuerda `layout (location = 0) in vec3 aPos;` - aqui le decimos "el atributo en location 0 se llena con estos datos".
  - `size` (GLint) = `3`: Cuantos componentes tiene este atributo. `aPos` es un `vec3`, asi que son **3** (x, y, z).
  - `type` (GLenum) = `GL_FLOAT`: Tipo de dato de cada componente (float de 32 bits).
  - `normalized` (GLboolean) = `GL_FALSE`: Si es `GL_TRUE`, los datos enteros se normalizan al rango [0,1] o [-1,1]. Para floats, no importa.
  - `stride` (GLsizei) = `3 * sizeof(float)` = **12 bytes**: Distancia en bytes entre un vertice y el siguiente. Como solo tenemos posicion (3 floats), son 12 bytes. Si tambien tuvieramos color (3 floats mas), el stride seria 6 * sizeof(float) = 24 bytes.
  - `offset` (const void*) = `(void*)0`: Donde empiezan los datos de este atributo dentro de cada vertice. Como la posicion es el primer (y unico) atributo, empieza en el byte 0.
- **Resultado**: Nada. Configura como leer los datos del VBO actual para el atributo especificado, y esta configuracion se guarda en el VAO activo.

**Visualizacion del layout en memoria**:

```
VBO: [x0 y0 z0 | x1 y1 z1 | x2 y2 z2 | x3 y3 z3]
      <-- 12B -->  <-- 12B -->  <-- 12B -->  <-- 12B -->
      ^ offset=0     (stride = 12 bytes entre vertices)
```

### `glEnableVertexAttribArray(index)`

- **Proposito**: **Activa** el atributo de vertice en la location dada. Por defecto estan desactivados.
- **Parametros**: `index` = `0` (la location del atributo).
- **Resultado**: Nada.

### Desvinculaciones

```cpp
glBindBuffer(GL_ARRAY_BUFFER, 0);

// NO desvincular el EBO mientras el VAO esta activo
//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

glBindVertexArray(0);
```

- **Desvincular el VBO** (`glBindBuffer(GL_ARRAY_BUFFER, 0)`): Es seguro porque `glVertexAttribPointer` ya registro cual VBO usar. El VAO recuerda la referencia.
- **NO desvincular el EBO** mientras el VAO esta activo: A diferencia del VBO, el VAO **si guarda directamente** cual EBO esta vinculado. Si lo desvinculas, el VAO pierde la referencia.
- **Desvincular el VAO** (`glBindVertexArray(0)`): Protege el VAO de modificaciones accidentales.

---

## 11. El Render Loop (Bucle de Renderizado)

```cpp
while (!glfwWindowShouldClose(window))
{
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

Este bucle se repite **cada frame** (tipicamente 60 veces por segundo) hasta que el usuario cierre la ventana.

### `glfwWindowShouldClose(window)`

- **Proposito**: Verifica si la ventana debe cerrarse (el usuario hizo clic en la X, o se llamo a `glfwSetWindowShouldClose`).
- **Resultado**: `true` si debe cerrarse, `false` si no.

### `glClearColor(r, g, b, a)`

- **Proposito**: Define el color con el que se "limpiara" la pantalla. Es como elegir el color del borrador.
- **Parametros**: Rojo=0.2, Verde=0.3, Azul=0.3, Alfa=1.0. Un verde-azulado oscuro (teal).
- **Resultado**: Nada. Solo configura el color. No limpia aun.

### `glClear(mask)`

- **Proposito**: Limpia (borra) uno o mas buffers.
- **Parametros**: `GL_COLOR_BUFFER_BIT` limpia el buffer de color (los pixeles). Otros: `GL_DEPTH_BUFFER_BIT` (profundidad para 3D), `GL_STENCIL_BUFFER_BIT`.
- **Resultado**: Todos los pixeles se rellenan con el color definido por `glClearColor`.

### `glUseProgram(program)`

- **Proposito**: Activa un programa de shaders. Todas las llamadas de dibujo posteriores usaran este programa.
- **Parametros**: ID del programa.
- **Resultado**: Nada.

### `glBindVertexArray(VAO)`

Activa el VAO que configuramos antes. OpenGL ahora sabe que datos usar y como interpretarlos.

### `glDrawElements(mode, count, type, indices)`

- **Proposito**: **Dibuja primitivas** usando vertices indexados (con EBO). Esta es la llamada que realmente genera pixeles.
- **Parametros**:
  - `mode` = `GL_TRIANGLES`: Que tipo de primitiva dibujar. Cada 3 indices forman un triangulo.
  - `count` = `6`: Cuantos indices usar (6 indices = 2 triangulos).
  - `type` = `GL_UNSIGNED_INT`: Tipo de dato de los indices.
  - `indices` = `0`: Offset en el EBO. `0` = empezar desde el principio.
- **Resultado**: Ejecuta el pipeline grafico completo: vertex shader para cada vertice unico, ensamblado, rasterizacion y fragment shader.

**Nota**: La alternativa sin indices seria `glDrawArrays(GL_TRIANGLES, 0, 6)`, pero necesitarias 6 vertices con repeticiones en lugar de 4+indices.

### Double Buffering (Doble Buffer)

### `glfwSwapBuffers(window)`

- **Proposito**: Intercambia el buffer **front** (lo que se muestra) con el buffer **back** (donde se dibujo).
- **Resultado**: La imagen renderizada aparece en pantalla.

**Por que doble buffer?** Si dibujaras directamente en la pantalla, el usuario veria el proceso de dibujo incompleto (parpadeo, artefactos). Con doble buffer:

1. Dibujas en un buffer oculto (back buffer)
2. Cuando terminas, intercambias: el back se convierte en front (visible) y el front en back (para dibujar el siguiente frame)

### `glfwPollEvents()`

- **Proposito**: Procesa todos los eventos pendientes (teclado, mouse, redimensionar ventana, etc.) y llama a los callbacks registrados.
- **Resultado**: Se ejecutan las funciones callback (como `framebuffer_size_callback` y las verificaciones en `processInput`).

---

## 12. Limpieza de Recursos

```cpp
glDeleteVertexArrays(1, &VAO);
glDeleteBuffers(1, &VBO);
glDeleteBuffers(1, &EBO);
glDeleteProgram(shaderProgram);

glfwTerminate();
return 0;
```

### `glDeleteVertexArrays(n, arrays)` / `glDeleteBuffers(n, buffers)`

- **Proposito**: Libera la memoria de la GPU usada por VAOs y buffers.
- **Parametros**: Cantidad y puntero al arreglo de IDs.

### `glDeleteProgram(program)`

- **Proposito**: Elimina el programa de shaders de la GPU.

### `glfwTerminate()`

- **Proposito**: Libera todos los recursos de GLFW (ventanas, contextos, etc.).

---

## 13. Funciones Auxiliares

### `processInput`

```cpp
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
```

### `glfwGetKey(window, key)`

- **Proposito**: Consulta el estado de una tecla.
- **Parametros**: La ventana y la tecla (`GLFW_KEY_ESCAPE`).
- **Resultado**: `GLFW_PRESS` si esta presionada, `GLFW_RELEASE` si no.

### `glfwSetWindowShouldClose(window, value)`

- **Proposito**: Marca la ventana para que se cierre en la siguiente verificacion de `glfwWindowShouldClose`.
- **Parametros**: La ventana y `true`/`false`.

### `framebuffer_size_callback`

```cpp
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
```

### `glViewport(x, y, width, height)`

- **Proposito**: Define la region de la ventana donde OpenGL dibujara. Mapea las coordenadas NDC (-1 a 1) a pixeles reales de la pantalla.
- **Parametros**:
  - `x, y`: Esquina inferior izquierda del viewport (0, 0 = toda la ventana).
  - `width, height`: Tamano del viewport en pixeles.
- **Resultado**: Nada. Actualiza el mapeo interno.

Sin esta funcion, si redimensionas la ventana, el dibujo se deformaria o quedaria en una esquina.

---

## Resumen del Flujo Completo

```
1.  Inicializar GLFW
2.  Crear ventana + contexto OpenGL
3.  Cargar funciones OpenGL con GLAD
4.  Compilar Vertex Shader (GLSL -> codigo GPU)
5.  Compilar Fragment Shader (GLSL -> codigo GPU)
6.  Enlazar ambos shaders en un programa
7.  Definir vertices (4 esquinas) e indices (2 triangulos)
8.  Crear y configurar VAO, VBO, EBO (subir datos a la GPU)
9.  LOOP por cada frame:
    a. Leer input (Escape para salir)
    b. Limpiar pantalla con color teal
    c. Activar shader program
    d. Activar VAO
    e. Dibujar 2 triangulos (= 1 rectangulo naranja)
    f. Intercambiar buffers (mostrar en pantalla)
    g. Procesar eventos
10. Liberar recursos (GPU + GLFW)
```

---

## Referencia Rapida de Todas las Funciones Usadas

| Funcion | Libreria | Proposito |
|---|---|---|
| `glfwInit()` | GLFW | Inicializar GLFW |
| `glfwWindowHint()` | GLFW | Configurar opciones de ventana |
| `glfwCreateWindow()` | GLFW | Crear ventana + contexto |
| `glfwMakeContextCurrent()` | GLFW | Activar contexto OpenGL |
| `glfwSetFramebufferSizeCallback()` | GLFW | Registrar callback de redimension |
| `glfwWindowShouldClose()` | GLFW | Verificar si cerrar ventana |
| `glfwSetWindowShouldClose()` | GLFW | Marcar ventana para cerrar |
| `glfwSwapBuffers()` | GLFW | Mostrar frame renderizado |
| `glfwPollEvents()` | GLFW | Procesar eventos de input |
| `glfwGetKey()` | GLFW | Consultar estado de tecla |
| `glfwTerminate()` | GLFW | Liberar recursos GLFW |
| `gladLoadGLLoader()` | GLAD | Cargar funciones OpenGL |
| `glCreateShader()` | OpenGL | Crear objeto shader |
| `glShaderSource()` | OpenGL | Asignar codigo GLSL |
| `glCompileShader()` | OpenGL | Compilar shader |
| `glGetShaderiv()` | OpenGL | Consultar estado de shader |
| `glGetShaderInfoLog()` | OpenGL | Obtener errores de shader |
| `glCreateProgram()` | OpenGL | Crear programa de shaders |
| `glAttachShader()` | OpenGL | Adjuntar shader a programa |
| `glLinkProgram()` | OpenGL | Enlazar programa |
| `glGetProgramiv()` | OpenGL | Consultar estado de programa |
| `glGetProgramInfoLog()` | OpenGL | Obtener errores de programa |
| `glDeleteShader()` | OpenGL | Eliminar shader |
| `glDeleteProgram()` | OpenGL | Eliminar programa |
| `glUseProgram()` | OpenGL | Activar programa para dibujar |
| `glGenVertexArrays()` | OpenGL | Generar IDs de VAO |
| `glGenBuffers()` | OpenGL | Generar IDs de buffers |
| `glBindVertexArray()` | OpenGL | Activar un VAO |
| `glBindBuffer()` | OpenGL | Vincular buffer a target |
| `glBufferData()` | OpenGL | Copiar datos CPU -> GPU |
| `glVertexAttribPointer()` | OpenGL | Definir layout de datos |
| `glEnableVertexAttribArray()` | OpenGL | Activar atributo de vertice |
| `glDeleteVertexArrays()` | OpenGL | Liberar VAO |
| `glDeleteBuffers()` | OpenGL | Liberar buffers |
| `glClearColor()` | OpenGL | Definir color de limpieza |
| `glClear()` | OpenGL | Limpiar pantalla |
| `glDrawElements()` | OpenGL | Dibujar con indices |
| `glViewport()` | OpenGL | Definir area de dibujo |
