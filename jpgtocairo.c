#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cairo.h>
#include <jpeglib.h>

int width;
int height;

cairo_surface_t * decodeJPEGIntoSurface(struct jpeg_decompress_struct *args) {
  int stride = width * 4;
  cairo_status_t status;

  uint8_t *data = (uint8_t *) malloc(width * height * 4);
  if (!data) {
    jpeg_abort_decompress(args);
    jpeg_destroy_decompress(args);
    return NULL;
  }

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


cairo_surface_t *  loadJPEGFromBuffer(uint8_t *buf, unsigned len) {
  // TODO: remove this duplicate logic
  // JPEG setup
  struct jpeg_decompress_struct args;
  struct jpeg_error_mgr err;
  args.err = jpeg_std_error(&err);
  jpeg_create_decompress(&args);

  jpeg_mem_src(&args, buf, len);
  jpeg_read_header(&args, 1);
  jpeg_start_decompress(&args);

  width = args.output_width;
  height = args.output_height;

  return decodeJPEGIntoSurface(&args);
}

#define MIN(a,b) (((a)<(b))?(a):(b))

cairo_surface_t* rescale(cairo_surface_t* surface){
    cairo_surface_t *surfacescaled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1366, 768);
    cairo_t* cr = cairo_create (surfacescaled);

    float w = cairo_image_surface_get_width (surface);
    float h = cairo_image_surface_get_height (surface);

    float width_ratio = 1366.0 / w;
    float height_ratio = 768.0 / h;
    float scale_xy = MIN(height_ratio, width_ratio);
    
    cairo_save(cr);
    cairo_set_source_surface (cr, surface, (1366.0/2)-(w/2), (768/height_ratio)-(h/height_ratio));
    //cairo_scale(cr, 1,1);
    printf("width_ratio %f, height_ratio %f\n", width_ratio, height_ratio);
    
    
    cairo_paint (cr);
    cairo_restore(cr);
return surfacescaled;

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
   //cinfo.in_color_space = JCS_EXT_BGRX;
   cinfo.in_color_space = cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32 ? JCS_EXT_BGRA : JCS_EXT_BGRX;
#else
   //cinfo.in_color_space = JCS_EXT_XRGB;
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





int main (void){
  uint8_t *buf;
  unsigned len;
  int i;
  char tmp[12];

  for ( i=1; i<11; i++){

    sprintf(tmp,"%.2d.jpg", i);
    printf("%s\n", tmp);
    FILE *stream = fopen(tmp, "rb");

    fseek(stream, 0, SEEK_END);
    len = ftell(stream);
    fseek(stream, 0L, SEEK_SET);

    buf = (uint8_t *) malloc(len);
    if (!buf){
            printf("%d habemus pete \n", __LINE__);
            return 0;
    }
    fread(buf, len, 1, stream);
    cairo_surface_t * surface = loadJPEGFromBuffer(buf, len);
      
    cairo_surface_t * rescaled = rescale(surface);
    char tmp2[12];
    sprintf(tmp2,"%s_rescaled.jpg", tmp);

    write_JPEG_file(tmp2, rescaled, 80);
   // sprintf(tmp2,"%s_rescaled.png", tmp);
   // cairo_surface_write_to_png (rescaled, tmp2);
    fclose(stream);
    free(buf);
  }
}