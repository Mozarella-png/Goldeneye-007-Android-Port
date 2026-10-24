# Goldeneye 007 — Android (en progreso)

Estado honesto de este directorio. Colocar en la raíz del repo como `android/`.

## Verificado
- `touch/` (layout de controles táctiles): lógica pura en C, con tests que pasan
  con gcc `-Wall -Wextra -pedantic -Werror`. CI lo corre en `touch-tests`.
- Iconos (`app/src/main/res/mipmap-*`), manifest y `build.gradle` bien formados.
- Workflow `.github/workflows/android.yml` validado como YAML.

## Pendiente (bloqueantes, en orden)
1. **Renderer GLES 3:** `port/fast3d/gfx_opengl.cpp` usa OpenGL de escritorio con glad.
   Hay que generar el loader para GLES 3 y revisar las llamadas no soportadas.
2. **Lista de fuentes del juego:** `SRC_GAME`, `SRC_ENGINE` y `SRC_LIBULTRA*` están en
   el CMakeLists raíz. Hay que exportarlas a un include compartido para que PC y Android
   usen la misma lista. Ahora `app/src/main/cpp/CMakeLists.txt` solo compila la capa `port/`.
3. **`main()` duplicado:** `port/src/main.c` define `main`. En Android hay que renombrarlo
   y llamarlo desde `android_main.cpp` vía `SDL_main`.
4. **Conversión del ROM en el dispositivo:** `romconvert.c` ejecuta un binario externo.
   Hay que compilar el convertidor dentro de `libmain.so` y llamarlo como función.
5. **Rutas de datos:** `data/`, `$S/`, `$E/` deben apuntar a `SDL_AndroidGetExternalStoragePath()`.
6. **Overlay táctil:** cargar los PNG de `touch/` como texturas, dibujar con la opacidad
   guardada, despachar toques con `touchLayoutHit()` y modo de edición (arrastrar,
   escalar, opacidad) que guarda con `touchLayoutSerialize()`.
7. **Ultrawide:** verificar en el dispositivo que la superficie use el tamaño completo
   y que el aspect ratio de la ventana sea el usado por el widescreen nativo.

## Build local (cuando esté el NDK)
    cd android && gradle assembleDebug
