#include "statuspanel.h"

#include "theme.h"

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {

QLabel *valueLabel()
{
    auto *l = new QLabel(QStringLiteral("—"));
    QFont f = l->font();
    f.setBold(true);
    l->setFont(f);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return l;
}

} // namespace

StatusPanel::StatusPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);

    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(28);
    grid->setVerticalSpacing(9);

    auto addRow = [&](int row, const QString &caption, QLabel *&out) {
        auto *cap = new QLabel(caption);
        QFont cf = cap->font();
        cf.setPointSize(cf.pointSize() - 1);
        cap->setFont(cf);
        cap->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        {
            QPalette p = cap->palette();
            p.setColor(QPalette::WindowText, Theme::dim(this));
            cap->setPalette(p);
        }
        out = valueLabel();
        grid->addWidget(cap, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(out, row, 2, Qt::AlignRight | Qt::AlignVCenter);
    };

    addRow(0, QStringLiteral("Hardware"), m_hardware);
    addRow(1, QStringLiteral("Telemetry"), m_telemetry);
    addRow(2, QStringLiteral("Kernel Driver"), m_driver);
    addRow(3, QStringLiteral("Interface"), m_interface);
    grid->setColumnStretch(1, 1);

    m_secondary = new QLabel(QString());
    {
        QFont f = m_secondary->font();
        f.setPointSize(f.pointSize() - 1);
        m_secondary->setFont(f);
        QPalette p = m_secondary->palette();
        p.setColor(QPalette::WindowText, Theme::dim(this));
        m_secondary->setPalette(p);
    }

    v->addLayout(grid);
    v->addWidget(m_secondary);
}

void StatusPanel::setInfo(const Info &info)
{
    m_hardware->setText(info.hardware);
    m_telemetry->setText(info.telemetry);
    m_driver->setText(info.driver);
    m_interface->setText(info.interfaceName);
    m_secondary->setText(info.secondary);
}
