#include "PrintController.h"

#include "Imc60gApi.h"
#include "Imc60gMotionController.h"
#include "PrintHardwarePreflight.h"
#include "PrintHardwareProfile.h"
#include "PrintFlowLogger.h"
#include "PrintJobRunner.h"
#include "PrintPositionSampler.h"
#include "Sv660nExposureController.h"
#include "V2D3DFramePresenter.h"
#include "V2PrintTiming.h"

#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMetaObject>
#include <QPointer>
#include <QThread>
#include <QTimer>

#include <atomic>

class PrintController::Worker final : public QObject
{
public:
    Worker(const QString& projectRoot, PrintController* owner)
        : owner_(owner)
        , profile_(loadPrintHardwareProfile(
              QDir(projectRoot).filePath("config/imc60g_print.ini"), &profileError_))
        , motion_(&api_, profile_)
        , exposure_(api_, profile_)
        , preflight_(motion_, exposure_, presenter_)
        , runner_(new PrintJobRunner(motion_, exposure_, presenter_, preflight_,
              PrintImageQueueFactory(),
              std::make_shared<AsyncPrintFlowLogger>(
                  QDir(projectRoot).filePath("print_flow.log")),
              this))
        , pollTimer_(new QTimer(this))
        , config_(loadPrint9030Config(
              QDir(projectRoot).filePath("config/print_9030.ini")))
    {
        pollTimer_->setInterval(250);
        connect(pollTimer_, &QTimer::timeout, this, [this] { pollPositions(); });
        connect(runner_, &PrintJobRunner::stateChanged, this,
            [this](PrintJobState state) {
                switch (state) {
                case PrintJobState::Ready: setState(PrintUiState::Ready); break;
                case PrintJobState::Printing: setState(PrintUiState::Printing); break;
                case PrintJobState::Paused: setState(PrintUiState::Paused); break;
                case PrintJobState::Stopping: setState(PrintUiState::Stopping); break;
                case PrintJobState::Fault: setState(PrintUiState::Fault); break;
                }
            });
        connect(runner_, &PrintJobRunner::progressChanged, this,
            [this](int value, const QString& detail) {
                post([value, detail](PrintController* owner) {
                    emit owner->progressChanged(value, detail);
                });
            });
        connect(runner_, &PrintJobRunner::frameAdvanced, this,
            [this](int, int, int) { publishPrintingPosition(); });
        connect(runner_, &PrintJobRunner::finished, this,
            [this](bool success, const QString& detail) {
                busy_ = false;
                const bool safelyCancelled = !success
                    && runner_->state() == PrintJobState::Ready;
                post([success, detail, safelyCancelled](PrintController* owner) {
                    emit owner->statusChanged(safelyCancelled
                            ? QStringLiteral("Print job cancelled; ready for the next job.")
                            : detail);
                    if (safelyCancelled) emit owner->errorChanged(QString());
                    else if (!success) emit owner->errorChanged(detail);
                    emit owner->safeStopCompleted();
                });
            });
    }

    void initialize()
    {
        positionClock_.start();
        pollTimer_->start();
    }

    void connectAndHome()
    {
        if (busy_ || (state_ != PrintUiState::Disconnected && state_ != PrintUiState::Fault)) return;
        if (!profileError_.isEmpty()) {
            fail(profileError_);
            return;
        }
        busy_ = true;
        setState(PrintUiState::Connecting);
        post([](PrintController* owner) { emit owner->statusChanged("Opening IMC60G Card0 and EtherCAT..."); });
        setState(PrintUiState::Homing);
        QString detail;
        if (!motion_.connectAndHome(&detail)) {
            busy_ = false;
            fail(detail.isEmpty() ? "IMC60G connect/home failed." : detail);
            return;
        }
        busy_ = false;
        setState(PrintUiState::Ready);
        publishHardwareStatus();
        pollPositions();
        post([](PrintController* owner) {
            emit owner->statusChanged("IMC60G Card0 ready; Y(Axis0) and X(Axis1) homed.");
            emit owner->errorChanged(QString());
        });
    }

