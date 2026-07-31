#pragma once

#include "PrintFrame.h"

#include <QSize>
#include <QString>

class IPrintFramePresenter
{
public:
    virtual ~IPrintFramePresenter() = default;

    virtual bool prepare(const PrintFrame& firstFrame, const QSize& targetSize, QString* errorMessage = nullptr) = 0;
    virtual bool present(const PrintFrame& frame, const QSize& targetSize, QString* errorMessage = nullptr) = 0;
    virtual bool waitForDisplayFrame(QString* errorMessage = nullptr) = 0;
    bool waitForVBlank(QString* errorMessage = nullptr) { return waitForDisplayFrame(errorMessage); }
    virtual void shutdown() = 0;
};
