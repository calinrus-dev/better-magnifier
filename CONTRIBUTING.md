# Contribuir

El proyecto se distribuye bajo GPL-3.0-or-later. Las correcciones y mejoras se pueden proponer por issue o pull request.

Incluye la compilación exacta de Windows, la versión de Windhawk, el modelo del touchpad y los pasos para reproducir un fallo. Distingue si ocurre al arrancar la Lupa desde cero, con ella abierta, al usar el teclado o al cambiar de modo. Evita adjuntar información personal o volcados completos del sistema.

El mod distribuible debe seguir siendo `mods/kde-pinch-zoom.wh.cpp`, autocontenido. No cambies la configuración de gestos de Windows ni sustituyas deslizamientos nativos por atajos simulados. Los hooks privados requieren confirmar su ABI y acotar las versiones compatibles antes de ampliar soporte.

Ejecuta `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check.ps1`. Este script no instala el mod ni cambia el zoom. La prueba `build/native-integration.exe --live` es optativa y cambia brevemente la ampliación: requiere el mod instalado y restaura su nivel inicial. Documenta por separado pruebas automáticas y gestos físicos.

No declares compatible otra versión de Windows, Windhawk, arquitectura o configuración de monitores solo porque compile. El soporte actual está limitado a lo descrito en el README.
