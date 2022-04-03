 /* -*- c++ -*- */
/* + + +   This Software is released under the "Simplified BSD License"  + + +
 * Copyright 2010 Moe Wheatley. All rights reserved.
 * Copyright 2011-2013 Alexandru Csete OZ9AEC
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
#include <QDesktopWidget>
#include <cstring>

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

  for (auto p : screens)
    if (p->geometry().height() > maxHeight)
      maxHeight = p->geometry().height();

  this->m_paletBuf.resize(256 * 4);
  this->m_rowCount = maxHeight;
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
  this->m_paletBuf.resize(256);

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

  for (int i = 0; i < maxRows && !this->m_history.empty(); ++i) {
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
    if (this->m_useMaxBlending)
      line.assignMax(fftData);
    else
      line.assignMean(fftData);
  } else {
    if (this->m_useMaxBlending)
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
    m_accum.assign(m_accum.size(), 0);
    m_firstAccum = false;
  }

  for (i = 0; i < size; ++i)
   m_accum[i] += .5f * (fftData[i] - m_accum[i]);
}

void
GLWaterfallOpenGLContext::commitFFTData(void)
{
  if (!m_firstAccum) {
    pushFFTData(m_accum.data(), m_accum.size());
    m_firstAccum = true;
  }
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

/////////////////////////////// GLWaterfall ///////////////////////////////////

#define STATUS_TIP \
    "Click, drag or scroll on spectrum to tune. " \
    "Drag and scroll X and Y axes for pan and zoom. " \
    "Drag filter edges to adjust filter."


void
GLWaterfall::initLayout(void)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_PaintOnScreen,false);
  setAutoFillBackground(false);
  setAttribute(Qt::WA_OpaquePaintEvent, false);
  setAttribute(Qt::WA_NoSystemBackground, true);
  setMouseTracking(true);
  setTooltipsEnabled(false);
  setStatusTip(tr(STATUS_TIP));
}

void
GLWaterfall::initDefaults(void)
{
  m_PeakHoldActive = false;
  m_PeakHoldValid = false;

  m_FftCenter = 0;
  m_CenterFreq = 144500000;
  m_DemodCenterFreq = 144500000;
  m_DemodHiCutFreq = 5000;
  m_DemodLowCutFreq = -5000;

  m_FLowCmin = -25000;
  m_FLowCmax = -1000;
  m_FHiCmin = 1000;
  m_FHiCmax = 25000;
  m_symetric = true;

  m_ClickResolution = 100;
  m_FilterClickResolution = 100;
  m_CursorCaptureDelta = CUR_CUT_DELTA;

  m_FilterBoxEnabled = true;
  m_CenterLineEnabled = true;
  m_BookmarksEnabled = true;
  m_Locked = false;
  m_freqDragLocked = false;
  m_Span = 96000;
  m_SampleFreq = 96000;

  m_HorDivs = 12;
  m_VerDivs = 6;
  m_PandMaxdB = m_WfMaxdB = 0.f;
  m_PandMindB = m_WfMindB = -150.f;

  m_FreqUnits = 1000000;
  m_CursorCaptured = NOCAP;
  m_Running = false;
  m_DrawOverlay = true;
  m_2DPixmap = QPixmap(0,0);
  m_OverlayPixmap = QPixmap(0,0);
  m_GLWaterfallImage = QImage();
  m_Size = QSize(0,0);
  m_GrabPosition = 0;
  m_Percent2DScreen = 30;	//percent of screen used for 2D display
  m_VdivDelta = 30;
  m_HdivDelta = 70;

  m_ZeroPoint = 0;
  m_dBPerUnit = 1;

  m_FreqDigits = 3;

  m_Peaks = QMap<int,int>();
  setPeakDetection(false, 2);
  m_PeakHoldValid = false;

  setFftPlotColor(QColor(0xFF,0xFF,0xFF,0xFF));
  setFftBgColor(QColor(PLOTTER_BGD_COLOR));
  setFftAxesColor(QColor(PLOTTER_GRID_COLOR));
  setFilterBoxColor(QColor(PLOTTER_FILTER_BOX_COLOR));
  setTimeStampColor(QColor(0xFF,0xFF,0xFF,0xFF));

  setFftFill(false);

  // always update waterfall
  tlast_wf_ms = 0;
  msec_per_wfline = 0;
  wf_span = 0;
  fft_rate = 15;

  m_fftData = nullptr;
  m_wfData  = nullptr;
  m_fftDataSize = 0;
}

///////////////////////////// GLWaterfall ////////////////////////////////////////
GLWaterfall::GLWaterfall(QWidget *parent) : QOpenGLWidget(parent)
{
  this->initLayout();
  this->initDefaults();
}

GLWaterfall::~GLWaterfall()
{
  makeCurrent();
  this->glCtx.finalize();
  doneCurrent();
}

QSize
GLWaterfall::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize
GLWaterfall::sizeHint() const
{
  return QSize(180, 180);
}

void
GLWaterfall::mouseMoveEvent(QMouseEvent *event)
{
  QPoint pt = event->pos();

  /* mouse enter / mouse leave events */
  if (m_OverlayPixmap.rect().contains(pt)) {
    //is in Overlay bitmap region
    if (event->buttons() == Qt::NoButton) {
      bool onTag = false;
      if (pt.y() < 15 * 10) { // FIXME
        for (int i = 0; i < m_BookmarkTags.size() && !onTag; i++) {
          if (m_BookmarkTags[i].first.contains(event->pos()))
            onTag = true;
        }
      }
      // if no mouse button monitor grab regions and change cursor icon
      if (onTag) {
        setCursor(QCursor(Qt::PointingHandCursor));
        m_CursorCaptured = BOOKMARK;
      } else if (isPointCloseTo(pt.x(), m_DemodFreqX, m_CursorCaptureDelta)) {
        // in move demod box center frequency region
        if (m_CursorCaptured != CENTER)
          setCursor(QCursor(Qt::SizeHorCursor));
        m_CursorCaptured = CENTER;
        if (m_TooltipsEnabled)
          QToolTip::showText(
                event->globalPos(),
                QString("Demod: %1 kHz").arg(
                  m_DemodCenterFreq / 1.e3f, 0, 'f', 3),
                this);
      } else if (isPointCloseTo(
                   pt.x(),
                   m_DemodHiCutFreqX,
                   m_CursorCaptureDelta)) {
        // in move demod hicut region
        if (m_CursorCaptured != RIGHT)
          setCursor(QCursor(Qt::SizeFDiagCursor));
        m_CursorCaptured = RIGHT;
        if (m_TooltipsEnabled)
          QToolTip::showText(
                event->globalPos(),
                QString("High cut: %1 Hz").arg(m_DemodHiCutFreq),
                this);
      } else if (isPointCloseTo(
                   pt.x(),
                   m_DemodLowCutFreqX,
                   m_CursorCaptureDelta)) {
        // in move demod lowcut region
        if (m_CursorCaptured != LEFT)
          setCursor(QCursor(Qt::SizeBDiagCursor));
        m_CursorCaptured = LEFT;
        if (m_TooltipsEnabled)
          QToolTip::showText(
                event->globalPos(),
                QString("Low cut: %1 Hz").arg(m_DemodLowCutFreq),
                this);
      } else if (isPointCloseTo(
                   pt.x(),
                   m_YAxisWidth / 2,
                   m_YAxisWidth / 2)) {
        if (m_CursorCaptured != YAXIS)
          setCursor(QCursor(Qt::OpenHandCursor));
        m_CursorCaptured = YAXIS;
        if (m_TooltipsEnabled)
          QToolTip::hideText();
      } else if (isPointCloseTo(
                   pt.y(),
                   m_XAxisYCenter,
                   m_CursorCaptureDelta + 5)) {
        if (m_CursorCaptured != XAXIS)
          setCursor(QCursor(Qt::OpenHandCursor));
        m_CursorCaptured = XAXIS;
        if (m_TooltipsEnabled)
          QToolTip::hideText();
      } else {	//if not near any grab boundaries
        if (m_CursorCaptured != NOCAP) {
          setCursor(QCursor(Qt::ArrowCursor));
          m_CursorCaptured = NOCAP;
        }
        if (m_TooltipsEnabled)
          QToolTip::showText(
                event->globalPos(),
                QString("F: %1 kHz").arg(freqFromX(pt.x())/1.e3f, 0, 'f', 3),
                this);
      }
      m_GrabPosition = 0;
    }
  } else {
    // not in Overlay region
    if (event->buttons() == Qt::NoButton) {
      if (NOCAP != m_CursorCaptured)
        setCursor(QCursor(Qt::ArrowCursor));

      m_CursorCaptured = NOCAP;
      m_GrabPosition = 0;
    }
    if (m_TooltipsEnabled) {
      QDateTime tt;
      tt.setMSecsSinceEpoch(msecFromY(pt.y()));

      QToolTip::showText(
            event->globalPos(),
            QString("%1\n%2 kHz")
              .arg(tt.toString("yyyy.MM.dd hh:mm:ss.zzz"))
              .arg(freqFromX(pt.x())/1.e3f, 0, 'f', 3),
            this);
    }
  }
  // process mouse moves while in cursor capture modes
  if (YAXIS == m_CursorCaptured) {
    if (event->buttons() & Qt::LeftButton) {
      setCursor(QCursor(Qt::ClosedHandCursor));
      // move Y scale up/down
      float delta_px = m_Yzero - pt.y();
      float delta_db =
          delta_px * fabs(m_PandMindB - m_PandMaxdB) / (float)m_OverlayPixmap.height();
      m_PandMindB -= delta_db;
      m_PandMaxdB -= delta_db;
      if (out_of_range(m_PandMindB, m_PandMaxdB)) {
        m_PandMindB += delta_db;
        m_PandMaxdB += delta_db;
      } else {
        emit pandapterRangeChanged(m_PandMindB, m_PandMaxdB);
        updateOverlay();
        m_PeakHoldValid = false;
        m_Yzero = pt.y();
      }
    }
  } else if (XAXIS == m_CursorCaptured) {
    if (event->buttons() & (Qt::LeftButton | Qt::MidButton)) {
      setCursor(QCursor(Qt::ClosedHandCursor));
      // pan viewable range or move center frequency
      int delta_px = m_Xzero - pt.x();
      qint64 delta_hz = delta_px * m_Span / m_OverlayPixmap.width();
      if (event->buttons() & m_freqDragBtn) {
        if (!m_Locked && !m_freqDragLocked) {
          qint64 centerFreq = boundCenterFreq(m_CenterFreq + delta_hz);
          delta_hz = centerFreq - m_CenterFreq;

          m_CenterFreq += delta_hz;
          m_DemodCenterFreq += delta_hz;

          ////////////// TODO: Edit this to fake spectrum scroll /////////
          ////////////// DONE: Something like this:
          ///
          m_tentativeCenterFreq += delta_hz;

          if (delta_hz != 0)
            emit newCenterFreq(m_CenterFreq);
        }
      } else {
        setFftCenterFreq(m_FftCenter + delta_hz);
      }

      updateOverlay();
      m_PeakHoldValid = false;
      m_Xzero = pt.x();
    }
  } else if (LEFT == m_CursorCaptured) {
    // moving in demod lowcut region
    if (event->buttons() & (Qt::LeftButton | Qt::RightButton)) {
      // moving in demod lowcut region with left button held
      if (m_GrabPosition != 0) {
        m_DemodLowCutFreq = freqFromX(pt.x() - m_GrabPosition ) - m_DemodCenterFreq;
        m_DemodLowCutFreq = roundFreq(m_DemodLowCutFreq, m_FilterClickResolution);

        if (m_symetric && (event->buttons() & Qt::LeftButton)) {
          m_DemodHiCutFreq = -m_DemodLowCutFreq;
        }
        clampDemodParameters();

        emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
        updateOverlay();
      } else {
        // save initial grab postion from m_DemodFreqX
        m_GrabPosition = pt.x()-m_DemodLowCutFreqX;
      }
    } else if (event->buttons() & ~Qt::NoButton) {
      setCursor(QCursor(Qt::ArrowCursor));
      m_CursorCaptured = NOCAP;
    }
  } else if (RIGHT == m_CursorCaptured) {
    // moving in demod highcut region
    if (event->buttons() & (Qt::LeftButton | Qt::RightButton)) {
      // moving in demod highcut region with right button held
      if (m_GrabPosition != 0) {
        m_DemodHiCutFreq = freqFromX( pt.x()-m_GrabPosition ) - m_DemodCenterFreq;
        m_DemodHiCutFreq = roundFreq(m_DemodHiCutFreq, m_FilterClickResolution);

        if (m_symetric && (event->buttons() & Qt::LeftButton)) {
          m_DemodLowCutFreq = -m_DemodHiCutFreq;
        }
        clampDemodParameters();

        emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
        updateOverlay();
      } else {
        // save initial grab postion from m_DemodFreqX
        m_GrabPosition = pt.x() - m_DemodHiCutFreqX;
      }
    } else if (event->buttons() & ~Qt::NoButton) {
      setCursor(QCursor(Qt::ArrowCursor));
      m_CursorCaptured = NOCAP;
    }
  } else if (CENTER == m_CursorCaptured) {
    // moving inbetween demod lowcut and highcut region
    if (event->buttons() & Qt::LeftButton) {   // moving inbetween demod lowcut and highcut region with left button held
      if (m_GrabPosition != 0) {
        if (!m_Locked) {
          m_DemodCenterFreq = roundFreq(
                freqFromX(pt.x() - m_GrabPosition),
                m_ClickResolution);
          emit newDemodFreq(
                m_DemodCenterFreq,
                m_DemodCenterFreq - m_CenterFreq);
          updateOverlay();
          m_PeakHoldValid = false;
        }
      } else {
        // save initial grab postion from m_DemodFreqX
        m_GrabPosition = pt.x() - m_DemodFreqX;
      }
    } else if (event->buttons() & ~Qt::NoButton) {
      setCursor(QCursor(Qt::ArrowCursor));
      m_CursorCaptured = NOCAP;
    }
  } else {
    // cursor not captured
    m_GrabPosition = 0;
  }

  if (!this->rect().contains(pt)) {
    if (NOCAP != m_CursorCaptured)
      setCursor(QCursor(Qt::ArrowCursor));
    m_CursorCaptured = NOCAP;
  }
}


