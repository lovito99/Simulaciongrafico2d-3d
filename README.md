# Entrega 2: Transformaciones geometricas 2D y animacion

Extiende la escena urbana 2D de la Entrega 1 incorporando matrices homogeneas 3x3 para
traslacion, rotacion, escala, reflexion, distorsion (shear) y composicion de transformaciones,
con animacion continua a ~60 fps mediante el timer de GLUT.

**Universidad Nacional de San Antonio Abad del Cusco**  
Escuela Profesional: Ing. Informatica y de Sistemas — Computacion Grafica I  
Docente: Dr. BORIS CHULLO LLAVE

## Estudiantes

| Nombre | Codigo |
|---|---|
| Romario Quispe Rimachi | 161425 |
| Jhamil Jhonatan Huahualuque Vargas | 225418 |
| Efrain Vitorino Marin | 160337 |


---

## Repositorio de laboratorios de referencia

Las transformaciones se basan en los laboratorios de
[lovito99/laboGrafica](https://github.com/lovito99/laboGrafica).
A continuacion se detalla que se tomo de cada uno y como se adapto.

---

### Practica 06 — Traslacion 2D
**Fuente:** [`labo6/Translate2D/src/main.cpp`](https://github.com/lovito99/laboGrafica/blob/main/labo6/Translate2D/src/main.cpp)

**Librerias del laboratorio:**
- `GL/glew.h` — carga de extensiones OpenGL en tiempo de ejecucion
- `GLFW/glfw3.h` — creacion de ventana y bucle de eventos (reemplaza a GLUT)
- OpenGL 3.3 Core Profile con VAO / VBO / Vertex Shader GLSL

**Funcion clave tomada del laboratorio:**

```cpp
// labo6 — construye la matriz de traslacion 3x3 en column-major para GPU
void matrizTraslacion(float tx, float ty, float M[9])
{
    M[0]=1; M[3]=0; M[6]=tx;
    M[1]=0; M[4]=1; M[7]=ty;
    M[2]=0; M[5]=0; M[8]=1;
}
```

La matriz se enviaba al vertex shader como `uniform mat3 uTranslation` y se aplicaba en GPU:
```glsl
vec3 p = uTranslation * vec3(aPos, 1.0);
```

**Adaptacion en Entrega 2 (`transformaciones.c`):**  
Se traslada el mismo concepto matematico a C puro con layout row-major, y la multiplicacion
se realiza en CPU antes de pasar las coordenadas a Bresenham/scan-line:

```c
Mat3 mat3_traslacion(float tx, float ty) {
    Mat3 r = {{1,0,tx,  0,1,ty,  0,0,1}};
    return r;
}
// Aplicacion CPU-side:
Punto mat3_punto(Mat3 m, Punto p) {
    float x = m.m[0]*p.x + m.m[1]*p.y + m.m[2];
    float y = m.m[3]*p.x + m.m[4]*p.y + m.m[5];
    return (Punto){(int)roundf(x), (int)roundf(y)};
}
```

**Uso en la escena:** vehiculo avanza en X (`M = T(vehiculo_tx, 0)`),
nube recorre el cielo (`M = T(nube_tx, 575)`).

---

### Practica 07 — Escalamiento y Rotacion 2D
**Fuente:** [`labo7/src/escalamiento.cpp`](https://github.com/lovito99/laboGrafica/blob/main/labo7/src/escalamiento.cpp)
y [`labo7/src/rotacion.cpp`](https://github.com/lovito99/laboGrafica/blob/main/labo7/src/rotacion.cpp)
y [`labo7/src/actividadRectangulo.cpp`](https://github.com/lovito99/laboGrafica/blob/main/labo7/src/actividadRectangulo.cpp)

**Librerias del laboratorio:**
- `GL/glew.h` — extensiones OpenGL
- `GLFW/glfw3.h` — ventana y eventos
- `glm/glm.hpp` y `glm/gtc/type_ptr.hpp` — algebra lineal (tipos `mat3`, `vec3`)
- OpenGL 3.3 Core Profile con shaders y `uniform mat3 uTransform`

**Funciones clave tomadas del laboratorio:**

```cpp
// labo7/escalamiento.cpp — escala con pivote (column-major)
void matrizEscalamientoAlrededorDe(float sx, float sy,
                                    float cx, float cy, float M[9]) {
    float tx = cx - (sx * cx);
    float ty = cy - (sy * cy);
    M[0]=sx;   M[3]=0;    M[6]=tx;
    M[1]=0;    M[4]=sy;   M[7]=ty;
    M[2]=0;    M[5]=0;    M[8]=1;
}

// labo7/rotacion.cpp — rotacion con pivote (column-major)
void matrizRotacionAlrededorDe(float grad, float cx, float cy, float M[9]) {
    float cosA = cos(grad * M_PI/180), sinA = sin(grad * M_PI/180);
    float tx = cx - (cosA*cx - sinA*cy);
    float ty = cy - (sinA*cx + cosA*cy);
    M[0]=cosA; M[3]=-sinA; M[6]=tx;
    M[1]=sinA; M[4]= cosA; M[7]=ty;
    M[2]=0;    M[5]=0;     M[8]=1;
}

// labo7/actividadRectangulo.cpp — composicion T(traslacion) luego R(rotacion)
void multiplicarMatrices(const float A[9], const float B[9], float C[9]);
// ... y el uso: TR = R * T  (aplica T primero, luego R)
multiplicarMatrices(R, T, TR);
```

**Adaptacion en Entrega 2 (`transformaciones.c`):**  
Se reformulan con el patron `T(pivot) * Op * T(-pivot)` explicitamente,
usando `mat3_mul` row-major:

```c
// Rotacion alrededor de (cx,cy)
Mat3 mat3_rotacion_pivote(float rad, float cx, float cy) {
    return mat3_mul(mat3_traslacion(cx, cy),
           mat3_mul(mat3_rotacion(rad),
                    mat3_traslacion(-cx, -cy)));
}

// Escala alrededor de (cx,cy)
Mat3 mat3_escala_pivote(float sx, float sy, float cx, float cy) {
    return mat3_mul(mat3_traslacion(cx, cy),
           mat3_mul(mat3_escala(sx, sy),
                    mat3_traslacion(-cx, -cy)));
}

// Composicion para ruedas (Actividad 2 del labo7 trasladada a la escena)
// M_rueda = T(vehiculo_tx, 0) * R_pivote(ang, centro_rueda)
Mat3 M_v = mat3_traslacion(estado->vehiculo_tx, 0.0f);
// angulo se computa en dibujar_vehiculo_t con cosf/sinf
```

**Uso en la escena:**
- Escala: arbol derecho con `mat3_escala_pivote(s, s, base_cx, base_y)`
- Rotacion: espolones de ruedas con `cosf/sinf(angulo_rueda)` sobre el centro transformado
- Composicion: `M_rueda = T(tx) * R_pivote` (demostrado con texto en pantalla)

---

### Practica 08 — Reflexion 2D
**Fuente:** [`lobo8/src/reflexion.cpp`](https://github.com/lovito99/laboGrafica/blob/main/lobo8/src/reflexion.cpp)

**Librerias del laboratorio:**
- `GL/glew.h` — extensiones OpenGL
- `GLFW/glfw3.h` — ventana y eventos
- OpenGL 3.3 Core Profile, matrices 4x4 enviadas como `uniform mat4 transform`

**Funciones clave tomadas del laboratorio:**

```cpp
// lobo8 — reflexion eje X: escala Sy = -1
createScaleMatrix(finalReflectXMatrix, 1.0f, -1.0f);

// lobo8 — reflexion eje Y: escala Sx = -1
createScaleMatrix(finalReflectYMatrix, -1.0f, 1.0f);

// lobo8 — escalamiento con punto fijo (patron T * S * T^-1)
createTranslationMatrix(matTranslateToOrigin, -fixedX, -fixedY);
createScaleMatrix(matScale, 1.5f, 1.5f);
createTranslationMatrix(matTranslateBack, fixedX, fixedY);
multiplyMatrices(matScale, matTranslateToOrigin, tempResult);
multiplyMatrices(matTranslateBack, tempResult, finalScaleMatrix);
```

**Adaptacion en Entrega 2 (`transformaciones.c`):**  
Las reflexiones se generalizan para actuar sobre cualquier linea horizontal o vertical,
no solo los ejes del origen:

```c
// Reflexion respecto a la linea horizontal y = y0
Mat3 mat3_reflexion_eje_x(float y0) {
    return mat3_mul(mat3_traslacion(0, y0),
           mat3_mul(mat3_escala(1, -1),
                    mat3_traslacion(0, -y0)));
}

// Reflexion respecto a la linea vertical x = x0
Mat3 mat3_reflexion_eje_y(float x0) {
    return mat3_mul(mat3_traslacion(x0, 0),
           mat3_mul(mat3_escala(-1, 1),
                    mat3_traslacion(-x0, 0)));
}
```

**Uso en la escena:**
- Reflexion eje-Y (`x0 = 190`): edificio izquierdo con copia fantasma a su izquierda
- Reflexion eje-X (`y0 = 88`): vehiculo reflejado en el piso de la carretera humeda

---

### Practica 09 — Distorsion (Shearing) 2D
**Fuente:** [`labo9/shearing.cpp`](https://github.com/lovito99/laboGrafica/blob/main/labo9/shearing.cpp)

**Librerias del laboratorio:**
- `GL/glew.h` — extensiones OpenGL
- `GLFW/glfw3.h` — ventana y eventos
- `glcommon.hpp` — helpers propios (`createIdentityMatrix`, `compileShaderProgram`)
- OpenGL 3.3 Core Profile, matriz 4x4 enviada como `uniform mat4 uTransform`

**Funcion clave tomada del laboratorio:**

```cpp
// labo9 — crea la matriz de shear 4x4 (column-major)
void createShearingMatrix(float* mat, float shx, float shy)
{
    createIdentityMatrix(mat);
    mat[4] = shx;   // SH_x en la posicion [col=1, row=0]
    mat[1] = shy;   // SH_y en la posicion [col=0, row=1]
}
// Formula: x' = x + shx*y    y' = shy*x + y
```

**Adaptacion en Entrega 2 (`transformaciones.c`):**  
Se porta a matriz 3x3 row-major con la ecuacion equivalente,
y se agrega version con pivote para aplicarla respecto al centro del objeto:

```c
// Shear horizontal 3x3 row-major: x' = x + shx*y
Mat3 mat3_shear_x(float shx) {
    Mat3 r = {{1,shx,0,  0,1,0,  0,0,1}};
    return r;
}

// Shear respecto al centro (cx, cy) del objeto
Mat3 mat3_shear_x_pivote(float shx, float cx, float cy) {
    return mat3_mul(mat3_traslacion(cx, cy),
           mat3_mul(mat3_shear_x(shx),
                    mat3_traslacion(-cx, -cy)));
}
```

**Uso en la escena:** senal de transito octogonal se inclina oscilatoriamente
(shx oscila entre -0.40 y +0.40) con el centro del octogono como pivote.
Se muestra el contorno original tenue como referencia de comparacion.

---

## Diferencia de pila tecnologica: laboratorios vs Entrega 2

| Aspecto | Laboratorios (labo6-9) | Entrega 2 |
|---|---|---|
| Lenguaje | C++ 17 | C 11 |
| Ventana / bucle | GLFW 3 | GLFW 3 (`glfwCreateWindow`, `glfwPollEvents`) |
| Carga de extensiones | GLEW | GLEW (`glewInit()` tras `glutCreateWindow`) |
| Algebra lineal | GLM (`glm::mat3`, `glm::value_ptr`) | `Mat3` implementado en `transformaciones.c` |
| Aplicacion de la matriz | GPU — vertex shader `uniform mat3 uTransform` | CPU — `mat3_punto()` antes de Bresenham/scan-line |
| Pipeline OpenGL | 3.3 Core Profile (VAO/VBO/shaders) | Legacy (`glBegin/glEnd`, `GL_POINTS`) |
| Algoritmos de dibujo | primitivas de GPU | Bresenham, punto medio, scan-line, Cohen-Sutherland (propios) |
| Sistema de construccion | CMake | Makefile |

La razon de mantener GLUT y el pipeline legacy es preservar los algoritmos propios de la
Entrega 1 (Bresenham, scan-line, clipping), que requieren coordenadas enteras de pixel.
Las matrices se aplican en CPU, se redondean con `roundf`, y los pixeles resultantes pasan
por los mismos algoritmos de la Entrega 1.

---

## Librerias usadas en Entrega 2

| Libreria | Header | Para que se usa |
|---|---|---|
| GLEW | `GL/glew.h` | Carga de extensiones OpenGL; debe incluirse antes que GLFW |
| GLFW 3 | `GLFW/glfw3.h` | Ventana, doble buffer, bucle de eventos, teclado |
| OpenGL | `GL/gl.h` (via glew) | `glBegin/glEnd`, `glVertex2i`, `glColor3ub` |
| math.h | `<math.h>` | `cosf`, `sinf`, `roundf`, `fabsf` para matrices |

Instalacion en Ubuntu/Debian:
```bash
sudo apt install build-essential libglfw3-dev libglew-dev
```

---

## Organizacion de archivos

```
transformaciones.h / .c  — tipo Mat3 y todas las operaciones matriciales
tipos.h                  — estructuras de datos + EstadoAnim + constantes OBJ_*
main.c                   — init GLFW + GLEW, bucle 60 fps, callback de teclado
escena.c / .h            — datos estaticos de la escena + BUS_AZUL
algoritmos.c / .h        — Bresenham, punto medio, scan-line, clipping
render_escena.c / .h     — renderizado animado con transformaciones y panel HUD
```

---

## Transformaciones implementadas

| Transformacion | Objeto en escena | Modo | Matriz |
|---|---|---|---|
| Traslacion | Vehiculo rojo avanza izq→der | Automatico | `T(tx, 0)` |
| Traslacion | Bus azul avanza der→izq (carril contrario) | Automatico | `T(bus_tx, 0)` |
| Traslacion | Nubes recorren el cielo | Automatico | `T(nube_tx, y)` |
| Traslacion | Edificio naranja / azul | Manual (flechas) | `T(tx, ty)` |
| Rotacion | Espolones de ruedas (vehiculo + bus) | Automatico | `T(cx,cy)*R(θ)*T(-cx,-cy)` |
| Rotacion | Edificio naranja / azul | Manual (`Q` `W`) | integrada en composicion |
| Escala | Edificio naranja / azul | Manual (`E` `S`) | integrada en composicion |
| Reflexion eje-X | Vehiculo rojo reflejado en el piso (y=88) | Automatico | `Ref_x(88)*T(tx,0)` |
| Reflexion eje-Y | Edificio naranja / azul | Manual (`R`) | `S(-1,1)` en composicion |
| Distorsion (shear) | Senal octogonal oscila | Automatico | `T(cx,cy)*SH_x(shx)*T(-cx,-cy)` |
| Distorsion (shear) | Edificio naranja / azul | Manual (`D` `F`) | integrada en composicion |
| Composicion | Edificios: todas las ops encadenadas | Manual | ver formula abajo |
| Composicion | Rueda = traslacion vehiculo + rotacion pivote | Automatico | `T(tx,0)*R_pivote(θ,cx,cy)` |

### Fundamento matematico

```
Traslacion:    T(tx,ty)  = | 1  0  tx |
                            | 0  1  ty |
                            | 0  0   1 |

Rotacion:      R(θ)      = | cos θ  -sen θ  0 |
                            | sen θ   cos θ  0 |
                            | 0       0      1 |

Escala:        S(sx,sy)  = | sx  0   0 |
                            |  0  sy  0 |
                            |  0   0  1 |

Shear X:       SH_x(shx) = | 1  shx  0 |   =>  x' = x + shx*y
                             | 0   1   0 |       y' = y
                             | 0   0   1 |

Reflexion X:   Ref_x(y0) = T(0,y0)  * S(1,-1)  * T(0,-y0)
Reflexion Y:   Ref_y()   = S(-1, 1) aplicada dentro de la composicion

Composicion edificios (M aplicada a cada vertice):
  M = T(tx,ty) * T(cx,cy) * R(ang) * S(esc,esc) * SH_x(sh) * [S(-1,1)?] * T(-cx,-cy)
       traslado    pivote    rota     escala        shear       reflexion    pivote

Composicion ruedas:
  M_rueda = T(tx, 0)  *  [ T(cx,cy) * R(θ) * T(-cx,-cy) ]
              traslacion      rotacion alrededor del pivote
```

---

## Compilacion y ejecucion

```bash
make        # compila  →  entrega2_transformaciones
make run    # compila y ejecuta
make clean  # elimina el binario
```

---

## Controles interactivos

### Seleccion de objeto activo

| Tecla | Objeto seleccionado |
|---|---|
| `1` | Vehiculo Rojo |
| `2` | Bus Azul |
| `3` | Edificio Naranja |
| `4` | Edificio Azul |
| `TAB` | Ciclar entre los 4 objetos |

---

### Vehiculo Rojo  (`tecla 1`)

| Tecla | Accion |
|---|---|
| `←` `→` | Mover manualmente en X |
| `ESPACIO` | Pausar / reanudar animacion automatica |
| `0` | Reiniciar posicion |
| `ESC` | Salir |

---

### Bus Azul  (`tecla 2`)

| Tecla | Accion |
|---|---|
| `←` `→` | Mover manualmente en X |
| `ESPACIO` | Pausar / reanudar animacion automatica |
| `0` | Reiniciar posicion (vuelve a entrar por la derecha) |

---

### Edificio Naranja  (`tecla 3`)  y  Edificio Azul  (`tecla 4`)

Ambos edificios comparten los mismos controles de transformacion:

| Tecla | Transformacion | Detalle |
|---|---|---|
| `←` `→` | **Traslacion X** | Desplaza el edificio horizontalmente |
| `↑` `↓` | **Traslacion Y** | Desplaza el edificio verticalmente |
| `E` | **Escala +** | Agranda respecto al centro del edificio |
| `S` | **Escala −** | Achica respecto al centro del edificio |
| `Q` | **Rotacion +** | Gira en sentido antihorario alrededor del centro |
| `W` | **Rotacion −** | Gira en sentido horario alrededor del centro |
| `D` | **Shear +** | Distorsiona en cizalla positiva (inclina hacia la derecha) |
| `F` | **Shear −** | Distorsiona en cizalla negativa (inclina hacia la izquierda) |
| `R` | **Reflexion** | Alterna reflexion eje-Y respecto al centro del edificio |
| `0` | **Reset** | Restablece todas las transformaciones al estado inicial |

> Cuando dos o mas transformaciones estan activas al mismo tiempo la etiqueta
> en pantalla muestra **[COMPOSICION]**, indicando que la matriz resultante es el
> producto de todas las matrices individuales encadenadas.

---

### Controles globales

| Tecla | Accion |
|---|---|
| `ESPACIO` | Pausa / reanuda toda la animacion automatica |
| `ESC` | Salir del programa |
