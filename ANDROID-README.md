# GoldenEye 007 en Android

Este repo contiene el port de PC y el proyecto Android. El proyecto Android está en la raíz (`app/`, `settings.gradle`) y usa los archivos de `port/` cuando activas `GE_BUILD_PORT`.

## Estado

| Parte | Estado |
|---|---|
| Controles táctiles: mapeo puro (`app/src/main/cpp/touch_map.*`) | Probado en host: `tests/test_touch_map.c` (11 comprobaciones) y `tests/test_touch_layout.c`, ambos en CI |
| Controles táctiles: gamepad virtual SDL + overlay GL (`port/src/touchpad.c`) | Escrito y comprobado en sintaxis contra SDL 2.30 y glad. **Sin compilar con NDK ni probado en teléfono** |
| Entrada del juego y ruta de datos (`SDL_main` en `port/src/main.c`) | Parche aplicado; compila en sintaxis |
| Renderer GLES 3 (`port/fast3d/gfx_opengl.cpp`, `gfx_sdl2.cpp`) | El código ya tiene rutas ES (precisión, `#version 300 es`, `in`/`out`, carga de glad con `OpenGL ES`, protecciones de depth clamp, timer queries y debug KHR). Contexto ES 3.0 se pide primero en Android. **Sin probar en GPU real** |
| Conversión del ROM | Herramienta HTML en `tools_pc/android/convertidor-rom.html` |
| Compilación nativa del port | Pendiente de ver el log de CI (`build-port-apk`); no se pudo compilar el NDK en el entorno de trabajo |

### Qué hace el control táctil

- Izquierda: stick flotante (se ancla donde pones el dedo) o el stick fijo del layout. Dibujo: anillo y knob.
- Derecha, zona libre: mirar (stick derecho, anclado al primer toque).
- Botones del layout: Fuego (gatillo derecho), Apuntar/zoom (gatillo izquierdo), Recargar (X), Usar (A), Arma anterior (D-pad izquierda), Arma siguiente (Y), Pausa (Start).
- Los toques ya no generan clics de ratón (`SDL_TOUCH_MOUSE_EVENTS=0`), para que un toque no active la captura del cursor.
- El gamepad virtual se abre igual que un mando físico, así que los presets de Input y el mapeo Jinx 1.1 funcionan sin cambios.

### Limitaciones conocidas de esta versión

- Los botones no tienen etiqueta: son círculos de color.
- No hay modo de edición (arrastrar y escalar). El layout es el de `touch_layout.c`.
- El menú de opciones (F10) no es accesible con táctil; se usa la pantalla de pausa de Start.
- Sin botón de gadget ni de agacharse.
- Si hay un mando físico conectado, el gamepad virtual puede quedar en un asiento distinto al 1.

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

1. Ver el log de `build-port-apk` (artefacto `build-port-log`) y resolver los errores de compilación del NDK. Es el primer bloqueo real.
2. Probar en el Poco X8 Pro Max (GPU Mali): que arranque, que el overlay se vea y que el juego responda al tacto. Mirar `ge007.log` en la carpeta de datos.
3. Etiquetas en los botones (texturas PNG), modo de edición y guardado del layout.
4. Botón de gadget, agacharse y acceso a opciones desde táctil.
5. Verificar que la superficie ocupe toda la pantalla ultrawide.
