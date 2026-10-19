#include <gint/video.h>
#include <gint/config.h>

// TODO: video: Have interface set by gint pre-main call instead
extern video_interface_t t6k11_video, r61524_video, r61523_video;
static video_interface_t const *current_intf =
    GINT_HW_SWITCH(NULL, &r61524_video, &r61523_video);
static int current_mode_index = GINT_HW_SWITCH(-1, 0, 0);

video_interface_t const *video_get_current_interface(void)
{
    return current_intf;
}

int video_get_current_mode_index(void)
{
    return current_mode_index;
}

video_mode_t const *video_get_current_mode(void)
{
    return (current_intf && current_mode_index >= 0)
           ? &current_intf->modes[current_mode_index]
           : NULL;
}

bool video_update(int x, int y, image_t const *fb, int flags)
{
    video_mode_t const *M = video_get_current_mode();

    if(!M || !current_intf->update || fb->format != M->format)
        return false;
    if(x < 0 || y < 0)
        return false;
    if(x + fb->width > M->width || y + fb->height > M->height)
        return false;

    return current_intf->update(x, y, fb, flags);
}
