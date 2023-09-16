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
                                       main(void)                                                                 \
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
                                       main(void)                                                                 \
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
GLLine::normalize(void)
{
  int i;
  int res = resolution();
  float *data = this->data();

#pragma GCC ivdep
  for (i = 0; i < res; ++i)
    data[i] = (data[i] - GL_WATERFALL_TEX_MIN_DB) / GL_WATERFALL_TEX_DR;

}

  void
GLLine::rescaleMean(void)
{
  float *data = this->data();
  int res = resolution();
  int i = 0, q = 0, p = res;
  int l = this->levels;

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
GLLine::rescaleMax(void)
{
  float *data = this->data();
  int res = resolution();
  int i = 0, q = 0, p = res;
  int l = this->levels;

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
  float *data = this->data();

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

GLWaterfallOpenGLContext::~GLWaterfallOpenGLContext(void)
{
  this->finalize();

  delete m_vertexShader;
  delete m_fragmentShader;
  delete m_waterfall;
  delete m_palette;
}

  void
GLWaterfallOpenGLContext::initialize(void)
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

  // Retrieve limits
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
  m_maxRowSize = GLLine::resolutionFor(texSize);

  if (m_rowCount > m_maxRowSize)
    m_rowCount = m_maxRowSize;

  if (m_rowSize > m_maxRowSize)
    m_rowSize = m_maxRowSize;

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
  this->resetWaterfall();

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
GLWaterfallOpenGLContext::resetWaterfall(void)
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
    glTexSubImage2D(
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
GLWaterfallOpenGLContext::flushPalette(void)
{
  glTexSubImage2D(
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
GLWaterfallOpenGLContext::disposeLastLine(void)
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
GLWaterfallOpenGLContext::flushOneLine(void)
{
  GLLine &line = m_history.back();
  int row = m_rowCount - (m_row % m_rowCount) - 1;

  if (m_rowSize == line.resolution()) {
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        row,
        line.allocation(),
        1,
        GL_RED,
        GL_FLOAT,
        line.data());
    this->disposeLastLine();
    m_row = (m_row + 1) % m_rowCount;
  } else {
    // Wrong line size. Just discard.
    this->disposeLastLine();
  }
}

  void
GLWaterfallOpenGLContext::flushLinesBulk(void)
{
  int maxRows = m_rowCount - (m_row % m_rowCount);
  int count = 0;
  int alloc = GLLine::allocationFor(m_rowSize);
  std::vector<float> bulkData;

  bulkData.resize(maxRows * alloc);

  for (int i = 0; i < maxRows && !m_history.empty(); ++i) {
    GLLine &line = m_history.back();

    if (m_rowSize != line.resolution()) {
      this->disposeLastLine();
      break;
    }

    memcpy(
        bulkData.data() + (maxRows - i - 1) * alloc,
        line.data(),
        alloc * sizeof(float));
    this->disposeLastLine();

    ++count;
  }

  if (count > 0) {
    glTexSubImage2D(
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
GLWaterfallOpenGLContext::flushLines(void)
{
  while (!m_history.empty()) {
    if (m_history.size() >= GL_WATERFALL_MIN_BULK_TRANSFER)
      this->flushLinesBulk();
    else
      this->flushOneLine();
  }
}

  void
GLWaterfallOpenGLContext::flushLinePool(void)
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
    this->flushLinePool();

    m_rowSize = size;
    this->resetWaterfall();
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
GLWaterfallOpenGLContext::averageFFTData(const float *fftData, int size)
{
  int i;

  if (m_accum.size() != static_cast<size_t>(size)) {
    m_accum.resize(static_cast<size_t>(size));
    m_firstAccum = true;
  }

  if (m_firstAccum) {
    std::memcpy(m_accum.data(), fftData, size * sizeof(float));
    m_firstAccum = false;
  } else {
    for (i = 0; i < size; ++i)
      m_accum[i] += .5f * (fftData[i] - m_accum[i]);
  }
}

  void
GLWaterfallOpenGLContext::commitFFTData(void)
{
  pushFFTData(m_accum.data(), m_accum.size());
  m_firstAccum = true;
}

  void
GLWaterfallOpenGLContext::setDynamicRange(float mindB, float maxdB)
{
  this->m  = (maxdB - mindB) / GL_WATERFALL_TEX_DR;
  this->x0 = (mindB - GL_WATERFALL_TEX_MIN_DB) / GL_WATERFALL_TEX_DR;
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
  this->c_x0 = 1.f - 1.f / static_cast<float>(1 << d);
  this->c_x1 = 1.f - 1.f / static_cast<float>(1 << (d + 1));

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
    this->recalcGeometric(width, height, zoom);

  glPushAttrib(GL_ALL_ATTRIB_BITS); // IMPORTANT TO PREVENT CONFLICTS WITH QPAINTER

  m_program.bind(); // Must be the first

  glViewport(x, height - m_rowCount - y, width, m_rowCount);

  glLoadIdentity();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_CULL_FACE);

  ortho.translate(2 * left, 0);
  ortho.scale(zoom, 1);

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
  m_program.setUniformValue("x0", this->x0);
  m_program.setUniformValue("m",  this->m);
  m_program.setUniformValue("c_x0", this->c_x0);
  m_program.setUniformValue("c_m",  this->c_x1 - this->c_x0);
  m_vao.release();
  m_vao.bind();

  // Add more lines (if necessary)
  m_waterfall->bind(0);
  flushLines();

  // Update palette (if necessary)
  m_palette->bind(1);
  if (m_updatePalette) {
    this->flushPalette();
    m_updatePalette = false;
  }

  m_program.setUniformValue("m_texture", 0);
  m_program.setUniformValue("m_palette", 1);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

  m_vao.release();
  m_program.disableAttributeArray("vertexcoords");
  m_program.disableAttributeArray("texture_coords");

  // ALL THIS MUST BE RELEASED, OTHERWISE QPAINTER WILL NOT WORK
  m_program.release();
  m_waterfall->release();
  m_palette->release();
  m_vbo.release();
  m_ibo.release();

  glPopAttrib();
}

///////////////////////////// GLWaterfall ////////////////////////////////////////
GLWaterfall::GLWaterfall(QWidget *parent) : AbstractWaterfall(parent)
{
}

GLWaterfall::~GLWaterfall()
{
  makeCurrent();
  this->glCtx.finalize();
  doneCurrent();
}

  void
GLWaterfall::clearWaterfall()
{
  m_WaterfallImage.fill(Qt::black);
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
GLWaterfall::initializeGL(void)
{
  this->glCtx.initialize();

  connect(
      this->context(),
      SIGNAL(aboutToBeDestroyed()),
      this,
      SLOT(onContextBeingDestroyed()));

  this->drawOverlay();
}

  void
GLWaterfall::onContextBeingDestroyed()
{
  makeCurrent();
  this->glCtx.finalize();
  doneCurrent();
}

void GLWaterfall::setWaterfallRange(float min, float max)
{
  if (out_of_range(min, max))
    return;

  m_WfMindB = min;
  m_WfMaxdB = max;

  this->glCtx.setDynamicRange(min - m_gain, max - m_gain);

  // no overlay change is necessary
}

//
//   |---------f-------------------------|
// -fs/2       S                        fs/2
//        |---------|
//        f0       f1
//

  void
GLWaterfall::paintGL(void)
{
  int y = m_Percent2DScreen * m_Size.height() / 100;
  qint64 f0 = m_FftCenter - m_Span / 2;
  qreal  left  = (qreal) ( - f0) / (qreal) m_Span - .5f;
  qreal  right = (qreal) (+m_SampleFreq - f0) / (qreal) m_Span - .5f;
  int dpi_factor = screen()->devicePixelRatio();

  this->glCtx.render(0, y * dpi_factor, width() * dpi_factor,
      height() * dpi_factor, left, right);
}

  void
GLWaterfall::resizeGL(int, int)
{
}


// Called by QT when screen needs to be redrawn
  void
GLWaterfall::paintEvent(QPaintEvent *ev)
{
  QOpenGLWidget::paintEvent(ev);
  QPainter painter(this);
  qint64  StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
  qint64  EndFreq = StartFreq + m_Span;

  m_DemodFreqX = xFromFreq(m_DemodCenterFreq);
  m_DemodLowCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodLowCutFreq);
  m_DemodHiCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodHiCutFreq);

  painter.setRenderHint(QPainter::Antialiasing);
  int y = m_Percent2DScreen * m_Size.height() / 100;

  painter.drawPixmap(0, 0, m_2DPixmap);

  // Draw named channel cutoffs
  if (m_channelsEnabled) {
    for (auto i = m_channelSet.find(StartFreq); i != m_channelSet.cend(); ++i) {
      auto p = i.value();
      int x_fCenter = xFromFreq(p->frequency);
      int x_fMin = xFromFreq(p->frequency + p->lowFreqCut);
      int x_fMax = xFromFreq(p->frequency + p->highFreqCut);

      if (EndFreq < p->frequency + p->lowFreqCut)
        break;

      WFHelpers::drawChannelCutoff(
          painter,
          y,
          x_fMin,
          x_fMax,
          x_fCenter,
          p->markerColor,
          p->cutOffColor,
          !p->bandLike);
    }
  }

  // Draw demod filter box
  if (m_FilterBoxEnabled)
    this->drawFilterBox(painter, y);

  drawSpectrum(painter, y);

  if (m_TimeStampsEnabled)
    paintTimeStamps(
        painter,
        QRect(2, y, this->width(), this->height()));
}

  void
GLWaterfall::drawSpectrum(QPainter &painter, int forceHeight)
{
  int     i, n;
  int     xmin, xmax;
  qint64  limit = ((qint64)m_SampleFreq + m_Span) / 2 - 1;
  QPoint  LineBuf[MAX_SCREENSIZE];
  int w = painter.device()->width();
  int h = forceHeight < 0 ? painter.device()->height() : forceHeight;

  // workaround for "fixed" line drawing since Qt 5
  // see http://stackoverflow.com/questions/16990326
#if QT_VERSION >= 0x050000
  painter.translate(0.5, 0.5);
#endif

  // Do we have valid FFT data?
  if (m_fftDataSize < 1)
    return;

  // get new scaled fft data
  getScreenIntegerFFTData(
      h,
      qMin(w, MAX_SCREENSIZE),
      m_PandMaxdB,
      m_PandMindB,
      qBound(
        -limit,
        m_tentativeCenterFreq + m_FftCenter,
        limit) - (qint64)m_Span/2,
      qBound(
        -limit,
        m_tentativeCenterFreq + m_FftCenter,
        limit) + (qint64)m_Span/2,
      m_fftData,
      m_fftbuf,
      &xmin,
      &xmax);

  // draw the pandapter
  painter.setPen(m_FftColor);
  n = xmax - xmin;
  for (i = 0; i < n; i++) {
    LineBuf[i].setX(i + xmin);
    LineBuf[i].setY(m_fftbuf[i + xmin]);
  }

  if (m_FftFill) {
    painter.setBrush(QBrush(m_FftFillCol, Qt::SolidPattern));
    if (n < MAX_SCREENSIZE-2) {
      LineBuf[n].setX(xmax-1);
      LineBuf[n].setY(h);
      LineBuf[n+1].setX(xmin);
      LineBuf[n+1].setY(h);
      painter.drawPolygon(LineBuf, n+2);
    } else {
      LineBuf[MAX_SCREENSIZE-2].setX(xmax-1);
      LineBuf[MAX_SCREENSIZE-2].setY(h);
      LineBuf[MAX_SCREENSIZE-1].setX(xmin);
      LineBuf[MAX_SCREENSIZE-1].setY(h);
      painter.drawPolygon(LineBuf, n);
    }
  } else {
    painter.drawPolyline(LineBuf, n);
  }

  // Peak detection
  if (m_PeakDetection > 0) {
    m_Peaks.clear();

    float   mean = 0;
    float   sum_of_sq = 0;
    for (i = 0; i < n; i++) {
      mean += m_fftbuf[i + xmin];
      sum_of_sq += m_fftbuf[i + xmin] * m_fftbuf[i + xmin];
    }
    mean /= n;
    float stdev= sqrt(sum_of_sq / n - mean * mean );

    int lastPeak = -1;
    for (i = 0; i < n; i++) {
      //m_PeakDetection times the std over the mean or better than current peak
      float d = (lastPeak == -1) ? (mean - m_PeakDetection * stdev) :
        m_fftbuf[lastPeak + xmin];

      if (m_fftbuf[i + xmin] < d)
        lastPeak=i;

      if (lastPeak != -1 &&
          (i - lastPeak > PEAK_H_TOLERANCE || i == n-1))
      {
        m_Peaks.insert(lastPeak + xmin, m_fftbuf[lastPeak + xmin]);
        painter.drawEllipse(lastPeak + xmin - 5,
            m_fftbuf[lastPeak + xmin] - 5, 10, 10);
        lastPeak = -1;
      }
    }
  }

  // Peak hold
  if (m_PeakHoldActive) {
    for (i = 0; i < n; i++) {
      if (!m_PeakHoldValid || m_fftbuf[i] < m_fftPeakHoldBuf[i])
        m_fftPeakHoldBuf[i] = m_fftbuf[i];

      LineBuf[i].setX(i + xmin);
      LineBuf[i].setY(m_fftPeakHoldBuf[i + xmin]);
    }
    painter.setPen(m_PeakHoldColor);
    painter.drawPolyline(LineBuf, n);

    m_PeakHoldValid = true;
  }
}

// Called to update spectrum data for displaying on the screen
  void
GLWaterfall::draw(bool everything)
{
  int     w;
  int     h;

  (void)everything; // suppress unused variable warning

  if (m_DrawOverlay) {
    drawOverlay();
    m_DrawOverlay = false;
  }

  // -----8<------------------------------------------------------------------
  // In the loving memory of a waterfall drawing code that use to hog my CPU
  // for years now. It's sad it's gone, I'm glad is not here anymore.
  // -----8<------------------------------------------------------------------

  // get/draw the 2D spectrum
  w = m_2DPixmap.width();
  h = m_2DPixmap.height();

  if (w != 0 && h != 0) {
    // first copy into 2Dbitmap the overlay bitmap.
    m_2DPixmap = m_OverlayPixmap.copy(0, 0, w, h);
  }

  // trigger a new paintEvent
  update();
}

/**
 * Set new FFT data.
 * @param fftData Pointer to the new FFT data used on the pandapter.
 * @param wfData Pointer to the FFT data used in the waterfall.
 * @param size The FFT size.
 *
 * This method can be used to set different FFT data set for the pandapter and the
 * waterfall.
 */

  void
GLWaterfall::setNewFftData(
    float *fftData,
    float *wfData,
    int size,
    QDateTime const &t,
    bool looped)
{
  /** FIXME **/
  if (!m_Running)
    m_Running = true;

  quint64 tnow_ms = SCAST(quint64, t.toMSecsSinceEpoch());

  if (looped) {
    TimeStamp ts;

    ts.counter = m_TimeStampCounter;
    ts.timeStampText =
      m_lastFft.toLocalTime().toString("hh:mm:ss.zzz")
      + " - "
      + t.toLocalTime().toString("hh:mm:ss.zzz");
    ts.utcTimeStampText =
      m_lastFft.toUTC().toString("hh:mm:ss.zzzZ")
      + " - "
      + t.toUTC().toString("hh:mm:ss.zzzZ");
    ts.marker = true;

    m_TimeStamps.push_front(ts);
    m_TimeStampCounter = 0;
  }

  m_wfData = wfData;
  m_fftData = fftData;
  m_fftDataSize = size;
  m_lastFft = t;

  if (m_tentativeCenterFreq != 0) {
    m_tentativeCenterFreq = 0;
    m_DrawOverlay = true;
  }

  if (m_TimeStampCounter >= m_TimeStampSpacing) {
    TimeStamp ts;

    ts.counter = m_TimeStampCounter;
    ts.timeStampText = t.toLocalTime().toString("hh:mm:ss.zzz");
    ts.utcTimeStampText = t.toUTC().toString("hh:mm:ss.zzzZ");

    m_TimeStamps.push_front(ts);
    m_TimeStampCounter = 0;
  }

  if (m_wfData != nullptr && m_fftDataSize > 0) {
    if (msec_per_wfline > 0) {
      this->glCtx.averageFFTData(m_wfData, m_fftDataSize);

      if (tnow_ms < tlast_wf_ms || tnow_ms - tlast_wf_ms >= msec_per_wfline) {
        int line_count = (tnow_ms - tlast_wf_ms) / msec_per_wfline;
        if (line_count >= 1 && line_count <= 20) {
          tlast_wf_ms += msec_per_wfline * line_count;
        } else {
          line_count = 1;
          tlast_wf_ms = tnow_ms;
        }
        for (int i = 0; i < line_count; i++) {
          this->glCtx.commitFFTData();
        }
        m_TimeStampCounter += line_count;
      }
    } else {
      tlast_wf_ms = tnow_ms;
      this->glCtx.pushFFTData(m_wfData, m_fftDataSize);
      ++m_TimeStampCounter;
    }
  }

  draw();
}

  int
GLWaterfall::drawFATs(
    GLDrawingContext &ctx,
    qint64 StartFreq,
    qint64 EndFreq)
{
  int count = 0;
  int w = ctx.width;
  int h = ctx.height;
  QString label;
  QRect rect;

  for (auto fat : m_FATs) {
    if (fat.second != nullptr) {
      FrequencyBandIterator p = fat.second->find(StartFreq);

      while (p != fat.second->cbegin() && p->second.max > StartFreq)
        --p;

      for (; p != fat.second->cend() && p->second.min < EndFreq; ++p) {
        int x0 = xFromFreq(p->second.min);
        int x1 = xFromFreq(p->second.max);
        bool leftborder = true;
        bool rightborder = true;
        int tw, boxw;


        if (x0 < m_YAxisWidth) {
          leftborder = false;
          x0 = m_YAxisWidth;
        }

        if (x1 >= w) {
          rightborder = false;
          x1 = w - 1;
        }

        if (x1 < m_YAxisWidth)
          continue;

        boxw = x1 - x0;

        ctx.painter->setBrush(QBrush(p->second.color));
        ctx.painter->setPen(p->second.color);

        ctx.painter->drawRect(
            x0,
            count * ctx.metrics->height(),
            x1 - x0 + 1,
            ctx.metrics->height());

        if (leftborder)
          ctx.painter->drawLine(
              x0,
              count * ctx.metrics->height(),
              x0,
              h);

        if (rightborder)
          ctx.painter->drawLine(
              x1,
              count * ctx.metrics->height(),
              x1,
              h);

        label = ctx.metrics->elidedText(
            QString::fromStdString(p->second.primary),
            Qt::ElideRight,
            boxw);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        tw = ctx.metrics->horizontalAdvance(label);
#else
        tw = ctx.metrics->width(label);
#endif // QT_VERSION_CHECK

        if (tw < boxw) {
          ctx.painter->setPen(m_FftTextColor);
          rect.setRect(
              x0 + (x1 - x0) / 2 - tw / 2,
              count * ctx.metrics->height(),
              tw,
              ctx.metrics->height());
          ctx.painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, label);
        }
      }

      ++count;
    }
  }

  return count * ctx.metrics->height();
}

  void
GLWaterfall::drawBookmarks(
    GLDrawingContext &ctx,
    qint64 StartFreq,
    qint64 EndFreq,
    int xAxisTop)
{
  m_BookmarkTags.clear();
  int fontHeight = ctx.metrics->ascent() + 1;
  int slant = 5;
  int levelHeight = fontHeight + 5;
  int x;
  const int nLevels = 10;

  QList<BookmarkInfo> bookmarks =
    m_BookmarkSource->getBookmarksInRange(
        StartFreq,
        EndFreq);
  int tagEnd[nLevels] = {0};

  for (int i = 0; i < bookmarks.size(); i++) {
    x = xFromFreq(bookmarks[i].frequency);
#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
    int nameWidth = ctx.metrics->width(bookmarks[i].name);
#else
    int nameWidth = ctx.metrics->boundingRect(bookmarks[i].name).width();
#endif

    int level = 0;
    int yMin = static_cast<int>(m_FATs.size()) * ctx.metrics->height();
    while (level < nLevels && tagEnd[level] > x)
      level++;

    if (level == nLevels)
      level = 0;

    tagEnd[level] = x + nameWidth + slant - 1;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    BookmarkInfo bookmark = bookmarks[i];
    m_BookmarkTags.append(
        qMakePair<QRect, BookmarkInfo>(
          QRect(x, yMin + level * levelHeight, nameWidth + slant, fontHeight),
          std::move(bookmark))); // Be more Cobol every day
#else
    m_BookmarkTags.append(
        qMakePair<QRect, BookmarkInfo>(
          QRect(x, yMin + level * levelHeight, nameWidth + slant, fontHeight),
          bookmarks[i]));
#endif // QT_VERSION

    QColor color = QColor(bookmarks[i].color);
    color.setAlpha(0x60);
    // Vertical line
    ctx.painter->setPen(
        QPen(color, 1, Qt::DashLine));
    ctx.painter->drawLine(
        x,
        yMin + level * levelHeight + fontHeight + slant,
        x,
        xAxisTop);

    // Horizontal line
    ctx.painter->setPen(
        QPen(color, 1, Qt::SolidLine));
    ctx.painter->drawLine(
        x + slant, yMin + level * levelHeight + fontHeight,
        x + nameWidth + slant - 1,
        yMin + level * levelHeight + fontHeight);
    // Diagonal line
    ctx.painter->drawLine(
        x + 1,
        yMin + level * levelHeight + fontHeight + slant - 1,
        x + slant - 1,
        yMin + level * levelHeight + fontHeight + 1);

    color.setAlpha(0xFF);
    ctx.painter->setPen(QPen(color, 2, Qt::SolidLine));
    ctx.painter->drawText(
        x + slant,
        yMin + level * levelHeight,
        nameWidth,
        fontHeight,
        Qt::AlignVCenter | Qt::AlignHCenter,
        bookmarks[i].name);
  }
}

  void
GLWaterfall::drawAxes(GLDrawingContext &ctx, qint64 StartFreq, qint64 EndFreq)
{
  int w = ctx.width;
  int h = ctx.height;

  int     x,y;
  float   pixperdiv;
  float   adjoffset;
  float   unitStepSize;
  float   minUnitAdj;
  QRect   rect;

  // solid background
  ctx.painter->setBrush(Qt::SolidPattern);
  ctx.painter->fillRect(0, 0, w, h, m_FftBgColor);

#define HOR_MARGIN 5
#define VER_MARGIN 5

  // X and Y axis areas
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  int tw = ctx.metrics->horizontalAdvance("XXXX");
#else
  int tw = ctx.metrics->width("XXXX");
#endif // QT_VERSION_CHECK

  m_YAxisWidth = tw + 2 * HOR_MARGIN;
  m_XAxisYCenter = h - ctx.metrics->height() / 2;
  int xAxisHeight = ctx.metrics->height() + 2 * VER_MARGIN;
  int xAxisTop = h - xAxisHeight;
  int fLabelTop = xAxisTop + VER_MARGIN;

  if (m_CenterLineEnabled) {
    x = xFromFreq(m_CenterFreq - m_tentativeCenterFreq);
    if (x > 0 && x < w) {
      ctx.painter->setPen(m_FftCenterAxisColor);
      ctx.painter->drawLine(x, 0, x, xAxisTop);
    }
  }

  // Frequency grid
  QString label;
  label.setNum(float(EndFreq / m_FreqUnits), 'f', m_FreqDigits);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  tw = ctx.metrics->horizontalAdvance(label) + ctx.metrics->horizontalAdvance("O");
#else
  tw = ctx.metrics->width(label) + ctx.metrics->width("O");
#endif // QT_VERSION_CHECK

  calcDivSize(
      StartFreq,
      EndFreq,
      qMin(w / tw, HORZ_DIVS_MAX),
      m_StartFreqAdj,
      m_FreqPerDiv,
      m_HorDivs);

  pixperdiv = (float)w * (float) m_FreqPerDiv / (float) m_Span;
  adjoffset = pixperdiv * float (m_StartFreqAdj - StartFreq) / (float) m_FreqPerDiv;

  ctx.painter->setPen(QPen(m_FftAxesColor, 1, Qt::DotLine));
  for (int i = 0; i <= m_HorDivs; i++) {
    x = (int)((float)i * pixperdiv + adjoffset);
    if (x > m_YAxisWidth)
      ctx.painter->drawLine(x, 0, x, xAxisTop);
  }

  // draw frequency values (x axis)
  makeFrequencyStrs();
  ctx.painter->setPen(m_FftTextColor);
  for (int i = 0; i <= m_HorDivs; i++) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int tw = ctx.metrics->horizontalAdvance(m_HDivText[i]);
#else
    int tw = ctx.metrics->width(m_HDivText[i]);
#endif // QT_VERSION_CHECK
    x = (int)((float)i*pixperdiv + adjoffset);
    if (x > m_YAxisWidth) {
      rect.setRect(x - tw/2, fLabelTop, tw, ctx.metrics->height());
      ctx.painter->drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
    }
  }


  // Level grid
  qint64 minUnitAdj64 = 0;
  qint64 unitDivSize = 0;
  qint64 unitSign    = m_dBPerUnit < 0 ? -1 : 1;
  qint64 pandMinUnit = unitSign * static_cast<qint64>(toDisplayUnits(m_PandMindB));
  qint64 pandMaxUnit = unitSign * static_cast<qint64>(toDisplayUnits(m_PandMaxdB));

  calcDivSize(pandMinUnit, pandMaxUnit,
      qMax(h/m_VdivDelta, VERT_DIVS_MIN), minUnitAdj64, unitDivSize,
      m_VerDivs);

  unitStepSize = (float) unitDivSize;
  minUnitAdj = minUnitAdj64;

  pixperdiv = (float) h * (float) unitStepSize / (pandMaxUnit - pandMinUnit);
  adjoffset = (float) h * (minUnitAdj - pandMinUnit) / (pandMaxUnit - pandMinUnit);

#ifdef PLOTTER_DEBUG
  qDebug() << "minDb =" << m_PandMindB << "maxDb =" << m_PandMaxdB
    << "mindbadj =" << mindbadj << "dbstepsize =" << dbstepsize
    << "pixperdiv =" << pixperdiv << "adjoffset =" << adjoffset;
#endif

  ctx.painter->setPen(QPen(m_FftAxesColor, 1, Qt::DotLine));
  for (int i = 0; i <= m_VerDivs; i++) {
    y = h - (int)((float) i * pixperdiv + adjoffset);
    if (y < h - xAxisHeight)
      ctx.painter->drawLine(m_YAxisWidth, y, w, y);
  }

  // draw amplitude values (y axis)
  int unit;
  int unitWidth;

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  m_YAxisWidth = ctx.metrics->horizontalAdvance("-160 ");
  unitWidth    = ctx.metrics->horizontalAdvance(m_unitName);
#else
  m_YAxisWidth = ctx.metrics->width("-160 ");
  unitWidth    = ctx.metrics->width(m_unitName);
#endif // QT_VERSION_CHECK

  if (unitWidth > m_YAxisWidth)
    m_YAxisWidth = unitWidth;

  ctx.painter->setPen(m_FftTextColor);
  int th = ctx.metrics->height();
  for (int i = 0; i < m_VerDivs; i++) {
    y = h - (int)((float) i * pixperdiv + adjoffset);

    if (y < h -xAxisHeight) {
      unit = minUnitAdj + unitStepSize * i;
      rect.setRect(HOR_MARGIN, y - th / 2, m_YAxisWidth, th);
      ctx.painter->drawText(
          rect,
          Qt::AlignRight|Qt::AlignVCenter,
          QString::number(unitSign * unit));
    }
  }

  // Draw unit name on top left corner
  rect.setRect(HOR_MARGIN, 0, unitWidth, th);
  ctx.painter->drawText(rect, Qt::AlignRight|Qt::AlignVCenter, m_unitName);

#ifdef GL_WATERFALL_BOOKMARKS_SUPPORT
  if (m_BookmarksEnabled && m_BookmarkSource != nullptr)
    this->drawBookmarks(ctx, StartFreq, EndFreq, xAxisTop);
#endif // GL_WATERFALL_BOOKMARKS_SUPPORT
}

// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
  void
GLWaterfall::drawOverlay()
{
  if (m_OverlayPixmap.isNull())
    return;

  int             bandY = 0;
  QFontMetrics    metrics(m_Font);
  QPainter        painter(&m_OverlayPixmap);
  GLDrawingContext ctx;

  qint64  StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
  qint64  EndFreq = StartFreq + m_Span;

  ctx.painter = &painter;
  ctx.metrics = &metrics;
  ctx.width   = m_OverlayPixmap.width();
  ctx.height  = m_OverlayPixmap.height();

  painter.setFont(m_Font);

  // Draw axes
  this->drawAxes(ctx, StartFreq, EndFreq);

  // Draw frequency allocation tables
  if (m_ShowFATs)
    bandY = this->drawFATs(ctx, StartFreq, EndFreq);

  // Draw named channel (boxes)

  if (m_channelsEnabled) {
    for (auto i = m_channelSet.find(StartFreq - m_Span); i != m_channelSet.cend(); ++i) {
      auto p = i.value();
      int x_fCenter = xFromFreq(p->frequency);
      int x_fMin = xFromFreq(p->frequency + p->lowFreqCut);
      int x_fMax = xFromFreq(p->frequency + p->highFreqCut);

      if (p->frequency + p->highFreqCut < StartFreq)
        continue;

      if (EndFreq < p->frequency + p->lowFreqCut)
        break;

      if (p->bandLike) {
        WFHelpers::drawChannelBox(
            painter,
            ctx.height,
            x_fMin,
            x_fMax,
            x_fCenter,
            p->boxColor,
            p->markerColor,
            p->name,
            p->markerColor,
            ctx.metrics->height() / 2,
            bandY + p->nestLevel * ctx.metrics->height());
      } else {
        WFHelpers::drawChannelBox(
            painter,
            ctx.height,
            x_fMin,
            x_fMax,
            x_fCenter,
            p->boxColor,
            p->markerColor,
            p->name,
            QColor(),
            -1,
            bandY + p->nestLevel * ctx.metrics->height());
      }
    }
  }

  // Draw info text (if enabled)
  if (!m_infoText.isEmpty()) {
    int flags = Qt::AlignRight |  Qt::AlignTop | Qt::TextWordWrap;
    QRectF pixRect = m_OverlayPixmap.rect();

    pixRect.setWidth(pixRect.width() - 10);

    QRectF rect = painter.boundingRect(
        pixRect,
        flags,
        m_infoText);

    rect.setX(pixRect.width() - rect.width());
    rect.setY(0);

    painter.setPen(QPen(m_infoTextColor, 2, Qt::SolidLine));
    painter.drawText(rect, flags, m_infoText);
  }

  if (!m_Running) {
    // if not running so is no data updates to draw to screen
    // copy into 2Dbitmap the overlay bitmap.
    m_2DPixmap = m_OverlayPixmap.copy(0, 0, ctx.width, ctx.height);

    // trigger a new paintEvent
    update();
  }

  painter.end();
}
