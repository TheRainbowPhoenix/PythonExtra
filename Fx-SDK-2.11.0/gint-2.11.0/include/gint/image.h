//---
// gint:image - Image manipulation and rendering
//
// Note: this module is currently only available on fx-CG.
//
// This header provides image manipulation functions. This mainly consists of a
// reference-based image format, various access and modification functions, and
// a number of high-performance transformations and rendering effects. If you
// find yourself limited by rendering time, note that RAM writing speed is
// often the bottleneck, and image rendering is much faster in Azur (which is
// what the renderer was initially designed for).
//
// This module supports 3 bit depths: full-color 16-bit (RGB565), indexed 8-bit
// (P8) and indexed 4-bit (P4). All three have an "alpha" variation where one
// color is treated as transparent, leading to 6 total formats.
//
// The image renderers support so-called *dynamic effects*, which are image
// transformations performed on-the-fly while rendering, without generating an
// intermediate image. They comprise straightforward transformations that
// achieve similar performance to straight rendering and can be combined to
// some extent, which makes them reliable whenever applicable.
//
// For images of the RGB16 and P8 bit depths, the module supports a rich API
// with image subsurfaces and a fairly large sets of geometric and color
// transforms, including some in-place. P4 is not supported in most of these
// functions because the dense bit packing is both impractical and slower for
// these applications.
//---

#ifndef GINT_IMAGE
#define GINT_IMAGE

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/config.h>
#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

struct dwindow;

//---
// Image structures
//---

/* Image formats. Note that transparency really only indicates the default
   rendering method, as a transparent background can always be added or removed
   by a dynamic effect on any image. */
enum {
    IMAGE_RGB565      = 0,  /* RGB565 without alpha */
    IMAGE_RGB565A     = 1,  /* RGB565 with one transparent color */
    IMAGE_P8_RGB565   = 4,  /* 8-bit palette, all opaque colors */
    IMAGE_P8_RGB565A  = 5,  /* 8-bit with one transparent color */
    IMAGE_P4_RGB565   = 6,  /* 4-bit palette, all opaque colors */
    IMAGE_P4_RGB565A  = 3,  /* 4-bit with one transparent color */

    IMAGE_DEPRECATED_P8 = 2,
};

/* Quick macros to compare formats by storage size */
#define IMAGE_IS_RGB16(format) \
    ((format) == IMAGE_RGB565 || (format) == IMAGE_RGB565A)
#define IMAGE_IS_P8(format) \
    ((format) == IMAGE_P8_RGB565 || (format) == IMAGE_P8_RGB565A)
#define IMAGE_IS_P4(format) \
    ((format) == IMAGE_P4_RGB565 || (format) == IMAGE_P4_RGB565A)
/* Check whether image format has an alpha color */
#define IMAGE_IS_ALPHA(format) \
    ((format) == IMAGE_RGB565A || \
     (format) == IMAGE_P8_RGB565A || \
     (format) == IMAGE_P4_RGB565A)
/* Check whether image format uses a palette */
#define IMAGE_IS_INDEXED(format) \
    (IMAGE_IS_P8(format) || IMAGE_IS_P4(format))

/* Image flags. These are used for memory management, mostly. */
enum {
    IMAGE_FLAGS_DATA_RO         = 0x01,  /* Data is read-only */
    IMAGE_FLAGS_PALETTE_RO      = 0x02,  /* Palette is read-only */
    IMAGE_FLAGS_DATA_ALLOC      = 0x04,  /* Data is malloc()'d */
    IMAGE_FLAGS_PALETTE_ALLOC   = 0x08,  /* Palette is malloc()'d */
};

/* image_t: gint's native bitmap image format
   Images of this format can be created through this header's API but also by
   using the fxSDK's built-in image converters with fxconv. */
