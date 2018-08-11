#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>

/* libexif headers */
#include <libexif/exif-data.h>
#include "libjpeg/jpeg-data.h"

/* struct used on each image */
struct image_gps_exif {
    ExifEntry *latitude;
    ExifEntry *longitude;
    ExifEntry *latitude_ref;
    ExifEntry *longitude_ref;
};

typedef struct coordinates {
    uint32_t h; //º
    uint32_t s1;
    uint32_t m; //'
    uint32_t s2;
    uint32_t s; //"
    uint32_t d;
} coords;

/* flags */
bool verbose = false;
bool jpeg_create_new = false;
bool delete_gps_data = false;
bool identify_gps_data = false;

/* Latitude references */
#define LATITUDE_REF_N "N"
#define LATITUDE_REF_S "S"
/* Longitude references */
#define LONGITUDE_REF_E "E"
#define LONGITUDE_REF_W "W"

/* exif magic */
//const unsigned char exif_magic[4] = { 0xFF, 0xD8, 0xFF, 0xE1 };
//const uint32_t exif_magic_s = sizeof(exif_magic);

//#define DEBUG

/* little -> big endian */
uint32_t __bswap_32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

/* error reporting */
enum {
    INFO = 0,
    WARN,
    ERROR
};

static void _perror(uint8_t t, char *f, ...)
{
    va_list va_m;

    switch (t) {
        case (ERROR):
            fprintf(stderr, "[ERROR] ");
            break;
        case (WARN):
            fprintf(stderr, "[WARN] ");
            break;
        default:
            fprintf(stderr, "[INFO] ");
            break;
    }
    va_start(va_m, f);
    vfprintf(stderr, f, va_m);
    va_end(va_m);
    fprintf(stderr, "\n");
}

void dump_hex(const char *name, unsigned char *data, size_t s)
{
    int n = 0;

    printf("%s:\n\t", name);
    while (s-->0) {
        n++;
        printf("%02x ", *data);
        if (n == 8) {
            printf("\n\t");
            n = 0;
        }
        data++;
    }
    printf("\n");
}

/* randomize latitude/longitude values */
void randomize(struct image_gps_exif *g) {
    uint8_t la_max = 90;
    uint8_t lo_max = 180;
    coords la, lo;

    /* latitude */
    la.h = __bswap_32(rand()%la_max);
    la.s1 = __bswap_32(1);
    la.m = __bswap_32(rand()%60);
    la.s2 = __bswap_32(1);
    la.s = __bswap_32(rand()%600);
    la.d = __bswap_32(10); // 01.f
    
    /* longitude */
    lo.h = __bswap_32(rand()%lo_max);
    lo.s1 = __bswap_32(1);
    lo.m = __bswap_32(rand()%60);
    lo.s2 = __bswap_32(1);
    lo.s = __bswap_32(rand()%600);
    lo.d = __bswap_32(10); // 01.f

    if (g->latitude->data != NULL)
        memcpy(g->latitude->data, &la, sizeof(coords));
    if (g->longitude->data != NULL)
        memcpy(g->longitude->data, &lo, sizeof(coords));
}

/* randomize latitude/longitude references */
void randomize_ref(struct image_gps_exif *g) {
    uint8_t la_ref = (uint8_t)rand()%2;
    uint8_t lo_ref = (la_ref^1);

    if (g->latitude_ref != NULL)
        switch (la_ref) {
            case(0): strncpy((char *)g->latitude_ref->data,
                             LATITUDE_REF_N, 2); break;
            case(1): strncpy((char *)g->latitude_ref->data,
                             LATITUDE_REF_S, 2); break;
        }

    if (g->longitude_ref != NULL)
        switch (lo_ref) {
            case(0): strncpy((char *)g->longitude_ref->data,
                             LONGITUDE_REF_E, 2); break;
            case(1): strncpy((char *)g->longitude_ref->data,
                             LONGITUDE_REF_W, 2); break;
        }
}

/* write new data to new file */
int write_image(char *path, ExifData *data)
{
    JPEGData *jpeg_out;
    char *new_path = NULL, *dir = NULL, *name_ptr;

    if (jpeg_create_new) {
#define NEW_PATH_CONCAT "rand_"

        if ((name_ptr = strrchr(path, '/')) == NULL) {
            name_ptr = path;
        } else {
            dir = malloc(4096);
            strncpy(dir, path, strlen(path) - strlen(name_ptr));
            name_ptr++;
        }

        new_path = malloc(4096);
        snprintf(new_path, 4096, "%s/%s%s", (dir?dir:"."), NEW_PATH_CONCAT,
                name_ptr);
        if (jpeg_create_new)
            _perror(INFO, "Creating new jpeg image: %s", new_path);
    }
    
    jpeg_out = jpeg_data_new_from_file(path);
    jpeg_data_set_exif_data(jpeg_out, data);
    jpeg_data_save_file(jpeg_out, (jpeg_create_new ? new_path : path));

    jpeg_data_free(jpeg_out);

    if (new_path)
        free(new_path);
    if (dir)
        free(dir);
    return 1;
}

