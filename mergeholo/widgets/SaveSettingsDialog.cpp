#include "SaveSettingsDialog.h"

#include "ui_SaveSettingsDialog.h"

#include <QCloseEvent>
#include <QPainter>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleOption>

namespace {

class CircularCheckBoxStyle : public QProxyStyle
{
public:
    int pixelMetric(
        PixelMetric metric,
        const QStyleOption* option,
        const QWidget* widget) const override
    {
        if (metric == PM_IndicatorWidth || metric == PM_IndicatorHeight) {
            return 24;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    void drawPrimitive(
        PrimitiveElement element,
        const QStyleOption* option,
        QPainter* painter,
        const QWidget* widget) const override
    {
        if (element != PE_IndicatorCheckBox) {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const bool enabled = option->state.testFlag(State_Enabled);
        const QColor color = enabled ? QColor(18, 18, 18) : QColor(145, 145, 145);
        painter->setPen(QPen(color, 3));
        painter->setBrush(Qt::white);
        const QRectF outer = option->rect.adjusted(3.0, 3.0, -3.0, -3.0);
        painter->drawEllipse(outer);

        if (option->state.testFlag(State_On)) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(color);
            const QRectF inner = outer.adjusted(5.0, 5.0, -5.0, -5.0);
            painter->drawEllipse(inner);
        }
        painter->restore();
    }
};

} // namespace

SaveSettingsDialog::SaveSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui_(new Ui::SaveSettingsDialog)
{
    ui_->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setWindowModality(Qt::ApplicationModal);

    CircularCheckBoxStyle* checkBoxStyle = new CircularCheckBoxStyle;
    checkBoxStyle->setParent(this);
    ui_->meshCheckBox->setStyle(checkBoxStyle);
    ui_->multiviewCheckBox->setStyle(checkBoxStyle);
    ui_->elementalCheckBox->setStyle(checkBoxStyle);

    ui_->closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    ui_->closeButton->setToolTip(QString::fromUtf8("关闭"));

    connect(ui_->confirmButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui_->cancelButton, &QPushButton::clicked, this, &SaveSettingsDialog::reject);
    connect(ui_->closeButton, &QToolButton::clicked, this, &SaveSettingsDialog::reject);
}

SaveSettingsDialog::~SaveSettingsDialog() = default;

void SaveSettingsDialog::setSaveSettings(const ResultSaveSettings& settings)
{
    ui_->meshCheckBox->setChecked(settings.mesh);
    ui_->multiviewCheckBox->setChecked(settings.multiview);
    ui_->elementalCheckBox->setChecked(settings.elemental);
}

ResultSaveSettings SaveSettingsDialog::saveSettings() const
{
    ResultSaveSettings settings;
    settings.mesh = ui_->meshCheckBox->isChecked();
    settings.multiview = ui_->multiviewCheckBox->isChecked();
    settings.elemental = ui_->elementalCheckBox->isChecked();
    return settings;
}

void SaveSettingsDialog::reject()
{
    clearSettingsAndReject();
}

void SaveSettingsDialog::closeEvent(QCloseEvent* event)
{
    clearSettingsAndReject();
    event->accept();
}

void SaveSettingsDialog::clearSettings()
{
    ui_->meshCheckBox->setChecked(false);
    ui_->multiviewCheckBox->setChecked(false);
    ui_->elementalCheckBox->setChecked(false);
}

void SaveSettingsDialog::clearSettingsAndReject()
{
    clearSettings();
    QDialog::reject();
}
