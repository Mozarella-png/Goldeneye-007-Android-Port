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

## Diagnóstico del log del 2026-10-25 (Poco X8 Pro Max)

Lo que el log confirma:

- Los controles táctiles funcionan a nivel de entrada: `touch: virtual gamepad attached (device 2)` y `opened gamepad 2 'GoldenEye touch controls' as seat 0`.
- El contexto GL es `GL3.0es` y los shaders usan `GLSL 300 es`.
- El audio abre con AAudio a 22050 Hz.

Lo que lo tira:

1. **La ROM no se mapea en `0x10000000`.** El heap de ART del proceso ocupa `02000000-12000000` (`[anon:dalvik-main space]`). Como el mapeo fijo falla, `romdataCartAddrValid()` devuelve 0 y todos los DMA de la ROM se descartan (`osPiStartDma: ROM read out of range`). Sin datos, el juego entra en basura y crashea con SIGNAL 11.
2. El fallback a copia en heap no sirve: los símbolos de assets desreferencian la dirección de cart directamente (lo dice el propio `romdata.c`).
3. El ruido `GL: could not find function: glPolygonMode…` no es un fallo: son funciones de escritorio que glad busca en GLES. Ya no se registran como error en ES.

Cambios para esto:

- `port/include/portaddr.h`: nuevo `PORT_CART_BASE`. PC sigue en `0x10000000`; Android usa `0x40000000`. `romdata.c`, `pcmodels.c`, `gfx_pc.cpp` (ventana de cart) y el log de `main.c` lo usan.
- `port/src/romassets_android_{e,u,j}.s`: copias de los `romassets_*.s` con todas las direcciones de cart desplazadas a `0x40000000`. Las genera `tools_pc/android/rebase_romassets.py`. Para `e` y `j` se comprobó que coinciden con la salida de `gen_romassets.py` (solo difieren comentarios). Para `u` no se puede regenerar en el repo porque falta `baserom.u.z64`, así que se usa la transformación; no quedan literales `0x10xxxxxx` en la variante.
- `tools_pc/android/gen_port_cmake.py` ya apunta `port.cmake` a la variante Android (`port.cmake` regenerado, reproducible).
- `romdata.c`: en POSIX, si el mapeo falla ahora aborta con un mensaje claro, igual que en Windows, en vez de seguir y crashear después.

Cómo comprobar el próximo log:

- Debe aparecer `ROM mapped at 0x40000000`.
- No deben aparecer `osPiStartDma: ROM read out of range`.
- Si `0x40000000` también está ocupado, el log lo dirá con `occupied:`. En ese caso hay que elegir otra base y regenerar la variante.

Riesgo que no se puede verificar desde aquí: que `0x40000000` esté libre en el teléfono y que los sidecars de modelos quepan por debajo de `0x70000000` (DRAM V1).

Para el crash en sí: el log no tiene símbolos. Con `PC=0x77e4439d20` y `MODULE=0x77e42f2000`, el offset dentro del módulo es `0x147D20`. Si el módulo es `libmain.so` sin stripear, `addr2line -e libmain.so 0x147d20` o `ndk-stack` dan la función. Conviene volver a mirarlo después de este arreglo, porque puede desaparecer con la causa.

## Segundo log (2026-10-25, tras el cambio a 0x40000000)

Confirmado:

- `ROM mapped at 0x40000000 (12582912 bytes)`, con los sidecars en `0x40C00000` y `0x40D42560`. El mapeo ya funciona.
- La ROM es completa para los assets: los archivos del CSV llegan a unos 9 MB dentro de los 12 MB.
- Los dos fallos de GL que quedan son los nombres de escritorio esperados en GLES, ya informativos.

Sigue fallando:

