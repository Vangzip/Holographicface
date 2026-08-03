#include "Print9030Dialog.h"

#include "PrintController.h"
#include "ui_Print9030Dialog.h"

#include <QCloseEvent>
#include <QCheckBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QSpinBox>
#include <QtConcurrent/QtConcurrentRun>

#include <limits>

namespace {

QString sourceText(const PrintImageSet& set)
{
    if (!set.isValid()) return QStringLiteral("未加载");
    if (set.sourceType() == PrintImageSourceType::ElementalMemory) {
        return QStringLiteral("内存 elemental: %1 张")
            .arg(static_cast<qulonglong>(set.imageCount()));
    }
    return QStringLiteral("文件夹: %1 张").arg(static_cast<qulonglong>(set.imageCount()));
}

bool isActive(PrintUiState state)
{
    return state == PrintUiState::Printing || state == PrintUiState::Paused
        || state == PrintUiState::Stopping;
}

QSpinBox* axisIntegerEditor(QTableWidget* table, int row, int column)
{
    return qobject_cast<QSpinBox*>(table->cellWidget(row, column));
}

QDoubleSpinBox* axisDistanceEditor(QTableWidget* table, int row)
{
    return qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, 6));
}

QCheckBox* axisDirectionEditor(QTableWidget* table, int row)
{
    return qobject_cast<QCheckBox*>(table->cellWidget(row, 7));
}

} // namespace

#ifndef PRINT9030_CONTROLLER_INTERFACE_ONLY
Print9030Dialog::Print9030Dialog(const QString& projectRoot, QWidget* parent)
    : Print9030Dialog(projectRoot, new PrintController(projectRoot), parent)
{
    ownsController_ = true;
    controller_->setParent(this);
}
#endif

Print9030Dialog::Print9030Dialog(const QString& projectRoot,
    IPrintController* controller, QWidget* parent)
    : QDialog(parent)
    , projectRoot_(QDir(projectRoot).absolutePath())
    , ui_(new Ui::Print9030Dialog)
    , controller_(controller)
    , stateStorage_(new PrintUiState(PrintUiState::Disconnected))
{
    Q_ASSERT(controller_);
    ui_->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    loadConfig();
    populateUiFromConfig();
    updateSourceSummary();
    applyState(PrintUiState::Disconnected);

    connect(ui_->browseFolderButton, &QPushButton::clicked, this, [this] { browseFolder(); });
    connect(ui_->previewButton, &QPushButton::clicked, this, [this] { showPreview(); });
    connect(ui_->folderPathEdit, &QLineEdit::editingFinished, this, [this] {
        const QString folder = QDir::fromNativeSeparators(
            ui_->folderPathEdit->text().trimmed());
        if (!folder.isEmpty()) setManualImageFolder(folder);
    });
    connect(ui_->connectHomeButton, &QPushButton::clicked,
        controller_, &IPrintController::connectAndHome);
    connect(ui_->disconnectButton, &QPushButton::clicked,
        controller_, &IPrintController::disconnect);
    connect(ui_->xNegativeButton, &QPushButton::clicked, this,
        [this] {
            updateConfigFromUi();
            controller_->moveXNegative(ui_->manualStepSpin->value(), config_.axisX);
        });
    connect(ui_->xPositiveButton, &QPushButton::clicked, this,
        [this] {
            updateConfigFromUi();
            controller_->moveXPositive(ui_->manualStepSpin->value(), config_.axisX);
        });
    connect(ui_->yNegativeButton, &QPushButton::clicked, this,
        [this] {
            updateConfigFromUi();
            controller_->moveYNegative(ui_->manualStepSpin->value(), config_.axisY);
        });
    connect(ui_->yPositiveButton, &QPushButton::clicked, this,
        [this] {
            updateConfigFromUi();
            controller_->moveYPositive(ui_->manualStepSpin->value(), config_.axisY);
        });
    connect(ui_->manualStopButton, &QPushButton::clicked,
        controller_, &IPrintController::stopManualMotion);
    connect(ui_->setOriginButton, &QPushButton::clicked,
        controller_, &IPrintController::setLogicalOrigin);
    connect(ui_->returnOriginButton, &QPushButton::clicked,
        controller_, &IPrintController::returnToLogicalOrigin);
    connect(ui_->startButton, &QPushButton::clicked, this, [this] { startPrint(); });
    connect(ui_->pauseButton, &QPushButton::clicked, controller_, &IPrintController::pause);
    connect(ui_->resumeButton, &QPushButton::clicked, controller_, &IPrintController::resume);
    connect(ui_->cancelButton, &QPushButton::clicked, controller_, &IPrintController::cancel);
    connect(ui_->closeButton, &QPushButton::clicked, this, [this] { requestClose(); });
    connect(ui_->memorySourceRadio, &QRadioButton::toggled, this, [this] { updateSourceSummary(); });
    connect(ui_->folderSourceRadio, &QRadioButton::toggled, this, [this] { updateSourceSummary(); });
    connect(&folderLoadWatcher_, &QFutureWatcher<FolderLoadCompletion>::finished,
        this, &Print9030Dialog::applyFolderLoadResult);

    connect(controller_, &IPrintController::stateChanged, this, &Print9030Dialog::applyState);
    connect(controller_, &IPrintController::statusChanged, ui_->statusLabel, &QLabel::setText);
    connect(controller_, &IPrintController::errorChanged, ui_->errorDetailLabel, &QLabel::setText);
    connect(controller_, &IPrintController::progressChanged, this,
        [this](int value, const QString& text) {
            ui_->printProgressBar->setValue(value);
            ui_->statusLabel->setText(text);
        });
    connect(controller_, &IPrintController::positionsChanged, this,
        [this](double x, double y) {
            ui_->xPositionLabel->setText(QString("X: %1 mm").arg(x, 0, 'f', 3));
            ui_->yPositionLabel->setText(QString("Y: %1 mm").arg(y, 0, 'f', 3));
        });
    connect(controller_, &IPrintController::hardwareStatusChanged, this,
        [this](bool ethercat, bool servoX, bool servoY, bool homeX, bool homeY) {
            ui_->ethercatStatusLabel->setText(ethercat ? "EtherCAT: OP" : "EtherCAT: 离线");
            ui_->servoStatusLabel->setText(servoX && servoY ? "Servo X/Y: ON" : "Servo X/Y: OFF");
            ui_->homeStatusLabel->setText(homeX && homeY ? "Home X/Y: 完成" : "Home X/Y: 未完成");
        });
    connect(controller_, &IPrintController::safeStopCompleted, this, [this] {
        if (pendingClose_) {
            pendingClose_ = false;
            accept();
        }
    });
}

