# Contribuir

El proyecto se distribuye bajo GPL-3.0-or-later. Las correcciones y mejoras se pueden proponer por issue o pull request.

Incluye la compilación exacta de Windows, la versión de Windhawk, el modelo del touchpad y los pasos para reproducir un fallo. Distingue si ocurre al arrancar la Lupa desde cero, con ella abierta, al usar el teclado o al cambiar de modo. Evita adjuntar información personal o volcados completos del sistema.

Para problemas con rueda, indica el modelo del ratón, si usas muescas o giro libre, el perfil de atajos y los ajustes de ganancia, transición y límites normal/rápido. Prueba por separado una muesca, un movimiento pequeño, un giro rápido con frenado y una inversión. No adjuntes capturas de entradas personales.

El mod distribuible debe seguir siendo `mods/kde-pinch-zoom.wh.cpp`, autocontenido. No cambies la configuración de gestos de Windows ni sustituyas deslizamientos nativos por atajos simulados. Los hooks privados requieren confirmar su ABI y acotar las versiones compatibles antes de ampliar soporte.

Ejecuta `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check.ps1`. Este script no instala el mod ni cambia el zoom. La prueba `build/native-integration.exe --live` es optativa y cambia brevemente la ampliación: requiere el mod instalado y restaura su nivel inicial. Documenta por separado pruebas automáticas y gestos físicos.

En 0.7.5 hay 701 comprobaciones automáticas. Al modificar la rueda, conserva la equivalencia entre una muesca y sus deltas fraccionarios, la cancelación al invertir y los límites de recorrido pendiente. Comprueba también la alternancia con teclado y pellizco. Los relays de Magnify no deben publicar ni limpiar el estado del proceso dueño, y los callbacks atrasados no deben deshacer objetivos gestionados. No ejecutes pruebas visibles de zoom durante el trabajo del usuario sin coordinación previa.

No declares compatible otra versión de Windows, Windhawk, arquitectura o configuración de monitores solo porque compile. El soporte actual está limitado a lo descrito en el README.
