# Guia corta para presentar Arkanoid 3D Simple

## Que hace el programa

En cada frame lee el teclado, actualiza la bola, comprueba colisiones y dibuja
el tablero. La logica se calcula en CPU y OpenGL usa la GPU para transformar
vertices, rasterizar triangulos y calcular colores.

## Recorrido de un vertice

1. `createCube` define el vertice en marco local.
2. `createMesh` lo copia al VBO y configura el VAO.
3. `drawObject` crea `model`: local a mundo.
4. `view = glm::lookAt(...)`: mundo a camara.
5. `projection = glm::perspective(...)`: camara a clip.
6. `mvp = projection * view * model` concatena las transformaciones.
7. El vertex shader escribe el resultado en `gl_Position`.
8. OpenGL realiza clipping, division entre `w`, NDC, viewport y rasterizacion.
9. El fragment shader aplica Phong y produce el color.

## Bola, luz y rotacion

- `uLightPos` recibe la posicion de la bola en cada frame; por eso la bola es
  una fuente puntual movil.
- `Ball::updateRotation` acumula continuamente los angulos de giro.
- `Ball::registerCollision` modifica el impulso de giro despues de cada choque.
- La matriz `model` aplica rotaciones X, Y y Z a la esfera.
- El fragment shader crea marcas usando `LocalPos`; permiten observar el giro
  aunque la silueta esferica no cambie.

## Movimiento y colisiones 3D

- La bola guarda `position` y `velocity` como `glm::vec3`, por lo que cambia
  en X, Y y Z.
- Las paredes laterales invierten X; el fondo invierte Y; piso y techo
  invierten Z.
- El paddle conserva Y fija y el teclado modifica X/Z.
- `sphereTouchesBox` detecta esfera contra caja para paddle y bloques.
- La funcion devuelve una normal de contacto y `glm::reflect` calcula el rebote
  en cualquiera de los tres ejes.
- Piso, techo y paredes reutilizan el mismo cubo local mediante distintas
  matrices `model`.

## Transparencia y animacion de bloques

- `GL_BLEND` combina el color nuevo con el framebuffer usando el canal alfa.
- El paddle usa alfa `0.38`, por lo que deja ver el cuarto a traves de el.
- Al dibujar paddle y bloques transparentes se usa `glDepthMask(GL_FALSE)`:
  siguen participando en la prueba de profundidad, pero no escriben profundidad.
- Un bloque golpeado activa `disappearing` y acumula `disappearTime`.
- Durante 0.72 segundos su matriz `model` reduce la escala, el color parpadea
  y `uAlpha` disminuye. Al terminar, `alive` cambia a falso.
- Mientras `disappearing` sea verdadero el bloque no acepta nuevas colisiones,
  evitando sumar puntaje varias veces.

## Que decir sobre cada marco

- Local: coordenadas originales del cubo y esfera alrededor del origen.
- Mundo: posicion final de bloques, bola, paddle y paredes.
- View: coordenadas relativas a la camara creada con `glm::lookAt`.
- Clip: salida de `projection * view * model`, escrita en `gl_Position`.
- NDC: resultado automatico de dividir clip entre `w`.
- Ventana: `glViewport` convierte NDC a pixeles.

## Archivos

- `main.cpp`: todo el flujo visible del juego y las transformaciones.
- `shader.h`: carga, compila y enlaza los shaders.
- `vertex_shader.glsl`: transforma posiciones y normales.
- `fragment_shader.glsl`: calcula Phong.

## Preguntas rapidas

**Por que VAO y VBO?** El VBO guarda los vertices en GPU; el VAO recuerda
como leer posicion y normal.

**Por que GLM?** Implementa operaciones matriciales estandar con nombres
legibles y reduce errores al construir transformaciones manualmente.

**Por que depth test?** Decide que fragmento esta delante cuando objetos se
superponen en pantalla.

**Usa EBO?** No. Usa `glDrawArrays`; un EBO permitiria reutilizar vertices.

**Donde esta Phong?** En `fragment_shader.glsl`: ambiental, difusa y especular.

**Por que la bola ilumina?** Su posicion se envia como `uLightPos`; Phong
calcula el vector desde cada fragmento hasta esa fuente puntual.

**Como resuelve colisiones 3D?** Busca el punto de la caja mas cercano a la
esfera. Si la distancia es menor que el radio, obtiene la normal de contacto y
refleja la velocidad con `glm::reflect`.

**Como funciona la transparencia?** El fragment shader entrega un alfa menor
que uno y blending lo combina con el color previamente guardado en framebuffer.

**Que se simplifico?** Se retiraron menu, HUD grafico, estela, pausa y efectos
de desaparicion. Se conservaron jugabilidad, escena 3D y conceptos de clase.
