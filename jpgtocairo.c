#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cairo.h>
#include <jpeglib.h>

cairo_surface_t * decodeJPEGIntoSurface(struct jpeg_decompress_struct *args, uint8_t* data){
  int width = args->output_width;
  int height = args->output_height;

  int stride = width * 4;
  cairo_status_t status;

  uint8_t *src = (uint8_t *) malloc(width * args->output_components);
  if (!src) {
    free(data);
    jpeg_abort_decompress(args);
    jpeg_destroy_decompress(args);
    return NULL;
  }
  
  int x=0, y=0;
  for (y = 0; y < height; ++y) {
    
    jpeg_read_scanlines(args, &src, 1);

    uint32_t *row = (uint32_t *)(data + stride * y);
    for (x = 0; x < width; ++x) {
      if (args->jpeg_color_space == 1) {
        uint32_t *pixel = row + x;
        *pixel = 255 << 24
          | src[x] << 16
          | src[x] << 8
          | src[x];
      } else {
        int bx = 3 * x;
        uint32_t *pixel = row + x;
        *pixel = 255 << 24
          | src[bx + 0] << 16
          | src[bx + 1] << 8
          | src[bx + 2];  

      }
    }
  }

  cairo_surface_t * _surface = cairo_image_surface_create_for_data(
      data
    , CAIRO_FORMAT_ARGB32
    , width
    , height
    , cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width));

  jpeg_finish_decompress(args);
  jpeg_destroy_decompress(args);
  status = cairo_surface_status(_surface);

  if (status) {
    free(data);
    free(src);
    return NULL;
  }

  free(src);

  return _surface;
}


cairo_surface_t *  loadJPEGFromBuffer(uint8_t *buf, uint8_t *data, unsigned len) {

  struct jpeg_decompress_struct args;
  struct jpeg_error_mgr err;
  args.err = jpeg_std_error(&err);
  jpeg_create_decompress(&args);

  jpeg_mem_src(&args, buf, len);
  jpeg_read_header(&args, 1);
  jpeg_start_decompress(&args);

  return decodeJPEGIntoSurface(&args, data);
}

#define MIN(a,b) (((a)<(b))?(a):(b))

void rescale(cairo_surface_t* surface, cairo_surface_t* surfacescaled, int width, int height){
    
  cairo_t* cr = cairo_create (surfacescaled);

  float w = cairo_image_surface_get_width (surface);
  float h = cairo_image_surface_get_height (surface);

  int _x = width/2 - w/2;
  int _y = height/2 - h/2;

  cairo_translate( cr, _x, _y ) ;
  cairo_set_source_surface (cr, surface, 0, 0); 
  cairo_paint (cr);
  cairo_destroy(cr);
  return;

}

void write_JPEG_file (char * filename, cairo_surface_t *surface, int quality)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE * outfile;   /* target file */
  JSAMPROW row_pointer[1];  /* pointer to JSAMPLE row[s] */
  int row_stride;   /* physical row width in image buffer */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }
  jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = cairo_image_surface_get_width(surface);  /* image width and height, in pixels */
  cinfo.image_height = cairo_image_surface_get_height(surface);
  cinfo.input_components = 4;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
   cinfo.in_color_space = cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32 ? JCS_EXT_BGRA : JCS_EXT_BGRX;
#else
   cinfo.in_color_space = cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32 ? JCS_EXT_ARGB : JCS_EXT_XRGB;
#endif

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  jpeg_start_compress(&cinfo, TRUE);

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = cairo_image_surface_get_data(surface) + (cinfo.next_scanline
            * cairo_image_surface_get_stride(surface));
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  fclose(outfile);

  jpeg_destroy_compress(&cinfo);
}


static void usage(){
  printf("\trescale IMAGE_NAME WIDTH HEIGHT QUALITY\n");
  exit(0);
}


int main (int argc, char *argv[]){
  uint8_t *buf;
  unsigned len;
  int i;
  char filename[64];
  int width, height, quality;

  if (argc < 5){
    printf("Error, not enought parameters\n");
    usage();
  }
  
  sprintf(filename, "%s", argv[1]);
  width = atoi(argv[2]);
  height = atoi(argv[3]);
  quality = atoi(argv[4]);

  FILE *stream = fopen(filename, "rb");
  fseek(stream, 0, SEEK_END);
  len = ftell(stream);
  fseek(stream, 0L, SEEK_SET);

  buf = (uint8_t *) malloc(len);
  if (!buf){
    return 0;
  }

  uint8_t *data = (uint8_t *) malloc(width * height * 4);
  if (!buf){
    return 0;
  }
  
  fread(buf, len, 1, stream);

  cairo_surface_t * surface = loadJPEGFromBuffer(buf, data, len);
  
  cairo_surface_t * surfacescaled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);  

  rescale(surface, surfacescaled, width, height);

  char tmp2[12];
  sprintf(tmp2, "%s_rescaled.jpg", filename);

  write_JPEG_file(tmp2, surfacescaled, quality);
  cairo_surface_destroy(surface);
  cairo_surface_destroy(surfacescaled);

  fclose(stream);
  free(buf);
  free(data);
 
}