void delete_entry(ExifEntry *e)
{
    exif_content_remove_entry(e->parent, e);
}

void delete_gps_entries(struct image_gps_exif *g)
{
    if (g->latitude != NULL)
        delete_entry(g->latitude);
    if (g->latitude_ref != NULL)
        delete_entry(g->latitude_ref);
    if (g->longitude != NULL)
        delete_entry(g->longitude);
    if (g->longitude_ref != NULL)
        delete_entry(g->longitude_ref);
}

ExifEntry *get_gps_content(ExifData *d, ExifTag t) {
    ExifEntry *e = exif_content_get_entry(d->ifd[EXIF_IFD_GPS], t);

    if (e != NULL && verbose) {
        dump_hex(exif_tag_get_name(t), (void *)e->data, e->size);
    }
    return (e);
}

void usage(const char *p)
{
    (void)fprintf(stderr,
            "usage: %s [-vh] [-n] [-d] [file ...]\n" \
            "\t-v\tVerbose (default: false)\n" \
            "\t-n\tCreate new JPEG file (default: false)\n" \
            "\t-d\tDelete GPS data\n" \
            "\t-i\tIdentify GPS data\n" \
            "\n",
            p);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
        usage(argv[0]);

    int ch = 0;
    while ((ch = getopt(argc, argv, "vhndi")) != -1) {
        switch (ch) {
            case 'v':
                verbose = true;
                break;
            case 'n':
                jpeg_create_new = true;
                break;
            case 'd':
                delete_gps_data = true;
                break;
            case 'i':
                identify_gps_data = true;
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }
    argc-=optind;
    argv+=optind;

    if (jpeg_create_new &&
            ( delete_gps_data || identify_gps_data ))
        printf("Ignoring -n flag.\n");

    if (delete_gps_data && identify_gps_data) {
        printf("You can't use -d and -i at the same time.\n");
        usage(argv[0]);
    }
    
    if (argc == 0)
        usage(argv[0]);

    /* start */
    srand(time(NULL));
    for (int i = 0; i < argc; i++) {
        struct image_gps_exif gps;

        if (argc > 1 || verbose)
            printf("=== %s ===\n", argv[i]);

        ExifData *exif_data;
        if (!(exif_data = exif_data_new_from_file(argv[i]))) {
            _perror(ERROR, "Couldn't load exif data from '%s'. "\
                    "No IFD GPS data or not even an image?", argv[i]);
            continue;
        }

        if (verbose) _perror(INFO, "Getting GPS content: ");
        /* check existence of latitude tag */
        gps.latitude = get_gps_content(exif_data, EXIF_TAG_GPS_LATITUDE);
        if (gps.latitude == NULL && verbose)
            _perror(INFO, "No latitude data.");

        /* check existence of latitude ref tag */
        gps.latitude_ref = get_gps_content(exif_data,
                EXIF_TAG_GPS_LATITUDE_REF);
        if (gps.latitude_ref == NULL && verbose)
            _perror(INFO, "No latitude reference data.");
        
        /* check existence of longitude tag */
        gps.longitude = get_gps_content(exif_data, EXIF_TAG_GPS_LONGITUDE);
        if (gps.longitude == NULL && verbose)
            _perror(INFO, "No longitude data.");
        
        /* check existence of longitude ref tag */
        gps.longitude_ref = get_gps_content(exif_data,
                EXIF_TAG_GPS_LONGITUDE_REF);
        if (gps.longitude_ref == NULL && verbose)
            _perror(INFO, "No longitude reference data.");

        if (delete_gps_data) {
            delete_gps_entries(&gps);
        } else if (identify_gps_data) {
            /* this will just check if theres any GPS data. */
            if (gps.latitude != NULL || gps.latitude_ref != NULL ||
                    gps.longitude != NULL || gps.longitude_ref != NULL)
                goto goaway;
            else
                _perror(INFO, "No GPS data present.");
        } else {
            randomize(&gps);
            randomize_ref(&gps);
        }

        if (!write_image(argv[i], exif_data))
                _perror(ERROR, "Couldn't write new image file");

#ifdef DEBUG
        exif_data_dump(exif_data);
#endif
goaway:
        /* to the next one or bail out */
        exif_data_unref(exif_data);
    }

    return 0;
}