int
GLWaterfall::getNearestPeak(QPoint pt)
{
  QMap<int, int>::const_iterator i =
      m_Peaks.lowerBound(pt.x() - PEAK_CLICK_MAX_H_DISTANCE);
  QMap<int, int>::const_iterator upperBound =
      m_Peaks.upperBound(pt.x() + PEAK_CLICK_MAX_H_DISTANCE);
  float   dist = 1.0e10;
  int     best = -1;

  for ( ; i != upperBound; i++) {
    int x = i.key();
    int y = i.value();

    if (abs(y - pt.y()) > PEAK_CLICK_MAX_V_DISTANCE)
      continue;

    float d = powf(y - pt.y(), 2) + powf(x - pt.x(), 2);
    if (d < dist) {
      dist = d;
      best = x;
    }
  }

  return best;
}

/** Set waterfall span in milliseconds */
void
GLWaterfall::setWaterfallSpan(quint64 span_ms)
{
  wf_span = span_ms;
  if (m_GLWaterfallImage.height() > 0)
    msec_per_wfline = wf_span / m_GLWaterfallImage.height();
  clearGLWaterfall();
}

void
GLWaterfall::clearGLWaterfall()
{
  m_GLWaterfallImage.fill(Qt::black);
}

/**
 * @brief Save waterfall to a graphics file
 * @param filename
 * @return TRUE if the save successful, FALSE if an erorr occurred.
 *
 * We assume that frequency strings are up to date
 */
