#include "ColorSchemePrefWidget.h"
#include "ui_ColorSchemePrefWidget.h"

#include <QMouseEvent>
#include <QJsonDocument>
#include <QPainter>
#include <QApplication>
#include <QColorDialog>
#include <QDebug>
#include <QFile>

void ColorOptionDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    painter->setPen(QPen(Qt::OpaqueMode));
    painter->setBrush(QBrush(index.model()->data(index,
                                                 Qt::UserRole).value<ColorOption>().backgroundColor));
    painter->drawRect(option.rect);

    QPalette pal;
    pal.setColor(QPalette::Text, index.model()->data(index,
                                                     Qt::UserRole).value<ColorOption>().textColor);
    QStyleOptionViewItem op = option;

    op.palette = pal;
    op.displayAlignment = Qt::AlignCenter;

    if (option.state & QStyle::State_Selected) {
        QLinearGradient lgrd = QLinearGradient(option.rect.topLeft(), option.rect.bottomRight());
        QColor highlighted = QApplication::palette().highlight().color(),
               highlightedOpague = highlighted;
        highlightedOpague.setAlpha(0);

        lgrd.setColorAt(0.00, highlighted);
        lgrd.setColorAt(0.25, highlightedOpague);
        lgrd.setColorAt(0.75, highlightedOpague);
        lgrd.setColorAt(1.00, highlighted);
        painter->setBrush(lgrd);
        painter->drawRect(option.rect);
        op.state &= ~ QStyle::State_Selected;
    }

    op.state &= ~ QStyle::State_Editing;
    pal = option.palette;
    pal.setColor(QPalette::Text, index.model()->data(index,
                                                     Qt::UserRole).value<ColorOption>().textColor);
    pal.setColor(QPalette::Background, index.model()->data(index,
                                                           Qt::UserRole).value<ColorOption>().backgroundColor);
    op.palette = pal;
    QStyledItemDelegate::paint(painter, op, index);
}

ColorViewButton::ColorViewButton(QWidget *parent) : QFrame (parent)
{
    setLineWidth(3);
    setFrameShape(QFrame::Panel);
    setFrameShadow(QFrame::Raised);
    setColor(palette().background().color());
    setMaximumWidth(100);
}

void ColorViewButton::setColor(const QColor &c)
{
    QPalette pal = palette();
    pal.setColor(QPalette::Background, c);
    setAutoFillBackground(true);
    setPalette(pal);
    repaint();
}

void ColorViewButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    QColorDialog d;
    d.setCurrentColor(palette().background().color());
    d.exec();
    emit newColor(d.selectedColor());
}

PreferencesListView::PreferencesListView(QWidget *parent) :
    QListView (parent)
{
    QFile f(path);
    f.open(QFile::ReadOnly);

    setModel(new ColorSettingsModel(QJsonDocument::fromJson(f.readAll()).object(), this));
    setItemDelegate(new ColorOptionDelegate(this));

    f.close();
}

void PreferencesListView::currentChanged(const QModelIndex &current,
                                         const QModelIndex &previous)
{
    emit indexChanged(current);
    QListView::currentChanged(current, previous);
}

ColorSchemePrefWidget::ColorSchemePrefWidget(QWidget *parent) : QWidget (parent),
    ui (new Ui::ColorSchemePrefWidget)
{
    ui->setupUi(this);
    connect(ui->colorViewBack, &ColorViewButton::newColor, this, &ColorSchemePrefWidget::newColor);
    connect(ui->colorViewFore, &ColorViewButton::newColor, this, &ColorSchemePrefWidget::newColor);
    connect(ui->preferencesListView, &PreferencesListView::indexChanged, this,
            &ColorSchemePrefWidget::indexChanged);
}

ColorSchemePrefWidget::~ColorSchemePrefWidget()
{
    apply();
    delete ui;
}

