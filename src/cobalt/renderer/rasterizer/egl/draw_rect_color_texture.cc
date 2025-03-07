// Copyright 2017 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cobalt/renderer/rasterizer/egl/draw_rect_color_texture.h"

#include <GLES2/gl2.h>

#include "base/basictypes.h"
#include "cobalt/renderer/backend/egl/utils.h"
#include "starboard/memory.h"

namespace cobalt {
namespace renderer {
namespace rasterizer {
namespace egl {

namespace {
struct VertexAttributes {
  float position[2];
  float texcoord[2];
  uint32_t color;
};
}  // namespace

DrawRectColorTexture::DrawRectColorTexture(
    GraphicsState* graphics_state, const BaseState& base_state,
    const math::RectF& rect, const render_tree::ColorRGBA& color,
    const backend::TextureEGL* texture,
    const math::Matrix3F& texcoord_transform, bool clamp_texcoords)
    : DrawObject(base_state),
      texcoord_transform_(texcoord_transform),
      color_transform_{},
      rect_(rect),
      color_(GetGLRGBA(GetDrawColor(color) * base_state_.opacity)),
      textures_{texture, NULL, NULL},
      vertex_buffer_(NULL),
      clamp_texcoords_(clamp_texcoords),
      tile_texture_(false) {
  DCHECK(textures_[0]);
  graphics_state->ReserveVertexData(4 * sizeof(VertexAttributes));
}

DrawRectColorTexture::DrawRectColorTexture(
    GraphicsState* graphics_state, const BaseState& base_state,
    const math::RectF& rect, const render_tree::ColorRGBA& color,
    const backend::TextureEGL* y_texture, const backend::TextureEGL* u_texture,
    const backend::TextureEGL* v_texture,
    const float (&color_transform_in_column_major)[16],
    const math::Matrix3F& texcoord_transform, bool clamp_texcoords)
    : DrawObject(base_state),
      texcoord_transform_(texcoord_transform),
      rect_(rect),
      color_(GetGLRGBA(GetDrawColor(color) * base_state_.opacity)),
      textures_{y_texture, u_texture, v_texture},
      vertex_buffer_(NULL),
      clamp_texcoords_(clamp_texcoords),
      tile_texture_(false) {
  DCHECK(textures_[0]);
  DCHECK(textures_[1]);
  DCHECK(textures_[2]);
  static_assert(
      sizeof(color_transform_) == sizeof(color_transform_in_column_major),
      "color_transform_ and color_transform_in_column_major size mismatch");

  SbMemoryCopy(color_transform_, color_transform_in_column_major,
               sizeof(color_transform_));
  graphics_state->ReserveVertexData(4 * sizeof(VertexAttributes));
}

void DrawRectColorTexture::ExecuteUpdateVertexBuffer(
    GraphicsState* graphics_state,
    ShaderProgramManager* program_manager) {
  VertexAttributes attributes[4] = {
    { { rect_.x(), rect_.bottom() },      // uv = (0,1)
      { texcoord_transform_(0, 1) + texcoord_transform_(0, 2),
        texcoord_transform_(1, 1) + texcoord_transform_(1, 2) }, color_ },
    { { rect_.right(), rect_.bottom() },  // uv = (1,1)
      { texcoord_transform_(0, 0) + texcoord_transform_(0, 1) +
          texcoord_transform_(0, 2),
        texcoord_transform_(1, 0) + texcoord_transform_(1, 1) +
          texcoord_transform_(1, 2) }, color_ },
    { { rect_.right(), rect_.y() },       // uv = (1,0)
      { texcoord_transform_(0, 0) + texcoord_transform_(0, 2),
        texcoord_transform_(1, 0) + texcoord_transform_(1, 2) }, color_ },
    { { rect_.x(), rect_.y() },           // uv = (0,0)
      { texcoord_transform_(0, 2), texcoord_transform_(1, 2) }, color_ },
  };
  COMPILE_ASSERT(sizeof(attributes) == 4 * sizeof(VertexAttributes),
                 bad_padding);
  vertex_buffer_ = graphics_state->AllocateVertexData(
      sizeof(attributes));
  SbMemoryCopy(vertex_buffer_, attributes, sizeof(attributes));

  // Find minimum and maximum texcoord values.
  texcoord_clamps_[0][0] = attributes[0].texcoord[0];
  texcoord_clamps_[0][1] = attributes[0].texcoord[1];
  texcoord_clamps_[0][2] = attributes[0].texcoord[0];
  texcoord_clamps_[0][3] = attributes[0].texcoord[1];
  for (int i = 1; i < arraysize(attributes); ++i) {
    float texcoord_u = attributes[i].texcoord[0];
    float texcoord_v = attributes[i].texcoord[1];
    if (texcoord_clamps_[0][0] > texcoord_u) {
      texcoord_clamps_[0][0] = texcoord_u;
    } else if (texcoord_clamps_[0][2] < texcoord_u) {
      texcoord_clamps_[0][2] = texcoord_u;
    }
    if (texcoord_clamps_[0][1] > texcoord_v) {
      texcoord_clamps_[0][1] = texcoord_v;
    } else if (texcoord_clamps_[0][3] < texcoord_v) {
      texcoord_clamps_[0][3] = texcoord_v;
    }
  }

  tile_texture_ =
      texcoord_clamps_[0][0] < 0.0f || texcoord_clamps_[0][1] < 0.0f ||
      texcoord_clamps_[0][2] > 1.0f || texcoord_clamps_[0][3] > 1.0f;

  for (int i = 1; i < SB_ARRAY_SIZE_INT(texcoord_clamps_); ++i) {
    SbMemoryCopy(texcoord_clamps_[i], texcoord_clamps_[0],
                 sizeof(texcoord_clamps_[0]));
  }
  if (clamp_texcoords_) {
    // Inset 0.5-epsilon so the border texels are still sampled, but nothing
    // beyond.
    const float kTexelInset = 0.499f;
    for (int i = 0; i < SB_ARRAY_SIZE_INT(texcoord_clamps_); ++i) {
      if (textures_[i] == NULL) {
        break;
      }
      texcoord_clamps_[i][0] += kTexelInset / textures_[i]->GetSize().width();
      texcoord_clamps_[i][1] += kTexelInset / textures_[i]->GetSize().height();
      texcoord_clamps_[i][2] -= kTexelInset / textures_[i]->GetSize().width();
      texcoord_clamps_[i][3] -= kTexelInset / textures_[i]->GetSize().height();
    }
  }
}

void DrawRectColorTexture::SetupVertexShader(
    GraphicsState* graphics_state,
    const ShaderVertexColorTexcoord& vertex_shader) {
  graphics_state->UpdateClipAdjustment(vertex_shader.u_clip_adjustment());
  graphics_state->UpdateTransformMatrix(vertex_shader.u_view_matrix(),
                                        base_state_.transform);
  graphics_state->Scissor(base_state_.scissor.x(), base_state_.scissor.y(),
                          base_state_.scissor.width(),
                          base_state_.scissor.height());
  graphics_state->VertexAttribPointer(
      vertex_shader.a_position(), 2, GL_FLOAT, GL_FALSE,
      sizeof(VertexAttributes),
      vertex_buffer_ + offsetof(VertexAttributes, position));
  graphics_state->VertexAttribPointer(
      vertex_shader.a_color(), 4, GL_UNSIGNED_BYTE, GL_TRUE,
      sizeof(VertexAttributes),
      vertex_buffer_ + offsetof(VertexAttributes, color));
  graphics_state->VertexAttribPointer(
      vertex_shader.a_texcoord(), 2, GL_FLOAT, GL_FALSE,
      sizeof(VertexAttributes),
      vertex_buffer_ + offsetof(VertexAttributes, texcoord));
  graphics_state->VertexAttribFinish();
}

template <typename FragmentShader>
void DrawRectColorTexture::SetupFragmentShaderAndDraw(
    GraphicsState* graphics_state, const FragmentShader& fragment_shader) {
  for (int i = 0; i < SB_ARRAY_SIZE_INT(textures_); ++i) {
    if (textures_[i] == NULL) {
      break;
    }

    GL_CALL(glUniform4fv(fragment_shader.u_texcoord_clamp(i), 1,
                         texcoord_clamps_[i]));
    if (tile_texture_) {
      graphics_state->ActiveBindTexture(fragment_shader.u_texture_texunit(i),
                                        textures_[i]->GetTarget(),
                                        textures_[i]->gl_handle(), GL_REPEAT);
    } else {
      graphics_state->ActiveBindTexture(fragment_shader.u_texture_texunit(i),
                                        textures_[i]->GetTarget(),
                                        textures_[i]->gl_handle());
    }
  }

  GL_CALL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

  if (!tile_texture_) {
    return;
  }

  for (int i = 0; i < SB_ARRAY_SIZE_INT(textures_); ++i) {
    if (textures_[i] == NULL) {
      break;
    }

    graphics_state->ActiveBindTexture(
        fragment_shader.u_texture_texunit(i), textures_[i]->GetTarget(),
        textures_[i]->gl_handle(), GL_CLAMP_TO_EDGE);
  }
}

void DrawRectColorTexture::ExecuteRasterize(
    GraphicsState* graphics_state, ShaderProgramManager* program_manager) {
  if (textures_[1] == NULL) {
    ShaderProgram<ShaderVertexColorTexcoord, ShaderFragmentColorTexcoord>*
        program;
    program_manager->GetProgram(&program);
    graphics_state->UseProgram(program->GetHandle());
    SetupVertexShader(graphics_state, program->GetVertexShader());
    SetupFragmentShaderAndDraw(graphics_state, program->GetFragmentShader());
  } else {
    ShaderProgram<ShaderVertexColorTexcoord, ShaderFragmentColorTexcoordYuv3>*
        program;
    program_manager->GetProgram(&program);
    graphics_state->UseProgram(program->GetHandle());
    SetupVertexShader(graphics_state, program->GetVertexShader());
    GL_CALL(glUniformMatrix4fv(
        program->GetFragmentShader().u_color_transform_matrix(), 1, GL_FALSE,
        color_transform_));
    SetupFragmentShaderAndDraw(graphics_state, program->GetFragmentShader());
  }
}

base::TypeId DrawRectColorTexture::GetTypeId() const {
  return textures_[1] == NULL
             ? ShaderProgram<ShaderVertexColorTexcoord,
                             ShaderFragmentColorTexcoord>::GetTypeId()
             : ShaderProgram<ShaderVertexColorTexcoord,
                             ShaderFragmentColorTexcoordYuv3>::GetTypeId();
}

}  // namespace egl
}  // namespace rasterizer
}  // namespace renderer
}  // namespace cobalt