bool
GLWaterfall::saveGLWaterfall(const QString &) const
{
  return false;
}

/** Get waterfall time resolution in milleconds / line. */
quint64
GLWaterfall::getWfTimeRes(void)
{
  if (msec_per_wfline)
    return msec_per_wfline;
  else
    return 1000 * fft_rate / m_GLWaterfallImage.height(); // Auto mode
}

void
GLWaterfall::setFftRate(int rate_hz)
{
  fft_rate = rate_hz;
  clearGLWaterfall();
}

// Called when a mouse button is pressed
void
GLWaterfall::mousePressEvent(QMouseEvent * event)
{
  QPoint pt = event->pos();

  if (NOCAP == m_CursorCaptured) {
    if (isPointCloseTo(pt.x(), m_DemodFreqX, m_CursorCaptureDelta)) {
      // move demod box center frequency region
      m_CursorCaptured = CENTER;
      m_GrabPosition = pt.x() - m_DemodFreqX;
    } else if (isPointCloseTo(
                 pt.x(),
                 m_DemodLowCutFreqX,
                 m_CursorCaptureDelta)) {
      // filter low cut
      m_CursorCaptured = LEFT;
      m_GrabPosition = pt.x() - m_DemodLowCutFreqX;
    } else if (isPointCloseTo(
                 pt.x(),
                 m_DemodHiCutFreqX,
                 m_CursorCaptureDelta)) {
      // filter high cut
      m_CursorCaptured = RIGHT;
      m_GrabPosition = pt.x() - m_DemodHiCutFreqX;
    } else {
      if (event->buttons() == Qt::LeftButton) {
        if (!m_Locked) {
          int  best = -1;

          if (m_PeakDetection > 0)
            best = getNearestPeak(pt);
          if (best != -1)
            m_DemodCenterFreq = freqFromX(best);
          else
            m_DemodCenterFreq = roundFreq(freqFromX(pt.x()), m_ClickResolution);

          // if cursor not captured set demod frequency and start demod box capture
          emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);

          // save initial grab postion from m_DemodFreqX
          // setCursor(QCursor(Qt::CrossCursor));
          m_CursorCaptured = CENTER;
          m_GrabPosition = 1;
          updateOverlay();
        }
      } else if (event->buttons() == Qt::MidButton) {
        if (!m_Locked && !m_freqDragLocked) {
          // set center freq
          m_CenterFreq
              = boundCenterFreq(roundFreq(freqFromX(pt.x()), m_ClickResolution));
          m_DemodCenterFreq = m_CenterFreq;
          emit newCenterFreq(m_CenterFreq);
          emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);
          updateOverlay();
        }
      } else if (event->buttons() == Qt::RightButton) {
        // reset frequency zoom
        resetHorizontalZoom();
        updateOverlay();
      }
    }
  } else {
    if (m_CursorCaptured == YAXIS)
      // get ready for moving Y axis
      m_Yzero = pt.y();
    else if (m_CursorCaptured == XAXIS) {
      m_Xzero = pt.x();
      if (event->buttons() == Qt::RightButton) {
        // reset frequency zoom
        resetHorizontalZoom();
        updateOverlay();
      }
    } else if (m_CursorCaptured == BOOKMARK) {
      if (!m_Locked) {
        for (int i = 0; i < m_BookmarkTags.size(); i++) {
          if (m_BookmarkTags[i].first.contains(event->pos())) {
            BookmarkInfo info = m_BookmarkTags[i].second;

            if (!info.modulation.isEmpty()) {
              emit newModulation(info.modulation);
            }

            m_DemodCenterFreq = info.frequency;
            emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);

            if (info.bandwidth() != 0) {
              emit newFilterFreq(info.lowFreqCut, info.highFreqCut);
            }

            break;
          }
        }
      }
    }
  }
}

