/* -*- c++ -*- */
/* + + +   This Software is released under the "Simplified BSD License"  + + +
 * Copyright 2010 Moe Wheatley. All rights reserved.
 * Copyright 2011-2013 Alexandru Csete OZ9AEC
 * Copyright 2018 Gonzalo José Carracedo Carballal - Minimal modifications for integration
 * Copyright 2024 Sultan Qasim Khan - Abstract class for OpenGL and QPainter waterfalls
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Moe Wheatley.
 */

#include <cmath>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QtGlobal>
#include <QToolTip>
#include <QDebug>
#include <QApplication>
#include <cstring>

// Apple deprecated OpenGL, I don't need to be warned, but for now it still works
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include "SuWidgetsHelpers.h"
#include "GLWaterfall.h"
#include "gradient.h"

// Comment out to enable plotter debug messages
//#define PLOTTER_DEBUG

#define GL_WATERFALL_TEX_MIN_DB  (-300.f)
#define GL_WATERFALL_TEX_MAX_DB  (200.f)
#define GL_WATERFALL_TEX_DR      (GL_WATERFALL_TEX_MAX_DB - GL_WATERFALL_TEX_MIN_DB)
#define GL_WATERFALL_MAX_LINE_POOL_SIZE 30
#define GL_WATERFALL_MIN_BULK_TRANSFER  10

struct vertex {
  float vertex_coords[3];
  float texture_coords[2];
};

static const vertex vertices[] =
{
  {{1, 1,   0},  {1.0, 0.0}},
  {{1, -1,    0},  {1.0, 1.0}},
  {{-1, -1,   0},  {0.0, 1.0}},
  {{-1, 1, 0},  {0.0, 0.0}}
};

static const unsigned short vertex_indices[] = {0, 1, 2, 2, 3, 0};

static const char *wfVertexShader   = "                                        \
                                       attribute vec3 vertex_coords;                                              \
                                       attribute vec2 texture_coords;                                             \
                                       varying   vec2 f_texture_coords;                                           \
                                       uniform   mat4 ortho;                                                      \
                                       \
                                     void                                                                       \
                                       main()                                                                 \
{                                                                          \
  gl_Position      = ortho * vec4(vertex_coords, 1.0);                    \
    f_texture_coords = texture_coords;                                      \
}                                                                          \
  ";

static const char *wfFragmentShader = "                                        \
                                       varying vec2      f_texture_coords;                                        \
                                       uniform sampler2D m_texture;                                               \
                                       uniform sampler2D m_palette;                                               \
                                       uniform float     t;                                                       \
                                       uniform float     x0;                                                      \
                                       uniform float     m;                                                       \
                                       uniform float     c_x0;                                                    \
                                       uniform float     c_m;                                                     \
                                       \
                                     void                                                                       \
                                       main()                                                                 \
{                                                                          \
  float x = f_texture_coords.x * c_m + c_x0;                               \
    float y = f_texture_coords.y + t - floor(f_texture_coords.y + t);        \
    vec2 coord = vec2(x, y);                                                 \
    \
    vec4 psd = texture2D(m_texture, coord);                                  \
    float paletteIndex = (psd.r - x0) / m;                                   \
    vec4 palColor       = texture2D(m_palette, vec2(paletteIndex, 0));       \
    \
    gl_FragColor = palColor;                                                 \
}                                                                          \
  ";

///////////////////////////// GLLine //////////////////////////////////////////
void
GLLine::normalize()
{
  int i;
  int res = resolution();
  float *data = this->data();

#pragma GCC ivdep
  for (i = 0; i < res; ++i)
    data[i] = (data[i] - GL_WATERFALL_TEX_MIN_DB) / GL_WATERFALL_TEX_DR;
}

void
GLLine::rescaleMean()
{
  float *data = this->data();
  int res = resolution();
  int i = 0, q = 0, p = res;
  int l = m_levels;

  normalize();

  while (l-- > 0) {
#pragma GCC unroll 4
    for (i = 0; i < res; i += 2) {
      data[p++] = .5f * (data[q] + data[q + 1]);
      q += 2;
    }

    res >>= 1;
  }
}

