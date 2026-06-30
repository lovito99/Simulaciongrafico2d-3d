# Entrega 2: Transformaciones geometricas 2D y animacion

Extiende la escena urbana 2D de la Entrega 1 con transformaciones mediante matrices homogeneas 3x3 y animacion continua. Cubre traslacion, rotacion, escala, reflexion, distorsion (shear) y composicion de transformaciones.

**Universidad Nacional de San Antonio Abad del Cusco**
Escuela Profesional: Ing. Informatica y de Sistemas — Computacion Grafica I
Docente: Dr. Hans Harley Ccacyahuillca Bejar

## Estudiantes

- Romario Quispe Rimachi (Cod. 161425)
- Jhamil Jhonatan Huahualuque Vargas (Cod. 225418)
- Efrain Vitorino Marin (Cod. 160337)
- Michael Augusto Baez Monge (Cod. 225418)

## Repositorio de laboratorios de referencia

Las transformaciones implementadas se basan en los siguientes laboratorios del repositorio
[lovito99/laboGrafica](https://github.com/lovito99/laboGrafica):

| Laboratorio | Contenido | Referencia |
|---|---|---|
| [labo6/Translate2D](https://github.com/lovito99/laboGrafica/tree/main/labo6/Translate2D) | Traslacion 2D con matriz homogenea 3x3 | Practica 06 |
| [labo7](https://github.com/lovito99/laboGrafica/tree/main/labo7) | Escalamiento y rotacion 2D, composicion | Practica 07 |
| [lobo8](https://github.com/lovito99/laboGrafica/tree/main/lobo8) | Reflexion eje X, eje Y y linea arbitraria | Practica 08 |
| [labo9](https://github.com/lovito99/laboGrafica/tree/main/labo9) | Shearing (distorsion) horizontal y combinado | Practica 09 |

A diferencia de los laboratorios (que usan GLFW + GLEW + GLM en C++), esta entrega
implementa el algebra matricial de forma manual en C puro, manteniendose sobre la misma
pila tecnologica de la Entrega 1 (OpenGL + GLUT, algoritmos propios).

## Librerias utilizadas

- **OpenGL / GL** — renderizado por puntos con `glBegin(GL_POINTS)` / `glVertex2i`
- **GLUT (freeglut)** — ventana, bucle de eventos, timer animado, teclado
- **GLU** — proyeccion ortogonal 2D con `gluOrtho2D`
- **math.h** — `cosf`, `sinf`, `roundf` para el modulo de matrices

No se usa GLM ni GLEW; las matrices 3x3 se implementan en `transformaciones.c`.

## Organizacion de archivos

```
main.c              — init GLUT, timer (~60 fps), callback de teclado, EstadoAnim global
tipos.h             — estructuras de datos, constantes y EstadoAnim
escena.c / .h       — datos estaticos de la escena (ESCENA_URBANA, VENTANA_RECORTE)
algoritmos.c / .h   — Bresenham, punto medio, scan-line, Cohen-Sutherland (sin cambios)
transformaciones.c / .h  — modulo nuevo: Mat3, primitivas y composicion
render_escena.c / .h     — renderizado animado usando las matrices de transformacion
```

## Transformaciones implementadas

| Transformacion | Objeto en escena | Matriz aplicada |
|---|---|---|
| Traslacion | Vehiculo avanza por la carretera; nube recorre el cielo | `T(tx, 0)` |
| Rotacion | Espolones de ruedas giran al avanzar el vehiculo | `T(cx,cy) * R(θ) * T(-cx,-cy)` |
| Escala | Arbol derecho oscila entre 0.65× y 1.45× | `T(base) * S(s,s) * T(-base)` |
| Reflexion eje-Y | Copia fantasma del edificio izquierdo (x=190) | `T(190,0) * S(-1,1) * T(-190,0)` |
| Reflexion eje-X | Vehiculo reflejado en el piso de la carretera | `Ref_x(88) * T(tx,0)` |
| Distorsion (shear) | Senal de transito se inclina oscilatoriamente | `T(cx,cy) * SH_x(shx) * T(-cx,-cy)` |
| Composicion | Rueda: rotacion en pivote local + traslacion del vehiculo | `M = T(tx,0) * R_pivote(θ, cx, cy)` |

### Fundamento matematico (coordenadas homogeneas)

```
Traslacion:   T(tx,ty) = | 1  0  tx |
                          | 0  1  ty |
                          | 0  0   1 |

Rotacion:     R(θ)     = | cos θ  -sen θ  0 |
                          | sen θ   cos θ  0 |
                          | 0       0      1 |

Escala:       S(sx,sy) = | sx  0   0 |
                          |  0  sy  0 |
                          |  0   0  1 |

Reflexion X:  Ref_x(y0) = T(0,y0) * S(1,-1) * T(0,-y0)
Reflexion Y:  Ref_y(x0) = T(x0,0) * S(-1,1) * T(-x0,0)

Shear:        SH_x(shx) = | 1  shx  0 |
                            | 0   1   0 |
                            | 0   0   1 |

Composicion de rueda:
  M_rueda = T(tx, 0)  *  [T(cx,cy) * R(θ) * T(-cx,-cy)]
```

## Compilacion

```bash
make
```

## Ejecucion

```bash
make run
```

## Controles de teclado

| Tecla | Accion |
|---|---|
| `ESPACIO` | Pausa / reanuda la animacion |
| `E` / `S` | Aumenta / disminuye escala del arbol |
| `D` / `F` | Aumenta / disminuye shear de la senal |
| `R` | Gira las ruedas manualmente |
| `ESC` | Salir |

## Dependencias en Ubuntu/Debian

```bash
sudo apt install build-essential freeglut3-dev
```
