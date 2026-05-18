# Entrega 1: Escena urbana 2D en C

Este proyecto implementa la entrega 1 solicitada: una escena urbana 2D estatica construida en C con OpenGL y GLUT, usando algoritmos graficos implementados en el codigo.

## Estudiantes

- Romario Quispe Rimachi (Cód. 1614257)
- Jhamil Jhonatan Huahualuque Vargas (Cód. 225418)
- Efrain Vitorino Marin (Cód. 160337)
- Michael Augusto Báez Monge (Cód. 225418)

## Organizacion de archivos

- main.c: inicializacion de GLUT y ciclo principal.
- tipos.h: estructuras de datos y constantes compartidas.
- escena.c y escena.h: datos de la escena y ventana de recorte.
- algoritmos.c y algoritmos.h: Bresenham, punto medio, scan-line y clipping.
- render_escena.c y render_escena.h: dibujo de los objetos compuestos y armado final de la escena.

## Requisitos cubiertos

- Lineas con Bresenham para carretera, contornos de edificios, vehiculo, rayos del sol, poste y otros trazos.
- Objetos compuestos por lineas: edificio, carretera y vehiculo.
- Circunferencias con el algoritmo del punto medio: ruedas del vehiculo, luces del semaforo y sol.
- Poligonos en la escena: techo de la casa, copa de los arboles, cabina del vehiculo y senal octogonal.
- Relleno scan-line aplicado en varios poligonos y rectangulos.
- Clipping mediante una ventana de visualizacion, aplicado a lineas con Cohen-Sutherland y a pixeles generados en relleno y circunferencias.
- Objetos almacenados en estructuras de datos: edificios, casa, arboles, vehiculo, carretera, semaforo, senal y sol.

## Escena incluida

- Carretera con divisores.
- Tres edificios con ventanas repetidas.
- Casa con techo triangular.
- Dos arboles.
- Un vehiculo con dos ruedas.
- Semaforo de tres luces.
- Senal de transito octogonal.
- Sol parcialmente fuera de la ventana para evidenciar clipping.

## Compilacion

make

## Ejecucion

make run

## Dependencia necesaria en Ubuntu o Debian

sudo apt install build-essential freeglut3-dev

## Nota del entorno actual

En esta maquina no se pudo completar la compilacion porque falta GL/glut.h. El codigo queda listo, pero la biblioteca de desarrollo de GLUT debe instalarse primero.