typedef struct
{
    /* Color format, one of the IMAGE_* values defined above */
    uint8_t format;
    /* Additional flags, a combination of IMAGE_FLAGS_* values */
    uint8_t flags;
    /* Number of colors in the palette; this includes alpha for transparent
       images, as alpha is always the first entry.
       RGB16: 0
       P8: Ranges between 1 and 256
       P4: 16 */
    int16_t color_count;
    /* Full width and height, in pixels */
    uint16_t width;
    uint16_t height;
    /* Byte stride between lines */
    int stride;

    /* Pixel data in row-major order, left to right.
       - RGB16: 2 bytes per entry, each row padded to 4 bytes for alignment.
         Each 2-byte value is an RGB565 color.
       - P8: 1 signed byte per entry. Each byte is a palette index shifted by
         128 (to access the color, use palette[<value>+128]).
       - P4: 4 bits per entry, each row padded to a full byte. Each entry is a
         direct palette index between 0 and 15. */
    void *data;

    /* For P8 and P4, color palette. The number of entries allocated in the
       array is equal to the color_count attribute. */
    uint16_t *palette;

} GPACKED(4) image_t;

/* Dynamic effects: these transformations can be applied on images while
   rendering. Not all effects can be combined; unless specified otherwise:
   - HFLIP and VFLIP can both be added regardless of any other effect
   - At most one color effect can be applied */
enum {
    /* Value 0x01 is reserved, because it is DIMAGE_NOCLIP, which although
       part of the old API still needs to be supported. */

    /* [Any]: Skip clipping the command against the source image */
    IMAGE_NOCLIP_INPUT   = 0x04,
    /* [Any]: Skip clipping the command against the output VRAM */
    IMAGE_NOCLIP_OUTPUT  = 0x08,
    /* [Any]: Skip clipping both */
    IMAGE_NOCLIP         = IMAGE_NOCLIP_INPUT | IMAGE_NOCLIP_OUTPUT,

    // Geometric effects. These values should remain at exactly bit 8 and
    // following, or change gint_image_mkcmd() along with it.

    /* [Any]: Flip image vertically */
    IMAGE_VFLIP          = 0x0100,
    /* [Any]: Flip image horizontally */
    IMAGE_HFLIP          = 0x0200,

    // Color effects

    /* [RGB565, P8_RGB565, P4_RGB565]: Make a color transparent
       Adds one argument:
       * Color to clear (RGB16: 16-bit value; P8/P4: palette index) */
    IMAGE_CLEARBG        = 0x10,
    /* [RGB565, P8_RGB565, P4_RGB565]: Turn a color into another
       Adds two arguments:
       * Color to replace (RGB16: 16-bit value; P8/P4: palette index)
       * Replacement color (16-bit value) */
    IMAGE_SWAPCOLOR      = 0x20,
    /* [RGB565A, P8_RGB565A, P4_RGB565A]: Add a background
        Adds one argument:
        * Background color (16-bit value) */
    IMAGE_ADDBG          = 0x40,
    /* [RGB565A, P8_RGB565A, P4_RGB565A]: Dye all non-transparent pixels
       Adds one argument:
       * Dye color (16-bit value) */
    IMAGE_DYE            = 0x80,
};

//---
// Image creation and destruction
//---

#if GINT_RENDER_RGB

/* image_alloc(): Create a new (uninitialized) image

   This function allocates a new image of the specified dimensions and format.
   It always allocates a new data array; if you need to reuse a data array, use
   the lower-level image_create() or image_create_sub().

   The first parameters [width] and [height] specify the dimensions of the new
   image in pixels. The [format] should be one of the IMAGE_* formats, for
   example IMAGE_RGB565A or IMAGE_P4_RGB565.

   This function does not specify or initialize the palette of the new image;
   use image_set_palette(), image_alloc_palette() or image_copy_palette()
   after calling this function.

   The returned image structure must be freed with image_free() after use.

   @width         Width of the new image
   @height        Height of the new image
   @format        Pixel format; one of the IMAGE_* formats defined above */
image_t *image_alloc(int width, int height, int format);

/* image_set_palette(): Specify an external palette for an image

   This function sets the image's palette to the provided address. The number
   of entries allocated must be specified in size. It is also the caller's
   responsibility to ensure that the palette covers all the indices used in the
   image data.

   The old palette, if owned by the image, is freed. If [owns=true] the
   palette's ownership is given to the image, otherwise it is kept external. */
void image_set_palette(image_t *img, uint16_t *palette, int size, bool owns);

/* image_alloc_palette(): Allocate a new palette for an image

   This function allocates a new palette for an image. The number of entries is
   specified in size; for P8 it can vary between 1 and 256, for P4 it is
   ignored (P4 images always have 16 colors).

   The old palette, if owned by the image, is freed. The entries of the new
   palette are all initialized to 0. If size is -1, the format's default
   palette size is used. Returns true on success. */