void
GLLine::rescaleMax()
{
  float *data = this->data();
  int res = resolution();
  int i = 0, q = 0, p = res;
  int l = m_levels;

  normalize();

  while (l-- > 0) {
#pragma GCC unroll 4
    for (i = 0; i < res; i += 2) {
      data[p++] = fmaxf(data[q], data[q + 1]);
      q += 2;
    }

    res >>= 1;
  }
}

void
GLLine::assignMean(const float *values)
{
  float *data = this->data();
  int res = resolution();

  memcpy(data, values, sizeof(float) * res);

  rescaleMean();
}

void
GLLine::assignMax(const float *values)
{
  float *data = this->data();
  int res = resolution();

  memcpy(data, values, sizeof(float) * res);

  rescaleMax();
}


void
GLLine::reduceMean(const float *values, int length)
{
  int res       = resolution();
  int chunkSize = length / res;
  float *data   = this->data();
  float k       = 1. / chunkSize;

  if (chunkSize > 0) {
    int i, j, p = 0;

#pragma GCC unroll 4
    for (i = 0; i < length; i += chunkSize) {
      float val = 0;
      for (j = 0; j < chunkSize; ++j)
        val += k * values[i + j];
      data[p++] = val;
    }

    rescaleMean();
  }
}

void
GLLine::reduceMax(const float *values, int length)
{
  int res       = resolution();
  int chunkSize = length / res;
  float *data   = this->data();

  if (chunkSize > 0) {
    int i, p = 0;

#pragma GCC unroll 4
    for (i = 0; i < length; i += chunkSize) {
      float max = -INFINITY;
      int j;
      for (j = 0; j < chunkSize; ++j)
        if (values[i + j] > max)
          max = values[i + j];

      data[p++] = max;
    }

    rescaleMax();
  }
}

/////////////////////// GLWaterfallOpenGLContext //////////////////////////////
GLWaterfallOpenGLContext::GLWaterfallOpenGLContext() :
  m_vbo(QOpenGLBuffer::VertexBuffer),
  m_ibo(QOpenGLBuffer::IndexBuffer)
{
  auto screens = QGuiApplication::screens();
  int maxHeight = 0;

  for (auto p : screens) {
    int h = p->geometry().height() * p->devicePixelRatio();
    if (h > maxHeight)
      maxHeight = h;
  }

  m_paletBuf.resize(256 * 4);
  m_rowCount = maxHeight;
}

GLWaterfallOpenGLContext::~GLWaterfallOpenGLContext()
{
  finalize();

  delete m_vertexShader;
  delete m_fragmentShader;
  delete m_waterfall;
  delete m_palette;
  delete m_functions;
}

void
GLWaterfallOpenGLContext::initialize()
{
  GLint texSize;
  QImage firstPal = QImage(256, 1, QImage::Format_RGBX8888);
  m_paletBuf.resize(256);

  for (int i = 0; i < 256; ++i) {
    firstPal.setPixel(
        i,
        0,
        qRgba(
          255 * wf_gradient[i][0],
          255 * wf_gradient[i][1],
          255 * wf_gradient[i][2],
          255));
  }

  if (m_functions == nullptr)
    m_functions = new QOpenGLFunctions(QOpenGLContext::currentContext());

  // Retrieve limits
  m_functions->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
  m_maxRowSize = GLLine::resolutionFor(texSize);

  if (m_rowCount > m_maxRowSize)
    m_rowCount = m_maxRowSize;

  if (m_rowSize > m_maxRowSize)
    m_rowSize = m_maxRowSize;

  m_functions->glEnable(GL_DEPTH_TEST);

#ifdef QT_OPENGL_3
  m_functions->glEnable(GL_MULTISAMPLE);
  m_functions->glEnable(GL_LINE_SMOOTH);
  m_functions->glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  m_functions->glEnable(GL_POINT_SMOOTH);
  m_functions->glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
#endif // QT_OPENGL_3

  m_functions->glEnable(GL_BLEND);
  m_functions->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  m_vao.create();

  if (m_vao.isCreated())
    m_vao.bind();

  m_vbo.create();
  m_vbo.bind();
  m_vbo.allocate(vertices, sizeof(vertices));

  m_ibo.create();
  m_ibo.bind();
  m_ibo.allocate(vertex_indices, sizeof(vertex_indices));

  m_waterfall = new QOpenGLTexture(QOpenGLTexture::Target2D);
  resetWaterfall();

  m_palette = new QOpenGLTexture(QOpenGLTexture::Target2D);
  m_palette->setWrapMode(QOpenGLTexture::ClampToEdge);
  m_palette->setMinificationFilter(QOpenGLTexture::Linear);
  m_palette->setMagnificationFilter(QOpenGLTexture::Linear);
  m_palette->setSize(256, 1);
  m_palette->setData(firstPal);
  m_palette->create();

  m_vertexShader   = new QOpenGLShader(QOpenGLShader::Vertex);
  m_vertexShader->compileSourceCode(wfVertexShader);
  m_fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment);
  m_fragmentShader->compileSourceCode(wfFragmentShader);

  m_program.addShader(m_vertexShader);
  m_program.addShader(m_fragmentShader);
  m_program.link();
  m_program.bind();
}

