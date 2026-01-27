#include "offline_storage.h"

#include <string.h>

#include "esp_log.h"
#include "esp_partition.h"
#include "esp_rom_crc.h"   // esp_rom_crc32_le()

#include <inttypes.h>


#define TAG "offline_storage"

// Debe coincidir con tu partitions.csv
#define OFFLINE_PART_LABEL "offline_data"
#define OFFLINE_PART_SUBTYPE 0x40

// Formato del registro en flash: [header][payload offline_hour_t][footer]
#define REC_MAGIC_HEAD 0x4F46464Cu  // 'OFFL'
#define REC_MAGIC_FOOT 0x4F464654u  // 'OFFT'
#define REC_VERSION    0x01

// commit_flag: lo escribimos AL FINAL (última operación)
// Se usa el truco de flash: 1->0 es posible, 0->1 no.
// - Erased = 0xFF
// - Committed = 0x00
#define COMMIT_ERASED   0xFF
#define COMMIT_DONE     0x00

typedef struct __attribute__((packed)) {
    uint32_t magic;      // REC_MAGIC_HEAD
    uint8_t  version;    // REC_VERSION
    uint8_t  reserved0;
    uint16_t payload_len; // sizeof(offline_hour_t)
    uint32_t seq;        // contador creciente
    uint32_t ts_hour_ms; // copia rápida del timestamp
    uint32_t hdr_crc32;  // CRC del header sin este campo (opcional)
} rec_hdr_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;       // REC_MAGIC_FOOT
    uint32_t payload_crc; // CRC32 de payload
    uint8_t  commit_flag; // COMMIT_DONE al final
    uint8_t  reserved[3];
} rec_ftr_t;

_Static_assert(sizeof(rec_ftr_t) == 12, "footer size must be 12");

// Estado interno
static const esp_partition_t *s_part = NULL;
static uint32_t s_write_off = 0;   // offset relativo donde escribir el próximo registro
static uint32_t s_count = 0;
static uint32_t s_seq = 0;

static inline uint32_t align4(uint32_t x) { return (x + 3u) & ~3u; }

static uint32_t crc32_le(const void *data, size_t len)
{
    // ESP-ROM CRC32 little-endian
    return esp_rom_crc32_le(0, (const uint8_t *)data, (uint32_t)len);
}

static esp_err_t part_read(uint32_t off, void *dst, size_t len)
{
    return esp_partition_read(s_part, off, dst, len);
}

static esp_err_t part_write(uint32_t off, const void *src, size_t len)
{
    // En flash, escribe en múltiplos de 4 bytes en ESP-IDF
    return esp_partition_write(s_part, off, src, len);
}

static bool looks_erased(const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;
    for (size_t i = 0; i < len; i++) {
        if (p[i] != 0xFF) return false;
    }
    return true;
}

static esp_err_t scan_partition(void)
{
    s_write_off = 0;
    s_count = 0;
    s_seq = 0;

    const uint32_t part_size = (uint32_t)s_part->size;

    while (1) {
        if (s_write_off + sizeof(rec_hdr_t) > part_size) {
            // No cabe ni header
            return ESP_OK;
        }

        rec_hdr_t hdr;
        esp_err_t err = part_read(s_write_off, &hdr, sizeof(hdr));
        if (err != ESP_OK) return err;

        // Si está borrado (0xFF...), consideramos fin del log
        if (looks_erased(&hdr, sizeof(hdr))) {
            return ESP_OK;
        }

        if (hdr.magic != REC_MAGIC_HEAD || hdr.version != REC_VERSION) {
            // Encontramos basura/corrupción -> paramos aquí
            ESP_LOGW(TAG, "Scan stop: invalid header at off=0x%08" PRIx32, s_write_off);
            return ESP_OK;
        }

        uint32_t payload_len = hdr.payload_len;
        if (payload_len != sizeof(offline_hour_t)) {
            ESP_LOGW(TAG, "Scan stop: unexpected payload_len=%" PRIu32 " at off=0x%08" PRIx32, payload_len, s_write_off);
            return ESP_OK;
        }

        uint32_t rec_size = sizeof(rec_hdr_t) + payload_len + sizeof(rec_ftr_t);
        uint32_t rec_size_aligned = align4(rec_size);

        if (s_write_off + rec_size > part_size) {
            ESP_LOGW(TAG, "Scan stop: record would overflow partition at off=0x%08" PRIx32, s_write_off);
            return ESP_OK;
        }

        // Leer footer
        rec_ftr_t ftr;
        err = part_read(s_write_off + sizeof(rec_hdr_t) + payload_len, &ftr, sizeof(ftr));
        if (err != ESP_OK) return err;

        if (ftr.magic != REC_MAGIC_FOOT) {
            ESP_LOGW(TAG, "Scan stop: invalid footer magic at off=0x%08" PRIx32, s_write_off);;
            return ESP_OK;
        }

        // Si no está committed, fin del log (registro incompleto)
        if (ftr.commit_flag != COMMIT_DONE) {
           ESP_LOGW(TAG, "Scan stop: uncommitted record at off=0x%08" PRIx32, s_write_off);
            return ESP_OK;
        }

        // Validar CRC de payload
        offline_hour_t payload;
        err = part_read(s_write_off + sizeof(rec_hdr_t), &payload, sizeof(payload));
        if (err != ESP_OK) return err;

        uint32_t crc = crc32_le(&payload, sizeof(payload));
        if (crc != ftr.payload_crc) {
            ESP_LOGW(TAG, "Scan stop: CRC mismatch at off=0x%08" PRIx32, s_write_off);
            return ESP_OK;
        }

        // Registro válido -> avanzamos
        s_count++;
        s_seq = hdr.seq + 1;
        s_write_off += rec_size_aligned;
    }
}