    void disconnect()
    {
        if (busy_ || state_ == PrintUiState::Printing || state_ == PrintUiState::Paused
            || state_ == PrintUiState::Stopping) return;
        busy_ = true;
        QString detail;
        const bool ok = motion_.disconnect(&detail);
        presenter_.shutdown();
        busy_ = false;
        if (!ok) {
            fail(detail.isEmpty() ? "IMC60G disconnect failed." : detail);
            return;
        }
        setState(PrintUiState::Disconnected);
        post([](PrintController* owner) {
            emit owner->hardwareStatusChanged(false, false, false, false, false);
            emit owner->statusChanged("IMC60G disconnected.");
        });
    }

    void move(PrintHardwareProfile::LogicalAxis axis, double millimeters,
        const PrintAxisConfig& config)
    {
        if (busy_ || state_ != PrintUiState::Ready) return;
        busy_ = true;
        QString detail;
        if (axis == PrintHardwareProfile::LogicalAxis::X) config_.axisX = config;
        else config_.axisY = config;
        const bool ok = motion_.moveRelative(axis, millimeters, config, &detail);
        busy_ = false;
        if (!ok) fail(detail.isEmpty() ? "Manual IMC60G move failed." : detail);
        else pollPositions();
    }

    void stopManualMotion()
    {
        if (state_ != PrintUiState::Ready || busy_) return;
        busy_ = true;
        QString xError;
        QString yError;
        const bool xStopped = motion_.stopAxis(PrintHardwareProfile::LogicalAxis::X, &xError);
        const bool yStopped = motion_.stopAxis(PrintHardwareProfile::LogicalAxis::Y, &yError);
        busy_ = false;
        if (!xStopped || !yStopped) fail((xError + " " + yError).trimmed());
        else pollPositions();
    }

    void setLogicalOrigin()
    {
        if (busy_ || state_ != PrintUiState::Ready) return;
        busy_ = true;
        QString detail;
        const bool ok = motion_.setCurrentPositionAsLogicalOrigin(&detail);
        busy_ = false;
        if (!ok) {
            fail(detail.isEmpty() ? "Setting IMC60G logical origin failed." : detail);
            return;
        }
        post([](PrintController* owner) {
            emit owner->positionsChanged(0.0, 0.0);
            emit owner->statusChanged("IMC60G X/Y logical origin set.");
            emit owner->errorChanged(QString());
        });
    }

    void returnToLogicalOrigin()
    {
        if (busy_ || state_ != PrintUiState::Ready) return;
        busy_ = true;
        QString detail;
        const int timeoutMs = 5000;
        const bool returned = motion_.returnToLogicalZeroWhenReady(config_.axisX,
            config_.axisY, timeoutMs, &detail);
        const bool verified = returned && motion_.verifyLogicalZero(&detail);
        busy_ = false;
        if (!verified) {
            fail(detail.isEmpty() ? "Returning IMC60G X/Y to logical origin failed." : detail);
            return;
        }
        if (!publishPositions(&detail)) {
            fail(detail.isEmpty() ? "Refreshing IMC60G origin position failed." : detail);
            return;
        }
        post([](PrintController* owner) {
            emit owner->statusChanged("IMC60G X/Y returned to logical origin.");
            emit owner->errorChanged(QString());
        });
    }

    void start(const Print9030Config& config, const PrintImageSet& images)
    {
        if (busy_ || state_ != PrintUiState::Ready || !images.isValid()) return;
        config_ = config;
        QString detail;
        PrintJobSnapshot job;
        job.config = config;
        job.profile = profile_;
        job.images = images;
        if (!presenter_.refreshAttachedDisplays(&detail)) {
            fail(detail.isEmpty() ? "Cannot enumerate attached displays." : detail);
            return;
        }
        const double refreshHz = presenter_.selectedRefreshHz(&detail);
        if (refreshHz <= 0.0) {
            fail(detail.isEmpty() ? "Cannot determine the print-display refresh rate." : detail);
            return;
        }
        job.plan = buildV2PrintPlan(config, profile_, refreshHz, &detail);
        if (!detail.isEmpty() || job.plan.rows.isEmpty()) {
            fail(detail.isEmpty() ? "V2 print timing plan is empty." : detail);
            return;
        }
        busy_ = true;
        if (!runner_->start(job, &detail) && runner_->state() != PrintJobState::Ready) {
            busy_ = false;
            if (!detail.isEmpty()) fail(detail);
        }
        if (runner_->state() != PrintJobState::Printing) busy_ = false;
    }

