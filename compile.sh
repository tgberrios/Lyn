#!/bin/bash# Eliminar cualquier archivo del directorio include que cause conflictos
rm -rf include/*.h

# Compilar usando solo los archivos de src
gcc -o lyn src/*.c -I./src
