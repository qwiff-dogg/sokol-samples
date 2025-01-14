//------------------------------------------------------------------------------
//  blend-wgpu.c
//  Test/demonstrate blend modes.
//------------------------------------------------------------------------------
#include "wgpu_entry.h"
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#include "HandmadeMath.h"
#define SOKOL_IMPL
#define SOKOL_WGPU
#include "sokol_gfx.h"
#include "sokol_log.h"

#define SAMPLE_COUNT (4)
#define NUM_BLEND_FACTORS (15)

typedef struct {
    hmm_mat4 mvp;
} vs_params_t;

typedef struct {
    float tick;
} fs_params_t;

static struct {
    sg_bindings bind;;
    sg_pipeline pips[NUM_BLEND_FACTORS][NUM_BLEND_FACTORS];
    sg_pipeline bg_pip;
    float r;
    vs_params_t quad_vs_params;
    fs_params_t bg_fs_params;
    sg_pass_action pass_action;
} state;

static void init(void) {
    sg_setup(&(sg_desc){
        .pipeline_pool_size = NUM_BLEND_FACTORS * NUM_BLEND_FACTORS + 1,
        .context = wgpu_get_context(),
        .logger.func = slog_func,
    });

    // a quad vertex buffer
    float vertices[] = {
        // pos               color
        -1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.5f,
        +1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.5f,
        -1.0f, +1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.5f,
        +1.0f, +1.0f, 0.0f,  1.0f, 1.0f, 0.0f, 0.5f
    };
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices),
        .label = "quas-vbuf",
    });

    // a shader for the fullscreen background quad
    sg_shader bg_shd = sg_make_shader(&(sg_shader_desc){
        .vs = {
            .source =
                "@vertex fn main(@location(0) pos: vec2f) -> @builtin(position) vec4f {\n"
                "  return vec4f(pos, 0.5, 1.0);\n"
                "}\n",
        },
        .fs = {
            .uniform_blocks[0].size = sizeof(fs_params_t),
            .source =
                "struct fs_params {\n"
                "  tick: f32,\n"
                "}\n"
                "@group(0) @binding(4) var<uniform> in: fs_params;\n"
                "@fragment fn main(@builtin(position) frag_coord: vec4f) -> @location(0) vec4f {\n"
                "  let xy: vec2f = fract((frag_coord.xy - vec2f(in.tick)) / 50.0);\n"
                "  return vec4f(vec3f(xy.x * xy.y), 1.0);\n"
                "}\n"
        },
        .label = "bg-shader",
    });

    // a pipeline state object for rendering the background quad
    state.bg_pip = sg_make_pipeline(&(sg_pipeline_desc){
        // we use the same vertex buffer as for the colored 3D quads,
        // but only the first two floats from the position, need to
        // provide a stride to skip the gap to the next vertex
        .layout = {
            .buffers[0].stride = 28,
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .shader = bg_shd,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    });

    // a shader for the blended quads
    sg_shader quad_shd = sg_make_shader(&(sg_shader_desc){
        .vs = {
            .uniform_blocks[0].size = sizeof(vs_params_t),
            .source =
                "struct vs_params {\n"
                "  mvp: mat4x4f,\n"
                "}\n"
                "@group(0) @binding(0) var<uniform> in: vs_params;\n"
                "struct vs_out {\n"
                "  @builtin(position) pos: vec4f,\n"
                "  @location(0) color: vec4f,\n"
                "}\n"
                "@vertex fn main(@location(0) pos: vec4f, @location(1) color: vec4f) -> vs_out {\n"
                "  var out: vs_out;\n"
                "  out.pos = in.mvp * pos;\n"
                "  out.color = color;\n"
                "  return out;\n"
                "}\n",
        },
        .fs = {
            .source =
                "@fragment fn main(@location(0) color: vec4f) -> @location(0) vec4f {\n"
                "  return color;\n"
                "}\n",
        },
    });

    // one pipeline object per blend-factor combination
    sg_pipeline_desc pip_desc = {
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT3,
                [1].format = SG_VERTEXFORMAT_FLOAT4
            }
        },
        .shader = quad_shd,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .blend_color = { 1.0f, 0.0f, 0.0f, 1.0f },
    };
    for (int src = 0; src < NUM_BLEND_FACTORS; src++) {
        for (int dst = 0; dst < NUM_BLEND_FACTORS; dst++) {
            const sg_blend_factor src_blend = (sg_blend_factor) (src+1);
            const sg_blend_factor dst_blend = (sg_blend_factor) (dst+1);
            // WebGL exceptions:
            //  - "GL_SRC_ALPHA_SATURATE as a destination blend function is disallowed in WebGL 1"
            //  - "constant color and constant alpha cannot be used together as source and
            //     destination factors in the blend function"
            bool valid = true;
            //if (dst_blend == SG_BLENDFACTOR_SRC_ALPHA_SATURATED) {
            //    valid = false;
            //}
            //else if ((src_blend == SG_BLENDFACTOR_BLEND_COLOR) || (src_blend == SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR)) {
            //    if ((dst_blend == SG_BLENDFACTOR_BLEND_ALPHA) || (dst_blend == SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA)) {
            //        valid = false;
            //    }
            //}
            //else if ((src_blend == SG_BLENDFACTOR_BLEND_ALPHA) || (src_blend == SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA)) {
            //    if ((dst_blend == SG_BLENDFACTOR_BLEND_COLOR) || (dst_blend == SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR)) {
            //        valid = false;
            //    }
            //}
            if (valid) {
                pip_desc.colors[0].blend = (sg_blend_state) {
                    .enabled = true,
                    .src_factor_rgb = src_blend,
                    .dst_factor_rgb = dst_blend,
                    .src_factor_alpha = SG_BLENDFACTOR_ONE,
                    .dst_factor_alpha = SG_BLENDFACTOR_ZERO
                };
                state.pips[src][dst] = sg_make_pipeline(&pip_desc);
                assert(state.pips[src][dst].id != SG_INVALID_ID);
            }
            else {
                state.pips[src][dst].id = SG_INVALID_ID;
            }
        }
    }
}

