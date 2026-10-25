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
| Fuentes del juego en el CMake de Android | Generadas desde el CMake de PC, detrás de `GE_BUILD_PORT`; sin compilar todavía |

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

    python3 tools_pc/android/gen_port_cmake.py     # regenera app/src/main/cpp/port.cmake
    gradle assembleDebug -PgeBuildPort=true

`port.cmake` no se escribe a mano: `gen_port_cmake.py` copia del `CMakeLists.txt` raíz las listas de fuentes, definiciones, includes y `configure_file`. Así el build de Android sigue al de PC sin duplicar la lista.

En CI, el job `build-port-apk` hace eso mismo y no bloquea el resto. El workflow `android.yml` se ejecuta en cada push a `main` y en cada PR, y también puede lanzarse a mano desde la pestaña Actions. `ci.yml` es solo manual. Sube `build-port-log` con el log completo del build, y el APK solo si compila.

## Pendientes, en orden

1. Backend GLES 3 en `port/fast3d/gfx_opengl.cpp`.
2. Resolver los errores que muestre el log de `build-port-apk`.
3. Integrar el overlay táctil: cargar los PNG de los controles, dibujarlos con la opacidad guardada, despachar toques con `touchLayoutHit()` y añadir el modo de edición (arrastrar, escalar, opacidad).
4. Verificar en un teléfono que la superficie ocupe toda la pantalla ultrawide.
