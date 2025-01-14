/* platform-agnostic WGPU demo scaffold functions */

#include "sokol_gfx.h"
#include "wgpu_entry.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

static wgpu_state_t state;

#define wgpu_def(val, def) ((val==0)?def:val)

void wgpu_start(const wgpu_desc_t* desc) {
    assert(desc);
    assert(desc->title);
    assert((desc->width > 0) && (desc->height > 0));
    assert(desc->init_cb && desc->frame_cb && desc->shutdown_cb);

    state.desc = *desc;
    state.width = state.desc.width;
    state.height = state.desc.height;
    state.desc.sample_count = wgpu_def(state.desc.sample_count, 1);

    wgpu_platform_start(&state);
}

int wgpu_width(void) {
    return state.width;
}

int wgpu_height(void) {
    return state.height;
}

void wgpu_key_down(wgpu_key_func fn) {
    state.key_down_cb = fn;
}

void wgpu_key_up(wgpu_key_func fn) {
    state.key_up_cb = fn;
}

void wgpu_char(wgpu_char_func fn) {
    state.char_cb = fn;
}

void wgpu_mouse_btn_down(wgpu_mouse_btn_func fn) {
    state.mouse_btn_down_cb = fn;
}

void wgpu_mouse_btn_up(wgpu_mouse_btn_func fn) {
    state.mouse_btn_up_cb = fn;
}

void wgpu_mouse_pos(wgpu_mouse_pos_func fn) {
    state.mouse_pos_cb = fn;
}

void wgpu_mouse_wheel(wgpu_mouse_wheel_func fn) {
    state.mouse_wheel_cb = fn;
}

static const void* wgpu_get_render_view(void* user_data) {
    assert((void*)0xABADF00D == user_data); (void)user_data;
    if (state.desc.sample_count > 1) {
        assert(state.msaa_view);
        return (const void*) state.msaa_view;
    } else {
        assert(state.swapchain_view);
        return (const void*) state.swapchain_view;
    }
}

static const void* wgpu_get_resolve_view(void* user_data) {
    assert((void*)0xABADF00D == user_data); (void)user_data;
    if (state.desc.sample_count > 1) {
        assert(state.swapchain_view);
        return (const void*) state.swapchain_view;
    } else {
        return 0;
    }
}

static const void* wgpu_get_depth_stencil_view(void* user_data) {
    assert((void*)0xABADF00D == user_data); (void)user_data;
    return (const void*) state.depth_stencil_view;
}

static sg_pixel_format wgpu_get_color_format(void) {
    switch (state.render_format) {
        case WGPUTextureFormat_RGBA8Unorm:  return SG_PIXELFORMAT_RGBA8;
        case WGPUTextureFormat_BGRA8Unorm:  return SG_PIXELFORMAT_BGRA8;
        default: return SG_PIXELFORMAT_NONE;
    }
}

sg_context_desc wgpu_get_context(void) {
    return (sg_context_desc) {
        .color_format = wgpu_get_color_format(),
        .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .sample_count = state.desc.sample_count,
        .wgpu = {
            .device = (const void*) state.device,
            .render_view_userdata_cb = wgpu_get_render_view,
            .resolve_view_userdata_cb = wgpu_get_resolve_view,
            .depth_stencil_view_userdata_cb = wgpu_get_depth_stencil_view,
            .user_data = (void*)0xABADF00D
        }
    };
}