Print9030Dialog::~Print9030Dialog()
{
    delete stateStorage_;
}

void Print9030Dialog::setElementalMemoryResult(std::shared_ptr<const ElementalMemoryResult> result)
{
    QString error;
    memoryImages_ = makePrintImageSetFromElementalMemory(std::move(result), &error);
    if (memoryImages_.isValid()) ui_->memorySourceRadio->setChecked(true);
    else if (!error.isEmpty()) ui_->errorDetailLabel->setText(error);
    updateSourceSummary();
}

void Print9030Dialog::setManualImageFolder(const QString& folderPath)
{
    ui_->folderPathEdit->setText(QDir::toNativeSeparators(folderPath));
    if (folderPath.trimmed().isEmpty()) {
        ++folderLoadRequestId_;
        folderLoading_ = false;
        folderImages_ = {};
        updateSourceSummary();
        return;
    }
    beginFolderLoad(folderPath);
}

void Print9030Dialog::beginFolderLoad(const QString& folderPath)
{
    const quint64 requestId = ++folderLoadRequestId_;
    folderLoading_ = true;
    folderImages_ = {};
    ui_->folderSourceRadio->setChecked(true);
    ui_->previewLabel->setPixmap(QPixmap());
    ui_->previewLabel->setText(QStringLiteral("正在加载图像..."));
    ui_->errorDetailLabel->setText(QString());
    updateSourceSummary();

    folderLoadWatcher_.setFuture(QtConcurrent::run([folderPath, requestId] {
        FolderLoadCompletion completion;
        completion.source = loadPrintImagesFromFolderWithGridInfo(
            folderPath, &completion.errorMessage);
        completion.requestId = requestId;
        return completion;
    }));
}

void Print9030Dialog::applyFolderLoadResult()
{
    const FolderLoadCompletion completion = folderLoadWatcher_.result();
    if (completion.requestId != folderLoadRequestId_) return;

    folderLoading_ = false;
    if (completion.source.images.isValid()) {
        folderImages_ = completion.source.images;
        ui_->folderSourceRadio->setChecked(true);
        if (completion.source.hasInferredGrid()) {
            ui_->gridRowsSpin->setValue(completion.source.gridRows);
            ui_->gridColumnsSpin->setValue(completion.source.gridColumns);
        }
        ui_->errorDetailLabel->setText(completion.source.gridWarning);
    } else {
        folderImages_ = {};
        ui_->errorDetailLabel->setText(completion.errorMessage.isEmpty()
                ? QStringLiteral("图像文件夹加载失败。") : completion.errorMessage);
    }
    updateSourceSummary();
}