void
GLWaterfallOpenGLContext::resetWaterfall()
{
  GLLine nullLine;

  nullLine.setResolution(m_rowSize);

  if (m_waterfall->isCreated())
    m_waterfall->destroy();

  m_waterfall->setAutoMipMapGenerationEnabled(true);
  m_waterfall->setSize(nullLine.allocation(), m_rowCount);
  m_waterfall->setFormat(QOpenGLTexture::TextureFormat::R16F);
  m_waterfall->setMinificationFilter(QOpenGLTexture::Linear);
  m_waterfall->setMagnificationFilter(QOpenGLTexture::Linear);
  m_waterfall->allocateStorage(
      QOpenGLTexture::PixelFormat::Red,
      QOpenGLTexture::PixelType::UInt32);
  m_waterfall->create();
  m_waterfall->bind(0);

  // Clear waterfall
  for (int i = 0; i < m_rowCount; ++i)
    m_functions->glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        i,
        nullLine.allocation(),
        1,
        GL_RED,
        GL_FLOAT,
        nullLine.data());

  m_row = 0;
}

void
GLWaterfallOpenGLContext::flushPalette()
{
  m_functions->glTexSubImage2D(
      GL_TEXTURE_2D,
      0,
      0,
      0,
      256,
      1,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      m_paletBuf.data());
}

void
GLWaterfallOpenGLContext::disposeLastLine()
{
  if (!m_history.empty()) {
    GLLine &line = m_history.back();

    // Can we reuse it?
    if (m_rowSize == line.resolution()
        && m_pool.size() < GL_WATERFALL_MAX_LINE_POOL_SIZE) {
      auto last = m_history.end();
      --last;
      m_pool.splice(m_pool.begin(), m_history, last);
    } else {
      m_history.pop_back();
    }
  }
}

void
GLWaterfallOpenGLContext::flushOneLine()
{
  GLLine &line = m_history.back();
  int row = m_rowCount - (m_row % m_rowCount) - 1;

  if (m_rowSize == line.resolution()) {
    m_functions->glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        row,
        line.allocation(),
        1,
        GL_RED,
        GL_FLOAT,
        line.data());
    disposeLastLine();
    m_row = (m_row + 1) % m_rowCount;
  } else {
    // Wrong line size. Just discard.
    disposeLastLine();
  }
}

void
GLWaterfallOpenGLContext::flushLinesBulk()
{
  int maxRows = m_rowCount - (m_row % m_rowCount);
  int count = 0;
  int alloc = GLLine::allocationFor(m_rowSize);
  std::vector<float> bulkData;

  bulkData.resize(maxRows * alloc);

  for (int i = 0; i < maxRows && !m_history.empty(); ++i) {
    GLLine &line = m_history.back();

    if (m_rowSize != line.resolution()) {
      disposeLastLine();
      break;
    }

    memcpy(
        bulkData.data() + (maxRows - i - 1) * alloc,
        line.data(),
        alloc * sizeof(float));
    disposeLastLine();

    ++count;
  }

  if (count > 0) {
    m_functions->glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        maxRows - count,
        alloc,
        count,
        GL_RED,
        GL_FLOAT,
        bulkData.data() + (maxRows - count) * alloc);
    m_row = (m_row + count) % m_rowCount;
  }
}

