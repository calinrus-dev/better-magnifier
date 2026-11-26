# Contribuir a Better Visual

[Inicio](README.md) · [Referencia](docs/REFERENCE.md) · [Validación](docs/VALIDATION.md)

Las correcciones y mejoras se pueden proponer mediante un issue o un pull request. El proyecto usa [GPL-3.0-or-later](LICENSE).

## Reproducir un fallo

- Indica la compilación exacta de Windows, la versión de Windhawk, el dispositivo y los pasos para reproducirlo.
- Distingue si ocurre al iniciar la Lupa, con ella abierta, al usar el teclado o al cambiar de modo. Añade la salida de `--status` si ayuda a identificar el modo, el nivel y las capacidades activas. Evita adjuntar datos personales o volcados completos del sistema.
- Para fallos con la rueda, indica el modelo del ratón, si usas muescas o giro libre, el perfil de atajos y el paso de zoom. Prueba una muesca, un movimiento pequeño, un giro rápido con frenado y una inversión. Anota qué rueda y qué sentido provocan el fallo.

## Compilar y comprobar

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check.ps1
```

Para Windhawk portátil, añade `-WindhawkPath 'D:\Herramientas\Windhawk'`. El script genera en `build/` la DLL, el host de diagnóstico y los binarios de prueba. Las comprobaciones automáticas usan entradas simuladas en el reconocedor, el broker de ratón y teclado, el runtime y el adaptador nativo. La compilación se puede ejecutar con el escritorio en uso.

La integración visible es optativa y requiere el mod instalado. Comprueba el renderizado nativo, las inversiones rápidas, el incremento por IPC y el relevo con el pellizco. Cambia brevemente el zoom y restaura el nivel inicial. Ejecútala cuando puedas dejar quietos el touchpad, el ratón y el teclado.

```powershell
.\build\native-integration.exe --live
```

Documenta las pruebas automáticas y las comprobaciones con hardware físico. Para ampliar la compatibilidad, registra la versión de Windows y Windhawk, la arquitectura y la configuración de monitores con que se comprobó el cambio.

## Cambios de código

- Conserva un solo archivo autocontenido para distribuir en Windhawk, `mods/kde-pinch-zoom.wh.cpp`. Mantén la configuración de gestos de Windows y aplica sus acciones nativas cuando corresponda.
- Al modificar la rueda, conserva la equivalencia entre una muesca y sus deltas fraccionarios, la cancelación al invertir y los límites de recorrido pendiente. Comprueba el relevo con teclado y pellizco.
- Los procesos auxiliares de Magnify dejan que la instancia propietaria controle su estado. Los callbacks retrasados respetan el objetivo de zoom vigente.
- Para ampliar el soporte de hooks privados, comprueba su ABI y acota las versiones compatibles.