bool image_alloc_palette(image_t *img, int size);

/* image_copy_palette(): Copy another image's palette

   This function allocates a new palette for an image, and initializes it with
   a copy of another image's palette. For P8 the palette can be resized by
   specifying a value other than -1 as the size; by default, the source image's
   palette size is used (within the limits of the new format). Retuns true on
   success. */
bool image_copy_palette(image_t const *src, image_t *dst, int size);

/* image_create(): Create a bare image with no data/palette

   This function allocates a new image structure but without data or palette.
   The [data] and [palette] members are NULL, [color_count] and [stride] are 0.

   This function is useful to create images that reuse externally-provided
   information. It is intended that the user of this function sets the [data]
   and [stride] fields themselves, along with the IMAGE_FLAGS_DATA_ALLOC flag
   if the image should own its data.

   The [palette] and [color_count] members can be set with image_set_palette(),
   image_alloc_palette(), image_copy_palette(), or manually.

   The returned image structure must be freed with image_free() after use. */
image_t *image_create(int width, int height, int format);

/* image_create_vram(): Create a reference to gint_vram

   This function creates a new RGB565 image that references gint_vram. Using
   this image as target for transformation functions can effectively render
   transformed images to VRAM.

   The value of gint_vram is captured when this function is called, and does
   not update after dupdate() when triple-buffering is used. The user should
   account for this option. (Using this function twice then replacing one of
   the [data] pointers is allowed.)

   The VRAM image owns no data but it does own its own structure so it must
   still be freed with image_free() after use. */
image_t *image_create_vram(void);

/* image_free(): Free and image and the data it owns

   This function frees the provided image structure and the data that it owns.
   Images converted by fxconv should not be freed; nonetheless, this functions
   distinguishes them and should work. Images are not expected to be created on
   the stack.

   If the image has the IMAGE_FLAGS_DATA_ALLOC flag, the data pointer is also
   freed. Similarly, the image has the IMAGE_FLAGS_PALETTE_ALLOC flag, the
   palette is freed. Make sure to not free images when references to them still
   exist, as this could cause the reference's pointers to become dangling. */
void image_free(image_t *img);

//---
// Basic image access and information
//---

/* image_valid(): Check if an image is valid
   An image is considered valid if it has a valid profile, a non-NULL data
   pointer, and for palette formats a valid palette pointer. */
bool image_valid(image_t const *img);

/* image_alpha(): Get the alpha value for an image format

   This function returns the alpha value for any specific image format:
   * RGB16: 0x0001
   * P8: -128 (0x80)
   * P4: 0
   For non-transparent formats, it returns a value that is different from all
   valid pixel values of the format, which means it is always safe to compare a
   pixel value to the image_alpha() of the format. */
int image_alpha(int format);

/* image_get_pixel(): Read a pixel from the data array

   This function reads a pixel from the image's data array at position (x,y).
   It returns the pixel's value, which is either a full-color value (RGB16) or
   a possibly-negative palette index (P8/P4). See the description of the [data]
   field of image_t for more details. The value of the pixel can be decoded
   into a 16-bit color either manually or by using the image_decode_pixel()
   function.

   Note that reading large amounts of image data with this function will be
   slow; if you need reasonable performance, consider iterating on the data
   array manually. */
int image_get_pixel(image_t const *img, int x, int y);

/* image_decode_pixel(): Decode a pixel value

   This function decodes a pixel's value obtained from the data array (for
   instance with image_get_pixel()). For RGB16 formats this does nothing, but
   for palette formats this accesses the palette at a suitable position.

   Note that reading large amounts of data with this function will be slow; if
   you need reasonable performance, consider inlining the format-specific
   method or iterating on the data array manually. */
int image_decode_pixel(image_t const *img, int pixel);

/* image_data_size(): Compute the size of the [data] array
   This function returns the size of the data array, in bytes. This can be used
   to duplicate it. Note that for sub-images this is a subsection of another
   image's data array, and might be much larger than the sub-image. */
int image_data_size(image_t const *img);

//---
// Basic image modifications
//---

/* image_set_pixel(): Set a pixel in the data array

   This function writes a pixel into the image's data array at position (x,y).
   The pixel value must be of the proper format, as specified in the definition
   of the [data] field of image_t.

   Formats: RGB16, P8, P4 */
void image_set_pixel(image_t const *img, int x, int y, int value);