void ColorSchemePrefWidget::apply()
{
    ColorSettingsModel *model = static_cast<ColorSettingsModel *>(ui->preferencesListView->model());
    QJsonObject mainObj, prefObj;
    for (int i = 0; i < model->rowCount(); i++) {
        prefObj.insert("background", QJsonValue::fromVariant(QVariant::fromValue(model->data(model->index(i), Qt::UserRole).value<ColorOption>().backgroundColor)));
        prefObj.insert("text", QJsonValue::fromVariant(QVariant::fromValue(model->data(model->index(i), Qt::UserRole).value<ColorOption>().textColor)));
        mainObj.insert(model->data(model->index(i)).toString(), prefObj);
    }
    QFile f(path);
    f.open(QFile::WriteOnly | QFile::Truncate);
    f.write(QJsonDocument(mainObj).toJson());
    f.close();
}

void ColorSchemePrefWidget::on_setDefBack_clicked()
{
    if (ui->preferencesListView->currentIndex().row() == -1)
        return;
    ColorSettingsModel *model = static_cast<ColorSettingsModel *>(ui->preferencesListView->model());
    QString currOption = model->data(ui->preferencesListView->currentIndex()).toString();
    ColorOption currData;
    for (int i = 0; i < model->rowCount(); i++) {
        currData = model->data(model->index(i, Qt::UserRole)).value<ColorOption>();
        if (currData.optionName == defaultBackgroundOptionName) {
            model->setColor(currOption, "background", currData.backgroundColor);
        }
    }
}

void ColorSchemePrefWidget::on_setDefFore_clicked()
{
    if (ui->preferencesListView->currentIndex().row() == -1)
        return;
    ColorSettingsModel *model = static_cast<ColorSettingsModel *>(ui->preferencesListView->model());
    QString currOption = model->data(ui->preferencesListView->currentIndex()).toString();
    ColorOption currData;
    for (int i = 0; i < model->rowCount(); i++) {
        currData = model->data(model->index(i, Qt::UserRole)).value<ColorOption>();
        if (currData.optionName == simpleTextOptionName) {
            model->setColor(currOption, "text", currData.backgroundColor);
        }
    }
}

void ColorSchemePrefWidget::newColor(const QColor &color)
{
    if (ui->preferencesListView->currentIndex().row() == -1)
        return;
    QString sender = QObject::sender()->objectName();
    QString option = ui->preferencesListView->model()->data(ui->preferencesListView->currentIndex(),
                                                            Qt::UserRole).value<ColorOption>().optionName;
    if (sender == "colorViewBack") {
        static_cast<ColorSettingsModel *>(ui->preferencesListView->model())->setColor(option, "background",
                                                                                      color);
    } else {
        static_cast<ColorSettingsModel *>(ui->preferencesListView->model())->setColor(option, "text",
                                                                                      color);
    }
    static_cast<ColorViewButton *>(QObject::sender())->setColor(color);
}

void ColorSchemePrefWidget::indexChanged(const QModelIndex &ni)
{
    ui->colorViewBack->setColor(ni.data(Qt::UserRole).value<ColorOption>().backgroundColor);
    ui->colorViewFore->setColor(ni.data(Qt::UserRole).value<ColorOption>().textColor);
}

ColorSettingsModel::ColorSettingsModel(const QJsonObject &pref,
                                       QObject *parent) : QAbstractListModel (parent)
{
    m_data.reserve(pref.size());
    for (const auto &it : pref.keys())
        m_data.push_back({it,
                          pref.value(it).toObject().value(simpleTextOptionName).toVariant().value<QColor>(),
                          pref.value(it).toObject().value(defaultBackgroundOptionName).toVariant().value<QColor>()});
}

QVariant ColorSettingsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
        return QVariant::fromValue(m_data.at(index.row()).optionName);

    if (role == Qt::UserRole)
        return QVariant::fromValue(m_data.at(index.row()));

    return QVariant();
}

void ColorSettingsModel::setColor(const QString &option, const QString &textOrBack,
                                  const QColor &color)
{
    int row = 0;
    for (auto &it : m_data) {
        if (it.optionName == option) {
            if (textOrBack == "background") {
                it.backgroundColor = color;
            } else if (textOrBack == "text") {
                it.textColor = color;
            }
            emit dataChanged(index(row), index(row));
            return;
        }
        row++;
    }
}
