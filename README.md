# Musikverb (VST3) — Propiedad de Keywyll

Reverb VST3 para FL Studio con presets, 4 salas (Cuarto, Estudio, Sala, Catedral),
6 perillas (Tamaño, Decay, Pre-delay, Damping, Tono, Mezcla) y un diseño que cambia
de color según la sala: mientras más grande, más oscuro.

## Cómo compilarlo en GitHub (sin instalar nada)

1. Entra a tu cuenta de GitHub y crea un repositorio nuevo (por ejemplo `Musikverb`).
2. Sube todo el contenido de esta carpeta al repositorio, respetando las carpetas:
   - `CMakeLists.txt`
   - `Source/` (con los 5 archivos .h y .cpp)
   - `.github/workflows/build.yml`
   - `.gitignore`
   Con "Add file > Upload files" puedes arrastrar las carpetas completas.
3. Ve a la pestaña **Actions**. La compilación empieza sola (también puedes darle a
   "Run workflow"). Tarda unos 10 a 15 minutos la primera vez.
4. Cuando termine con la palomita verde, abre esa ejecución y baja hasta **Artifacts**.
   Descarga **Musikverb-VST3**.
5. Descomprime y copia la carpeta `Musikverb.vst3` a:
   `C:\Program Files\Common Files\VST3`
6. En FL Studio: **Options > Manage plugins > Find plugins** (o "Start scan").
   Después busca **Musikverb** en la lista de efectos.

## Si algo falla

Entra a la ejecución que salió en rojo en Actions, abre el paso que falló y copia
el texto del error. Con eso se corrige rápido.

## Ajustes fáciles

- Colores de las salas: `roomColours` en `Source/PluginEditor.cpp`.
- Presets: arreglo `presets` en `Source/PluginProcessor.cpp`.
- Tamaño de la ventana: `setSize (570, 638)` en `Source/PluginEditor.cpp`.
