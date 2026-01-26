# Sistema de Adquisición de Datos con ESP32, BLE y OTA

## Resumen Técnico de Problemas y Soluciones

Este documento detalla los problemas encontrados durante el desarrollo, sus causas raíz y las soluciones implementadas, ordenados por complejidad creciente.

---

## Índice

1. [Protocolo BLE bloqueando ACKs de OTA](#1-protocolo-ble-bloqueando-acks-de-ota-durante-actualización)
2. [Callbacks de estado ejecutándose múltiples veces](#2-callbacks-de-estado-ejecutándose-múltiples-veces)
3. [Watchdog Timer Reset al detener sensor de pulso](#3-watchdog-timer-reset-al-detener-sensor-de-pulso)
4. [Assert en xTaskPriorityDisinherit](#4-assert-en-xtaskprioritydisinherit-al-desinicializar-adc)
5. [Sensor MPU6050 con mismo patrón de lifecycle](#5-sensor-mpu6050-con-mismo-patrón-de-lifecycle)
6. [Arquitectura Final](#arquitectura-final-del-sistema)
7. [Lecciones Clave](#lecciones-clave)

---

## 1. Protocolo BLE bloqueando ACKs de OTA durante actualización

### Problema

El ESP32 recibía correctamente el comando `OTA_CMD_START` y detenía los sensores, pero la aplicación móvil nunca recibía el ACK y por tanto no enviaba los paquetes de datos OTA.

**Logs del problema:**
```
I (16738) OTA_MGR: OTA START command received
I (17268) OTA_MGR: Starting OTA to partition ota_0 at 0x110000
I (17428) OTA_MGR: OTA session started, ready for data
// ... pero nunca llegan paquetes DATA
```

### Causa raíz

En `bluetooth.c`, la tarea `ble_tx_task` descartaba **todos** los paquetes durante el modo OTA, incluyendo los ACKs críticos del protocolo OTA:
```c
// CÓDIGO PROBLEMÁTICO
static void ble_tx_task(void *arg) {
  ble_packet_t pkt;
  while (1) {
    if (xQueueReceive(ble_tx_queue, &pkt, portMAX_DELAY)) {
      if (system_state_get() == SYS_STATE_OTA) {  // Bloquea ACKs
        continue;
      }
      bluetooth_notify(pkt.data, pkt.len);
      vTaskDelay(pdMS_TO_TICKS(15));
    }
  }
}
```

Además, `bluetooth_tx_enqueue()` rechazaba directamente los envíos:
```c
// CÓDIGO PROBLEMÁTICO
bool bluetooth_tx_enqueue(const uint8_t *data, uint16_t len) {
  if (system_state_get() == SYS_STATE_OTA) {  // Rechaza ACKs
    return false;
  }
  // ...
}
```

### Solución implementada

Eliminar las verificaciones de estado OTA en el subsistema BLE, ya que los sensores detenidos no generan tráfico:
```c
// CÓDIGO CORREGIDO - bluetooth.c
static void ble_tx_task(void *arg) {
  ble_packet_t pkt;
  while (1) {
    if (xQueueReceive(ble_tx_queue, &pkt, portMAX_DELAY)) {
      // Sensores ya detenidos, solo pasan ACKs OTA
      bluetooth_notify(pkt.data, pkt.len);
      vTaskDelay(pdMS_TO_TICKS(15));
    }
  }
}

bool bluetooth_tx_enqueue(const uint8_t *data, uint16_t len) {
  if (!ble_tx_queue || len > 247) {
    return false;
  }
  
  ble_packet_t pkt;
  pkt.len = len;
  memcpy(pkt.data, data, len);
  
  return xQueueSend(ble_tx_queue, &pkt, pdMS_TO_TICKS(100)) == pdTRUE;
}
```

**Resultado:** Los ACKs OTA ahora se envían correctamente y la actualización completa.

---

## 2. Callbacks de estado ejecutándose múltiples veces

### Problema

El callback `system_state_cb` en `sensor_manager` se ejecutaba repetidamente, intentando detener sensores que ya estaban detenidos:

**Logs del problema:**
```
W (17818) SENSOR_MGR: OTA mode - stopping sensors
W (18028) SENSOR_MGR: OTA mode - stopping sensors
W (35518) SENSOR_MGR: OTA mode - stopping sensors
```

### Causa raíz

`ota_manager_handle_packet()` llamaba a `system_state_set(SYS_STATE_OTA)` cada vez que procesaba un comando, y `system_state_set()` notificaba a todos los callbacks sin verificar si el estado realmente había cambiado.
```c
// CÓDIGO PROBLEMÁTICO - ota_manager.c
void ota_manager_handle_packet(const uint8_t *data, uint16_t len) {
  // ...
  if (p.cmd == OTA_CMD_START) {
    system_state_set(SYS_STATE_OTA);  // Se llama múltiples veces
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  // ...
  xQueueSend(s_queue, &p, 0);
}
```
```c
// CÓDIGO PROBLEMÁTICO - system_state.c
void system_state_set(system_state_t state) {
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  s_state = state;  // Sin verificar si cambió
  xSemaphoreGive(s_mutex);

  for (int i = 0; i < s_cb_count; i++) {
    s_cbs[i](state);  // Llama siempre
  }
}
```

### Solución implementada

**1. Eliminar llamada duplicada en `ota_manager.c`:**
```c
// CÓDIGO CORREGIDO
void ota_manager_handle_packet(const uint8_t *data, uint16_t len) {
  // ...
  // Cambio de estado movido a ota_task, no aquí
  if (p.len > 0) {
    memcpy(p.data, &data[5], p.len);
  }
  xQueueSend(s_queue, &p, pdMS_TO_TICKS(100));
}
```

**2. Protección contra cambios duplicados en `system_state.c`:**
```c
// CÓDIGO CORREGIDO
void system_state_set(system_state_t state) {
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  
  if (s_state == state) {  // Verificar cambio real
    xSemaphoreGive(s_mutex);
    return;
  }
  
  s_state = state;
  xSemaphoreGive(s_mutex);

  for (int i = 0; i < s_cb_count; i++) {
    s_cbs[i](state);
  }
}
```

**Resultado:** Cada transición de estado se procesa exactamente una vez.

---

## 3. Watchdog Timer Reset al detener sensor de pulso

### Problema

Al iniciar OTA, el sistema se congelaba y el watchdog reseteaba la placa:

**Logs del problema:**
```
I (23548) PULSE: Task exiting loop, will clean up ADC...
W (23588) SENSOR_MGR: OTA mode - stopping sensors
ets Jul 29 2019 12:21:46
rst:0x8 (TG1WDT_SYS_RESET),boot:0x13
```

### Causa raíz

La tarea `pulse_task` quedaba bloqueada en `adc_driver_read_multi()` esperando datos del ADC. Cuando `pulse_sensor_stop()` ponía `s_running = false`, la tarea no podía salir del bloqueo y nunca señalizaba el semáforo.
```c
// CÓDIGO PROBLEMÁTICO - pulse_sensor.c
static void pulse_task(void *arg) {
  while (s_running) {
    // ...
    int n = adc_driver_read_multi(s_adc, &res, 1);  // Bloqueo 20ms
    // ...
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  xSemaphoreGive(s_stop_sem);
  vTaskDelete(NULL);
}

void pulse_sensor_stop(void) {
  s_running = false;
  xSemaphoreTake(s_stop_sem, pdMS_TO_TICKS(500));  // Timeout, tarea bloqueada
}
```

### Solución implementada

Detener el ADC **antes** de señalizar `s_running = false`, liberando inmediatamente la tarea bloqueada:
```c
// CÓDIGO CORREGIDO - pulse_sensor.c
void pulse_sensor_stop(void) {
  if (!s_task) return;
  
  ESP_LOGI(TAG, "Stopping ADC first to unblock task...");
  if (s_adc) {
    adc_continuous_stop(s_adc);  // Libera adc_continuous_read()
  }
  
  s_running = false;  // Ahora la tarea puede salir
  
  if (xSemaphoreTake(s_stop_sem, pdMS_TO_TICKS(1000)) == pdTRUE) {
    if (s_adc) {
      vTaskDelay(pdMS_TO_TICKS(50));
      adc_continuous_deinit(s_adc);  // Limpieza desde fuera
      s_adc = NULL;
    }
    s_task = NULL;
  }
}
```

**Arquitectura clave:**
- La tarea solo sale del loop
- El manager externo limpia los recursos
- No hay conflictos de ownership de mutex

**Resultado:** Parada limpia sin WDT.

---

## 4. Assert en `xTaskPriorityDisinherit` al desinicializar ADC

### Problema

Crash crítico con assert de FreeRTOS al detener el sensor de pulso:

**Logs del problema:**
```
assert failed: xTaskPriorityDisinherit tasks.c:5156 
  (pxTCB == pxCurrentTCBs[ xPortGetCoreID() ])

Backtrace:
--- pulse_task → adc_driver_deinit → adc_continuous_stop → adc_lock_release
```

### Causa raíz

El driver ADC usa mutex con herencia de prioridad. La tarea `pulse_task` intentaba destruir el driver ADC que ella misma estaba usando, violando las reglas de FreeRTOS sobre ownership de mutex.
```c
// CÓDIGO PROBLEMÁTICO - pulse_sensor.c (versión inicial)
static void pulse_task(void *arg) {
  while (s_running) {
    int n = adc_driver_read_multi(s_adc, &res, 1);
    // ...
  }
  
  adc_driver_deinit(s_adc);  // La propia tarea destruye su recurso
  xSemaphoreGive(s_stop_sem);
  vTaskDelete(NULL);
}
```

### Evolución de soluciones intentadas

**Intento 1: Añadir delays (falló)**
```c
// NO FUNCIONÓ
void adc_driver_deinit(adc_continuous_handle_t handle) {
  vTaskDelay(pdMS_TO_TICKS(50));  // ❌ No resuelve el ownership
  adc_continuous_stop(handle);
  vTaskDelay(pdMS_TO_TICKS(50));
  adc_continuous_deinit(handle);
}
```

**Intento 2: Limpieza desde la tarea (falló)**
```c
// NO FUNCIONÓ
static void pulse_task(void *arg) {
  while (s_running) { /* ... */ }
  
  vTaskDelay(pdMS_TO_TICKS(100));  // Delay no cambia ownership
  adc_driver_deinit(s_adc);
  xSemaphoreGive(s_stop_sem);
  vTaskDelete(NULL);
}
```

### Solución final implementada

Separación estricta de responsabilidades: la tarea solo notifica su salida, el manager limpia los recursos:
```c
// CÓDIGO CORREGIDO - pulse_sensor.c
static void pulse_task(void *arg) {
  while (s_running) {
    if (!s_running) break;  // Doble verificación
    
    int n = adc_driver_read_multi(s_adc, &res, 1);
    if (n > 0) {
      packet_feed_pulse_raw(res.average, esp_timer_get_time() / 1000ULL);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  
  // Solo señaliza salida, NO toca hardware
  xSemaphoreGive(s_stop_sem);
  vTaskDelete(NULL);
}

void pulse_sensor_stop(void) {
  if (!s_task) return;
  
  // Detener ADC desde ESTA tarea (sensor_manager_task)
  if (s_adc) {
    adc_continuous_stop(s_adc);
  }
  
  s_running = false;
  
  if (xSemaphoreTake(s_stop_sem, pdMS_TO_TICKS(1000)) == pdTRUE) {
    // Limpieza desde el manager
    if (s_adc) {
      vTaskDelay(pdMS_TO_TICKS(50));
      adc_continuous_deinit(s_adc);
      s_adc = NULL;
    }
    s_task = NULL;
  }
}
```

**Principio arquitectónico aplicado:**

> El que crea el hardware, lo destruye. Las tareas solo lo usan.

**Resultado:** Sin asserts, parada limpia del ADC.

---

## 5. Sensor MPU6050 con mismo patrón de lifecycle

### Problema

Mismo error potencial con el sensor I2C:
```c
// CÓDIGO PROBLEMÁTICO (versión inicial)
static void mpu6050_task(void *arg) {
  while (s_running) { /* leer I2C */ }
  
  i2c_driver_delete(s_port);  // Tarea destruye su propio driver
  vTaskDelete(NULL);
}
```

### Solución implementada

Aplicar el mismo patrón de separación de responsabilidades:
```c
// CÓDIGO CORREGIDO - mpu6050.c
static void mpu6050_task(void *arg) {
  while (s_running) {
    if (mpu_read(REG_ACCEL_XOUT, raw, sizeof(raw)) == ESP_OK) {
      packet_feed_imu_raw(ax, ay, az, gx, gy, gz, 
                         esp_timer_get_time() / 1000ULL);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  
  // Solo señaliza
  xSemaphoreGive(s_stop_sem);
  vTaskDelete(NULL);
}

void mpu6050_stop(void) {
  if (!s_task) return;
  
  s_running = false;
  
  if (xSemaphoreTake(s_stop_sem, pdMS_TO_TICKS(500)) == pdTRUE) {
    // Limpieza desde sensor_manager_task
    i2c_driver_delete(s_port);
    s_task = NULL;
  }
}
```

**Resultado:** Consistencia arquitectónica en todos los sensores.

---

## Arquitectura Final del Sistema

### Flujo de control correcto
```
app_main()
  ├─ system_state_init()
  ├─ bluetooth_init()
  ├─ ota_manager_init()
  ├─ packet_manager_init()
  ├─ sensor_manager_init()
  └─ system_state_set(SYS_STATE_RUNNING)

Estados: INIT → RUNNING → OTA

Durante transición a OTA:
  1. system_state_set(SYS_STATE_OTA)
  2. sensor_manager recibe callback
  3. mpu6050_stop() + pulse_sensor_stop()
  4. Tareas señalizan su salida
  5. Managers limpian hardware
  6. OTA procede con sistema estable
```

### Patrón de lifecycle de tareas
```c
// PATRÓN CORRECTO PARA CUALQUIER TAREA CON HARDWARE

// === EN LA TAREA ===
static void hw_task(void *arg) {
  while (s_running) {
    // Usar hardware
    if (!s_running) break;  // Salida rápida
  }
  
  // NO tocar hardware aquí
  xSemaphoreGive(s_stop_sem);
  vTaskDelete(NULL);
}

// === EN EL STOP ===
void hw_stop(void) {
  if (s_hw_handle) {
    hw_driver_stop(s_hw_handle);  // Liberar bloqueos
  }
  
  s_running = false;
  
  if (xSemaphoreTake(s_stop_sem, timeout) == pdTRUE) {
    // Ahora sí, destruir hardware
    hw_driver_deinit(s_hw_handle);
    s_hw_handle = NULL;
    s_task = NULL;
  }
}
```

---

## Lecciones Clave

1. **Ownership de recursos**: El que inicializa el hardware debe destruirlo, no la tarea que lo usa.

2. **Máquina de estados global**: Un sistema sin estados claros termina en condiciones de carrera.

3. **Callbacks idempotentes**: `system_state_set()` debe verificar cambios reales antes de notificar.

4. **Liberación de bloqueos**: Detener drivers antes de señalizar parada de tareas para evitar deadlocks.

5. **BLE y OTA conviven**: No bloquear el canal BLE durante OTA; los sensores detenidos no generan tráfico.

6. **Delays no solucionan arquitectura**: Si un delay "arregla" un crash, el diseño está mal.

---

## Orden de Problemas por Complejidad

| # | Problema | Complejidad | Impacto |
|---|----------|-------------|---------|
| 1 | BLE bloqueando ACKs OTA | Baja | Alto |
| 2 | Callbacks duplicados | Media | Medio |
| 3 | WDT por tarea bloqueada | Media | Alto |
| 4 | Assert xTaskPriorityDisinherit | **Alta** | **Crítico** |
| 5 | Consistencia MPU6050 | Baja | Preventivo |

El problema #4 fue el más complejo porque requería entender:
- Herencia de prioridad en FreeRTOS
- Ownership de mutex en drivers ESP-IDF
- Contextos de ejecución entre tareas

---

**Estado final:** Sistema estable con sensores, BLE y OTA funcionando correctamente bajo arquitectura bien definida.
