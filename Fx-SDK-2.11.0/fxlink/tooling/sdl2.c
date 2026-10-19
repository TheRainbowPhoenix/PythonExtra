//---------------------------------------------------------------------------//
// ==>/[_]\   fxlink: A community communication tool for CASIO calculators.  //
//    |:::|   Made by Lephe' as part of the fxSDK.                           //
//    \___/   License: MIT <https://opensource.org/licenses/MIT>             //
//---------------------------------------------------------------------------//

#include <fxlink/tooling/sdl2.h>
#include <fxlink/logging.h>

#ifdef FXLINK_DISABLE_SDL2

void fxlink_sdl2_display_raw(struct fxlink_message_image_raw const *raw)
{
    log_("SDL2 disabled at compiled-time, skipping frame");
}

void fxlink_sdl2_handle_events(void)
{
}

#else /* FXLINK_DISABLE_SDL2 */

static SDL_Window *window = NULL;

static int init(size_t width, size_t height)
{
    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        int rc = SDL_Init(SDL_INIT_VIDEO);
        if(rc < 0)
            return elog("cannot initialize SDL: %s\n", SDL_GetError());
    }

    window = SDL_CreateWindow("fxlink", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, width, height, 0);
    return 0;
}

__attribute__((destructor))
static void quit(void)
{
    if(!window)
        return;
    SDL_DestroyWindow(window);
    window = NULL;
}

/* Generate an RGB888 surface from image data. */
static SDL_Surface *surface_for_image(uint8_t **RGB888_rows, int width,
    int height, int scale)
{
    /* Little endian setup for RGB */
    SDL_Surface *s = SDL_CreateRGBSurface(0, width*scale, height*scale, 24,
        0x000000ff, 0x0000ff00, 0x0000ff00, 0);
    if(!s) {
        elog("cannot create surface for image\n");
        return NULL;
    }

    for(int y = 0; y < height; y++) {
        for(int dy = 0; dy < scale; dy++) {
            uint8_t *src = RGB888_rows[y];
            uint8_t *dst = s->pixels + (y*scale+dy) * s->pitch;
            for(int x = 0; x < width; x++) {
                for(int dx = 0; dx < scale; dx++) {
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst += 3;
                }
                src += 3;
            }
        }
    }

    return s;
}

void fxlink_sdl2_display_raw(struct fxlink_message_image_raw const *raw)
{
    if(!window && init(raw->width, raw->height))
        return;

    int current_w, current_h;
    int scale = 1;

    SDL_GetWindowSize(window, &current_w, &current_h);
    if(current_w != raw->width * scale || current_h != raw->height * scale)
        SDL_SetWindowSize(window, raw->width * scale, raw->height * scale);

    SDL_Surface *src =
        surface_for_image(raw->data, raw->width, raw->height, scale);
    if(!src)
        return;

    SDL_Surface *dst = SDL_GetWindowSurface(window);
    SDL_BlitSurface(src, NULL, dst, NULL);
    SDL_FreeSurface(src);

    SDL_UpdateWindowSurface(window);
}

void fxlink_sdl2_handle_events(void)
{
    if(!window)
        return;

    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT)
            quit();
    }
}

#endif /* FXLINK_DISABLE_SDL2 */