- Las lecturas DMA usan fuentes que no están en el mapa del cart, por ejemplo `src=0x239E8250`. Esas fuentes no siguen el cambio de base, así que no son offsets de la ROM. Las direcciones bajas coinciden entre ejecuciones (`…250`, `…F20`, `…A10`) mientras cambian los bits altos. Eso se parece a punteros de memoria del host truncados a 32 bits en algún `hw_address` de la tabla de archivos.
- Por eso el audio falla después (`bad bank revision 0x0000`): el bank se lee de una fuente mala.

Lo que se añade para localizarlo:

- `pcmodelsReportBadTableEntries()` (en `port/src/pcmodels.c`) se ejecuta después de parchear la tabla. Recorre `file_resource_table` y registra cada `hw_address` que no cae dentro del mapa del cart, con el nombre del archivo. Al arrancar sale una línea `table: entry N 'nombre' hw_address=0x… is outside the cart map` por cada entrada mala, o `table: all hw_address entries are inside the cart map`.

Qué buscar en el próximo log:

- Si aparecen entradas malas, el nombre dice qué asset se carga mal y de dónde viene. Ahí está el origen del puntero.
- Si la tabla sale limpia, el puntero malo lo calcula el juego en tiempo de ejecución. En ese caso hay que sacar el caller del DMA, con la pila que ya registra `piServiceDma` para destinos inválidos, y resolverlo con `addr2line` sobre `libmain.so` sin stripear.

## Tercer log (2026-10-25): causa de los punteros malos

El log tiene dos pistas:

- `table: 214 entries point outside the cart map`. Pero la validación corría antes de que `pccg` parchee las entradas `bg/*`. Ahora corre al final de `pccgPatchTable()`, con las dos tablas ya parchadas.
- El crash ya no es SIGSEGV sino SIGBUS (signal 7), con una dirección de 64 bits.

La causa de fondo:

El decomp asume que los punteros caben en 32 bits. Por eso `portAllocLowStack()` fuerza las pilas de los hilos del juego por debajo de 2 GB con `MAP_32BIT`, para que los `(u32)compbuffer` del decomp no se trunquen. Pero `MAP_32BIT` solo existe en x86. En Android la función devolvía NULL y las pilas quedaban en `0x77…`, por encima de 4 GB. Cada truncamiento a 32 bits corrompía la fuente del DMA, que es justo lo que se ve en los logs (`0x239E8250`, `0x5314b700`).

Cambio:

- En Android, `portAllocLowStack()` busca una ventana libre por debajo de 4 GB con `MAP_FIXED_NOREPLACE`, probando desde `0x20000000` en pasos de 16 MB. El kernel rechaza las regiones ya ocupadas, así que no pisa el heap de ART ni la ROM en `0x40000000`.

Lo que puede seguir fallando:

- El heap (`malloc`) también queda por encima de 4 GB en Android. Si siguen apareciendo fuentes de DMA truncadas después de este cambio, el siguiente paso es un asignador propio para la memoria del juego, en una ventana baja, igual que el DRAM del port.

Qué buscar en el próximo log:

- Debe desaparecer el bloque `table: entry N … outside the cart map`, o quedar solo unas pocas entradas, que serían las que el juego fija en tiempo de ejecución.
- Desaparecen los `osPiStartDma: ROM read out of range`, o se reducen.
- Si el crash persiste, la dirección de fallo debería estar por debajo de 4 GB. Si no lo está, el problema es el heap y hay que ir a por el asignador.

## Pendientes, en orden

1. Probar en el Poco. Mirar `table: …` (ya al final del arranque) y si las fuentes de DMA quedan por debajo de 4 GB. Si el heap sigue causando punteros truncados, un asignador de juego en ventana baja (ver arriba).
2. Símbolos del crash con `addr2line` sobre `libmain.so` sin stripear (offset `PC - MODULE`).
3. Ver el log de `build-port-apk` si el build NDK falla (no se pudo compilar el NDK aquí).
4. Etiquetas en los botones (texturas PNG), modo de edición y guardado del layout.
5. Botón de gadget, agacharse y acceso a opciones desde táctil.
6. Verificar que la superficie ocupe toda la pantalla ultrawide.