/* image_copy(): Convert and copy an image

   This function copies an image into another image while converting certain
   formats. Unlike transforms, this function does clip, so there are no
   conditions on the size of the target.

   If [copy_alpha] is true, transparent pixels are copied verbatim, which
   effectively replaces the top-left corner of [dst] with [src]. If it's false,
   transparent pixels of [src] are skipped, effectively rendering [src] over
   the top-left corner of [src].

   This function converts between all formats except from RGB16 to P8/P4, since
   this requires generating a palette (which is a complex endeavour).
   Conversions from P8/P4 to RGB16 simply decode the palette. Conversions
   between P8/P4 preserve the contents but renumber the palette entries. From
   P4 to P8, the image is always preserved. From P8 to P4, the image is only
   preserved if it has less than 16 colors (this is intended to allow P4 images
   to be converted to P8 for edition by this library, and then back to P4). The
   following table summarizes the conversions:

     Source format →      RGB16           P8                P4
     Target format ↓  +-----------+----------------+------------------+
               RGB16  |    Copy     Decode palette    Decode palette  |
                  P8  |     -            Copy        Enlarge palette  |
                  P4  |     -       Narrow palette         Copy       |
                      +-----------+----------------+------------------+

   Note that conversions to RGB16 are not lossless because RGB565, P8 and P4
   can represent any color; if a color equal to image_alpha(IMAGE_RGB565A) is
   found during conversion, this function transforms it slightly to look
   similar instead of erroneously generating a transparent pixel.

   Formats: RGB16 → RGB16, P8 → Anything, P4 → Anything
   Size requirement: none (clipping is performed)
   Supports in-place: No (useless) */
void image_copy(image_t const *src, image_t *dst, bool copy_alpha);

/* image_copy_alloc(): Convert and copy into a new image
   This function is similar to image_copy(), but it allocates a target image of
   the desired format before copying. */
image_t *image_copy_alloc(image_t const *src, int new_format);

/* image_fill(): Fill an image with a single pixel value */
void image_fill(image_t *img, int value);

/* image_clear(): Fill a transparent image with its transparent value */
void image_clear(image_t *img);

//---
// Sub-image extraction
//---

/* image_sub(): Build a reference to a sub-image

   This function is used to create references to sub-images of RGB16 and P8
   images. The [data] pointer of the sub-image points somewhere within the data
   array of the source, and its [palette] pointer is identical to the source's.

   The last parameter is a pointer to a preallocated image_t structure (usually
   on the stack) that gets filled with the data. Doing this instead of
   allocating a new object with malloc() means that there is no need to
   image_free() the sub-image, and thus it can be used inline:

     image_t tmp;
     image_hflip(src, image_sub(dst, x, y, w, h, &tmp));

   A preprocessor macro is used to make the last parameter optional. If it's
   not specified, a pointer to a static image_t will be returned instead. This
   is useful in inline calls as shown above, which then simplify to:

     image_hflip(src, image_sub(dst, x, y, w, h));

   However, another call to image_sub() or image_at() will override the
   sub-image, so you should only use this in such temporary settings. If you
   need multiple image_sub() or image_at() calls in the same statement, only
   one can use the short form.

   If the requested rectangle does not intersect the source, the sub-image will
   be of dimension 0x0. If the image format does not support sub-images (P4),
   the sub-image will test invalid with image_valid(). */
image_t *image_sub(image_t const *src, int x, int y, int w, int h,
    image_t *dst);

/* Make the last parameter optional */
#define image_sub1(src, x, y, w, h, dst, ...) image_sub(src, x, y, w, h, dst)
#define image_sub(...) image_sub1(__VA_ARGS__, NULL)

/* image_at(): Build a reference to a position within a sub-image */
#define image_at(img, x, y) image_sub(img, x, y, -1, -1)