void
GLWaterfall::mouseReleaseEvent(QMouseEvent * event)
{
  QPoint pt = event->pos();

  if (!m_OverlayPixmap.rect().contains(pt)) {
    // not in Overlay region
    if (NOCAP != m_CursorCaptured)
      setCursor(QCursor(Qt::ArrowCursor));

    m_CursorCaptured = NOCAP;
    m_GrabPosition = 0;
  } else {
    if (YAXIS == m_CursorCaptured) {
      setCursor(QCursor(Qt::OpenHandCursor));
      m_Yzero = -1;
    } else if (XAXIS == m_CursorCaptured) {
      setCursor(QCursor(Qt::OpenHandCursor));
      m_Xzero = -1;
    }
  }
}


// Make a single zoom step on the X axis.
void
GLWaterfall::zoomStepX(float step, int x)
{
  // calculate new range shown on FFT
  qreal new_range = qBound(10.0,
                           (qreal)(m_Span) * step,
                           (qreal)(m_SampleFreq) * 10.0);

  // Frequency where event occured is kept fixed under mouse
  qreal ratio = (qreal)x / (qreal)m_OverlayPixmap.width();
  qreal fixed_hz = fftFreqFromX(x);
  qreal f_max = fixed_hz + (1.0 - ratio) * new_range;
  qreal f_min = f_max - new_range;

  qint64 fc = (qint64)(f_min + (f_max - f_min) / 2.0);

  setFftCenterFreq(fc);
  setSpanFreq(new_range);

  qreal factor = (qreal)m_SampleFreq / (qreal)m_Span;
  emit newZoomLevel(factor);

  m_PeakHoldValid = false;
}

// Zoom on X axis (absolute level)
void
GLWaterfall::zoomOnXAxis(float level)
{
  float current_level = (float)m_SampleFreq / (float)m_Span;

  zoomStepX(current_level / level, xFromFreq(m_DemodCenterFreq));
}

// Called when a mouse wheel is turned
void
GLWaterfall::wheelEvent(QWheelEvent * event)
{
  QOpenGLWidget::wheelEvent(event);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QPointF pt = event->position();
#else
    QPointF pt = event->pos();
#endif // QT_VERSION
  int numDegrees = event->angleDelta().y() / 8;
  int numSteps = numDegrees / 15;  /** FIXME: Only used for direction **/

  /** FIXME: zooming could use some optimisation **/
  if (m_CursorCaptured == YAXIS) {
    // Vertical zoom. Wheel down: zoom out, wheel up: zoom in
    // During zoom we try to keep the point (dB or kHz) under the cursor fixed
    qreal zoom_fac = event->angleDelta().y() < 0 ? 1. / .9 : 0.9;
    qreal ratio = pt.y() / m_OverlayPixmap.height();
    qreal db_range = m_PandMaxdB - m_PandMindB;
    qreal y_range = m_OverlayPixmap.height();
    qreal db_per_pix = db_range / y_range;
    qreal fixed_db = m_PandMaxdB - pt.y() * db_per_pix;

    db_range = qBound(
          10.,
          db_range * zoom_fac,
          SCAST(qreal, FFT_MAX_DB - FFT_MIN_DB));
    m_PandMaxdB = fixed_db + ratio * db_range;
    if (m_PandMaxdB > FFT_MAX_DB)
      m_PandMaxdB = FFT_MAX_DB;

    m_PandMindB = m_PandMaxdB - db_range;
    m_PeakHoldValid = false;

    emit pandapterRangeChanged(m_PandMindB, m_PandMaxdB);
  } else if (m_CursorCaptured == XAXIS) {
    zoomStepX(event->angleDelta().y() < 0 ? 1.1 : 0.9, pt.x());
  } else if (event->modifiers() & Qt::ControlModifier) {
    // filter width
    m_DemodLowCutFreq -= numSteps * m_ClickResolution;
    m_DemodHiCutFreq += numSteps * m_ClickResolution;
    clampDemodParameters();
    emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
  } else if (event->modifiers() & Qt::ShiftModifier) {
    if (!m_Locked) {
      // filter shift
      m_DemodLowCutFreq += numSteps * m_ClickResolution;
      m_DemodHiCutFreq += numSteps * m_ClickResolution;
      clampDemodParameters();
      emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
    }
  } else {
    if (!m_Locked) {
      // inc/dec demod frequency
      m_DemodCenterFreq += (numSteps * m_ClickResolution);
      m_DemodCenterFreq = roundFreq(m_DemodCenterFreq, m_ClickResolution );
      emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);
    }
  }

  updateOverlay();
}