void Print9030Dialog::applyState(PrintUiState state)
{
    *stateStorage_ = state;
    const bool ready = state == PrintUiState::Ready;
    const bool printing = state == PrintUiState::Printing;
    const bool paused = state == PrintUiState::Paused;
    const bool stopping = state == PrintUiState::Stopping;
    const bool locked = printing || paused || stopping;
    const PrintImageSet& source = ui_->folderSourceRadio->isChecked() ? folderImages_ : memoryImages_;

    ui_->connectHomeButton->setEnabled(state == PrintUiState::Disconnected || state == PrintUiState::Fault);
    ui_->disconnectButton->setEnabled(ready || state == PrintUiState::Fault);
    ui_->startButton->setEnabled(ready && source.isValid() && !folderLoading_);
    ui_->pauseButton->setEnabled(printing);
    ui_->resumeButton->setEnabled(paused);
    ui_->cancelButton->setEnabled(printing || paused);
    ui_->manualMotionGroup->setEnabled(ready);
    ui_->setOriginButton->setEnabled(ready);
    ui_->returnOriginButton->setEnabled(ready);
    ui_->sourceGroup->setEnabled(!locked && !folderLoading_);
    ui_->previewButton->setEnabled(!folderLoading_ && source.isValid());
    ui_->configTabs->setEnabled(!locked);
    ui_->closeButton->setEnabled(!stopping);

    const char* names[] = {"未连接", "连接中", "回零中", "就绪", "打印中", "已暂停", "安全停止中", "故障"};
    ui_->stateLabel->setText(QString::fromUtf8(names[static_cast<int>(state)]));
}

