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
#ifndef GL_WATERFALL_H
#define GL_WATERFALL_H

#include "AbstractWaterfall.h"
#include <QOpenGLFunctions>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <QOpenGLVertexArrayObject>
#  include <QOpenGLBuffer>
#  include <QOpenGLTexture>
#  include <QOpenGLShader>
#  include <QOpenGLShaderProgram>
#endif

//
// CX:       1 bin,  1 level
// BBCX:     2 bins, 2 levels
// AAAABBCX: 4 bins, 3 levels
//

class GLLine : public std::vector<float>
{
  int m_levels = 0;

  public:
  inline void initialize()
  {
    assign(size(), 0);
  }

  static inline int
  allocationFor(int res)
  {
    return res << 1;
  }

  static inline int
  resolutionFor(int alloc)
  {
    return alloc >> 1;
  }

  inline void
  setResolution(int res)
  {
    m_levels = static_cast<int>(ceil(log2(res))) + 1;
    resize(static_cast<size_t>(allocationFor(res)));
    initialize();
  }

  inline int
  allocation() const
  {
    return static_cast<int>(size());
  }

  inline int
  resolution() const
  {
    return resolutionFor(allocation());
  }

  inline void
  setValueMax(int index, float val)
  {
    int p = 0;
    int res = resolution();
    int l = m_levels;
    float *data = this->data();
    size_t i;

#pragma GCC unroll 4
    while (l-- > 0) {
      i = static_cast<size_t>(p + index);
      data[i] = fmaxf(val, data[i]);

      p += res;

      index >>= 1;
      res   >>= 1;
    }
  }

  inline void
  setValueMean(int index, float val)
  {
    int p = 0;
    int res = resolution();
    int l = m_levels;
    float *data = this->data();
    float k = 1.;
    size_t i;

#pragma GCC unroll 4
    while (l-- > 0) {
      i = static_cast<size_t>(p + index);
      data[i] += k * val;

      p += res;

      index >>= 1;
      res   >>= 1;
      k     *= .5f;
    }
  }

  void normalize();

  void rescaleMean();
  void rescaleMax();

  void assignMean(const float *values);
  void assignMax(const float *values);

  void reduceMean(const float *values, int length);
  void reduceMax(const float *values, int length);
};

typedef std::list<GLLine> GLLineHistory;

struct GLWaterfallOpenGLContext {
  QOpenGLFunctions        *m_functions = nullptr;
  QOpenGLVertexArrayObject m_vao;
  QOpenGLBuffer            m_vbo;
  QOpenGLBuffer            m_ibo;
  QOpenGLShaderProgram     m_program;
  QOpenGLTexture          *m_waterfall      = nullptr;
  QOpenGLTexture          *m_palette        = nullptr;
  QOpenGLShader           *m_vertexShader   = nullptr;
  QOpenGLShader           *m_fragmentShader = nullptr;
  GLLineHistory            m_history, m_pool;
  std::vector<uint8_t>     m_paletBuf;
  bool                     m_firstAccum = true;

  // Texture geometry
  int                      m_row        = 0;
  int                      m_rowSize    = 8192;
  int                      m_rowCount   = 2048;
  int                      m_maxRowSize = 0;
  bool                     m_useMaxBlending = false;

  // Level adjustment
  float                    m_m             = 1.f;
  float                    m_x0            = 0.f;
  bool                     m_updatePalette = false;

  // Geometric parameters
  float                    m_c_x0     = 0;
  float                    m_c_x1     = 0;
  float                    m_zoom     = 1;
  int                      m_width    = 0;
  int                      m_height   = 0;

  GLWaterfallOpenGLContext();
  ~GLWaterfallOpenGLContext();

  void                     initialize();
  void                     finalize();

  void                     recalcGeometric(int, int, float);
  void                     setPalette(const QColor *table);
  void                     pushFFTData(const float *fftData, int size);
  void                     flushOneLine();
  void                     disposeLastLine();
  void                     flushLinesBulk();
  void                     flushLines();
  void                     flushLinePool();
  void                     flushPalette();
  void                     setDynamicRange(float, float);
  void                     resetWaterfall();
  void                     render(int, int, int, int, float, float);
};


class GLWaterfall : public AbstractWaterfall
{
  Q_OBJECT

  GLWaterfallOpenGLContext m_glCtx;

  public:
    explicit GLWaterfall(QWidget *parent = nullptr);
    ~GLWaterfall() override;

    void initializeGL() override;
    void paintGL() override;

    bool isHdpiAware() override { return true; }

    void setPalette(const QColor *table) override;

    void setMaxBlending(bool val) override;

    void setWaterfallRange(float min, float max) override;

    void clearWaterfall() override;
    bool saveWaterfall(const QString & filename) const override;

  public slots:
    // Behavioral slots
    void onContextBeingDestroyed();

  protected:
    void addNewWfLine(const float *wfData, int size, int repeats) override;
};

#endif // GL_WATERFALL_H
