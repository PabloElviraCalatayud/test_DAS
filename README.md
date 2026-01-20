# test_DAS
# 🧩 Resumen completo de problemas y lecciones aprendidas (OTA + Sensores)

Este documento resume **todos los problemas reales detectados durante la implementación de OTA por BLE**, cómo se manifestaron, por qué ocurrían y qué conclusiones técnicas se han obtenido. Sirve como referencia para evitar repetir errores y como base sólida para continuar el desarrollo.

---

## 1️⃣ OTA sin ACK/NACK → Flujo inseguro

### Problema
Los paquetes OTA se enviaban por BLE sin confirmación de recepción.

### Consecuencias
- Pérdida de paquetes
- Escrituras fuera de orden
- Posible corrupción de firmware
- Resets aparentemente aleatorios

### Solución
Definir un protocolo OTA con:
- Comandos `START / DATA / END`
- Número de secuencia (`SEQ`)
- Respuestas `ACK / NACK`

✔️ OTA fiable y controlada

---

## 2️⃣ Ejecución de OTA en contexto BLE → Watchdog

### Problema
Funciones como `esp_ota_write()` se ejecutaban:
- desde callbacks BLE
- o desde funciones llamadas directamente por BLE

### Por qué falla
- BLE corre en contexto de alta prioridad
- `esp_ota_write()` es bloqueante (ms)
- El scheduler no puede ejecutar otras tareas

### Síntoma
TG1WDT_SYS_RESET

### Solución
- Mover OTA a:
  - una task dedicada
  - o una cola (`QueueHandle_t`)
- BLE solo encola datos

✔️ Arquitectura correcta

---

## 3️⃣ Mezcla de dos arquitecturas OTA incompatibles

### Problema
Coexistían:
- OTA basada en cola + task
- OTA ejecutada directamente desde BLE

### Consecuencias
- Estados incoherentes
- Handles OTA mal gestionados
- ACK inconsistentes
- Resets sin patrón claro

### Solución
- Eliminar versiones antiguas
- Mantener **un único `ota_manager` limpio**

✔️ Diseño simplificado y estable

---

## 4️⃣ WDT aparente que no era OTA

### Síntoma
- Resets tras iniciar OTA
- Resets tras flashear firmware OTA

### Realidad
La OTA **sí funcionaba correctamente**.

### Prueba definitiva
✔️ `main_ota_only.c`  
✔️ OTA estable  
✔️ Flasheo correcto  
✔️ Sin resets

➡️ OTA descartada como causa

---

## 5️⃣ El verdadero problema: inicialización de sensores

### Evidencia
Crash consistente en:

ESP_ERROR_CHECK failed: ESP_ERR_TIMEOUT
mpu6050_start()
i2c_master_write_to_device(...)

### Significado
- El MPU6050 no responde a I²C
- Se produce timeout
- `ESP_ERROR_CHECK()` llama a `abort()`
- Reset inmediato del sistema

---

## 6️⃣ Reset confundido con WDT

### Importante
No es un WDT real.

Es:
abort() → panic → reboot

Provocado por:
- `ESP_ERROR_CHECK()` en drivers
- Fallos de comunicación hardware

---

## 7️⃣ Problemas típicos detectados en sensores

### A) Inicialización demasiado temprana
- BLE, PHY, RF y calibraciones aún en curso
- I²C no completamente estable

### B) Uso de `ESP_ERROR_CHECK()` en drivers
- Correcto para debug
- Letal en producción

### C) Sensores activos durante OTA
- Competencia por CPU, I²C y flash
- Latencias excesivas
- Posible starvation

### D) Falta de aislamiento por estados
- Sensores no pausados durante OTA
- Falta de máquina de estados global

---

## 8️⃣ Buenas decisiones tomadas

✔️ Aislar OTA completamente  
✔️ Crear `main_ota_only.c`  
✔️ Probar hipótesis con firmware mínimo  
✔️ No asumir causas sin verificar  
✔️ Confirmar con pruebas reales  

➡️ Metodología correcta de depuración embedded

---

## 📌 Estado actual del sistema

| Módulo           | Estado |
|------------------|--------|
| OTA BLE          | ✅ Funciona |
| Protocolo ACK    | ✅ Correcto |
| BLE              | ✅ Estable |
| packet_manager   | ✅ OK |
| MPU6050          | ❌ Causa reset |
| Pulse sensor     | ⚠️ Pendiente |
| Inicialización I²C | ❌ Demasiado agresiva |

---

## 🔜 Próximos pasos recomendados

1. Inicialización segura y retardada de sensores
2. Eliminar `ESP_ERROR_CHECK()` de drivers
3. Pausar sensores durante OTA
4. Añadir reintentos y timeouts tolerantes
5. Aislar I²C y sensores en tasks dedicadas

---

**Conclusión:**  
La OTA por BLE funciona correctamente.  
Los resets están causados por la inicialización y gestión de sensores, no por la OTA.

