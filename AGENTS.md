# Desarrollo

- Este proyecto es independiente de cualquier otro proyecto del escritorio.
- El mod distribuible es un único archivo: `mods/kde-pinch-zoom.wh.cpp`.
- Mantén los gestos de Windows intactos: no escribas en la configuración del touchpad.
- Ejecuta `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check.ps1` antes de publicar.
- Las pruebas sintéticas no demuestran compatibilidad con un touchpad físico. Documenta por separado las comprobaciones realizadas en hardware.
- No habilites el mod ni inicies otro Windhawk automáticamente desde scripts de compilación.
- El usuario ha autorizado integrar `magnifier-headless`: el instalador puede desactivar ese mod antiguo, conservándolo instalado, para evitar dos hooks de ocultación simultáneos. No modifiques otros mods.