static void frame(void) {
    // view-projection matrix
    hmm_mat4 proj = HMM_Perspective(90.0f, (float)wgpu_width()/(float)wgpu_height(), 0.01f, 100.0f);
    hmm_mat4 view = HMM_LookAt(HMM_Vec3(0.0f, 0.0f, 25.0f), HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f));
    hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);

    // start rendering
    sg_begin_default_pass(&state.pass_action, wgpu_width(), wgpu_height());

    // draw a background quad
    sg_apply_pipeline(state.bg_pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &SG_RANGE(state.bg_fs_params));
    sg_draw(0, 4, 1);

    // draw the blended quads
    float r0 = state.r;
    for (int src = 0; src < NUM_BLEND_FACTORS; src++) {
        for (int dst = 0; dst < NUM_BLEND_FACTORS; dst++, r0+=0.6f) {
            if (state.pips[src][dst].id != SG_INVALID_ID) {
                // compute new model-view-proj matrix
                hmm_mat4 rm = HMM_Rotate(r0, HMM_Vec3(0.0f, 1.0f, 0.0f));
                const float x = ((float)(dst - NUM_BLEND_FACTORS/2)) * 3.0f;
                const float y = ((float)(src - NUM_BLEND_FACTORS/2)) * 2.2f;
                hmm_mat4 model = HMM_MultiplyMat4(HMM_Translate(HMM_Vec3(x, y, 0.0f)), rm);
                state.quad_vs_params.mvp = HMM_MultiplyMat4(view_proj, model);
                sg_apply_pipeline(state.pips[src][dst]);
                sg_apply_bindings(&state.bind);
                sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(state.quad_vs_params));
                sg_draw(0, 4, 1);
            }
        }
    }
    sg_end_pass();
    sg_commit();
    state.r += 0.6f;
    state.bg_fs_params.tick += 1.0f;
}

static void shutdown(void) {
    sg_shutdown();
}

int main() {
    wgpu_start(&(wgpu_desc_t){
        .init_cb = init,
        .frame_cb = frame,
        .shutdown_cb = shutdown,
        .sample_count = SAMPLE_COUNT,
        .width = 640,
        .height = 480,
        .title = "blend-wgpu"
    });
    return 0;
}