esp_err_t offline_storage_init(void)
{
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, OFFLINE_PART_SUBTYPE, OFFLINE_PART_LABEL);
    if (!s_part) {
        ESP_LOGE(TAG, "Partition not found: type=data subtype=0x%02x label=%s",
                 OFFLINE_PART_SUBTYPE, OFFLINE_PART_LABEL);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Using partition '%s' @0x%08x size=%u",
             s_part->label, (unsigned)s_part->address, (unsigned)s_part->size);

    return scan_partition();
}

uint32_t offline_storage_count(void)
{
    return s_count;
}

esp_err_t offline_storage_append_hour(const offline_hour_t *rec)
{
    if (!s_part) return ESP_ERR_INVALID_STATE;
    if (!rec) return ESP_ERR_INVALID_ARG;

    const uint32_t payload_len = sizeof(offline_hour_t);
    const uint32_t rec_size = sizeof(rec_hdr_t) + payload_len + sizeof(rec_ftr_t);
    const uint32_t rec_size_aligned = align4(rec_size);

    if (s_write_off + rec_size > (uint32_t)s_part->size) {
        return ESP_ERR_NO_MEM;
    }

    // 1) Escribir header
    rec_hdr_t hdr = {0};
    hdr.magic = REC_MAGIC_HEAD;
    hdr.version = REC_VERSION;
    hdr.payload_len = (uint16_t)payload_len;
    hdr.seq = s_seq;
    hdr.ts_hour_ms = rec->ts_hour_ms;

    // hdr_crc32 opcional
    hdr.hdr_crc32 = 0;
    hdr.hdr_crc32 = crc32_le(&hdr, offsetof(rec_hdr_t, hdr_crc32));

    esp_err_t err = part_write(s_write_off, &hdr, sizeof(hdr));
    if (err != ESP_OK) return err;

    // 2) Escribir payload
    err = part_write(s_write_off + sizeof(rec_hdr_t), rec, payload_len);
    if (err != ESP_OK) return err;

    // 3) Escribir footer con commit al final
    rec_ftr_t ftr = {0};
    ftr.magic = REC_MAGIC_FOOT;
    ftr.payload_crc = crc32_le(rec, payload_len);
    ftr.commit_flag = COMMIT_DONE; // escribimos al final ya committed

    err = part_write(s_write_off + sizeof(rec_hdr_t) + payload_len, &ftr, sizeof(ftr));
    if (err != ESP_OK) return err;

    // 4) Si hace falta, rellenar padding a 0xFF? (no necesario)
    // Avanzar
    s_write_off += rec_size_aligned;
    s_count++;
    s_seq++;

    return ESP_OK;
}

void offline_iter_begin(offline_iter_t *it)
{
    if (!it) return;
    it->offset = 0;
    it->index = 0;
}

esp_err_t offline_iter_next(offline_iter_t *it, offline_hour_t *out)
{
    if (!s_part) return ESP_ERR_INVALID_STATE;
    if (!it || !out) return ESP_ERR_INVALID_ARG;

    if (it->index >= s_count) return ESP_ERR_NOT_FOUND;

    rec_hdr_t hdr;
    esp_err_t err = part_read(it->offset, &hdr, sizeof(hdr));
    if (err != ESP_OK) return err;

    if (hdr.magic != REC_MAGIC_HEAD || hdr.version != REC_VERSION || hdr.payload_len != sizeof(offline_hour_t)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // leer payload
    err = part_read(it->offset + sizeof(rec_hdr_t), out, sizeof(*out));
    if (err != ESP_OK) return err;

    // leer footer y validar
    rec_ftr_t ftr;
    err = part_read(it->offset + sizeof(rec_hdr_t) + sizeof(*out), &ftr, sizeof(ftr));
    if (err != ESP_OK) return err;

    if (ftr.magic != REC_MAGIC_FOOT || ftr.commit_flag != COMMIT_DONE) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint32_t crc = crc32_le(out, sizeof(*out));
    if (crc != ftr.payload_crc) {
        return ESP_ERR_INVALID_CRC;
    }

    // avanzar
    uint32_t rec_size = sizeof(rec_hdr_t) + sizeof(*out) + sizeof(rec_ftr_t);
    it->offset += align4(rec_size);
    it->index++;

    return ESP_OK;
}

esp_err_t offline_storage_erase_all(void)
{
    if (!s_part) return ESP_ERR_INVALID_STATE;

    // Borramos toda la partición
    esp_err_t err = esp_partition_erase_range(s_part, 0, s_part->size);
    if (err != ESP_OK) return err;

    // Reset estado
    s_write_off = 0;
    s_count = 0;
    s_seq = 0;

    return ESP_OK;
}
