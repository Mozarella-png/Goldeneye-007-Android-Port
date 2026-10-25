# GoldenEye 007 en Android

Este repo contiene el port de PC y el proyecto Android. El proyecto Android está en la raíz (`app/`, `settings.gradle`) y usa los archivos de `port/` cuando activas `GE_BUILD_PORT`.

## Estado

| Parte | Estado |
|---|---|
| Módulo de controles táctiles (`app/src/main/cpp/touch_layout.*`) | Probado: tests en `tests/` que corre CI en Linux |
| Proyecto Gradle, manifest, icono "Goldeneye 007" | Compila en CI (`build-apk`) |
| Entrada del juego y ruta de datos (`port/src/main.c`, `SDL_main`) | Parche aplicado, sin compilar todavía |
| Conversión del ROM | Herramienta HTML en `tools_pc/android/convertidor-rom.html` |
| Renderer GLES 3 | Pendiente: el backend usa OpenGL de escritorio |
| Fuentes del juego en el CMake de Android | Pendiente: ver `GE_BUILD_PORT` abajo |

## Carpeta de datos en el teléfono

    /sdcard/Android/data/com.goldeneye007.port/files/data/
    ├── ge007.ntsc-final.z64
    ├── pcmodels-ntsc-final/
    └── pccg-ntsc-final/

Los datos `pcmodels-*` y `pccg-*` los genera `tools_pc/android/convertidor-rom.html` a partir de tu ROM, en tu navegador. La ROM no se sube a ningún lado.

## Compilar

El APK de debug se construye en GitHub Actions (`.github/workflows/android.yml`). Para compilar en local hace falta Android Studio o el SDK con NDK `26.3.11579264` y CMake 3.22.1:

    gradle assembleDebug

Los tests del módulo táctil corren sin Android:

    gcc -std=c99 -Wall -Wextra -Werror -Iapp/src/main/cpp -o t tests/test_touch_layout.c app/src/main/cpp/touch_layout.c && ./t

## GE_BUILD_PORT

Por defecto, el CMake de Android solo compila el módulo táctil y SDL. Para incluir el port:

    gradle assembleDebug -PgeBuildPort=true

Eso requiere exportar la lista de fuentes del juego (`SRC_GAME`, `SRC_ENGINE`, `SRC_LIBULTRA*` del CMakeLists.txt raíz de `port/`) a un include compartido, y el backend GLES. Hasta entonces, mantén la opción apagada: el build de CI no depende de ella.

## Pendientes, en orden

1. Backend GLES 3 en `port/fast3d/gfx_opengl.cpp`.
2. Lista de fuentes compartida entre el build de PC y el de Android.
3. Integrar el overlay táctil: cargar los PNG de los controles, dibujarlos con la opacidad guardada, despachar toques con `touchLayoutHit()` y añadir el modo de edición (arrastrar, escalar, opacidad).
4. Verificar en un teléfono que la superficie ocupe toda la pantalla ultrawide.