//---
// Geometric image transforms
//
// All geometric transforms render to position (0,0) of the target image and
// fail if the target image is not large enough to hold the transformed result
// (unlike the rendering functions which render only the visible portion).
//
// To render at position (x,y) of the target image, use img_at(). For instance:
//   image_hflip(src, image_at(dst, x, y));
//
// Each transform function has an [_alloc] variant which does the same
// transform but allocates the target image on the fly and returns it. Remember
// that allocation can fail, so you need to check whether the returned image is
// valid.
//
// (You can still pass invalid images to transform functions. The invalid image
//  will be ignored or returned unchanged, so you can chain calls and check for
//  validity at the end of the chain.)
//
// Some functions support in-place transforms. This means they can be called
// with the source as destination, and will transform the image without needing
// new memory. For instance, image_hflip(src, src) flips in-place and replaces
// src with a flipped version of itself.
//
// (However, it is not possible to transform in-place if the source and
//  destination intersect in non-trivial ways. The result will be incorrect.)
//
// When transforming to a new image, transparent pixels are ignored, so if the
// destination already has some data, it will not be erased automatically. Use
// image_clear() beforehand to achieve that effect. This allows alpha blending
// while transforming, which is especially useful on the VRAM.
//---

/* image_hflip(): Flip horizontally
   Formats: RGB16, P8
   Size requirement: destination at least as large as source (no clipping)
   Supports in-place: Yes */
void image_hflip(image_t const *src, image_t *dst, bool copy_alpha);
image_t *image_hflip_alloc(image_t const *src);

/* image_vflip(): Flip vertically
   Formats: RGB16, P8
   Size requirement: destination at least as large as source (no clipping)
   Supports in-place: Yes */
void image_vflip(image_t const *src, image_t *dst, bool copy_alpha);
image_t *image_vflip_alloc(image_t const *src);

/* image_linear(): Linear transformation

   This function implements a generic linear transformation. This is a powerful
   function that can perform any combination of rotation, mirroring and scaling
   with nearest-neighbor sampling.

   The [image_linear_map] structure defines the settings for the transform.
   Users familiar with linear algebra might want to use it directly, but they
   are most conveniently generated with the rotation and scaling functions
   listed below.

   Note: Currently the structure for the transform is modified by the
   operation and cannot be reused.

   The image_linear_alloc() variant allocates a new image in addition to
   performing the transform. The image is created with size (map->dst_w,
   map->dst_h) which is always a reasonable default. If a target image of
   smaller size is supplied to image_linear(), clipping is performed; only the
   top-left corner of the full output is actually rendered.

   Formats: RGB16, P8
   Size requirement: none (clipping is performed)
   Supports in-place: No */

struct image_linear_map {
    /* Dimensions of the source and destination */
    int src_w, src_h, dst_w, dst_h;
    /* Input and output stride in bytes */
    int src_stride, dst_stride;

    /* The following parameters define the linear transformation as a mapping
       from coordinates in the destination image (x and y) into coordinates in
       the source image (u and v).
       - (u, v) indicate where the top-left corner of the destination lands in
         the source image.
       - (dx_u, dx_v) indicate the source-image movement for each movement of
         x += 1 in the destination.
       - (dy_u, dy_v) indicate the source-image movement for each movement of
         y += 1 in the destination.
       All of these values are specified as 16:16 fixed-point, ie. they encode
       decimal values by multiplying them by 65536. */
    int u, v, dx_u, dx_v, dy_u, dy_v;
};

void image_linear(image_t const *src, image_t *dst,
    struct image_linear_map *map);
image_t *image_linear_alloc(image_t const *src,
    struct image_linear_map *map);

/* image_scale(): Upscale or downscale an image

   This function generates a linear map to be used in image_linear() to scale
   the input image. The scaling factor gamma can be specified independently for
   the x and y dimensions. It is expressed as 16:16 fixed-point; you can set
   any decimal value multiplied by 65536, for instance 1.5*65536 to increase
   the width and height by 50%. */
void image_scale(image_t const *src, int gamma_x, int gamma_y,
    struct image_linear_map *map);

/* image_rotate(): Rotate an image around its center

   This function generates a linear map to be used in image_linear() to perform
   a rotation around the center of an image. If [resize=true], the target is
   enlarged to make sure all the rotated pixels can be represented. This can
   increase the final surface by a factor of up to 2. If the original image
   doesn't extend to its corners, it is recommended to leave [resize=false] as
   it noticeably affects performance. */
void image_rotate(image_t const *src, float angle, bool resize,
    struct image_linear_map *map);

/* image_rotate_around(): Rotate an image around any point

   This function generalizes image_rotate() by allowing rotations around any
   center, even a point not within the image. The center is specified through
   two coordinates (*center_x, *center_y). If the center is near the side of
   the image, a normal rotation would move most of the pixels out of frame;
   this function moves the frame to make sure the whole image remains visible.
   *center_x and *center_y are updated to indicate the position of the center
   of rotation within the new frame (the target image). */
