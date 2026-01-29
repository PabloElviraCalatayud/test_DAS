## 📁 lib/app/

### Propósito
Arranque de la aplicación, configuración global y navegación.

### Archivos

| Archivo | Responsabilidad |
|------|----------------|
| `app.dart` | Widget raíz de la aplicación |
| `app_bootstrap.dart` | Inicialización de servicios (BLE, stores, decoder) |
| `app_routes.dart` | Definición centralizada de rutas |

**Por qué existe:**  
Evita mezclar lógica de infraestructura con UI y mantiene `main.dart` limpio.

---

## 📁 lib/core/

### Propósito
Código transversal reutilizable, **sin dependencia de features ni UI**.

---

### 📁 core/constants/

| Archivo | Función |
|------|--------|
| `ble_constants.dart` | UUIDs BLE, MTU, servicios |
| `packet_constants.dart` | Tamaños y límites del protocolo |

---

### 📁 core/errors/

Errores de dominio BLE y comunicación.

---

### 📁 core/utils/

Helpers puros y reutilizables.

| Archivo | Función |
|------|--------|
| `byte_utils.dart` | Parsing binario |
| `log_utils.dart` | Logging controlado |

---

## 📁 lib/data/

### Propósito
**Fuente de verdad de los datos**.  
Aquí NO hay widgets ni lógica de presentación.

---

## 📁 data/bluetooth/

Infraestructura BLE.

| Archivo | Responsabilidad |
|------|----------------|
| `ble_packet.dart` | Wrapper del paquete BLE crudo |
| `ble_manager.dart` | Conexión, notificaciones BLE |
| `packet_service.dart` | Entrada BLE → PacketDecoder |

---

## 📁 data/sensors/

Datos ya estructurados, independientes del protocolo BLE.

---

### 📁 data/sensors/imu/

| Archivo | Responsabilidad |
|------|----------------|
| `imu_sample.dart` | Una medición IMU puntual |
| `imu_state.dart` | Estado agregado (si aplica) |
| `imu_store.dart` | Buffer, stream y último valor IMU |

---

### 📁 data/sensors/pulse/

| Archivo | Responsabilidad |
|------|----------------|
| `pulse_sample.dart` | Pulso puntual |
| `pulse_state.dart` | Estado agregado |
| `pulse_store.dart` | Buffer, stream y BPM |

---

### 📄 sensor_clock.dart

Fuente temporal común para timestamps coherentes.

---

## 📁 lib/features/

Aquí vive el **comportamiento de la aplicación**.

---

## 📁 features/packet/

### Propósito
Traducción del paquete binario (88B) al dominio.

| Archivo | Responsabilidad |
|------|----------------|
| `packet.dart` | Modelo lógico del paquete |
| `packet_decoder.dart` | Decodifica bytes y distribuye a stores |

**Regla:**  
Este módulo **NO conoce UI ni widgets**.

---

## 📁 features/debug/

Pantalla de inspección técnica.

- RAW BLE
- RAW sensores
- Datos procesados

Ideal para validar firmware y protocolo.

---

## 📁 lib/shared/

Componentes reutilizables de UI.

| Subcarpeta | Contenido |
|---------|---------|
| `theme/` | Colores y temas |
| `widgets/` | Botones, cards, indicadores |

**Nunca dependen de features.**

---

## 📄 main.dart

Debe contener únicamente:

```dart
void main() {
  runApp(const App());
}