void Print9030Dialog::closeEvent(QCloseEvent* event)
{
    if (isActive(*stateStorage_)) {
        pendingClose_ = true;
        controller_->cancel();
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}

void Print9030Dialog::requestClose() { close(); }

void Print9030Dialog::loadConfig()
{
    QString error;
    config_ = loadPrint9030Config(configPath(), &error);
    if (!error.isEmpty()) ui_->errorDetailLabel->setText(error);
}

void Print9030Dialog::saveConfigFromUi()
{
    updateConfigFromUi();
    QString error;
    if (!savePrint9030Config(configPath(), config_, &error))
        ui_->errorDetailLabel->setText(error);
}

void Print9030Dialog::populateUiFromConfig()
{
    ui_->rowSpacingSpin->setValue(config_.main.rowSpacingMm);
    ui_->columnSpacingSpin->setValue(config_.main.columnSpacingMm);
    ui_->gridRowsSpin->setValue(config_.main.gridRows);
    ui_->gridColumnsSpin->setValue(config_.main.gridColumns);
    ui_->widthScaleSpin->setValue(config_.main.widthScale);
    ui_->heightScaleSpin->setValue(config_.main.heightScale);
    ui_->addTempPulseSpin->setValue(config_.main.addTempPulse);
    ui_->leadPulseSpin->setValue(static_cast<int>(config_.main.leadPulse));
    writeAxisToUi(0, config_.axisX);
    writeAxisToUi(1, config_.axisY);
}

void Print9030Dialog::updateConfigFromUi()
{
    config_.main.rowSpacingMm = ui_->rowSpacingSpin->value();
    config_.main.columnSpacingMm = ui_->columnSpacingSpin->value();
    config_.main.gridRows = ui_->gridRowsSpin->value();
    config_.main.gridColumns = ui_->gridColumnsSpin->value();
    config_.main.widthScale = ui_->widthScaleSpin->value();
    config_.main.heightScale = ui_->heightScaleSpin->value();
    config_.main.addTempPulse = ui_->addTempPulseSpin->value();
    config_.main.leadPulse = ui_->leadPulseSpin->value();
    config_.axisX = readAxisFromUi(0);
    config_.axisY = readAxisFromUi(1);
}

void Print9030Dialog::updateSourceSummary()
{
    const PrintImageSet& active = ui_->folderSourceRadio->isChecked() ? folderImages_ : memoryImages_;
    ui_->sourceSummaryLabel->setText(folderLoading_ && ui_->folderSourceRadio->isChecked()
            ? QStringLiteral("文件夹加载中...") : sourceText(active));
    applyState(*stateStorage_);
}

void Print9030Dialog::showPreview()
{
    const PrintImageSet& active = ui_->folderSourceRadio->isChecked()
        ? folderImages_ : memoryImages_;
    PrintFrame frame;
    QString error;
    if (!active.copyFrame(0, &frame, &error) || !frame.isValid()) {
        ui_->previewLabel->setPixmap(QPixmap());
        ui_->previewLabel->setText(error.isEmpty()
                ? QStringLiteral("预览不可用") : error);
        return;
    }
    const QImage::Format format = frame.format == PrintPixelFormat::Bgr24
        ? QImage::Format_BGR888 : QImage::Format_ARGB32;
    const QImage image(reinterpret_cast<const uchar*>(frame.pixels.constData()),
        frame.width, frame.height, frame.stride, format);
    if (image.isNull()) {
        ui_->previewLabel->setPixmap(QPixmap());
        ui_->previewLabel->setText(QStringLiteral("预览图像格式无效"));
        return;
    }
    ui_->previewLabel->setText(QString());
    ui_->previewLabel->setPixmap(QPixmap::fromImage(image.copy()).scaled(
        ui_->previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void Print9030Dialog::browseFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(this,
        QStringLiteral("选择 9030 打印图片文件夹"),
        ui_->folderPathEdit->text().isEmpty() ? projectRoot_ : ui_->folderPathEdit->text());
    if (!folder.isEmpty()) setManualImageFolder(folder);
}

void Print9030Dialog::startPrint()
{
    saveConfigFromUi();
    const PrintImageSet active = ui_->folderSourceRadio->isChecked() ? folderImages_ : memoryImages_;
    if (!active.isValid()) {
        ui_->errorDetailLabel->setText(QStringLiteral("请选择有效打印图像源。"));
        return;
    }
    const qint64 required = static_cast<qint64>(config_.main.gridRows)
        * static_cast<qint64>(config_.main.gridColumns);
    if (required <= 0
        || static_cast<quint64>(required) != static_cast<quint64>(active.imageCount())) {
        ui_->errorDetailLabel->setText(
            QStringLiteral("图像数量必须严格等于行数×列数：需要 %1，实际 %2。")
                .arg(required)
                .arg(static_cast<qulonglong>(active.imageCount())));
        ui_->gridRowsSpin->setFocus(Qt::OtherFocusReason);
        return;
    }
    controller_->start(config_, active);
}

QString Print9030Dialog::configPath() const
{
    return QDir(projectRoot_).filePath("config/print_9030.ini");
}

PrintAxisConfig Print9030Dialog::readAxisFromUi(int row) const
{
    PrintAxisConfig axis;
    if (row < 0 || row >= ui_->axisTable->rowCount()) return axis;
    axis.subdivision = axisIntegerEditor(ui_->axisTable, row, 0)->value();
    axis.resolution = axisIntegerEditor(ui_->axisTable, row, 1)->value();
    axis.speedOfMovement = axisIntegerEditor(ui_->axisTable, row, 2)->value();
    axis.acceleratedVelocity = axisIntegerEditor(ui_->axisTable, row, 3)->value();
    axis.startSpeed = axisIntegerEditor(ui_->axisTable, row, 4)->value();
    axis.stopSpeed = axisIntegerEditor(ui_->axisTable, row, 5)->value();
    axis.maxDistance = axisDistanceEditor(ui_->axisTable, row)->value();
    axis.changeDirection = axisDirectionEditor(ui_->axisTable, row)->isChecked();
    axis.electricalStatus = false;
    return axis;
}

void Print9030Dialog::writeAxisToUi(int row, const PrintAxisConfig& axis)
{
    if (row < 0 || row >= ui_->axisTable->rowCount()) return;
    const int values[] = {axis.subdivision, axis.resolution,
        static_cast<int>(axis.speedOfMovement), static_cast<int>(axis.acceleratedVelocity),
        static_cast<int>(axis.startSpeed), static_cast<int>(axis.stopSpeed)};
    for (int column = 0; column < 6; ++column) {
        auto* editor = new QSpinBox(ui_->axisTable);
        editor->setFrame(false);
        editor->setAlignment(Qt::AlignCenter);
        editor->setRange(column <= 3 ? 1 : 0,
            std::numeric_limits<int>::max());
        editor->setValue(values[column]);
        if (column >= 2) {
            editor->setSuffix(column == 3 ? " pulse/s²" : " pulse/s");
        }
        ui_->axisTable->setCellWidget(row, column, editor);
    }
    auto* distance = new QDoubleSpinBox(ui_->axisTable);
    distance->setFrame(false);
    distance->setAlignment(Qt::AlignCenter);
    distance->setDecimals(3);
    distance->setRange(0.001, 1000000.0);
    distance->setSuffix(" mm");
    distance->setValue(axis.maxDistance);
    ui_->axisTable->setCellWidget(row, 6, distance);
    auto* direction = new QCheckBox(ui_->axisTable);
    direction->setChecked(axis.changeDirection);
    direction->setToolTip(QStringLiteral("反转该逻辑轴的运动方向"));
    ui_->axisTable->setCellWidget(row, 7, direction);
}