// Called when screen size changes so must recalculate bitmaps
void
GLWaterfall::resizeEvent(QResizeEvent* ev)
{
  if (ev != nullptr)
    QOpenGLWidget::resizeEvent(ev);

  if (!size().isValid())
    return;

  if (m_Size != size()) {
    // if changed, resize pixmaps to new screensize
    int     fft_plot_height;

    m_Size = size();
    fft_plot_height = m_Percent2DScreen * m_Size.height() / 100;
    m_OverlayPixmap = QPixmap(m_Size.width(), fft_plot_height);
    m_OverlayPixmap.fill(Qt::black);
    m_2DPixmap = QPixmap(m_Size.width(), fft_plot_height);
    m_2DPixmap.fill(Qt::black);

    int height = (100 - m_Percent2DScreen) * m_Size.height() / 100;
    if (m_GLWaterfallImage.isNull()) {
      m_GLWaterfallImage = QImage(
            m_Size.width(),
            height,
            QImage::Format::Format_RGB32);
      m_GLWaterfallImage.fill(Qt::black);
    } else {
      m_GLWaterfallImage = m_GLWaterfallImage.scaled(
            m_Size.width(),
            height,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);
    }

    m_PeakHoldValid = false;

    if (wf_span > 0)
      msec_per_wfline = wf_span / height;
  }

  updateOverlay();
}