void image_rotate_around(image_t const *src, float angle, bool resize,
    int *center_x, int *center_y, struct image_linear_map *map);

/* image_rotate_around_scale(): Rotate an image around any point and scale it

   This function generalizes image_rotate_around() by adding a scaling factor
   to the transformation. The scaling factor gamma is expressed as 16:16
   fixed-point. If [resize=true] the image is further extended to make sure no
   parts are cut out, as in other rotation functions. */
void image_rotate_around_scale(
    image_t const *src, float angle, int gamma,
    bool resize, int *center_x, int *center_y,
    struct image_linear_map *map);

//---
// Color transforms
//---

/* TODO: Color transforms */

//---
// Image rendering functions
//
// The following functions extend dimage() and dsubimage(). The [effects]
// parameter takes a combination of IMAGE_* flags and effects, limited to the
// combinations previously described, with additional arguments depending on
// the color effect being applied.
//
//   dimage_effect(x, y, img, effects, ...)
//   dsubimage_effect(x, y, img, left, top, w, h, effects, ...)
//
// However if you use these super-generic functions you will link the code for
// all effects and all formats into your add-in, which takes a fair amount of
// space. If that's a problem, you can use the more specific functions below:
//
// * dimage_<FORMAT>_<EFFECT>() for one particular format (rgb16, p8, p4) along
//   with one particular color effect (clearbg, swapcolor, addbg, dye).
// * dimage_<FORMAT>() is like the above when no color effect is applied.
//
// All of them support the HFLIP and VFLIP flags. For effect-specific functions
// the corresponding effect flag can be omitted (fi. IMAGE_CLEARBG is implicit
// when using dimage_p8_clearbg()).
//---

