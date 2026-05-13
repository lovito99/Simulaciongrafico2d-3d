# Entrega 1 del PDF: Cohen-Sutherland en C

Este proyecto corresponde a la entrega 1 de la practica de recorte de lineas con el algoritmo de Cohen-Sutherland, implementada en C usando OpenGL y GLUT.

## Correspondencia con el PDF

- Parte 1: definicion de codigos de region de 4 bits en la funcion compute_out_code.
- Parte 2: bucle de recorte, aceptacion, rechazo e intersecciones en la funcion cohen_sutherland_clip.
- Visualizacion del resultado en una ventana OpenGL con lineas de prueba y rectangulo de recorte.

## Salida grafica

- Rectangulo de recorte en amarillo.
- Lineas originales en gris.
- Segmentos aceptados tras el recorte en verde.
- Extremos de lineas rechazadas en rojo.

## Compilacion

make

## Ejecucion

make run

## Requisito del sistema en Ubuntu o Debian

sudo apt install build-essential freeglut3-dev

## Nota

En este entorno no se pudo completar la compilacion porque falta la cabecera GL/glut.h, que se instala con freeglut3-dev.