void
GLWaterfall::initializeGL(void)
{
  this->glCtx.initialize();
  this->drawOverlay();
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

  this->glCtx.render(0, y, width(), height(), left, right);
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
  painter.setRenderHint(QPainter::Antialiasing);
  int y = m_Percent2DScreen * m_Size.height() / 100;

  painter.drawPixmap(0, 0, m_2DPixmap);

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
GLWaterfall::paintTimeStamps(
    QPainter &painter,
    QRect const &where)
{
  QFontMetrics metrics(m_Font);
  int y = where.y();
  int textWidth;
  int textHeight = metrics.height();
  int items = 0;
  int leftSpacing = 0;

  auto it = m_TimeStamps.begin();

  painter.setFont(m_Font);

  y += m_TimeStampCounter;

  if (m_TimeStampMaxHeight < where.height())
    m_TimeStampMaxHeight = where.height();

  painter.setPen(QPen(m_TimeStampColor));

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  leftSpacing = metrics.horizontalAdvance("00:00:00.000");
#else
  leftSpacing = metrics.width("00:00:00.000");
#endif // QT_VERSION_CHECK

  while (y < m_TimeStampMaxHeight + textHeight && it != m_TimeStamps.end()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    textWidth = metrics.horizontalAdvance(it->timeStampText);
#else
    textWidth = metrics.width(it->timeStampText);
#endif // QT_VERSION_CHECK

    if (it->marker) {
      painter.drawText(
            where.x() + where.width() - textWidth - 2,
            y - 2,
            it->timeStampText);
      painter.drawLine(where.x() + leftSpacing, y, where.width() - 1, y);
    } else {
      painter.drawText(where.x(), y - 2, it->timeStampText);
      painter.drawLine(where.x(), y, textWidth + where.x(), y);
    }

    y += it->counter;
    ++it;
    ++items;
  }

  // TimeStamps from here could not be painted due to geometry restrictions.
  // we silently discard them.

  while (items < m_TimeStamps.size())
    m_TimeStamps.removeLast();
}


void
GLWaterfall::drawFilterCutOff(QPainter &painter, int y)
{
  int h = painter.device()->height();
  QPen pen = QPen(m_TimeStampColor);
  pen.setStyle(Qt::DashLine);
  pen.setWidth(1);

  painter.save();
  painter.setPen(pen);
  painter.setOpacity(1);

  painter.drawLine(
        m_DemodLowCutFreqX,
        y,
        m_DemodLowCutFreqX,
        h - 1);

  painter.drawLine(
        m_DemodHiCutFreqX,
        y,
        m_DemodHiCutFreqX,
        h - 1);
  painter.restore();
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
          m_fftData, m_fftbuf,
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
GLWaterfall::draw()
{
  int     w;
  int     h;


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
 * @param fftData Pointer to the new FFT data (same data for pandapter and waterfall).
 * @param size The FFT size.
 *
 * When FFT data is set using this method, the same data will be used for both the
 * pandapter and the waterfall.
 */
void
GLWaterfall::setNewFftData(
    float *fftData,
    int size,
    QDateTime const &t,
    bool looped)
{
  if (m_tentativeCenterFreq != 0) {
    m_tentativeCenterFreq = 0;
    m_DrawOverlay = true;
  }

  this->setNewFftData(fftData, fftData, size, t, looped);
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

  quint64 tnow_ms = time_ms();

  if (looped) {
    TimeStamp ts;

    ts.counter = m_TimeStampCounter;
    ts.timeStampText =
        this->m_lastFft.toString("hh:mm:ss.zzz")
          + " - "
          + t.toString("hh:mm:ss.zzz");
    ts.marker = true;

    m_TimeStamps.push_front(ts);
    m_TimeStampCounter = 0;
  }

  m_wfData = wfData;
  m_fftData = fftData;
  m_fftDataSize = size;
  m_tentativeCenterFreq = 0;
  m_lastFft = t;

  if (m_TimeStampCounter >= m_TimeStampSpacing) {
    TimeStamp ts;

    ts.counter = m_TimeStampCounter;
    ts.timeStampText = t.toString("hh:mm:ss.zzz");

    m_TimeStamps.push_front(ts);
    m_TimeStampCounter = 0;
  }

  if (m_wfData != nullptr && m_fftDataSize > 0) {
    if (msec_per_wfline > 0) {
      this->glCtx.averageFFTData(m_wfData, m_fftDataSize);

      if (tnow_ms - tlast_wf_ms >= msec_per_wfline) {
        tlast_wf_ms = tnow_ms;
        this->glCtx.commitFFTData();
        ++this->m_TimeStampCounter;
      }
    } else {
      tlast_wf_ms = tnow_ms;
      this->glCtx.pushFFTData(m_wfData, m_fftDataSize);
      ++this->m_TimeStampCounter;
    }
  }

  draw();
}

void
GLWaterfall::getScreenIntegerFFTData(
    qint32 plotHeight,
    qint32 plotWidth,
    float maxdB,
    float mindB,
    qint64 startFreq,
    qint64 stopFreq,
    float *inBuf,
    qint32 *outBuf,
    int *xmin,
    int *xmax)
{
  qint32 i;
  qint32 y;
  qint32 x;
  qint32 ymax = 10000;
  qint32 xprev = -1;
  qint32 minbin, maxbin;
  qint32 m_BinMin, m_BinMax;
  qint32 m_FFTSize = m_fftDataSize;
  float *m_pFFTAveBuf = inBuf;

  mindB -= m_gain;
  maxdB -= m_gain;

  float  dBGainFactor = ((float)plotHeight) / fabs(maxdB - mindB);
  qint32* m_pTranslateTbl = new qint32[qMax(m_FFTSize, plotWidth)];

  /** FIXME: qint64 -> qint32 **/
  m_BinMin = (qint32)((float)startFreq * (float)m_FFTSize / m_SampleFreq);
  m_BinMin += (m_FFTSize/2);
  m_BinMax = (qint32)((float)stopFreq * (float)m_FFTSize / m_SampleFreq);
  m_BinMax += (m_FFTSize/2);

  minbin = qBound(0, m_BinMin, m_FFTSize - 1);
  maxbin = qBound(0, m_BinMax, m_FFTSize - 1);

  bool largeFft = (maxbin - minbin) > plotWidth; // true if more fft point than plot points

  if (largeFft) {
    // more FFT points than plot points
    for (i = minbin; i < maxbin; i++)
      m_pTranslateTbl[i] = ((qint64)(i-m_BinMin)*plotWidth) / (m_BinMax - m_BinMin);
    *xmin = m_pTranslateTbl[minbin];
    *xmax = m_pTranslateTbl[maxbin - 1];
  } else {
    // more plot points than FFT points
    for (i = 0; i < plotWidth; i++)
      m_pTranslateTbl[i] = m_BinMin + (i*(m_BinMax - m_BinMin)) / plotWidth;
    *xmin = 0;
    *xmax = plotWidth;
  }

  if (largeFft) {
    // more FFT points than plot points
    for (i = minbin; i < maxbin; i++) {
      y = (qint32)(dBGainFactor*(maxdB-m_pFFTAveBuf[i]));

      if (y > plotHeight)
        y = plotHeight;
      else if (y < 0)
        y = 0;

      x = m_pTranslateTbl[i];	//get fft bin to plot x coordinate transform

      if (x == xprev) {
        if (y < ymax) {
          outBuf[x] = y;
          ymax = y;
        }
      } else {
        outBuf[x] = y;
        xprev = x;
        ymax = y;
      }
    }
  } else {
    // more plot points than FFT points
    for (x = 0; x < plotWidth; x++) {
      i = m_pTranslateTbl[x]; // get plot to fft bin coordinate transform
      if(i < 0 || i >= m_FFTSize)
        y = plotHeight;
      else
        y = (qint32)(dBGainFactor*(maxdB-m_pFFTAveBuf[i]));

      if (y > plotHeight)
        y = plotHeight;
      else if (y < 0)
        y = 0;

      outBuf[x] = y;
    }
  }

  delete [] m_pTranslateTbl;
}

void
GLWaterfall::setFftRange(float min, float max)
{
  setWaterfallRange(min, max);
  setPandapterRange(min, max);
}

void
GLWaterfall::setPandapterRange(float min, float max)
{
  if (out_of_range(min, max))
    return;

  m_PandMindB = min;
  m_PandMaxdB = max;
  updateOverlay();
  m_PeakHoldValid = false;
}

void
GLWaterfall::setWaterfallRange(float min, float max)
{
  if (out_of_range(min, max))
    return;

  m_WfMindB = min;
  m_WfMaxdB = max;

  this->glCtx.setDynamicRange(min - m_gain, max - m_gain);

  // no overlay change is necessary
}

void
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

  for (auto fat : this->m_FATs) {
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
      this->m_BookmarkSource->getBookmarksInRange(
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
    int yMin = static_cast<int>(this->m_FATs.size()) * ctx.metrics->height();
    while (level < nLevels && tagEnd[level] > x)
      level++;

    if (level == nLevels)
      level = 0;

    tagEnd[level] = x + nameWidth + slant - 1;
    m_BookmarkTags.append(
          qMakePair<QRect, BookmarkInfo>(
            QRect(
              x,
              yMin + level * levelHeight,
              nameWidth + slant, fontHeight),
            bookmarks[i]));

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
GLWaterfall::drawFilterBox(QPainter &painter, int h)
{
  m_DemodFreqX = xFromFreq(m_DemodCenterFreq);
  m_DemodLowCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodLowCutFreq);
  m_DemodHiCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodHiCutFreq);

  int dw = m_DemodHiCutFreqX - m_DemodLowCutFreqX;

  painter.setOpacity(0.3);
  painter.fillRect(m_DemodLowCutFreqX, 0, dw, h, m_FilterBoxColor);

  painter.setOpacity(1.0);
  painter.setPen(QColor(PLOTTER_FILTER_LINE_COLOR));
  painter.drawLine(m_DemodFreqX, 0, m_DemodFreqX, h);

  drawFilterCutOff(painter, h);
}

void
GLWaterfall::drawFilterBox(GLDrawingContext &ctx)
{
  drawFilterBox(*ctx.painter, ctx.height);
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
  m_YAxisWidth = ctx.metrics->horizontalAdvance("-120 ");
  unitWidth    = ctx.metrics->horizontalAdvance(m_unitName);
#else
  m_YAxisWidth = ctx.metrics->width("-120 ");
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
  if (m_BookmarksEnabled && this->m_BookmarkSource != nullptr)
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
  if (this->m_ShowFATs)
    this->drawFATs(ctx, StartFreq, EndFreq);

  if (!m_Running) {
    // if not running so is no data updates to draw to screen
    // copy into 2Dbitmap the overlay bitmap.
    m_2DPixmap = m_OverlayPixmap.copy(0, 0, ctx.width, ctx.height);

    // trigger a new paintEvent
    update();
  }

  painter.end();
}

static QString
formatFreqUnits(int units)
{
  switch (units) {
    case 1:
      return QString("");

    case 1000:
      return QString("K");

    case 1000000:
      return QString("M");

    case 1000000000:
      return QString("G");
  }

  return QString("");
}


// Create frequency division strings based on start frequency, span frequency,
// and frequency units.
// Places in QString array m_HDivText
// Keeps all strings the same fractional length
void GLWaterfall::makeFrequencyStrs()
{
    qint64  StartFreq = m_StartFreqAdj;
    float   freq;
    int     i,j;

    if ((1 == m_FreqUnits) || (m_FreqDigits == 0))
    {
        // if units is Hz then just output integer freq
        for (int i = 0; i <= m_HorDivs; i++)
        {
            freq = (float)StartFreq/(float)m_FreqUnits;
            m_HDivText[i].setNum((int)freq);
            StartFreq += m_FreqPerDiv;
        }
        return;
    }
    // here if is fractional frequency values
    // so create max sized text based on frequency units
    for (int i = 0; i <= m_HorDivs; i++)
    {
        freq = (float)StartFreq / (float)m_FreqUnits;
        m_HDivText[i].setNum(freq,'f', m_FreqDigits);
        StartFreq += m_FreqPerDiv;
    }
    // now find the division text with the longest non-zero digit
    // to the right of the decimal point.
    int max = 0;
    for (i = 0; i <= m_HorDivs; i++)
    {
        int dp = m_HDivText[i].indexOf('.');
        int l = m_HDivText[i].length()-1;
        for (j = l; j > dp; j--)
        {
            if (m_HDivText[i][j] != '0')
                break;
        }
        if ((j - dp) > max)
            max = j - dp;
    }
    // truncate all strings to maximum fractional length
    StartFreq = m_StartFreqAdj;
    for (i = 0; i <= m_HorDivs; i++)
    {
        freq = (float)StartFreq/(float)m_FreqUnits;
        m_HDivText[i].setNum(freq,'f', max);
        m_HDivText[i] += formatFreqUnits(m_FreqUnits);
        StartFreq += m_FreqPerDiv;
    }
}

// Convert from screen coordinate to frequency
int GLWaterfall::xFromFreq(qint64 freq)
{
    int w = m_OverlayPixmap.width();
    qint64 StartFreq = m_CenterFreq + m_FftCenter - m_Span/2;
    int x = (int) w * ((qreal)freq - StartFreq)/(qreal)m_Span;
    if (x < 0)
        return 0;
    if (x > (int)w)
        return m_OverlayPixmap.width();
    return x;
}

// Convert from screen coordinate to Fft frequency
qint64 GLWaterfall::fftFreqFromX(int x)
{
    int w = width();
    qreal f0 = m_FftCenter - m_Span / 2;
    qint64 f = (qint64) (f0 + (qreal) m_Span * (qreal) x / (qreal) w);
    return f;
}

// Convert from frequency to screen coordinate
qint64 GLWaterfall::freqFromX(int x)
{
    int w = width();
    qint64 StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
    qint64 f = (qint64)(StartFreq + (qreal)m_Span * (qreal)x / (qreal)w);
    return f;
}

/** Calculate time offset of a given line on the waterfall */
quint64 GLWaterfall::msecFromY(int y)
{
    // ensure we are in the waterfall region
    if (y < m_OverlayPixmap.height())
        return 0;

    int dy = y - m_OverlayPixmap.height();

    if (msec_per_wfline > 0)
        return tlast_wf_ms - dy * msec_per_wfline;
    else
        return tlast_wf_ms - dy * 1000 / fft_rate;
}

// Round frequency to click resolution value
qint64 GLWaterfall::roundFreq(qint64 freq, int resolution)
{
    qint64 delta = resolution;
    qint64 delta_2 = delta / 2;
    if (freq >= 0)
        return (freq - (freq + delta_2) % delta + delta_2);
    else
        return (freq - (freq + delta_2) % delta - delta_2);
}

// Clamp demod freqeuency limits of m_DemodCenterFreq
void GLWaterfall::clampDemodParameters()
{
    if(m_DemodLowCutFreq < m_FLowCmin)
        m_DemodLowCutFreq = m_FLowCmin;
    if(m_DemodLowCutFreq > m_FLowCmax)
        m_DemodLowCutFreq = m_FLowCmax;

    if(m_DemodHiCutFreq < m_FHiCmin)
        m_DemodHiCutFreq = m_FHiCmin;
    if(m_DemodHiCutFreq > m_FHiCmax)
        m_DemodHiCutFreq = m_FHiCmax;
}

void GLWaterfall::setDemodRanges(qint64 FLowCmin, qint64 FLowCmax,
                              qint64 FHiCmin, qint64 FHiCmax,
                              bool symetric)
{
    m_FLowCmin=FLowCmin;
    m_FLowCmax=FLowCmax;
    m_FHiCmin=FHiCmin;
    m_FHiCmax=FHiCmax;
    m_symetric=symetric;
    clampDemodParameters();
    updateOverlay();
}

void GLWaterfall::setCenterFreq(qint64 f)
{
  f = boundCenterFreq(f);

  if (m_CenterFreq == f)
    return;

  m_tentativeCenterFreq += f - m_CenterFreq;
  m_CenterFreq = f;

  updateOverlay();

  m_PeakHoldValid = false;
}

void GLWaterfall::setFrequencyLimits(qint64 min, qint64 max)
{
  this->m_lowerFreqLimit = min;
  this->m_upperFreqLimit = max;

  if (this->m_enforceFreqLimits)
    this->setCenterFreq(this->m_CenterFreq);
}

void GLWaterfall::setFrequencyLimitsEnabled(bool enabled)
{
  this->m_enforceFreqLimits = enabled;

  if (this->m_enforceFreqLimits)
    this->setCenterFreq(this->m_CenterFreq);
}

// Ensure overlay is updated by either scheduling or forcing a redraw
void GLWaterfall::updateOverlay()
{
  if (m_Running) {
    m_DrawOverlay = true;
    // If the update rate is slow, draw now.
    if (this->slow())
      draw();
  } else {
    // Not the case. Draw now.
    drawOverlay();
  }
}

/** Reset horizontal zoom to 100% and centered around 0. */
void GLWaterfall::resetHorizontalZoom(void)
{
    setFftCenterFreq(0);
    setSpanFreq(static_cast<qint64>(m_SampleFreq));
    m_PeakHoldValid = false;
    emit newZoomLevel(1);
}

/** Center FFT plot around 0 (corresponds to center freq). */
void GLWaterfall::moveToCenterFreq(void)
{
    setFftCenterFreq(0);
    updateOverlay();
    m_PeakHoldValid = false;
}

/** Center FFT plot around the demodulator frequency. */
void GLWaterfall::moveToDemodFreq(void)
{
    setFftCenterFreq(m_DemodCenterFreq-m_CenterFreq);
    updateOverlay();

    m_PeakHoldValid = false;
}

/** Set FFT plot color. */
void GLWaterfall::setFftPlotColor(const QColor color)
{
    m_FftColor = color;
    m_FftFillCol = color;
    m_FftFillCol.setAlpha(0x1A);
    m_PeakHoldColor = color;
    m_PeakHoldColor.setAlpha(60);
    updateOverlay();
}

/** Set filter box color */
void GLWaterfall::setFilterBoxColor(const QColor color)
{
  m_FilterBoxColor = color;
  updateOverlay();
}

/** Set timestamp color */
void GLWaterfall::setTimeStampColor(const QColor color)
{
  m_TimeStampColor = color;
  updateOverlay();
}

/** Set FFT bg color. */
void GLWaterfall::setFftBgColor(const QColor color)
{
    m_FftBgColor = color;
    updateOverlay();
}

/** Set FFT axes color. */
void GLWaterfall::setFftAxesColor(const QColor color)
{
    m_FftCenterAxisColor = color;
    m_FftAxesColor = color;
}

/** Set FFT text color. */
void GLWaterfall::setFftTextColor(const QColor color)
{
    m_FftTextColor = color;
    updateOverlay();
}

/** Enable/disable filling the area below the FFT plot. */
void GLWaterfall::setFftFill(bool enabled)
{
    m_FftFill = enabled;
}

/** Set peak hold on or off. */
void GLWaterfall::setPeakHold(bool enabled)
{
    m_PeakHoldActive = enabled;
    m_PeakHoldValid = false;
}

/**
 * Set peak detection on or off.
 * @param enabled The new state of peak detection.
 * @param c Minimum distance of peaks from mean, in multiples of standard deviation.
 */
void GLWaterfall::setPeakDetection(bool enabled, float c)
{
    if(!enabled || c <= 0)
        m_PeakDetection = -1;
    else
        m_PeakDetection = c;
}

void GLWaterfall::calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs)
{
#ifdef PLOTTER_DEBUG
    qDebug() << "low: " << low;
    qDebug() << "high: " << high;
    qDebug() << "divswanted: " << divswanted;
#endif

    if (divswanted == 0)
        return;

    static const qint64 stepTable[] = { 1, 2, 5 };
    static const int stepTableSize = sizeof (stepTable) / sizeof (stepTable[0]);
    qint64 multiplier = 1;
    step = 1;
    qint64 divsLong = high - low;
    int index = 0;
    adjlow = (low / step) * step;

    while (divsLong > divswanted)
    {
        step = stepTable[index] * multiplier;
        divsLong = (high - low) / step;
        adjlow = (low / step) * step;
        index = index + 1;
        if (index == stepTableSize)
        {
            index = 0;
            multiplier = multiplier * 10;
        }
    }
    if (adjlow < low)
        adjlow += step;


    divs = static_cast<int>(divsLong);

#ifdef PLOTTER_DEBUG
    qDebug() << "adjlow: "  << adjlow;
    qDebug() << "step: " << step;
    qDebug() << "divs: " << divs;
#endif
}

void GLWaterfall::pushFAT(const FrequencyAllocationTable *fat)
{
  this->m_FATs[fat->getName()] = fat;

  if (this->m_ShowFATs)
    this->updateOverlay();
}

bool GLWaterfall::removeFAT(std::string const &name)
{
  auto p = this->m_FATs.find(name);

  if (p == this->m_FATs.end())
    return false;

  this->m_FATs.erase(p);

  if (this->m_ShowFATs)
    this->updateOverlay();

  return true;
}

void GLWaterfall::setFATsVisible(bool visible)
{
  this->m_ShowFATs = visible;
  this->updateOverlay();
}