/* dimage_effect(): Generalized dimage() supporting dynamic effects */
#define dimage_effect(x, y, img, eff, ...) \
    dsubimage_effect(x, y, img, 0, 0, (img)->width, (img)->height, eff, \
        ##__VA_ARGS__)
/* dsubimage_effect(): Generalized dsubimage() supporting dynamic effects */
void dsubimage_effect(int x, int y, image_t const *img,
    int left, int top, int w, int h, int effects, ...);

/* Specific versions for each format */
#define DIMAGE_SIG1(NAME, ...) \
    void dimage_ ## NAME(int x, int y, image_t const *img,##__VA_ARGS__); \
    void dsubimage_ ## NAME(int x, int y, image_t const *img, \
        int left, int top, int w, int h, ##__VA_ARGS__);
#define DIMAGE_SIG(NAME, ...) \
    DIMAGE_SIG1(rgb16 ## NAME, ##__VA_ARGS__) \
    DIMAGE_SIG1(p8 ## NAME, ##__VA_ARGS__) \
    DIMAGE_SIG1(p4 ## NAME, ##__VA_ARGS__)

/* d[sub]image_{rgb16,p8,p4}_effect(..., effects, <extra arguments>) */
DIMAGE_SIG(_effect, int effects, ...)
/* d[sub]image_{rgb16,p8,p4}(..., effects) (no color effect, like dimage()) */
DIMAGE_SIG(, int effects)
/* d[sub]image_{rgb16,p8,p4}_clearbg(..., effects, bg_color_or_index) */
DIMAGE_SIG(_clearbg, int effects, int bg_color_or_index)
/* d[sub]image_{rgb16,p8,p4}_swapcolor(..., effects, source, replacement) */
DIMAGE_SIG(_swapcolor, int effects, int source, int replacement)
/* d[sub]image_{rgb16,p8,p4}_addbg(..., effects, bg_color) */
DIMAGE_SIG(_addbg, int effects, int bg_color)
/* d[sub]image_{rgb16,p8,p4}_dye(..., effects, dye_color) */
DIMAGE_SIG(_dye, int effects, int dye_color)

/* d[sub]image_p4_clearbg_alt(..., effects, bg_index)
   This is functionally identical to CLEARBG, but it uses an alternative
   rendering method that is faster for larger images with wide transparent
   areas. You can swap it with the normal CLEARBG freely. */
DIMAGE_SIG1(p4_clearbg_alt, int effects, int bg_index)

#define dimage_rgb16_effect(x, y, img, eff, ...) \
    dsubimage_rgb16_effect(x, y, img, 0, 0, (img)->width, (img)->height, \
        eff, ##__VA_ARGS__)
#define dimage_p8_effect(x, y, img, eff, ...) \
    dsubimage_p8_effect(x, y, img, 0, 0, (img)->width, (img)->height, \
        eff, ##__VA_ARGS__)
#define dimage_p4_effect(x, y, img, eff, ...) \
    dsubimage_p4_effect(x, y, img, 0, 0, (img)->width, (img)->height, \
        eff, ##__VA_ARGS__)

#undef DIMAGE_SIG
#undef DIMAGE_SIG1

//---
// Clipping utilities
//---

/* Double box specifying both a source and target area */
struct gint_image_box
{
    /* Target location of top-left corner */
    int x, y;
    /* Width and height of rendered sub-image */
    int w, h;
    /* Source bounding box (low included, high excluded) */
    int left, top;
};

/* Clip the provided box against the input. If, after clipping, the box no
   longer intersects the output window, returns false. Otherwise, returns
   true. */
bool gint_image_clip_input(image_t const *img, struct gint_image_box *box,
    struct dwindow const *window);

/* Clip the provided box against the output. */
void gint_image_clip_output(struct gint_image_box *b,
   struct dwindow const *window);

//---
// Internal image rendering routines
//
// The following functions (or non-functions) are implemented in assembler and
// make up the internal interface of the image renderer. If you just want to
// display images, use dimage() and variations; these are only useful if you
// have a different rendering system and wish to use image rendering with
// dynamic effects in it.
//---

/* Renderer command. This structure includes most of the information used by
   the image renderer to perform blits. Some of the information on the target
   is also passed as direct arguments, which is more convenient and slightly
   faster.

   Most of the values here can be set with gint_image_mkcmd(). The last two
   members, along with the return values of the gint_image_FORMAT_loop()
   functions, are used to update the command if one needs to draw *parts* of
   the image and resume the rendering later. This is used in Azur. */
struct gint_image_cmd
{
    /* Shader ID. This is used in Azur, and ignored in gint */
    uint8_t shader_id;
    /* Dynamic effects not already dispatched by renderer
        Bit 0: VFLIP
        Bit 1: HFLIP */
    uint8_t effect;

    /* Number of pixels to render per line. For formats that force either x
       or width alignment (most of them), this is already adjusted to a
       suitable multiple (usually a multiple of 2). */
    int16_t columns;

    /* Stride of the input image (number of pixels between each row), in
       pixels, without subtracting the number of columns */
    int16_t input_stride;

    /* Number of lines in the command. This can be adjusted freely, and is
       particularly useful in Azur for fragmented rendering. */
    uint8_t lines;

    /* [Any effect]: Offset of first edge */
    int8_t edge_1;

    /* Core loop; this is an internal label of the renderer */
    void const *loop;
    /* Output pixel array, offset by target x/y */
    void const *output;
    /* Input pixel array, offset by source x/y. For formats that force x
       alignment, this is already adjusted. */
    void const *input;
    /* Palette, when applicable */
    uint16_t const *palette;

    /* [Any effect]: Offset of right edge */
    int16_t edge_2;
    /* [CLEARBG, SWAPCOLOR]: Source color */
    uint16_t color_1;
    /* [SWAPCOLOR]: Destination color */
    uint16_t color_2;

    /* Remaining height (for updates between fragments) */
    int16_t height;
    /* Local x position (for updates between fragments) */
    int16_t x;
};

/* gint_image_mkcmd(): Prepare a rendering command with dynamic effects

   This function crafts an image renderer command. It loads all the settings
   except for effect-dependent parameters: the [.loop] label, the color section
   of [.effect], and color effect settings. See the effect-specific functions
   to see how they are defined.

   The benefit of this approach is that the rendering code does not need to be
   linked in unless an effect is actually used, which avoids blowing up the
   size of the add-in as the number of support dynamic effects increases.

   @box         Requested on-screen box (will be clipped depending on effects)
   @img         Source image
   @effects     Set of dynamic effects to be applied, as an [IMAGE_*] bitmask
   @left_edge   Whether to force 2-alignment on the input (box->left)
   @right_edge  Whether to force 2-alignment on the width
   @cmd         Command to be filled
   @window      Rendering window (usually {0, 0, DWIDTH, DHEIGHT})

   Returns false if there is nothing to render because of clipping (in which
   case [cmd] is unchanged), true otherwise. [*box] is also updated to reflect
   the final box after clipping but not accounting for edges.  */
bool gint_image_mkcmd(struct gint_image_box *box, image_t const *img,
    int effects, bool left_edge, bool right_edge,
    struct gint_image_cmd *cmd, struct dwindow const *window);

/* Entry point of the renderers. These functions can be called normally as long
   as you can build the commands (eg. by using gint_image_mkcmd() then filling
   the effect-specific information). */
void *gint_image_rgb16_loop  (int output_width, struct gint_image_cmd *cmd);
void *gint_image_p8_loop     (int output_width, struct gint_image_cmd *cmd);
void *gint_image_p4_loop     (int output_width, struct gint_image_cmd *cmd);

/* Renderer fragments. The following can absolutely not be called from C code
   as they aren't full functions (and this isn't their prototype). These are
   continuations to be specified in the [.loop] field of a command before using
   one of the functions above. */

void gint_image_rgb16_normal(void);
void gint_image_rgb16_clearbg(void);
void gint_image_rgb16_swapcolor(void);
void gint_image_rgb16_dye(void);

void gint_image_p8_normal(void);
void gint_image_p8_clearbg(void);
void gint_image_p8_swapcolor(void);
void gint_image_p8_dye(void);

void gint_image_p4_normal(void);
void gint_image_p4_clearbg(void);
void gint_image_p4_clearbg_alt(void);
void gint_image_p4_swapcolor(void);
void gint_image_p4_dye(void);

//---
// Image library utilities
//
// The following functions and macros are mostly internal utilities; they are
// exposed here in case user applications want to extend the set of image
// transforms with custom additions.
//---

/* image_target(): Check if an image can be used as target for a transform

   This function is used to quickly check whether a transform from [src] to
   [dst] is possible. It requires image_valid(src) and image_valid(dst), plus
   any optional constraints specified as variadic arguments. These constraints
   can be:

   * NOT_P4: fails if [dst] is P4.
   * DATA_RW: fails if [dst] is not data-writable.
   * PALETTE_RW: fails if [dst] is not palette-writable.
   * SAME_SIZE: fails if [dst] is not at least as large as [src].

   For example, in image_hflip(), we write:
     if(!image_target(src, dst, NOT_P4, DATA_RW, SAME_SIZE)) return; */

enum {
    IMAGE_TARGET_NONE,
    IMAGE_TARGET_NOT_P4,
    IMAGE_TARGET_DATA_RW,
    IMAGE_TARGET_PALETTE_RW,
    IMAGE_TARGET_SAME_SIZE,
    IMAGE_TARGET_SAME_FORMAT,
    IMAGE_TARGET_SAME_DEPTH,
};
bool image_target(image_t const *src, image_t *dst, ...);

#define image_target(src, dst, ...) \
    image_target(src, dst, image_target_arg1(__VA_ARGS__ __VA_OPT__(,) NONE))
#define image_target_arg1(c, ...) \
    IMAGE_TARGET_ ## c __VA_OPT__(, image_target_arg2(__VA_ARGS__))
#define image_target_arg2(c, ...) \
    IMAGE_TARGET_ ## c __VA_OPT__(, image_target_arg3(__VA_ARGS__))
#define image_target_arg3(c, ...) \
    IMAGE_TARGET_ ## c __VA_OPT__(, image_target_arg4(__VA_ARGS__))
#define image_target_arg4(c, ...) \
    IMAGE_TARGET_ ## c __VA_OPT__(, image_target_arg5(__VA_ARGS__))
#define image_target_arg5(c, ...) \
    IMAGE_TARGET_ ## c __VA_OPT__(, image_target_arg6(__VA_ARGS__))
#define image_target_arg6(c, ...) \
    IMAGE_TARGET_ ## c __VA_OPT__(, image_target_too_many_args(__VA_ARGS__))

/* image_alpha_2(): Conditional alpha */
#define image_alpha_2(fmt, copy_alpha) \
        ((copy_alpha) ? 0x10000 : image_alpha(fmt))

#endif /* GINT_RENDER_RGB */

#ifdef __cplusplus
}
#endif

#endif /* GINT_IMAGE */