    void resume()
    {
        if (state_ != PrintUiState::Paused || busy_) return;
        busy_ = true;
        QString detail;
        if (!runner_->resume(&detail) && runner_->state() == PrintJobState::Fault)
            fail(detail);
        busy_ = runner_->state() == PrintJobState::Printing;
    }

    void processCancel()
    {
        if (runner_->state() == PrintJobState::Paused) {
            QString detail;
            runner_->processPendingControl(&detail);
            if (!detail.isEmpty()) post([detail](PrintController* owner) { emit owner->errorChanged(detail); });
        }
    }

    void shutdown()
    {
        runner_->cancel();
        motion_.requestCancellation();
        processCancel();
        if (motion_.state() != Imc60gConnectionState::Disconnected) {
            QString detail;
            motion_.disconnect(&detail);
            if (!detail.isEmpty()) post([detail](PrintController* owner) { emit owner->errorChanged(detail); });
        }
        presenter_.shutdown();
    }

    PrintJobRunner* runner() const { return runner_; }
    Imc60gMotionController* motion() { return &motion_; }

private:
    template <typename Function>
    void post(Function function)
    {
        QPointer<PrintController> owner(owner_);
        QMetaObject::invokeMethod(owner_, [owner, function] {
            if (owner) function(owner.data());
        }, Qt::QueuedConnection);
    }

    void setState(PrintUiState state)
    {
        state_ = state;
        post([state](PrintController* owner) { emit owner->stateChanged(state); });
    }

    void fail(const QString& detail)
    {
        setState(PrintUiState::Fault);
        post([detail](PrintController* owner) {
            emit owner->errorChanged(detail);
            emit owner->statusChanged("IMC60G fault; disconnect or reconnect after correcting the detail.");
        });
    }

    void publishHardwareStatus()
    {
        QString ignored;
        const PrintMotionReadiness readiness = motion_.printReadiness(&ignored);
        post([readiness](PrintController* owner) {
            emit owner->hardwareStatusChanged(readiness.ethercatReady,
                readiness.servosReady, readiness.servosReady,
                readiness.axesHomed, readiness.axesHomed);
        });
    }

    bool publishPositions(QString* errorMessage = nullptr)
    {
        if (errorMessage) errorMessage->clear();
        int xPulses = 0;
        int yPulses = 0;
        QString detail;
        if (!motion_.readMappedPlannedPositions(&xPulses, &yPulses, &detail)) {
            if (errorMessage) *errorMessage = detail;
            return false;
        }
        QPointF position;
        if (!PrintPositionSampler::toMillimeters(xPulses, yPulses,
                config_.axisX, config_.axisY, &position)) {
            if (errorMessage) *errorMessage = "IMC60G position scale is invalid.";
            return false;
        }
        post([position](PrintController* owner) {
            emit owner->positionsChanged(position.x(), position.y());
        });
        return true;
    }

    void pollPositions()
    {
        if (busy_ || state_ != PrintUiState::Ready) return;
        QString detail;
        if (!publishPositions(&detail)) fail(detail);
    }

    void publishPrintingPosition()
    {
        if (state_ != PrintUiState::Printing || !positionSampler_.isDue(positionClock_.elapsed())) {
            return;
        }
        QString detail;
        if (!publishPositions(&detail)) {
            runner_->cancel();
            motion_.requestCancellation();
            post([detail](PrintController* owner) {
                emit owner->errorChanged(detail.isEmpty()
                        ? QStringLiteral("IMC60G position refresh failed; cancelling print safely.")
                        : detail);
            });
        }
    }

    PrintController* owner_;
    QString profileError_;
    Imc60gApi api_;
    PrintHardwareProfile profile_;
    Imc60gMotionController motion_;
    Sv660nExposureController exposure_;
    V2D3DFramePresenter presenter_;
    PrintHardwarePreflight preflight_;
    PrintJobRunner* runner_;
    QTimer* pollTimer_;
    QElapsedTimer positionClock_;
    PrintPositionSampler positionSampler_;
    Print9030Config config_;
    PrintUiState state_ = PrintUiState::Disconnected;
    bool busy_ = false;
};

