#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>

/* file/dir processing */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

/* libexif headers */
#include <libexif/exif-data.h>
/* libjpeg */
#include "libjpeg/jpeg-data.h"

/* struct used on each image */
struct image_gps_exif {
    ExifEntry *latitude;
    ExifEntry *longitude;
    ExifEntry *latitude_ref;
    ExifEntry *longitude_ref;
    ExifEntry *timestamp;
    ExifEntry *datestamp;
    int8_t n_entries;
};

typedef struct rationale64u {
    uint32_t data[6];
} r64_t;

typedef r64_t coords;
typedef r64_t times;


/* flags */
bool verbose = false;
bool recursive = false;
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

/* randomize timestamp and datetime */
void randomize_datetime(struct image_gps_exif *g)
{
    time_t now = rand() % time(NULL);
    struct tm *tm_data;

    tm_data = gmtime(&now);

    /* GPSTimeStamp */
    if (g->timestamp != NULL) {
        times t;
        t.data[0] = __bswap_32(tm_data->tm_hour);
        t.data[2] = __bswap_32(tm_data->tm_min);
        t.data[4] = __bswap_32(tm_data->tm_sec);

        t.data[1] = __bswap_32(0x01);
        t.data[3] = t.data[5] = t.data[1];
        memcpy(g->timestamp->data, &t, sizeof(t));
    }
    
    /* GPSDateStamp */
    if (g->datestamp != NULL) {
        char d[11];
#define GPS_DATESTAMP_FMT "%Y:%m:%d"
        strftime(d, 11, GPS_DATESTAMP_FMT, tm_data);
        memcpy(g->datestamp->data, &d, strlen(d));
    }
}

/* randomize latitude/longitude values */
void randomize(struct image_gps_exif *g)
{
    uint8_t la_max = 90;
    uint8_t lo_max = 180;
    coords la, lo;

    /* latitude */
    la.data[0] = __bswap_32(rand()%la_max);
    la.data[1] = __bswap_32(1);
    la.data[2] = __bswap_32(rand()%60);
    la.data[3] = __bswap_32(1);
    la.data[4] = __bswap_32(rand()%600);
    la.data[5] = __bswap_32(10); // 01.f
    
    /* longitude */
    lo.data[0] = __bswap_32(rand()%lo_max);
    lo.data[1] = __bswap_32(1);
    lo.data[2] = __bswap_32(rand()%60);
    lo.data[3] = __bswap_32(1);
    lo.data[4] = __bswap_32(rand()%600);
    lo.data[5] = __bswap_32(10); // 01.f

    if (g->latitude != NULL)
        memcpy(g->latitude->data, &la, sizeof(coords));
    if (g->longitude != NULL)
        memcpy(g->longitude->data, &lo, sizeof(coords));
}

/* randomize latitude/longitude references */
void randomize_ref(struct image_gps_exif *g)
{
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
    if (g->timestamp != NULL)
        delete_entry(g->timestamp);
    if (g->datestamp != NULL)
        delete_entry(g->datestamp);
}

ExifEntry *get_gps_content(ExifData *d, ExifTag t) {
    ExifEntry *e = exif_content_get_entry(d->ifd[EXIF_IFD_GPS], t);

    if (e != NULL && verbose) {
        dump_hex(exif_tag_get_name(t), (void *)e->data, e->size);
    }
    return (e);
}