void
GLWaterfallOpenGLContext::flushLines()
{
  while (!m_history.empty()) {
    if (m_history.size() >= GL_WATERFALL_MIN_BULK_TRANSFER)
      flushLinesBulk();
    else
      flushOneLine();
  }
}

void
GLWaterfallOpenGLContext::flushLinePool()
{
  m_pool.clear();
}

void
GLWaterfallOpenGLContext::setPalette(const QColor *table)
{
  uint8_t *asBytes = reinterpret_cast<uint8_t *>(m_paletBuf.data());

  for (int i = 0; i < 256; ++i) {
    asBytes[4 * i + 0] = static_cast<uint8_t>(table[i].red());
    asBytes[4 * i + 1] = static_cast<uint8_t>(table[i].green());
    asBytes[4 * i + 2] = static_cast<uint8_t>(table[i].blue());
    asBytes[4 * i + 3] = 255;
  }

  m_updatePalette = true;
}

void
GLWaterfallOpenGLContext::pushFFTData(
    const float *__restrict__ fftData,
    int size)
{
  int dataSize = size;

  if (dataSize > m_maxRowSize)
    size = m_maxRowSize;

  if (size != m_rowSize) {
    flushLinePool();

    m_rowSize = size;
    resetWaterfall();
  }

  if (!m_pool.empty()) {
    auto last = m_pool.end();
    --last;
    m_history.splice(m_history.begin(), m_pool, last);
  } else {
    m_history.push_front(GLLine());
  }

  // If there are more lines than the ones that fit into the screen, we
  // simply discard the older ones
  if (m_history.size() > SCAST(unsigned, m_rowCount))
    m_history.pop_back();


  GLLine &line = m_history.front();
  line.setResolution(size);

  /////////////////// Set line data ////////////////////
  if (size == dataSize) {
    if (m_useMaxBlending)
      line.assignMax(fftData);
    else
      line.assignMean(fftData);
  } else {
    if (m_useMaxBlending)
      line.reduceMax(fftData, dataSize);
    else
      line.reduceMean(fftData, dataSize);
  }
}

void
GLWaterfallOpenGLContext::setDynamicRange(float mindB, float maxdB)
{
  m_m  = (maxdB - mindB) / GL_WATERFALL_TEX_DR;
  m_x0 = (mindB - GL_WATERFALL_TEX_MIN_DB) / GL_WATERFALL_TEX_DR;
}

void
GLWaterfallOpenGLContext::finalize()
{
  if (m_vao.isCreated())
    m_vao.destroy();

  m_vbo.destroy();

  if (m_waterfall != nullptr && m_waterfall->isCreated())
    m_waterfall->destroy();

  if (m_palette != nullptr && m_palette->isCreated())
    m_palette->destroy();
}

void
GLWaterfallOpenGLContext::recalcGeometric(int width, int height, float zoom)
{
  int d = static_cast<int>(floor(log2(m_rowSize / (width * zoom))));

  if (d < 0)
    d = 0;

  // Define the texture coordinates where the best mipmap can be found
  m_c_x0 = 1.f - 1.f / static_cast<float>(1 << d);
  m_c_x1 = 1.f - 1.f / static_cast<float>(1 << (d + 1));

  m_width  = width;
  m_height = height;
  m_zoom   = zoom;
}

