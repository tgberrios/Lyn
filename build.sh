#!/bin/bash

# Colores para los mensajes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Compilar el compilador Lyn con -rdynamic para exportar símbolos
echo -e "${YELLOW}Compilando el compilador Lyn...${NC}"
CFLAGS="${CFLAGS} -Wno-unused-variable"
gcc $CFLAGS -o lyn src/*.c -rdynamic -I./src

if [ $? -ne 0 ]; then
    echo -e "${RED}Error compilando el compilador${NC}"
    exit 1
fi

# Comprobar si se proporcionó un archivo
if [ $# -ne 1 ]; then
    echo -e "${RED}Uso: $0 <archivo.lyn>${NC}"
    exit 1
fi

# Compilar y ejecutar el programa Lyn
echo -e "${YELLOW}Compilando $1...${NC}"
./lyn "$1"

echo -e "${GREEN}¡Compilación completada!${NC}"