void process_file(char *path)
{
    struct image_gps_exif gps;
    gps.n_entries = 0;

    if (verbose)
        printf("=== %s ===\n", path);

    ExifData *exif_data;
    if (!(exif_data = exif_data_new_from_file(path))) {
        if (verbose)
            _perror(INFO, "Couldn't load exif data from '%s'. "\
                    "No IFD GPS data or not even an image?", path);
        return;
    }

    if (verbose) _perror(INFO, "Getting GPS content: ");
    /* check existence of latitude tag */
    gps.latitude = get_gps_content(exif_data, EXIF_TAG_GPS_LATITUDE);
    if (gps.latitude == NULL && verbose)
        _perror(INFO, "No latitude data.");
    else
        gps.n_entries++;

    /* check existence of latitude ref tag */
    gps.latitude_ref = get_gps_content(exif_data,
            EXIF_TAG_GPS_LATITUDE_REF);
    if (gps.latitude_ref == NULL && verbose)
        _perror(INFO, "No latitude reference data.");
    else
        gps.n_entries++;
    
    /* check existence of longitude tag */
    gps.longitude = get_gps_content(exif_data, EXIF_TAG_GPS_LONGITUDE);
    if (gps.longitude == NULL && verbose)
        _perror(INFO, "No longitude data.");
    else
        gps.n_entries++;
    
    /* check existence of longitude ref tag */
    gps.longitude_ref = get_gps_content(exif_data,
            EXIF_TAG_GPS_LONGITUDE_REF);
    if (gps.longitude_ref == NULL && verbose)
        _perror(INFO, "No longitude reference data.");
    else
        gps.n_entries++;
    
    /* check existence of timestamp tag */
    gps.timestamp = get_gps_content(exif_data,
            EXIF_TAG_GPS_TIME_STAMP);
    if (gps.timestamp == NULL && verbose)
        _perror(INFO, "No timestamp data.");
    else
        gps.n_entries++;
    
    /* check existence of datestamp tag */
    gps.datestamp = get_gps_content(exif_data,
            EXIF_TAG_GPS_DATE_STAMP);
    if (gps.datestamp == NULL && verbose)
        _perror(INFO, "No datestamp data.");
    else
        gps.n_entries++;

    if (delete_gps_data) {
        delete_gps_entries(&gps);
    } else if (identify_gps_data) {
        /* this will just check if theres any GPS data. */
        if (gps.n_entries > 0)
            goto goaway;
        else
            _perror(INFO, "No GPS data present.");
    } else {
        randomize(&gps);
        randomize_ref(&gps);
        randomize_datetime(&gps);
    }

    if (!write_image(path, exif_data))
            _perror(ERROR, "Couldn't write new image file");

#ifdef DEBUG
    exif_data_dump(exif_data);
#endif
goaway:
    /* to the next one or bail out */
    exif_data_unref(exif_data);
}

void process_dir(char *path)
{
    DIR *dir;
    struct dirent *dirlist;
    
    if ((dir = opendir(path)) == NULL) {
        _perror(ERROR, "Can't open directory '%s'", path);
        return;
    }

    while ((dirlist = readdir(dir)) != NULL) {
        if ((strcmp(dirlist->d_name, ".") == 0) ||
                (strcmp(dirlist->d_name, "..") == 0))
            continue;

        char next_path[1024];
        snprintf((char *)&next_path, sizeof(next_path), "%s/%s",
                path, dirlist->d_name);

        struct stat st;
        if ((stat(next_path, &st)) == -1) {
            _perror (ERROR, "stat(2) returned -1.");
            continue;
        }

        if ((st.st_mode & S_IFMT) == S_IFDIR) {
            process_dir(next_path);
        } else {
            process_file(next_path);
        }
    }

    closedir(dir);
}

void usage(const char *p)
{
    (void)fprintf(stderr,
            "usage: %s [-vhR] [-n] [-d] [-i] [file|dir ...]\n" \
            "\t-v\tVerbose (default: false)\n" \
            "\t-n\tCreate new JPEG file (default: false)\n" \
            "\t-d\tDelete GPS data\n" \
            "\t-i\tIdentify GPS data\n" \
            "\t-R\tRecursive if dir specified (default: false)\n" \
            "\n",
            p);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
        usage(argv[0]);

    int ch = 0;
    while ((ch = getopt(argc, argv, "vhndiR")) != -1) {
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
            case 'R':
                recursive = true;
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
        struct stat st;
        if ((stat(argv[i], &st)) == -1) {
            _perror (ERROR, "stat(2) returned -1.");
            continue;
        }

        if ((st.st_mode & S_IFMT) == S_IFDIR) {
            /* argv is a dir */
            if (recursive)
                process_dir(argv[i]);
            else 
                _perror(INFO,
                        "Not processing %s because -R was not specified.",
                        argv[i]);
        } else {
            /* argv is a file */
            process_file(argv[i]);
        }
    }

    return 0;
}
