/****************************************************************************
 * This file is part of Hawaii.
 *
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * Author(s):
 *    Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * $BEGIN_LICENSE:QTLGPL$
 *
 * GNU Lesser General Public License Usage
 * This file may be used under the terms of the GNU Lesser General
 * Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.LGPLv3 included in the
 * packaging of this file. Please review the following information to
 * ensure the GNU Lesser General Public License version 3 requirements
 * will be met: https://www.gnu.org/licenses/lgpl.html.
 *
 * GNU General Public License Usage
 * Alternatively, this file may be used under the terms of the GNU
 * General Public License version 2.0 or (at your option) the GNU General
 * Public license version 3 or any later version approved by the KDE Free
 * Qt Foundation. The licenses are as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPLv2 and LICENSE.GPLv3
 * included in the packaging of this file. Please review the following
 * information to ensure the GNU General Public License requirements will
 * be met: https://www.gnu.org/licenses/gpl-2.0.html and
 * https://www.gnu.org/licenses/gpl-3.0.html.
 *
 * $END_LICENSE$
 ***************************************************************************/

#ifndef STATEGUARD_H
#define STATEGUARD_H

#include <QtGui/QOpenGLFunctions>

#define STATE_GUARD_VERTEX_ATTRIB_COUNT 2

class StateGuard
{
public:
    StateGuard()
    {
        QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());

        glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *) &m_program);
        glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint *) &m_activeTextureUnit);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint *) &m_texture);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &m_fbo);
        glGetIntegerv(GL_VIEWPORT, m_viewport);
        glGetIntegerv(GL_DEPTH_WRITEMASK, &m_depthWriteMask);
        glGetIntegerv(GL_COLOR_WRITEMASK, m_colorWriteMask);
        m_blend = glIsEnabled(GL_BLEND);
        m_depth = glIsEnabled(GL_DEPTH_TEST);
        m_cull = glIsEnabled(GL_CULL_FACE);
        m_scissor = glIsEnabled(GL_SCISSOR_TEST);
        for (int i = 0; i < STATE_GUARD_VERTEX_ATTRIB_COUNT; ++i) {
            glFuncs.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, (GLint *) &m_vertexAttribs[i].enabled);
            glFuncs.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, (GLint *) &m_vertexAttribs[i].arrayBuffer);
            glFuncs.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &m_vertexAttribs[i].size);
            glFuncs.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &m_vertexAttribs[i].stride);
            glFuncs.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, (GLint *) &m_vertexAttribs[i].type);
            glFuncs.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, (GLint *) &m_vertexAttribs[i].normalized);
            glFuncs.glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &m_vertexAttribs[i].pointer);
        }
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint *) &m_minFilter);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint *) &m_magFilter);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint *) &m_wrapS);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint *) &m_wrapT);
    }

    ~StateGuard()
    {
        QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());

        glFuncs.glUseProgram(m_program);
        glActiveTexture(m_activeTextureUnit);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
        glDepthMask(m_depthWriteMask);
        glColorMask(m_colorWriteMask[0], m_colorWriteMask[1], m_colorWriteMask[2], m_colorWriteMask[3]);
        if (m_blend)
            glEnable(GL_BLEND);
        if (m_depth)
            glEnable(GL_DEPTH_TEST);
        if (m_cull)
            glEnable(GL_CULL_FACE);
        if (m_scissor)
            glEnable(GL_SCISSOR_TEST);
        for (int i = 0; i < STATE_GUARD_VERTEX_ATTRIB_COUNT; ++i) {
            if (m_vertexAttribs[i].enabled)
                glFuncs.glEnableVertexAttribArray(i);
            GLuint prevBuf;
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *) &prevBuf);
            glFuncs.glBindBuffer(GL_ARRAY_BUFFER, m_vertexAttribs[i].arrayBuffer);
            glFuncs.glVertexAttribPointer(i, m_vertexAttribs[i].size, m_vertexAttribs[i].type,
                                          m_vertexAttribs[i].normalized, m_vertexAttribs[i].stride,
                                          m_vertexAttribs[i].pointer);
            glFuncs.glBindBuffer(GL_ARRAY_BUFFER, prevBuf);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrapT);
    }

private:
    GLuint m_program;
    GLenum m_activeTextureUnit;
    GLuint m_texture;
    GLuint m_fbo;
    GLint m_depthWriteMask;
    GLint m_colorWriteMask[4];
    GLboolean m_blend;
    GLboolean m_depth;
    GLboolean m_cull;
    GLboolean m_scissor;
    GLint m_viewport[4];
    struct VertexAttrib {
        bool enabled;
        GLuint arrayBuffer;
        GLint size;
        GLint stride;
        GLenum type;
        bool normalized;
        void *pointer;
    } m_vertexAttribs[STATE_GUARD_VERTEX_ATTRIB_COUNT];
    GLenum m_minFilter;
    GLenum m_magFilter;
    GLenum m_wrapS;
    GLenum m_wrapT;
};

#endif // STATEGUARD_H

