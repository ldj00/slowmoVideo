/*
This file is part of slowmoVideo.
Copyright (C) 2011  Simon A. Eugster (Granjow)  <simon.eu@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "slowmoRenderer_sV.h"

#include "project/project_sV.h"
#include "project/xmlProjectRW_sV.h"
#include "project/renderTask_sV.h"
#include "project/imagesRenderTarget_sV.h"
#include "project/videoRenderTarget_sV.h"
#include "project/v3dFlowSource_sV.h"

#include <iostream>

Error::Error(std::string message) :
    message(message) {}

SlowmoRenderer_sV::SlowmoRenderer_sV() :
    m_project(NULL),
    m_taskSize(0),
    m_lastProgress(0),
    m_start(0),
    m_end(0),
    m_renderTargetSet(false)
{
}

SlowmoRenderer_sV::~SlowmoRenderer_sV()
{
    delete m_project;
}

void SlowmoRenderer_sV::load(QString filename) throw(Error)
{
    if (m_project != NULL) {
        delete m_project;
        m_project = NULL;
    }
    QString warning;
    try {
        Project_sV *proj = XmlProjectRW_sV::loadProject(QString(filename), &warning);

        if (warning.length() > 0) {
            std::cout << warning.toStdString() << std::endl;
        }

        m_project = proj;

        RenderTask_sV *task = new RenderTask_sV(m_project);
        m_project->replaceRenderTask(task);
        m_start = m_project->nodes()->startTime();
        m_end = m_project->nodes()->endTime();
        task->setTimeRange(m_start, m_end);

        bool b = true;
        b &= connect(m_project->renderTask(), SIGNAL(signalNewTask(QString,int)), this, SLOT(slotTaskSize(QString,int)));
        b &= connect(m_project->renderTask(), SIGNAL(signalTaskProgress(int)), this, SLOT(slotProgressInfo(int)));
        b &= connect(m_project->renderTask(), SIGNAL(signalRenderingAborted(QString)), this, SLOT(slotFinished()));
        b &= connect(m_project->renderTask(), SIGNAL(signalRenderingFinished(QString)), this, SLOT(slotFinished()));
        b &= connect(m_project->renderTask(), SIGNAL(signalRenderingStopped(QString)), this, SLOT(slotFinished()));
        Q_ASSERT(b);

    } catch (Error_sV &err) {
        throw Error(err.message().toStdString());
    }
}

void SlowmoRenderer_sV::setTimeRange(double start, double end)
{
    m_start = start;
    m_end = end;
    m_project->renderTask()->setTimeRange(m_start, m_end);
}
void SlowmoRenderer_sV::setStart(double start)
{
    m_start = start;
    m_end = qMax(m_end, m_start);
    m_project->renderTask()->setTimeRange(m_start, m_end);
}
void SlowmoRenderer_sV::setEnd(double end)
{
    m_end = end;
    m_project->renderTask()->setTimeRange(m_start, m_end);
}

void SlowmoRenderer_sV::setFps(double fps)
{
    m_project->renderTask()->setFPS(fps);
}

void SlowmoRenderer_sV::setVideoRenderTarget(QString filename, QString codec)
{
    VideoRenderTarget_sV *vrt = new VideoRenderTarget_sV(m_project->renderTask());
    vrt->setTargetFile(QString(filename));
    vrt->setVcodec(QString(codec));
    m_project->renderTask()->setRenderTarget(vrt);
    m_renderTargetSet = true;
}

void SlowmoRenderer_sV::setImagesRenderTarget(QString filenamePattern, QString directory)
{
    ImagesRenderTarget_sV *irt = new ImagesRenderTarget_sV(m_project->renderTask());
    irt->setFilenamePattern(QString(filenamePattern));
    irt->setTargetDir(QString(directory));
    m_project->renderTask()->setRenderTarget(irt);
    m_renderTargetSet = true;
}

void SlowmoRenderer_sV::setInterpolation(InterpolationType interpolation)
{
    m_project->renderTask()->setInterpolationType(interpolation);
}

void SlowmoRenderer_sV::setSize(bool original)
{
    if (original) {
        m_project->renderTask()->setSize(FrameSize_Orig);
    } else {
        m_project->renderTask()->setSize(FrameSize_Small);
    }
}

void SlowmoRenderer_sV::setV3dLambda(float lambda)
{
    V3dFlowSource_sV *v3d;
    if ((v3d = dynamic_cast<V3dFlowSource_sV*>(m_project->flowSource())) != NULL) {
        v3d->setLambda(lambda);
    } else {
        std::cout << "Could not set v3d lambda; Not a v3d flow source." << std::endl;
    }
}

void SlowmoRenderer_sV::start()
{
    m_project->renderTask()->slotContinueRendering();
}
void SlowmoRenderer_sV::abort()
{
    m_project->renderTask()->slotStopRendering();
}


void SlowmoRenderer_sV::slotProgressInfo(int progress)
{
    m_lastProgress = progress;
}
void SlowmoRenderer_sV::slotTaskSize(QString desc, int size)
{
    std::cout << desc.toStdString() << std::endl;
    m_taskSize = size;
}

void SlowmoRenderer_sV::slotFinished()
{
    std::cout << std::endl << "Rendering finished." << std::endl;
}


void SlowmoRenderer_sV::printProgress()
{
    std::cout << m_lastProgress << "/" << m_taskSize << std::endl;
}

bool SlowmoRenderer_sV::isComplete(QString &message) const
{
    bool b = true;
    if (!m_renderTargetSet) {
        b = false;
        message.append("No render target set.\n");
    }
    return b;
}

