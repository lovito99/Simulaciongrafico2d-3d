# Entrega 2: Transformaciones geometricas 2D y animacion

Extiende la escena urbana 2D de la Entrega 1 incorporando matrices homogeneas 3x3 para
traslacion, rotacion, escala, reflexion, distorsion (shear) y composicion de transformaciones,
con animacion continua a ~60 fps mediante el timer de GLUT.

**Universidad Nacional de San Antonio Abad del Cusco**  
Escuela Profesional: Ing. Informatica y de Sistemas — Computacion Grafica I  
Docente: Dr. Hans Harley Ccacyahuillca Bejar

## Estudiantes

| Nombre | Codigo |
|---|---|
| Romario Quispe Rimachi | 161425 |
| Jhamil Jhonatan Huahualuque Vargas | 225418 |
| Efrain Vitorino Marin | 160337 |
| Michael Augusto Baez Monge | 225418 |

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
| Ventana / bucle | GLFW 3 | GLUT (freeglut) |
| Carga de extensiones | GLEW | no necesario (OpenGL legacy) |
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
| OpenGL | `GL/gl.h` (via glut) | `glBegin/glEnd`, `glVertex2i`, `glColor3ub` |
| GLU | `GL/glu.h` (via glut) | `gluOrtho2D` para proyeccion 2D |
| GLUT (freeglut) | `GL/glut.h` | Ventana, doble buffer, timer animado, teclado |
| math.h | `<math.h>` | `cosf`, `sinf`, `roundf`, `fabsf` para matrices |

Instalacion en Ubuntu/Debian:
```bash
sudo apt install build-essential freeglut3-dev
```

---

## Organizacion de archivos

```
transformaciones.h / .c  — NUEVO: tipo Mat3 y todas las operaciones matriciales
tipos.h                  — estructuras de datos + EstadoAnim (nuevo)
main.c                   — init GLUT, timer 60 fps, teclado, EstadoAnim global
escena.c / .h            — datos estaticos de la escena (sin cambios respecto E1)
algoritmos.c / .h        — Bresenham, punto medio, scan-line, clipping (sin cambios)
render_escena.c / .h     — renderizado animado con transformaciones aplicadas
```

---

## Transformaciones implementadas

| Transformacion | Objeto en escena | Matriz | Archivo |
|---|---|---|---|
| Traslacion | Vehiculo avanza en X; nube recorre el cielo | `T(tx, 0)` | `render_escena.c:dibujar_vehiculo_t` |
| Rotacion | Espolones de ruedas giran al avanzar | `T(cx,cy)*R(θ)*T(-cx,-cy)` | `render_escena.c:dibujar_vehiculo_t` |
| Escala | Arbol derecho oscila entre 0.65x y 1.45x | `T(base)*S(s,s)*T(-base)` | `render_escena.c:dibujar_arbol_escalado` |
| Reflexion eje-Y | Edificio izquierdo: copia fantasma a su izquierda (x=190) | `T(190,0)*S(-1,1)*T(-190,0)` | `render_escena.c:dibujar_edificio_reflexion` |
| Reflexion eje-X | Vehiculo reflejado en el piso de la carretera (y=88) | `Ref_x(88)*T(tx,0)` | `render_escena.c:dibujar_vehiculo_reflexion` |
| Distorsion (shear) | Senal octogonal se deforma oscilatoriamente | `T(cx,cy)*SH_x(shx)*T(-cx,-cy)` | `render_escena.c:dibujar_senal_t` |
| Composicion | Rueda = traslacion del vehiculo compuesta con rotacion pivote | `M = T(tx,0) * R_pivote(θ,cx,cy)` | `render_escena.c:dibujar_escena_animada` |

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

Reflexion X:   Ref_x(y0) = T(0,y0) * S(1,-1) * T(0,-y0)
Reflexion Y:   Ref_y(x0) = T(x0,0) * S(-1,1) * T(-x0,0)

Composicion rueda:
  M_rueda = T(tx, 0)  *  [ T(cx,cy) * R(θ) * T(-cx,-cy) ]
              traslacion      rotacion alrededor del pivote
```

---

## Compilacion y ejecucion

```bash
make        # compila entrega2_transformaciones
make run    # compila y ejecuta
make clean  # elimina el binario
```

## Controles interactivos

| Tecla | Accion |
|---|---|
| `ESPACIO` | Pausa / reanuda la animacion |
| `E` / `S` | Sube / baja escala del arbol derecho |
| `D` / `F` | Aumenta / reduce shear de la senal |
| `R` | Gira las ruedas manualmente un paso |
| `ESC` | Salir |