void
GLWaterfallOpenGLContext::render(
    int x,
    int y,
    int width,
    int height,
    float left,
    float right)
{
  QMatrix4x4 ortho;
  float zoom = right - left;

  // We only care about horizontal zoom
  if (width != m_width || fabsf(zoom - m_zoom) > 1e-6f)
    recalcGeometric(width, height, zoom);


#ifdef QT_OPENGL_3
  glPushAttrib(GL_ALL_ATTRIB_BITS); // IMPORTANT TO PREVENT CONFLICTS WITH QPAINTER
#endif // QT_OPENGL_3

  m_program.bind(); // Must be the first

  m_functions->glViewport(x, height - m_rowCount - y, width, m_rowCount);

  m_functions->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  m_functions->glDisable(GL_CULL_FACE);

  ortho.translate(2 * left, 0);
  ortho.scale(zoom, 1);

  m_vbo.bind();
  m_ibo.bind();

  m_program.setAttributeBuffer(
      "vertex_coords",
      GL_FLOAT,
      0,
      3,
      sizeof(vertex));

  m_program.setAttributeBuffer(
      "texture_coords",
      GL_FLOAT,
      sizeof(vertices[0].vertex_coords),
      2,
      sizeof(vertex));

  m_program.enableAttributeArray("vertex_coords");
  m_program.enableAttributeArray("texture_coords");

  m_program.setUniformValue("ortho", ortho);
  m_program.setUniformValue("t", -m_row / (float) m_rowCount);
  m_program.setUniformValue("x0", m_x0);
  m_program.setUniformValue("m",  m_m);
  m_program.setUniformValue("c_x0", m_c_x0);
  m_program.setUniformValue("c_m",  m_c_x1 - m_c_x0);
  m_vao.release();
  m_vao.bind();

  // Add more lines (if necessary)
  m_waterfall->bind(0);
  flushLines();

  // Update palette (if necessary)
  m_palette->bind(1);
  if (m_updatePalette) {
    flushPalette();
    m_updatePalette = false;
  }

  m_program.setUniformValue("m_texture", 0);
  m_program.setUniformValue("m_palette", 1);

  m_functions->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

  m_vao.release();
  m_program.disableAttributeArray("vertexcoords");
  m_program.disableAttributeArray("texture_coords");

  // ALL THIS MUST BE RELEASED, OTHERWISE QPAINTER WILL NOT WORK
  m_program.release();
  m_waterfall->release();
  m_palette->release();
  m_vbo.release();
  m_ibo.release();

#ifdef QT_OPENGL_3
  glPopAttrib();
#endif // QT_OPENGL_3
}

///////////////////////////// GLWaterfall ////////////////////////////////////////
GLWaterfall::GLWaterfall(QWidget *parent) : AbstractWaterfall(parent)
{
}

GLWaterfall::~GLWaterfall()
{
  makeCurrent();
  m_glCtx.finalize();
  doneCurrent();
}

void
GLWaterfall::clearWaterfall()
{
}

/**
 * @brief Save waterfall to a graphics file
 * @param filename
 * @return TRUE if the save successful, FALSE if an erorr occurred.
 *
 * We assume that frequency strings are up to date
 */
bool
GLWaterfall::saveWaterfall(const QString &) const
{
  return false;
}

void
GLWaterfall::initializeGL()
{
  m_glCtx.initialize();

  connect(
      context(),
      SIGNAL(aboutToBeDestroyed()),
      this,
      SLOT(onContextBeingDestroyed()));

  drawOverlay();
}

void
GLWaterfall::onContextBeingDestroyed()
{
  makeCurrent();
  m_glCtx.finalize();
  doneCurrent();
}

void
GLWaterfall::setPalette(const QColor *table)
{
  m_glCtx.setPalette(table);
  update();
}

void
GLWaterfall::setMaxBlending(bool val)
{
  m_glCtx.m_useMaxBlending = val;
}

void GLWaterfall::setWaterfallRange(float min, float max)
{
  if (out_of_range(min, max))
    return;

  m_WfMindB = min;
  m_WfMaxdB = max;

  m_glCtx.setDynamicRange(min - m_gain, max - m_gain);

  // no overlay change is necessary
}

//
//   |---------f-------------------------|
// -fs/2       S                        fs/2
//        |---------|
//        f0       f1
//

void
GLWaterfall::paintGL()
{
  int y = m_Percent2DScreen * m_Size.height() / 100;
  qint64 f0 = m_FftCenter - m_Span / 2;
  qreal  left  = (qreal) ( - f0) / (qreal) m_Span - .5f;
  qreal  right = (qreal) (+m_SampleFreq - f0) / (qreal) m_Span - .5f;
  qreal  dpi_factor = screen()->devicePixelRatio();

  m_glCtx.render(0, y * dpi_factor, width() * dpi_factor,
      height() * dpi_factor, left, right);
}

void GLWaterfall::addNewWfLine(const float* wfData, int size, int repeats)
{
  makeCurrent();

  for (int i = 0; i < repeats; i++)
    m_glCtx.pushFFTData(wfData, size);
}