PrintController::PrintController(const QString& projectRoot, QObject* parent)
    : IPrintController(parent)
    , worker_(new Worker(projectRoot, this))
    , workerThread_(new QThread(this))
{
    qRegisterMetaType<PrintUiState>("PrintUiState");
    worker_->moveToThread(workerThread_);
    connect(workerThread_, &QThread::started, worker_, [this] { worker_->initialize(); });
    workerThread_->start();
}

PrintController::~PrintController()
{
    if (!worker_) return;
    worker_->runner()->cancel();
    worker_->motion()->requestCancellation();
    QEventLoop shutdownLoop;
    connect(worker_, &QObject::destroyed, &shutdownLoop, &QEventLoop::quit);
    Worker* worker = worker_;
    QMetaObject::invokeMethod(worker, [worker] {
        worker->shutdown();
        worker->deleteLater();
    }, Qt::QueuedConnection);
    shutdownLoop.exec();
    worker_ = nullptr;
    workerThread_->quit();
    workerThread_->wait();
}

void PrintController::connectAndHome()
{
    QMetaObject::invokeMethod(worker_, [this] { worker_->connectAndHome(); }, Qt::QueuedConnection);
}

void PrintController::disconnect()
{
    QMetaObject::invokeMethod(worker_, [this] { worker_->disconnect(); }, Qt::QueuedConnection);
}

void PrintController::moveXNegative(double millimeters, const PrintAxisConfig& config)
{
    QMetaObject::invokeMethod(worker_, [this, millimeters, config] {
        worker_->move(PrintHardwareProfile::LogicalAxis::X, -millimeters, config);
    }, Qt::QueuedConnection);
}

void PrintController::moveXPositive(double millimeters, const PrintAxisConfig& config)
{
    QMetaObject::invokeMethod(worker_, [this, millimeters, config] {
        worker_->move(PrintHardwareProfile::LogicalAxis::X, millimeters, config);
    }, Qt::QueuedConnection);
}

void PrintController::moveYNegative(double millimeters, const PrintAxisConfig& config)
{
    QMetaObject::invokeMethod(worker_, [this, millimeters, config] {
        worker_->move(PrintHardwareProfile::LogicalAxis::Y, -millimeters, config);
    }, Qt::QueuedConnection);
}

void PrintController::moveYPositive(double millimeters, const PrintAxisConfig& config)
{
    QMetaObject::invokeMethod(worker_, [this, millimeters, config] {
        worker_->move(PrintHardwareProfile::LogicalAxis::Y, millimeters, config);
    }, Qt::QueuedConnection);
}

void PrintController::stopManualMotion()
{
    QMetaObject::invokeMethod(worker_, [this] { worker_->stopManualMotion(); }, Qt::QueuedConnection);
}

void PrintController::setLogicalOrigin()
{
    QMetaObject::invokeMethod(worker_, [this] { worker_->setLogicalOrigin(); }, Qt::QueuedConnection);
}

void PrintController::returnToLogicalOrigin()
{
    QMetaObject::invokeMethod(worker_, [this] { worker_->returnToLogicalOrigin(); },
        Qt::QueuedConnection);
}

void PrintController::start(const Print9030Config& config, const PrintImageSet& images)
{
    QMetaObject::invokeMethod(worker_, [this, config, images] { worker_->start(config, images); },
        Qt::QueuedConnection);
}

void PrintController::pause()
{
    worker_->runner()->requestPause();
}

void PrintController::resume()
{
    QMetaObject::invokeMethod(worker_, [this] { worker_->resume(); }, Qt::QueuedConnection);
}

void PrintController::cancel()
{
    emit stateChanged(PrintUiState::Stopping);
    worker_->runner()->cancel();
    worker_->motion()->requestCancellation();
    QMetaObject::invokeMethod(worker_, [this] { worker_->processCancel(); }, Qt::QueuedConnection);
}
