/* DreamShell ##version##

   qr_code.c - DreamEye app QR Code scanner
   Copyright (C) 2024 SWAT 
*/

#include <ds.h>
#include <quirc.h>

static struct quirc *qr = NULL;
static struct quirc_code code;
static struct quirc_data data;

static void yuyv_to_luma(const uint8_t *src, int src_pitch,
          int w, int h, uint8_t *dst, int dst_pitch) {
    int y;

    for (y = 0; y < h; y++) {
        int x;
        const uint8_t *srow = src + y * src_pitch;
        uint8_t *drow = dst + y * dst_pitch;

        for (x = 0; x < w; x += 2) {
            *(drow++) = srow[0];
            *(drow++) = srow[2];
            srow += 4;
        }
    }
}

int qr_scan_frame(int bpp, uint8_t *frame, size_t len, char *qr_data) {

    int i, w, h, count, rv = -1;
    uint8_t *buf;
    uint64_t begin, end;

    if (!qr) {
        return rv;
    }

    begin = timer_ns_gettime64();
    buf = quirc_begin(qr, &w, &h);

    switch (bpp) {
        case 12:
            /* TODO: Maybe just map it possible? */
            memcpy(buf, frame, w * h);
            break;
        case 16:
            yuyv_to_luma(frame, w * 2, w, h, buf, w);
            break;
        default:
            quirc_end(qr);
            return rv;
    }

    quirc_end(qr);
    count = quirc_count(qr);

    for (i = 0; i < count; i++) {

        quirc_extract(qr, i, &code);

        if (!quirc_decode(&code, &data)) {

            end = timer_ns_gettime64();

            if(data.data_type != QUIRC_DATA_TYPE_BYTE) {
                ds_printf("QR data: unsupported, time %d ns\n",
                    (end - begin) / 1000000);
                ds_printf("Version: %d, ECC: %c, Mask: %d, Type: %d\n",
                    data.version, "MLHQ"[data.ecc_level],
                    data.mask, data.data_type);
                continue;
            }

            ds_printf("QR data: \"%s\", time %d ms\n",
                data.payload, (end - begin) / 1000000);
            rv = 0;

            if(qr_data) {
                memcpy(qr_data, data.payload, strlen((char *)data.payload) + 1);
            }
        }
    }
    return rv;
}

int qr_scan_resize(int w, int h) {
    if (quirc_resize(qr, w, h) < 0) {
        ds_printf("DS_ERROR: Couldn't allocate QR buffer\n");
        return -1;
    }
    return 0;
}

int qr_scan_init() {
    if (qr) {
        return 0;
    }
    qr = quirc_new();
    if (qr) {
        return 0;
    }
    return -1;
}

void qr_scan_shutdown() {
    if (!qr) {
        return;
    }
    quirc_destroy(qr);
}
